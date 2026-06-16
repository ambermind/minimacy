// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include "minimacy.h"

#define MAX_BIT_LENGTH 12	// mandatory for GIF
#define MAX_WORDS (1<<MAX_BIT_LENGTH)
typedef struct
{
	short parent;
	short child;
	short brother;
}Dictcase;

typedef struct Lzw Lzw;
struct Lzw
{
	Dictcase tape[MAX_WORDS];
	char data[MAX_WORDS];
	char buffer[MAX_WORDS];

	LINT fixedRootBuffer;
	LINT fixedRootSrc;

	int done;

	int dataBitSize;
	int dataMask;
	int CLEARCODE;
	int EOI;

	int i;      // index of the next free entry in the dictionary
	int iMax;
	int lastCode;
	int nbits;

	int inputStream;
	int inputBitSize;  // number of bits in inputStream
	int outputStream;
	int outputBitSize;  // number of bits in outputStream
};

#define LZW_DONE 1
#define LZW_ONGOING 0
#define LZW_ERR (-1)

const int LZW_MASKS[33]=
{0x00000000,0x00000001,0x00000003,0x00000007,
 0x0000000f,0x0000001f,0x0000003f,0x0000007f,
 0x000000ff,0x000001ff,0x000003ff,0x000007ff,
 0x00000fff,0x00001fff,0x00003fff,0x00007fff,
 0x0000ffff,0x0001ffff,0x0003ffff,0x0007ffff,
 0x000fffff,0x001fffff,0x003fffff,0x007fffff,
 0x00ffffff,0x01ffffff,0x03ffffff,0x07ffffff,
 0x0fffffff,0x1fffffff,0x3fffffff,0x7fffffff,
 0xffffffff};

// add a data into the dictionary
void _lzwAddToDict(Lzw* z, char data, int parent)
{
	z->data[z->i]=data;
	z->tape[z->i].parent=parent;
	z->tape[z->i].child=-1;
	if (parent!=-1)
	{
		z->tape[z->i].brother=z->tape[parent].child;
		z->tape[parent].child=z->i;
	}
	else z->tape[z->i].brother=-1;
	z->i++;
}

// reset the dictionary
void _lzwResetDict(Lzw* z)
{
	int i;
	z->i=0;
	z->lastCode=-1;
	z->nbits = z->dataBitSize + 1;
	for(i=0;i<=z->dataMask;i++) _lzwAddToDict(z,(char)i,-1);
	_lzwAddToDict(z,0,-1);  // CLEARCODE 
	_lzwAddToDict(z,0,-1);  // EOI 
}

//--------------------------Encode

// search for a data in the dictionary, return its index or -1 when not found 
int _lzwFindMotif(Lzw* z,char data,int parent)
{
	int i=z->tape[parent].child;
	while(i!=-1)
	{
		if (z->data[i]==data) return i;
		i=z->tape[i].brother;
	}
	return -1;
}

// send a word to the output
void _lzwPrintWord(Lzw* z, int word)
{
	while ((z->nbits < MAX_BIT_LENGTH) && (z->i > (1 << z->nbits))) z->nbits++;

	z->outputStream = (word << z->outputBitSize) + z->outputStream;
	z->outputBitSize += z->nbits;
	while (z->outputBitSize >= z->dataBitSize)
	{
		if (fixedBufferAddChar(z->fixedRootBuffer, z->outputStream & z->dataMask)) return;
		z->outputBitSize -= z->dataBitSize;
		z->outputStream >>= z->dataBitSize;
	}
}

// finalize the encoding
void _lzwEncodeLastChar(Lzw* z)
{
	if (z->lastCode != -1)
	{
		_lzwPrintWord(z, z->lastCode);
		_lzwAddToDict(z, 0, z->lastCode);
	}
	_lzwPrintWord(z, z->EOI);
	if (z->outputBitSize) if (fixedBufferAddChar(z->fixedRootBuffer, z->outputStream & LZW_MASKS[z->outputBitSize])) return;
}

// encode the next char 
void _lzwEncodeChar(Lzw* z,int c)
{
	int k;
	c &= z->dataMask;
	if (z->lastCode==-1) k=c;
	else k=_lzwFindMotif(z,(char)c,z->lastCode);
	if (k!=-1)
	{
		z->lastCode=k;
		return;
	}
	_lzwPrintWord(z,z->lastCode);
	_lzwAddToDict(z, (char)c, z->lastCode);
	if (z->i >= z->iMax)
	{
		_lzwPrintWord(z,z->CLEARCODE);
		_lzwResetDict(z);
	}
	z->lastCode=c;
}

//--------------------------Decode
// output a full sequence from a word
void _lzwPrintFromWord(Lzw* z, int word)
{
	char* p = z->buffer;
	do
	{
		*(p++) = z->data[word];
		word = z->tape[word].parent;
	} while (word != -1);

	while (p != z->buffer) {
		if (fixedBufferAddChar(z->fixedRootBuffer, *(--p))) return;
	}
}

int _lzwGetRoot(Lzw* z, int i)
{
	while (z->tape[i].parent != -1) i = z->tape[i].parent;
	return i;
}

// decode a word from the bitstream
int _lzwDecodeWord(Lzw* z,int word)
{
	int root;
	if (word == z->EOI) return LZW_DONE;
	if (word==z->CLEARCODE)
	{
		_lzwResetDict(z);
		return LZW_ONGOING;
	}
	if ((word < 0) || (word > z->i))
	{
//		PRINTF(LOG_DEV,"word is out of range %z/%z\n", word, z->i);
		return LZW_ERR;
	}
	if (z->lastCode!=-1)
	{
		root = _lzwGetRoot(z, (word == z->i) ? z->lastCode: word);
		if (z->i >= z->iMax)
		{
//			PRINTF(LOG_DEV,"%z exceeds Max number of patterns %z\n", z->i, z->iMax);
			return LZW_ERR;
		}
		_lzwAddToDict(z,z->data[root],z->lastCode);
	}
	z->lastCode=word;
	_lzwPrintFromWord(z, word);
	return LZW_ONGOING;
}

void _lzwDecodeStream(Lzw* z, LINT len)
{
	LINT i = 0;
	while (z->done== LZW_ONGOING)
	{
		int word,done;

		while ((z->nbits< MAX_BIT_LENGTH)&&(z->i >= (1 << z->nbits))) z->nbits++;
		while (z->inputBitSize < z->nbits)
		{
			int c;
			if (i >= len)
			{
//				PRINTF(LOG_DEV,"reach end of file %x / %x\n", i, len);
				return;
			}
			c = (STR_START(fixedRootValue(z->fixedRootSrc)))[i++] & 255;
//			PRINTF(LOG_DEV,"b:%x.", c);
			z->inputStream = (c << z->inputBitSize) + z->inputStream;
			z->inputBitSize += 8;
		}
		word = z->inputStream & LZW_MASKS[z->nbits];
		z->inputBitSize -= z->nbits;
		z->inputStream >>= z->nbits;

//		PRINTF(LOG_DEV,"c:%x nbits:%z\n", word, z->nbits);
//		if (word == z->EOI) PRINTF(LOG_DEV,"found EOI at %x / %x\n", i, len);
//		if (word == z->CLEARCODE) PRINTF(LOG_DEV,"found CLEARCODE at %x / %x\n", i, len);

		done = _lzwDecodeWord(z, word);
		z->done = done;
	}
}

void _lzwInit(Lzw* z, LINT dataBitSize)
{
	z->done = LZW_ONGOING;
	z->inputBitSize = 0;
	z->outputBitSize = 0;
	z->iMax = MAX_WORDS;
	z->inputStream = 0;
	z->outputStream = 0;

	z->dataBitSize = (int)dataBitSize;
	z->dataMask = (1 << z->dataBitSize) - 1;
	z->CLEARCODE = z->dataMask + 1;
	z->EOI = z->CLEARCODE + 1;
	_lzwResetDict(z);
}
//---------------------------------------------------

WORKER_START _lzwDeflate(Worker* w)
{
#ifdef USE_WORKER_ASYNC
	Lzw ZZ;
	Lzw* z = &ZZ;
#else
	Lzw* z = (Lzw*)WorkerScratchpad;
#endif
	LINT i, len;
	Thread* th = workerThread(w);

	LINT dataBitSize = STACK_INT(th, 0);
	LB* src = STACK_PNT(th, 1);
	Buffer* out = (Buffer*)STACK_PNT(th, 2);
	if (!out) return workerDoneNil(w);
	if ((dataBitSize < 0) || (dataBitSize > MAX_BIT_LENGTH)) return workerDoneNil(w);
	if (dataBitSize == 0) dataBitSize = 8;
	bufferSetWorker(out, w);
	_lzwInit(z, dataBitSize);
	z->fixedRootBuffer = fixedRootAlloc((LB*)out);
	z->fixedRootSrc = fixedRootAlloc(src);
	len = STR_LENGTH(src);
	for (i = 0; (!w->OM) && (i < len); i++) _lzwEncodeChar(z, (STR_START(fixedRootValue(z->fixedRootSrc)))[i]);
	if (!w->OM) _lzwEncodeLastChar(z);
	fixedRootRelease(z->fixedRootBuffer);
	fixedRootRelease(z->fixedRootSrc);
	return workerDoneBool(w, 1);
}

int fun_lzwDeflate(Thread* th) { return workerStart(th, 3, _lzwDeflate); }

WORKER_START _lzwInflate(Worker* w)
{
#ifdef USE_WORKER_ASYNC
	Lzw ZZ;
	Lzw* z = &ZZ;
#else
	Lzw* z = (Lzw*)WorkerScratchpad;
#endif
	Thread* th = workerThread(w);

	LINT dataBitSize = STACK_INT(th, 0);
	LB* src = STACK_PNT(th, 1);
	Buffer* out = (Buffer*)STACK_PNT(th, 2);
	if (!out) return workerDoneNil(w);
	if ((dataBitSize < 0) || (dataBitSize > MAX_BIT_LENGTH)) return workerDoneNil(w);
	if (dataBitSize == 0) dataBitSize = 8;
	bufferSetWorker(out, w);
	_lzwInit(z, dataBitSize);
	z->fixedRootBuffer = fixedRootAlloc((LB*)out);
	z->fixedRootSrc = fixedRootAlloc(src);

	_lzwDecodeStream(z, STR_LENGTH(src));

	fixedRootRelease(z->fixedRootBuffer);
	fixedRootRelease(z->fixedRootSrc);
	if (z->done == LZW_DONE) return workerDoneBool(w, 1);
	if (z->done == LZW_ERR) return workerDoneBool(w, 0);
	return workerDoneNil(w);
}
int fun_lzwInflate(Thread* th) { return workerStart(th, 3, _lzwInflate); }

int systemLzwInit(Pkg* system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "_lzwDeflate", fun_lzwDeflate, "fun Buffer Str Int Int -> Bool"},
		{ NATIVE_FUN, "_lzwInflate", fun_lzwInflate, "fun Buffer Str Int Int -> Bool"},
	};
	NATIVE_DEF(nativeDefs);
	return 0;
}

