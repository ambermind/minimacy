// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include "minimacy.h"
#include "system_inflate.h"

// this is a basic implementation of deflate, without LZ77,
// and always with dynamic huffman blocks (others are implemented but never called)

#define BLOCK_MAX_LENGTH 16384

typedef struct Deflate Deflate;
struct Deflate
{
	LB header;
	FORGET forget;
	MARK mark;

	volatile Deflate** link;

	volatile Buffer** pout;
	volatile LB* srcBlock;
	volatile char* src;
	int srcLen;

	int bit;
	int val;
};

int bwAlign(volatile Deflate** pz)
{
	Deflate* z = (Deflate*)*pz;
	if ((z->bit) && bufferAddCharWorker(z->pout, z->val)) return -1;	// should be OM error
	z = (Deflate*)*pz;
	z->bit = z->val = 0;
	return 0;
}
int bwFinal(volatile Deflate** pz)
{
	return bwAlign(pz);
}
int bwChar(volatile Deflate** pz, int val)
{
	if (bwAlign(pz)) return -1;
	if (bufferAddCharWorker((*pz)->pout, val)) return -1;
	return 0;
}
int bwBytes(volatile Deflate** pz, int offset, int len)
{
	char* dst;
	if (bwAlign(pz)) return -1;
	dst = bufferRequireWorker((*pz)->pout, len); if (!dst) return -1;
	memcpy(dst, (void*) &(*pz)->src[offset], len);
	return 0;
}
int bwBitsLsb(volatile Deflate** pz, int data, int nbits)
{
	int i;
	Deflate* z = (Deflate*)*pz;
	for (i = 0; i < nbits; i++) {
		z->val |= (((data >> i) & 1) << z->bit);
		if (z->bit >= 7) {
			if (bufferAddCharWorker(z->pout, z->val)) return -1;
			z = (Deflate*)*pz;
			z->val = z->bit = 0;
		}
		else z->bit++;
	}
	return 0;
}
int bwBitsLsbInv(volatile Deflate** pz, int data, int nbits)
{
	int i;
	Deflate* z = (Deflate*)*pz;
	for (i = 0; i < nbits; i++) {
		z->val |= (((data >> (nbits - i - 1)) & 1) << z->bit);
		if (z->bit >= 7) {
			if (bufferAddCharWorker(z->pout, z->val)) return -1;
			z = (Deflate*)*pz;
			z->val = z->bit = 0;
		}
		else z->bit++;
	}
	return 0;
}

int huffmanBuildEncoder(int* lens, int nbLens, int* values)
{
	int i;
	int maxBits = 0;
	int bl_count[MAX_BITS];
	int next_code[MAX_BITS];
	int code = 0;

	for (i = 0; i < MAX_BITS; i++) bl_count[i] = 0;

	for (i = 0; i < nbLens; i++) if (lens[i] > maxBits) maxBits = lens[i];
	maxBits++;
	if (maxBits > MAX_BITS) return -1;	// can this even happen ?
	for (i = 0; i < nbLens; i++) bl_count[lens[i]] = 1 + bl_count[lens[i]];
	bl_count[0] = 0;
	next_code[0] = 0;
	for (i = 1; i < maxBits; i++)
	{
		code = (code + bl_count[i - 1]) << 1;
		next_code[i] = code;
	}
	for (i = 0; i < nbLens; i++) {
		int len = lens[i];
		if (len) {
			values[i] = next_code[len];
			next_code[len]++;
		}
	}
	return 0;
}
HuffNode* huffmanQuickSort(HuffNode* list)
{
	HuffNode* pivot = NULL;
	HuffNode* lower = NULL;
	HuffNode* greater = NULL;
	HuffNode* q;

	if (!list) return NULL;
	if (!list->next) return list;
	pivot = list;
	list = list->next;
	while (list) {
		q = list;
		list = list->next;
		if (q->count < pivot->count) {
			q->next = lower;
			lower = q;
		}
		else {
			q->next = greater;
			greater = q;
		}
	}
	lower = huffmanQuickSort(lower);
	greater = huffmanQuickSort(greater);
	pivot->next = greater;
	if (!lower) return pivot;
	q = lower;
	while (q->next) q = q->next;
	q->next = pivot;
	return lower;
}

HuffNode* huffmanDumpList(HuffNode* t)
{
	while (t) {
		_huffmanDump(t);
		PRINTF(LOG_DEV,":");
		t = t->next;
	}
	PRINTF(LOG_DEV,"nil\n");
	return t;
}

HuffNode* _huffmanBuild(HuffNode* list, HuffNode* available)
{
	while (list->next)
	{
		HuffNode* a = list;
		HuffNode* b = list->next;
		list = b->next;
		if (!available) return NULL;
		available->count = a->count + b->count;
		available->left = a;
		available->right = b;
		a = available;
		available = available->next;
		if ((!list)||(a->count <= list->count)) {
			a->next = list;
			list = a;
		}
		else {
			b = list;
			while (b->next && (a->count > b->next->count)) b = b->next;
			a->next = b->next;
			b->next = a;
		}
	}
	return list;
}
void _huffmanComputeLens(HuffNode* tree, int* lens, int depth)
{
	if (tree->val >= 0) lens[tree->val] = depth;
	else {
		_huffmanComputeLens(tree->left, lens, depth + 1);
		_huffmanComputeLens(tree->right, lens, depth + 1);
	}
}
int huffmanComputeCodeLengths(int* count, int n, int* lens)
{
	int i;
	HuffNode set[MAX_CODE * 2];
	HuffNode* list=NULL;
	HuffNode* available=NULL;
	for (i = 0; i < n; i++) lens[i] = 0;
	for (i = MAX_CODE * 2-1; i >= 0; i--) {
		set[i].left = set[i].right = NULL;
		if ((i<n)&&count[i]) {
			set[i].count = count[i];
			set[i].val = i;
			set[i].next = list;
			list = &set[i];
		}
		else {
			set[i].count = 0;
			set[i].val = -1;
			set[i].next = available;
			available = &set[i];
		}
	}
	list = huffmanQuickSort(list);
	list = _huffmanBuild(list,available);
	if (!list) return -1;
	_huffmanComputeLens(list, lens, 0);
	return 0;
}
int lenEncode(volatile Deflate** pz, int* lenLengths, int* lenValues, int* lens, int n)
{
	int i;
	for(i=0;i<n;i++) if (bwBitsLsbInv(pz, lenValues[lens[i]], lenLengths[lens[i]])) return -1;
	return 0;
}
int _checkLengths(int* lens, int n, int maxValue)
{
	int i;
	for (i = 0; i < n; i++) {
//		PRINTF(LOG_DEV,"%d: %d\n", i, lens[i]);
		if (lens[i] > maxValue) return 0;
	}
//	PRINTF(LOG_DEV,"ok\n");
	return 1;
}
void huffmanComputeLenLengths(int* lenCounts, int n, int* lenLengths)
{
	while(1)
	{
		int i;
		huffmanComputeCodeLengths(lenCounts, n, lenLengths);
		if (_checkLengths(lenLengths, MAX_LENGTH, 7)) return;
		for (i = 0; i < n; i++) if (lenCounts[i] > 1) lenCounts[i] >>= 1;
	}
}
int _deflateLengths(volatile Deflate** pz, int* codeLengths, int nbCodes, int* distLengths, int nbDist)
{
	int i;
	int lenCounts[MAX_LENGTH];
	int lenLengths[MAX_LENGTH];
	int lenValues[MAX_LENGTH];
	if (bwBitsLsb(pz, nbCodes-257, 5)) return -1;
	if (bwBitsLsb(pz, nbDist-1, 5)) return -1;
	if (bwBitsLsb(pz, MAX_LENGTH -4, 4)) return -1;

	for (i = 0; i < MAX_LENGTH; i++) lenCounts[i] = lenValues[i]= 0;

	for (i = 0; i < nbCodes; i++) lenCounts[codeLengths[i]]++;
	for (i = 0; i < nbDist; i++) lenCounts[distLengths[i]]++;

	huffmanComputeLenLengths(lenCounts, MAX_LENGTH, lenLengths);	

	huffmanBuildEncoder(lenLengths, MAX_LENGTH, lenValues);
	for (i = 0; i < MAX_LENGTH; i++) {
//		PRINTF(LOG_DEV,"write %d\n", lenLengths[LEN_ORDER[i]]);
		if (bwBitsLsb(pz, lenLengths[LEN_ORDER[i]], 3)) return -1;
	}

	if (lenEncode(pz, lenLengths, lenValues, codeLengths, nbCodes)) return -1;
	if (lenEncode(pz, lenLengths, lenValues, distLengths, nbDist)) return -1;
	return 0;
}

int deflateBlockDynamic(volatile Deflate** pz, int offset, int len)
{
	int i;
	int counts[257];
	int codeLengths[257];
	int codeValues[257];
	int distLengths[1];

	if (bwBitsLsb(pz, 2, 2)) return -1;
	for (i = 0; i < 257; i++) counts[i] = 0;
	for (i = 0; i < len; i++) {
		int c = (*pz)->src[offset+i] & 255;
		counts[c]++;
	}
	counts[256] = 1;	// for final code
	huffmanComputeCodeLengths(counts, 257, codeLengths);
	counts[0] = 1;
	huffmanComputeCodeLengths(counts, 1, distLengths);

	if (_deflateLengths(pz, codeLengths, 257, distLengths, 1)) return -1;

	huffmanBuildEncoder(codeLengths, 257, codeValues);
	for (i = 0; i < len; i++) {
		int code = (*pz)->src[offset + i] & 255;
		if (bwBitsLsbInv(pz, codeValues[code], codeLengths[code])) return -1;
	}
	if (bwBitsLsbInv(pz, codeValues[256], codeLengths[256])) return -1;

	return 0;

}
int deflateBlockFixed(volatile Deflate** pz, int offset, int len)
{
	int i;
	int codeLengths[MAX_CODE];
	int codeValues[MAX_CODE];
	//	int distLengths[MAX_DIST];
	//	int distValues[MAX_DIST];

	if (bwBitsLsb(pz, 1, 2)) return -1;

	getFixedCodeLengths(codeLengths);
	huffmanBuildEncoder(codeLengths, MAX_CODE, codeValues);
	for (i = 0; i < len; i++) {
		int code = (*pz)->src[offset + i] & 255;
		if (bwBitsLsbInv(pz, codeValues[code], codeLengths[code])) return -1;
	}
	if (bwBitsLsbInv(pz, codeValues[256], codeLengths[256])) return -1;
	return 0;
}
int deflateBlockNoCompression(volatile Deflate** pz, int offset, int len)
{
	int coLen = ~len;
	if (bwBitsLsb(pz, 0, 2)) return -1;
	if (bwChar(pz, len)) return -1;
	if (bwChar(pz, len>>8)) return -1;
	if (bwChar(pz, coLen)) return -1;
	if (bwChar(pz, coLen>>8)) return -1;
	return bwBytes(pz, offset, len);
}
int deflateLoop(volatile Deflate** pz, int isFinal)
{
	int i;
	int srcLen = (*pz)->srcLen;
	for (i = 0; i < srcLen; i += BLOCK_MAX_LENGTH)
	{
		int final = (i + BLOCK_MAX_LENGTH >= srcLen) ? isFinal : 0;
		int toCompress = srcLen - i;
		if (toCompress > BLOCK_MAX_LENGTH) toCompress = BLOCK_MAX_LENGTH;
		if (bwBitsLsb(pz, final, 1)) return -1;
//		if (deflateBlockNoCompression(pz, i, toCompress)) return -1;
//		if (deflateBlockFixed(pz, i, toCompress)) return -1;
		if (deflateBlockDynamic(pz, i, toCompress)) return -1;
	}
	if (isFinal && bwFinal(pz)) return -1;
	return ((*pz)->bit&0xff)|(((*pz)->val&0xff)<<8);
}

void deflateInit(Deflate* z, LB* src, LINT state, volatile Buffer** pout)
{
	z->pout = pout;

	z->srcBlock = src;
	z->src = STR_START(src);
	z->srcLen = (int)STR_LENGTH(src);

	z->bit = state&0xff;
	z->val = (state>>8)&0xff;
}

//--------------------------------------
WORKER_START _deflate(volatile Thread* th)
{
	int k;

	volatile Deflate* z = (volatile Deflate*)STACK_PNT(th, 0);
	LB* isFinal = STACK_PNT(th, 1);
	LINT state = STACK_INT(th, 2);
	LB* src= STACK_PNT(th, 3);
	volatile Buffer* out = (Buffer*)STACK_PNT(th, 4);
	deflateInit((Deflate*)z, src, state, &out);
	bufferSetWorkerThread(&out, &th);
	z->link = &z;
	k = deflateLoop(&z,(isFinal == MM._true));
	bufferUnsetWorkerThread(&out, &th);
	z->link = NULL;
	return workerDoneInt(th,k);
}

void _deflateMark(LB* user)
{
	if (MOVING_BLOCKS) {
		Deflate* z = (Deflate*)user;
		MARK_OR_MOVE(z->srcBlock);
		z->src = STR_START(z->srcBlock);
		if (z->link) *z->link = (Deflate*)z->header.listMark;
	}
}

int fun_deflate(Thread* th) {
	Deflate* z;
//	LB* isFinal = STACK_PNT(th, 0);
//	LINT state = STACK_INT(th, 1);
	LB* src = STACK_PNT(th, 2);
	Buffer* out = (Buffer*)STACK_PNT(th, 3);
	if ((!src) || (!out)) FUN_RETURN_FALSE;
	z = (Deflate*)memoryAllocNative(sizeof(Deflate), DBG_BIN, NULL, _deflateMark);
	if (!z) return EXEC_OM;
	STACK_PUSH_PNT_ERR(th, z, EXEC_OM);
	return workerStart(th, 5, _deflate);
}
