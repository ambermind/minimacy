/* Copyright (c) 2022, Sylvain Huet, Ambermind
   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License, version 2.0, as
   published by the Free Software Foundation.
   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License,
   version 2.0, for more details.
   You should have received a copy of the GNU General Public License along
   with this program. If not, see <https://www.gnu.org/licenses/>. */
#include "minimacy.h"
#include "system_inflate.h"

// this is a basic implementation of deflate, without LZ77,
// and always with dynamic huffman blocks (others are implemented but never called)

#define BLOCK_MAX_LEN 16384

typedef struct
{
	Thread* th;
	Buffer* out;

	int bit;
	int val;
} Deflate;

int bwAlign(Deflate* z)
{
	if ((z->bit) && bufferAddChar(z->th, z->out, z->val)) return -1;	// should be OM error
	z->bit = z->val = 0;
	return 0;
}
int bwFinal(Deflate* z)
{
	return bwAlign(z);
}
int bwChar(Deflate* z, int val)
{
	if (bwAlign(z)) return -1;
	if (bufferAddChar(z->th, z->out, val)) return -1;
	return 0;
}
int bwBytes(Deflate* z, char* data, int len)
{
	if (bwAlign(z)) return -1;
	if (bufferAddBin(z->th, z->out, data, len)) return -1;
	return 0;
}
int bwBitsLsb(Deflate* z, int data, int nbits)
{
	int i;
	for (i = 0; i < nbits; i++) {
		z->val |= (((data >> i) & 1) << z->bit);
		if (z->bit >= 7) {
			if (bufferAddChar(z->th, z->out, z->val)) return -1;
			z->val = z->bit = 0;
		}
		else z->bit++;
	}
	return 0;
}
int bwBitsLsbInv(Deflate* z, int data, int nbits)
{
	int i;
	for (i = 0; i < nbits; i++) {
		z->val |= (((data >> (nbits - i - 1)) & 1) << z->bit);
		if (z->bit >= 7) {
			if (bufferAddChar(z->th, z->out, z->val)) return -1;
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
	if (maxBits > MAX_BITS) return -1;
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
int lenEncode(Deflate* z, int* lenLengths, int* lenValues, int* lens, int n)
{
	int i;
	for(i=0;i<n;i++) if (bwBitsLsbInv(z, lenValues[lens[i]], lenLengths[lens[i]])) return -1;
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
		if (_checkLengths(lenLengths, MAX_LEN, 7)) return;
		for (i = 0; i < n; i++) if (lenCounts[i] > 1) lenCounts[i] >>= 1;
	}
}
int _deflateLengths(Deflate* z, int* codeLengths, int nbCodes, int* distLengths, int nbDist)
{
	int i;
	int lenCounts[MAX_LEN];
	int lenLengths[MAX_LEN];
	int lenValues[MAX_LEN];
	if (bwBitsLsb(z, nbCodes-257, 5)) return -1;
	if (bwBitsLsb(z, nbDist-1, 5)) return -1;
	if (bwBitsLsb(z, MAX_LEN -4, 4)) return -1;

	for (i = 0; i < MAX_LEN; i++) lenCounts[i] = lenValues[i]= 0;

	for (i = 0; i < nbCodes; i++) lenCounts[codeLengths[i]]++;
	for (i = 0; i < nbDist; i++) lenCounts[distLengths[i]]++;

	huffmanComputeLenLengths(lenCounts, MAX_LEN, lenLengths);	

	huffmanBuildEncoder(lenLengths, MAX_LEN, lenValues);
	for (i = 0; i < MAX_LEN; i++) {
//		PRINTF(LOG_DEV,"write %d\n", lenLengths[LEN_ORDER[i]]);
		if (bwBitsLsb(z, lenLengths[LEN_ORDER[i]], 3)) return -1;
	}

	if (lenEncode(z, lenLengths, lenValues, codeLengths, nbCodes)) return -1;
	if (lenEncode(z, lenLengths, lenValues, distLengths, nbDist)) return -1;
	return 0;
}

int deflateBlockDynamic(Deflate* z, char* start, int len)
{
	int i;
	int counts[257];
	int codeLengths[257];
	int codeValues[257];
	int distLengths[1];

	if (bwBitsLsb(z, 2, 2)) return -1;
	for (i = 0; i < 257; i++) counts[i] = 0;
	for (i = 0; i < len; i++) {
		int c = start[i] & 255;
		counts[c]++;
	}
	counts[256] = 1;	// for final code
	huffmanComputeCodeLengths(counts, 257, codeLengths);
	counts[0] = 1;
	huffmanComputeCodeLengths(counts, 1, distLengths);

	if (_deflateLengths(z, codeLengths, 257, distLengths, 1)) return -1;

	huffmanBuildEncoder(codeLengths, 257, codeValues);
	for (i = 0; i < len; i++) {
		int code = start[i] & 255;
		if (bwBitsLsbInv(z, codeValues[code], codeLengths[code])) return -1;
	}
	if (bwBitsLsbInv(z, codeValues[256], codeLengths[256])) return -1;

	return 0;

}
int deflateBlockFixed(Deflate* z, char* start, int len)
{
	int i;
	int codeLengths[MAX_CODE];
	int codeValues[MAX_CODE];
	//	int distLengths[MAX_DIST];
	//	int distValues[MAX_DIST];

	if (bwBitsLsb(z, 1, 2)) return -1;

	getFixedCodeLengths(codeLengths);
	huffmanBuildEncoder(codeLengths, MAX_CODE, codeValues);
	for (i = 0; i < len; i++) {
		int code = start[i] & 255;
		if (bwBitsLsbInv(z, codeValues[code], codeLengths[code])) return -1;
	}
	if (bwBitsLsbInv(z, codeValues[256], codeLengths[256])) return -1;
	return 0;
}
int deflateBlockNoCompression(Deflate* z, char* start, int len)
{
	int coLen = ~len;
	if (bwBitsLsb(z, 0, 2)) return -1;
	if (bwChar(z, len)) return -1;
	if (bwChar(z, len>>8)) return -1;
	if (bwChar(z, coLen)) return -1;
	if (bwChar(z, coLen>>8)) return -1;
	return bwBytes(z, start, len);
	return 0;
}
void deflateInit(Deflate* z,Thread* th, Buffer* out)
{
	z->th = th;
	z->out = out;

	z->bit = 0;
	z->val = 0;
}

int deflateLoop(Deflate* z, char* src, int srcLen)
{
	int i;
	for (i = 0; i < srcLen; i += BLOCK_MAX_LEN)
	{
		int final = (i + BLOCK_MAX_LEN >= srcLen) ? 1 : 0;
		int toCompress = srcLen - i;
		if (toCompress > BLOCK_MAX_LEN) toCompress = BLOCK_MAX_LEN;
		if (bwBitsLsb(z, final, 1)) return -1;
//		if (deflateBlockNoCompression(z, src + i, toCompress)) return -1;
//		if (deflateBlockFixed(z, src + i, toCompress)) return -1;
		if (deflateBlockDynamic(z, src + i, toCompress)) return -1;
	}
	if (bwFinal(z)) return -1;
	return 0;
}
//--------------------------------------
MTHREAD_START _deflate(Thread* th)
{
	Deflate z;

	LB* src= STACKPNT(th, 0);
	Buffer* out = (Buffer*)STACKPNT(th, 1);
	if ((!src)||(!out)) return workerDonePnt(th,MM._false);

	deflateInit(&z, th, out);
	if (deflateLoop(&z, STRSTART(src),(int)STRLEN(src))) return workerDonePnt(th,MM._false);
	return workerDonePnt(th,MM._true);
}
int fun_deflate(Thread* th) { return workerStart(th, 2, _deflate); }
