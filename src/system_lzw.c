// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include "minimacy.h"


#define MAX_WORDS 4096
#define MAX_BIT_LENGTH 12
typedef struct
{
	int data;
	int parent;
	int child;
	int brother;
}Dictcase;

typedef struct Lzw Lzw;
struct Lzw
{
	LB header;
	FORGET forget;
	MARK mark;

	volatile Lzw** link;

	volatile Buffer** pout;
	volatile LB* srcBlock;
	volatile char* src;

	int done;

	int dataBitSize;
	int dataMask;
	int CLEARCODE;
	int EOI;

	int i;      // index of the next free entry in the dictionary
	int iMax;
	Dictcase tape[MAX_WORDS];
	int lastCode;
	int nbits;

	char buffer[MAX_WORDS];

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
void _lzwAddToDict(Lzw* z, int data, int parent)
{
	z->tape[z->i].data=data;
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
	for(i=0;i<=z->dataMask;i++) _lzwAddToDict(z,i,-1);
	_lzwAddToDict(z,0,-1);  // CLEARCODE 
	_lzwAddToDict(z,0,-1);  // EOI 
}

//--------------------------Encode

// search for a data in the dictionary, return its index or -1 when not found 
int _lzwFindMotif(Lzw* z,int data,int parent)
{
	int i=z->tape[parent].child;
	while(i!=-1)
	{
		if (z->tape[i].data==data) return i;
		i=z->tape[i].brother;
	}
	return -1;
}

// send a word to the output
void _lzwPrintWord(volatile Lzw** pz, int word)
{
	Lzw* z = (Lzw*)*pz;
	while ((z->nbits < MAX_BIT_LENGTH) && (z->i > (1 << z->nbits))) z->nbits++;

	z->outputStream = (word << z->outputBitSize) + z->outputStream;
	z->outputBitSize += z->nbits;
	while (z->outputBitSize >= z->dataBitSize)
	{
		if (bufferAddCharWorker(z->pout, z->outputStream & z->dataMask)) return;
		z = (Lzw*)*pz;
		z->outputBitSize -= z->dataBitSize;
		z->outputStream >>= z->dataBitSize;
	}
}

// finalize the encoding
void _lzwEncodeLastChar(volatile Lzw** pz)
{
	Lzw* z = (Lzw*)*pz;
	if (z->lastCode != -1)
	{
		_lzwPrintWord(pz, z->lastCode);
		z = (Lzw*)*pz;
		_lzwAddToDict(z, 0, z->lastCode);
	}
	_lzwPrintWord(pz, z->EOI);
	z = (Lzw*)*pz;
	if (z->outputBitSize) if (bufferAddCharWorker(z->pout, z->outputStream & LZW_MASKS[z->outputBitSize])) return;
}

// encode the next char 
void _lzwEncodeChar(volatile Lzw** pz,int c)
{
	int k;
	Lzw* z = (Lzw*)*pz;
	c &= z->dataMask;
	if (z->lastCode==-1) k=c;
	else k=_lzwFindMotif(z,c,z->lastCode);
	if (k!=-1)
	{
		z->lastCode=k;
		return;
	}
	_lzwPrintWord(pz,z->lastCode);
	z = (Lzw*)*pz;
	_lzwAddToDict(z, c, z->lastCode);
	if (z->i >= z->iMax)
	{
		_lzwPrintWord(pz,z->CLEARCODE);
		z = (Lzw*)*pz;
		_lzwResetDict(z);
	}
	z->lastCode=c;
}

//--------------------------Decode
// output a full sequence from a word
void _lzwPrintFromWord(volatile Lzw** pz, int word)
{
	Lzw* z = (Lzw*)*pz;
	char* p = z->buffer;
	do
	{
		*(p++) = z->tape[word].data;
		word = z->tape[word].parent;
	} while (word != -1);

	while (p != z->buffer) {
		if (bufferAddCharWorker(z->pout, *(--p))) return;
		z = (Lzw*)*pz;
	}
}

int _lzwGetRoot(Lzw* z, int i)
{
	while (z->tape[i].parent != -1) i = z->tape[i].parent;
	return i;
}

// decode a word from the bitstream
int _lzwDecodeWord(volatile Lzw** pz,int word)
{
	int root;
	Lzw* z = (Lzw*)*pz;
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
		_lzwAddToDict(z,z->tape[root].data,z->lastCode);
	}
	z->lastCode=word;
	_lzwPrintFromWord(pz, word);
	return LZW_ONGOING;
}

void _lzwDecodeStream(volatile Lzw** pz,LINT offset, LINT len)
{
	LINT i = 0;
	Lzw* z = (Lzw*)*pz;
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
			c = z->src[offset+(i++)] & 255;
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

		done = _lzwDecodeWord(pz, word);
		z = (Lzw*)*pz;
		z->done = done;
	}
}

void _lzwInit(Lzw* z, LINT dataBitSize)
{
	z->pout = NULL;
	z->link = NULL;
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
void _lzwMark(LB* user)
{
	if (MOVING_BLOCKS) {
		Lzw* z = (Lzw*)user;
		MARK_OR_MOVE(z->srcBlock);
		z->src = STR_START(z->srcBlock);
		if (z->link) *z->link = (Lzw*)z->header.listMark;
	}
}
int fun_lzwCreate(Thread* th)
{
	Lzw* z;

	LINT dataBitSize = STACK_INT(th,0);
	if ((dataBitSize<0)||(dataBitSize> MAX_BIT_LENGTH)) FUN_RETURN_NIL;
	if (dataBitSize == 0) dataBitSize = 8;
	z = (Lzw*)memoryAllocNative(sizeof(Lzw), DBG_BIN, NULL, _lzwMark); if (!z) return EXEC_OM;
	_lzwInit(z, dataBitSize);

	FUN_RETURN_PNT((LB*)z);
}


WORKER_START _lzwDeflate(volatile Thread* th)
{
	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT index = STACK_INT(th, 1);
	LB* src = STACK_PNT(th, 2);
	volatile Buffer* out = (Buffer*)STACK_PNT(th, 3);
	volatile Lzw* z = (Lzw*)STACK_PNT(th, 4);
	if ((!out)||(!z)||(z->done!= LZW_ONGOING)) return workerDoneNil(th);
	bufferSetWorkerThread(&out, &th);
	z->link = &z;
	z->pout = &out;
	if (src)
	{
		LINT i;
		z->srcBlock = src;
		z->src = STR_START(z->srcBlock);
		WORKER_SUBSTR(src, index, len, lenIsNil, STR_LENGTH(src));
		for (i = 0; i < len; i++) _lzwEncodeChar(&z, z->src[index+i]);
		z->srcBlock = NULL;
	}
	else
	{
		_lzwEncodeLastChar(&z);
		z->done = LZW_DONE;
	}
	bufferUnsetWorkerThread(&out, &th);
	z->link = NULL;
	return workerDonePnt(th, MM._true);
}

int fun_lzwDeflate(Thread* th) { return workerStart(th, 5, _lzwDeflate); }

WORKER_START _lzwInflate(volatile Thread* th)
{
	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT index = STACK_INT(th, 1);
	LB* src = STACK_PNT(th, 2);
	volatile Buffer* out = (Buffer*)STACK_PNT(th, 3);
	volatile Lzw* z = (Lzw*)STACK_PNT(th, 4);
	if ((!out)||(!z)||(z->done!= LZW_ONGOING)) return workerDoneNil(th);
	bufferSetWorkerThread(&out, &th);
	z->link = &z;
	z->pout = &out;
	z->srcBlock = src;
	z->src=STR_START(z->srcBlock);
	WORKER_SUBSTR(src, index, len, lenIsNil, STR_LENGTH(src));
	_lzwDecodeStream(&z, index, len);
	z->srcBlock = NULL;
	bufferUnsetWorkerThread(&out, &th);
	z->link = NULL;
	if (z->done == LZW_DONE) return workerDonePnt(th, MM._true);
	if (z->done == LZW_ERR) return workerDonePnt(th, MM._false);
	return workerDoneNil(th);
}
int fun_lzwInflate(Thread* th) { return workerStart(th, 5, _lzwInflate); }

int systemLzwInit(Pkg* system)
{
	pkgAddType(system, "_Lzw");
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "_lzwCreate", fun_lzwCreate, "fun Int -> _Lzw"},
		{ NATIVE_FUN, "_lzwDeflate", fun_lzwDeflate, "fun _Lzw Buffer Str Int Int -> Bool"},
		{ NATIVE_FUN, "_lzwInflate", fun_lzwInflate, "fun _Lzw Buffer Str Int Int -> Bool"},
	};
	NATIVE_DEF(nativeDefs);

	return 0;
}
