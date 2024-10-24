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
typedef struct
{
	Buffer* out;

	HuffNode* lenDecoder;
	HuffNode lenSet[MAX_LENGTH *2];
	HuffNode* codeDecoder;
	HuffNode codeSet[MAX_CODE *2];
	HuffNode* distDecoder;
	HuffNode distSet[32*2];

	char* src;
	int srcLen;
	int bit;
	int done;
} Inflate;
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
	if (i>= z->srcLen) return -1;	// should not happen
	z->bit++;
	return 1 & (z->src[i >> 3] >> (i & 7));
}
int brBitsLsb(Inflate* z, int n)
{
	int i;
	int val = 0;
	if ((z->bit + n) >= z->srcLen) return -1;	// should not happen
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
	if ((z->bit + 8 * n) >= z->srcLen) return -1;	// should not happen on a correct file
	for (i = 0; i < n; i++) val+= ((z->src[(z->bit >> 3) + i] & 255)<<(i<<3));
	z->bit += 8 * n;
	return val;
}
char* brBytesSkip(Inflate* z, int n)
{
	char* start;
	z->bit = (z->bit + 7) & ~7;
	if ((z->bit + 8 * n) > z->srcLen) return NULL;	// should not happen on a correct file
	start = &z->src[z->bit >> 3];
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
int _inflateData(Inflate* z)
{
	int code, dist, extra, offset, len, i;
	while (1) {
		code = huffmanDecodeCode0To7(z, z->codeDecoder);
		if (code < 0) return -1;
		else if (code < 256) bufferAddChar(z->out, code);
		else if (code == 256) return 0;
		else {
			code -= 257;
			extra = LENS257[code * 2];
			offset = LENS257[code * 2+1];
			len = brBitsLsb(z, extra); if (len < 0) return -1;
			len += offset;
			dist = huffmanDecodeCode0To7(z, z->distDecoder); if (dist < 0) return -1;
			extra = DISTANCES[dist * 2];
			offset = DISTANCES[dist * 2 + 1];
			dist = brBitsLsb(z, extra); if (dist < 0) return -1;
			dist += offset;
			for (i = 0; i < len; i++) bufferAddChar(z->out, bufferGetChar(z->out, -dist));
		}
	}
}
int _inflateLenLens(Inflate* z)
{
	int i;
	int lenLengths[MAX_LENGTH];
	int nbLen = brBitsLsb(z, 4); if (nbLen < 0) return -1;
	nbLen += 4;
	for (i = 0; i < MAX_LENGTH; i++) lenLengths[i] = 0;
	for (i = 0; i < nbLen; i++) {
		int len = brBitsLsb(z, 3); if (len < 0) return -1;
		lenLengths[LEN_ORDER[i]] = len;
	}
//	for (i = 0; i < nbLen; i++) PRINTF(LOG_DEV,"%d, ", lenLengths[i]);

	z->lenDecoder = _huffmanBuildDecoder(z->lenSet, lenLengths, MAX_LENGTH);
//	huffmanDump(z->lenDecoder);
	if (!z->lenDecoder) return -1;
	return 0;
}
int _inflateLens(Inflate* z, int* lens, int nb)
{
	int n,j;
	int i = 0;
	while (i < nb) {
		int len = huffmanDecodeCode0To7(z, z->lenDecoder); if (len < 0) return -1;
		switch (len) {
		case 16:
			n = brBitsLsb(z, 2); if (n < 0) return -1;
			n += 3; if (i + n > nb) return -1;
			for (j = 0; j < n; j++) lens[i + j] = lens[i - 1];
			i += n;
			break;
		case 17:
			n = brBitsLsb(z, 3); if (n < 0) return -1;
			n += 3; if (i + n > nb) return -1;
			for (j = 0; j < n; j++) lens[i + j] = 0;
			i += n;
			break;
		case 18:
			n = brBitsLsb(z, 7); if (n < 0) return -1;
			n += 11; if (i + n > nb) return -1;
			for (j = 0; j < n; j++) lens[i + j] = 0;
			i += n;
			break;
		default:
			if (len > 15) return -1;
			lens[i++] = len;
		}
	}
	return 0;
}

int _inflateBlockDynamic(Inflate* z)
{
	int lens[MAX_CODE + MAX_DIST];
	int nbCodes, nbDist;
	nbCodes = brBitsLsb(z, 5); if (nbCodes < 0) return -1;
	nbDist = brBitsLsb(z, 5); if (nbDist < 0) return -1;
	nbCodes += 257; if (nbCodes > MAX_CODE) return -1;
	nbDist += 1; if (nbDist > MAX_DIST) return -1;
	if (_inflateLenLens(z)) return -1;
	if (_inflateLens(z, lens, nbCodes + nbDist)) return -1;

	z->codeDecoder = _huffmanBuildDecoder(z->codeSet, lens, nbCodes);
//	huffmanDump(z->codeDecoder);
	if (!z->codeDecoder) return -1;

	z->distDecoder = _huffmanBuildDecoder(z->distSet, lens+nbCodes, nbDist);
//	huffmanDump(z->distDecoder);
	if (!z->distDecoder) return -1;

	return _inflateData(z);
}

int _inflateBlockFixed(Inflate* z)
{
	int lens[MAX_CODE];

	getFixedCodeLengths(lens);
	z->codeDecoder = _huffmanBuildDecoder(z->codeSet, lens, MAX_CODE);
//	huffmanDump(z->codeDecoder);
	if (!z->codeDecoder) return -1;

	getFixedDistLengths(lens);
	z->distDecoder = _huffmanBuildDecoder(z->distSet, lens, MAX_DIST);
	if (!z->distDecoder) return -1;
//	huffmanDump(z->distDecoder);

	return _inflateData(z);
}

int _inflateBlockNoCompression(Inflate* z)
{
	char* start;
	int len = brBytesLsb(z, 2);
	int coLen = brBytesLsb(z, 2);
	if ((len + coLen) != 0xffff) return -1;

	start = brBytesSkip(z, len); if (!start) return -1;
	if (bufferAddBin(z->out, start, len)) return -1;	// should return OM
	return 0;
	//	PRINTF(LOG_DEV,"len=%x coLen=%x i=%d sum=%x\n", len, coLen,z->bit,len+coLen);
}

int inflateLoop(Inflate* z)
{
	while (1) {
		int final, type;
		final = brBitsLsb(z, 1); if (final < 0) return 0;
		type = brBitsLsb(z, 2); if (type < 0) return 0;
//		PRINTF(LOG_DEV,"inflate type %d\n", type);
		switch (type) {
			case 0:
				if (_inflateBlockNoCompression(z)) return 0;
				break;
			case 1:
				if (_inflateBlockFixed(z)) return 0;
				break;
			case 2:
				if (_inflateBlockDynamic(z)) return 0;
				break;
			default:
//				PRINTF(LOG_DEV,"unsupported block type %d\n", type);
				return 0;
		}
		if (final) break;
	}
	z->done = 1;
	return 0;
}
void inflateInit(Inflate* z, LB* src, Buffer* out)
{
	z->out = out;

	z->src = STR_START(src);
	z->srcLen = ((int)STR_LENGTH(src))<<3;
	z->bit = 0;

	z->done = 0;
}
//--------------------------------------
MTHREAD_START _inflate(Thread* th)
{
	Inflate z;

	LB* src= STACK_PNT(th, 0);
	Buffer* out = (Buffer*)STACK_PNT(th, 1);
	if ((!src)||(!out)) return workerDonePnt(th,MM._false);
	bufferSetWorkerThread(out, th);
	inflateInit(&z, src, out);
	if (inflateLoop(&z) || !z.done) {
		bufferSetWorkerThread(out, NULL);
		return workerDonePnt(th,MM._false);
	}
	bufferSetWorkerThread(out, NULL);
	return workerDonePnt(th,MM._true);
}

int fun_inflate(Thread* th) { return workerStart(th, 2, _inflate); }

int coreInflateInit(Pkg* system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "_deflate", fun_deflate, "fun Buffer Str -> Bool"},
		{ NATIVE_FUN, "_deflateBytes", fun_deflate, "fun Buffer Bytes -> Bool"},
		{ NATIVE_FUN, "_inflate", fun_inflate, "fun Buffer Str -> Bool"},
	};
	NATIVE_DEF(nativeDefs);
	return 0;
}
