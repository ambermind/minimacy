// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include "minimacy.h"
#include "system_inflate.h"
typedef struct Inflate Inflate;
struct Inflate
{
	LB header;
	FORGET forget;
	MARK mark;

	volatile Inflate** link;

	volatile Buffer** pout;
	volatile LB* srcBlock;
	volatile char* src;

	HuffNode* lenDecoder;
	HuffNode lenSet[MAX_LENGTH *2];
	HuffNode* codeDecoder;
	HuffNode codeSet[MAX_CODE *2];
	HuffNode* distDecoder;
	HuffNode distSet[32*2];

	int srcLen;
	int blockStart;
	int bit;
};
const int LEN_ORDER[] = {	//19
	16, 17, 18, 0, 8, 7, 9, 6,
	10, 5, 11, 4, 12, 3, 13, 2,
	14, 1, 15,
};
const int LENS257[] = {
	0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0, 9, 0, 10,
	1, 11, 1, 13, 1, 15, 1, 17, 2, 19, 2, 23, 2, 27, 2, 31,
	3, 35, 3, 43, 3, 51, 3, 59, 4, 67, 4, 83, 4, 99, 4, 115,
	5, 131, 5, 163, 5, 195, 5, 227, 0, 258
};
const int DISTANCES[] = {
	0, 1, 0, 2, 0, 3, 0, 4, 1, 5, 1, 7, 2, 9, 2, 13,
	3, 17, 3, 25, 4, 33, 4, 49, 5, 65, 5, 97, 6, 129, 6, 193,
	7, 257, 7, 385, 8, 513, 8, 769, 9, 1025, 9, 1537, 10, 2049, 10, 3073,
	11, 4097, 11, 6145, 12, 8193, 12, 12289, 13, 16385, 13, 24577
};

void getFixedCodeLengths(int* lens)
{
	int i;
	for (i = 0; i < 144; i++) lens[i] = 8;
	for (i = 144; i < 256; i++) lens[i] = 9;
	for (i = 256; i < 280; i++) lens[i] = 7;
	for (i = 280; i < MAX_CODE; i++) lens[i] = 8;
}
void getFixedDistLengths(int* lens)
{
	int i;
	for (i = 0; i < MAX_DIST; i++) lens[i] = 5;
}
// br... like bit reader
int brBit0To7(Inflate* z)
{
	int i = z->bit;
	if (i>= z->srcLen) return ERR_UNCOMPLETE;
	z->bit++;
	return 1 & (z->src[i >> 3] >> (i & 7));
}
int brBitsLsb(Inflate* z, int n)
{
	int i;
	int val = 0;
	if ((z->bit + n) >= z->srcLen) return ERR_UNCOMPLETE;
	for (i = 0; i < n;i++)
	{
		int j = z->bit + i;
		char c = z->src[j >> 3];
		val+= (1 & (c >> (j & 7)))<<i;
	}
	z->bit += n;
	return val;
}
int brBytesLsb(Inflate* z, int n)
{
	int val = 0;
	int i;
	z->bit = (z->bit + 7) & ~7;
	if ((z->bit + 8 * n) >= z->srcLen) return ERR_UNCOMPLETE;
	for (i = 0; i < n; i++) val+= ((z->src[(z->bit >> 3) + i] & 255)<<(i<<3));
	z->bit += 8 * n;
	return val;
}
char* brBytesSkip(Inflate* z, int n)
{
	char* start;
	z->bit = (z->bit + 7) & ~7;
	if ((z->bit + 8 * n) > z->srcLen) return NULL;
	start = (char*)&z->src[z->bit >> 3];
	z->bit += 8 * n;
	return start;
}
void _huffmanDump(HuffNode* t)
{
	if (!t) PRINTF(LOG_DEV,"NULL");
	else if (t->val >= 0) PRINTF(LOG_DEV,"%d", t->val);
	else {
		PRINTF(LOG_DEV,"[");
		_huffmanDump(t->left);
		PRINTF(LOG_DEV,", ");
		_huffmanDump(t->right);
		PRINTF(LOG_DEV,"]");
	}
}
HuffNode* huffmanDump(HuffNode* t)
{
	_huffmanDump(t);
	PRINTF(LOG_DEV,"\n");
	return t;
}
HuffNode* _huffmanTreeFromFifos(HuffNode** fifos, HuffNode** available, int depth, int maxDepth)
{
	HuffNode* p;
	HuffNode* q;
	HuffNode* r;
	if (depth >= maxDepth) return NULL;
	if (fifos[depth]) {
		p = fifos[depth];
		fifos[depth] = p->next;
		return p;
	}
	if (!*available) return NULL;	// we'll need one HuffNode
	p = _huffmanTreeFromFifos(fifos, available, depth + 1, maxDepth);
	if (!p) return NULL;
	q = _huffmanTreeFromFifos(fifos, available, depth + 1, maxDepth);
	if (!q) return NULL;
	r = *available;
	*available = r->next;
	r->val = -1;
	r->left = p;
	r->right = q;
	return r;
}

HuffNode* _huffmanBuildDecoder(HuffNode* set, int* lens, int nbLens)
{
	int i;
	int maxBits = 0;
	int nbSets = nbLens * 2;
	HuffNode* fifos[MAX_BITS];
	HuffNode* available = NULL;

	for(i = 0; i < nbLens; i++) if (lens[i] > maxBits) maxBits = lens[i];
	if (maxBits == 0) {
		set[0].left = set[0].right = NULL;
		set[0].val = 0;
		return &set[0];
	}
	maxBits++;
	if (maxBits > MAX_BITS) return NULL;
	for (i = 0; i < maxBits; i++) fifos[i] = NULL;
	for (i = nbSets - 1; i >= 0; i--) {
		set[i].next = available;
		available= &set[i];
	}
	for (i = nbLens - 1; i >= 0; i--) if (lens[i]) {
		HuffNode* t = available;
		if (!t) return NULL;
		available = t->next;
		t->val = i;
		t->next = fifos[lens[i]];
		fifos[lens[i]] = t;
	}
	return _huffmanTreeFromFifos(fifos, &available, 0, maxBits);
}
int huffmanDecodeCode0To7(Inflate* z, HuffNode* p)
{
	while (p->val < 0) {
		int b = brBit0To7(z);
		if (b < 0) return b;
		p = b ? p->right : p->left;
	}
	return p->val;
}
int _inflateData(volatile Inflate** pz)
{
	Inflate* z = (Inflate*)*pz;
	int code, dist, extra, offset, len, i;
	while (1) {
		code = huffmanDecodeCode0To7(z, z->codeDecoder);
		if (code < 0) return code;
		else if (code < 256) {
			if (bufferAddCharWorker(z->pout, code)) return ERR_OM;
			z = (Inflate*)*pz;
		}
		else if (code == 256) return 0;
		else {
			code -= 257;
			extra = LENS257[code * 2];
			offset = LENS257[code * 2+1];
			len = brBitsLsb(z, extra); if (len < 0) return len;
			len += offset;
			dist = huffmanDecodeCode0To7(z, z->distDecoder); if (dist < 0) return dist;
			extra = DISTANCES[dist * 2];
			offset = DISTANCES[dist * 2 + 1];
			dist = brBitsLsb(z, extra); if (dist < 0) return dist;
			dist += offset;
			for (i = 0; i < len; i++) {
				if (bufferAddCharWorker(z->pout, bufferGetChar((Buffer*)*z->pout, -dist))) return ERR_OM;
				z = (Inflate*)*pz;
			}
		}
	}
}
int _inflateLenLens(Inflate* z)
{
	int i;
	int lenLengths[MAX_LENGTH];
	int nbLen = brBitsLsb(z, 4); if (nbLen < 0) return nbLen;
	nbLen += 4;
	for (i = 0; i < MAX_LENGTH; i++) lenLengths[i] = 0;
	for (i = 0; i < nbLen; i++) {
		int len = brBitsLsb(z, 3); if (len < 0) return len;
		lenLengths[LEN_ORDER[i]] = len;
	}
//	for (i = 0; i < nbLen; i++) PRINTF(LOG_DEV,"%d, ", lenLengths[i]);

	z->lenDecoder = _huffmanBuildDecoder(z->lenSet, lenLengths, MAX_LENGTH);
//	huffmanDump(z->lenDecoder);
	if (!z->lenDecoder) return ERR_FORMAT;
	return 0;
}
int _inflateLens(Inflate* z, int* lens, int nb)
{
	int n,j;
	int i = 0;
	while (i < nb) {
		int len = huffmanDecodeCode0To7(z, z->lenDecoder); if (len < 0) return len;
		switch (len) {
		case 16:
			n = brBitsLsb(z, 2); if (n < 0) return n;
			n += 3; if (i + n > nb) return ERR_FORMAT;
			for (j = 0; j < n; j++) lens[i + j] = lens[i - 1];
			i += n;
			break;
		case 17:
			n = brBitsLsb(z, 3); if (n < 0) return n;
			n += 3; if (i + n > nb) return ERR_FORMAT;
			for (j = 0; j < n; j++) lens[i + j] = 0;
			i += n;
			break;
		case 18:
			n = brBitsLsb(z, 7); if (n < 0) return n;
			n += 11; if (i + n > nb) return ERR_FORMAT;
			for (j = 0; j < n; j++) lens[i + j] = 0;
			i += n;
			break;
		default:
			if (len > 15) return ERR_FORMAT;
			lens[i++] = len;
		}
	}
	return 0;
}

int _inflateBlockDynamic(volatile Inflate** pz)
{
	int k;
	int lens[MAX_CODE + MAX_DIST];
	int nbCodes, nbDist;
	Inflate* z = (Inflate*)*pz;

	nbCodes = brBitsLsb(z, 5); if (nbCodes < 0) return nbCodes;
	nbDist = brBitsLsb(z, 5); if (nbDist < 0) return nbDist;
	nbCodes += 257; if (nbCodes > MAX_CODE) return ERR_FORMAT;
	nbDist += 1; if (nbDist > MAX_DIST) return ERR_FORMAT;
	if ((k=_inflateLenLens(z))) return k;
	if ((k=_inflateLens(z, lens, nbCodes + nbDist))) return k;

	z->codeDecoder = _huffmanBuildDecoder(z->codeSet, lens, nbCodes);
//	huffmanDump(z->codeDecoder);
	if (!z->codeDecoder) return ERR_FORMAT;

	z->distDecoder = _huffmanBuildDecoder(z->distSet, lens+nbCodes, nbDist);
//	huffmanDump(z->distDecoder);
	if (!z->distDecoder) return ERR_FORMAT;

	return _inflateData(pz);
}

int _inflateBlockFixed(volatile Inflate** pz)
{
	int lens[MAX_CODE];
	Inflate* z = (Inflate*)*pz;

	getFixedCodeLengths(lens);
	z->codeDecoder = _huffmanBuildDecoder(z->codeSet, lens, MAX_CODE);
//	huffmanDump(z->codeDecoder);
	if (!z->codeDecoder) return ERR_FORMAT;

	getFixedDistLengths(lens);
	z->distDecoder = _huffmanBuildDecoder(z->distSet, lens, MAX_DIST);
	if (!z->distDecoder) return ERR_FORMAT;
//	huffmanDump(z->distDecoder);

	return _inflateData(pz);
}

int _inflateBlockNoCompression(volatile Inflate** pz)
{
	char* start;
	int len, coLen;
	Inflate* z = (Inflate*)*pz;
	len = brBytesLsb(z, 2);	if (len < 0) return len;
	coLen = brBytesLsb(z, 2); if (coLen < 0) return coLen;
	if ((len + coLen) != 0xffff) return ERR_FORMAT;

	start = brBytesSkip(z, len); if (!start) return ERR_UNCOMPLETE;
	if (bufferAddBinWorker(z->pout, start, len)) return ERR_OM;
	return 0;
	//	PRINTF(LOG_DEV,"len=%x coLen=%x i=%d sum=%x\n", len, coLen,z->bit,len+coLen);
}

int inflateLoop(volatile Inflate** pz)
{
	while (1) {
		int k, final, type;
		Inflate* z = (Inflate*)*pz;
		final = brBitsLsb(z, 1); if (final < 0) return final;
		type = brBitsLsb(z, 2); if (type < 0) return type;
//		PRINTF(LOG_DEV,"inflate type %d\n", type);
		switch (type) {
			case 0:
				if ((k=_inflateBlockNoCompression(pz))) return k;
				break;
			case 1:
				if ((k=_inflateBlockFixed(pz))) return k;
				break;
			case 2:
				if ((k=_inflateBlockDynamic(pz))) return k;
				break;
			default:
//				PRINTF(LOG_DEV,"unsupported block type %d\n", type);
				return ERR_FORMAT;
		}
		(*pz)->blockStart=(*pz)->bit;
		if (final) break;
	}
	return 0;
}
void inflateInit(Inflate* z, LB* src, int blockStart, volatile Buffer** pout)
{
	z->pout = pout;

	z->srcBlock = src;
	z->src = STR_START(src);
	z->srcLen = ((int)STR_LENGTH(src))<<3;
	z->bit = z->blockStart = blockStart;
}
//--------------------------------------
WORKER_START _inflate(volatile Thread* th)
{
	int k;
	LINT result;
	volatile Inflate* z= (volatile Inflate*)STACK_PNT(th, 0);
	LINT startBit=STACK_INT(th,1);
	LB* src = STACK_PNT(th, 2);
	volatile Buffer* out = (Buffer*)STACK_PNT(th, 3);
	inflateInit((Inflate*)z, src, (int)startBit, &out);
	bufferSetWorkerThread(&out, &th);
	z->link = &z;
	k=inflateLoop(&z);
	// -1: done, <0: error, >=0 new bitStart
	result=(!k)?-1:((k==ERR_UNCOMPLETE)?z->blockStart:-2);
	bufferUnsetWorkerThread(&out, &th);
	z->link = NULL;
	return workerDoneInt(th,result);
}

void _inflateMark(LB* user)
{
	if (MOVING_BLOCKS) {
		Inflate* z = (Inflate*)user;
		MARK_OR_MOVE(z->srcBlock);
		z->src = STR_START(z->srcBlock);
		if (z->link) *z->link = (Inflate*)z->header.listMark;
	}
}

int fun_inflate(Thread* th) { 
	Inflate* z;
	LINT startBit=STACK_INT(th,0);
	LB* src = STACK_PNT(th, 1);
	Buffer* out = (Buffer*)STACK_PNT(th, 2);
	if ((!src) || (!out) || (startBit<0) || (startBit*8>STR_LENGTH(src))) FUN_RETURN_FALSE;
	z = (Inflate*)memoryAllocNative(sizeof(Inflate), DBG_BIN, NULL, _inflateMark);
	if (!z) return EXEC_OM;
	STACK_PUSH_PNT_ERR(th, z, EXEC_OM);
	return workerStart(th, 4, _inflate);
}

int systemInflateInit(Pkg* system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "_deflate", fun_deflate, "fun Buffer Str Int Bool -> Int"},
		{ NATIVE_FUN, "_deflateBytes", fun_deflate, "fun Buffer Bytes Int Bool -> Int"},
		{ NATIVE_FUN, "_inflate", fun_inflate, "fun Buffer Str Int -> Int"},
	};
	NATIVE_DEF(nativeDefs);
	return 0;
}
