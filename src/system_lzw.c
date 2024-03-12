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


#define MAX_WORDS 4096
#define MAX_BIT_LENGTH 12
typedef struct
{
	int data;
	int parent;
	int child;
	int brother;
}Dictcase;

typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	Thread* th;
	Buffer* output;
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
} Dict;

#define LZW_DONE 1
#define LZW_ONGOING 0
#define LZW_ERR (-1)


#define _lzwPrintChar(d,c) bufferAddChar(d->th,d->output,c);
/*void _lzwPrintChar(Dict* d,int c)
{
	bufferAddChar(d->th,d->output,c);
}
*/
int LZW_MASKS[33]=
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
void _lzwAddToDict(Dict* d, int data, int parent)
{
	d->tape[d->i].data=data;
	d->tape[d->i].parent=parent;
	d->tape[d->i].child=-1;
	if (parent!=-1)
	{
		d->tape[d->i].brother=d->tape[parent].child;
		d->tape[parent].child=d->i;
	}
	else d->tape[d->i].brother=-1;
	d->i++;
}

// reset the dictionary
void _lzwResetDict(Dict* d)
{
	int i;
	d->i=0;
	d->lastCode=-1;
	d->nbits = d->dataBitSize + 1;
	for(i=0;i<=d->dataMask;i++) _lzwAddToDict(d,i,-1);
	_lzwAddToDict(d,0,-1);  // CLEARCODE 
	_lzwAddToDict(d,0,-1);  // EOI 
}

//--------------------------Encode

// search for a data in the dictionary, return its index or -1 when not found 
int _lzwFindMotif(Dict* d,int data,int parent)
{
	int i=d->tape[parent].child;
	while(i!=-1)
	{
		if (d->tape[i].data==data) return i;
		i=d->tape[i].brother;
	}
	return -1;
}

// send a word to the output
void _lzwPrintWord(Dict* d, int word)
{
	while ((d->nbits < MAX_BIT_LENGTH) && (d->i > (1 << d->nbits))) d->nbits++;

	d->outputStream = (word << d->outputBitSize) + d->outputStream;
	d->outputBitSize += d->nbits;
	while (d->outputBitSize >= d->dataBitSize)
	{
		_lzwPrintChar(d, d->outputStream & d->dataMask);
		d->outputBitSize -= d->dataBitSize;
		d->outputStream >>= d->dataBitSize;
	}
}

// finalize the encoding
void _lzwEncodeLastChar(Dict* d)
{
	if (d->lastCode != -1)
	{
		_lzwPrintWord(d, d->lastCode);
		_lzwAddToDict(d, 0, d->lastCode);
	}
	_lzwPrintWord(d, d->EOI);
	if (d->outputBitSize) _lzwPrintChar(d, d->outputStream & LZW_MASKS[d->outputBitSize]);
}

// encode the next char 
void _lzwEncodeChar(Dict* d,int c)
{
	int k;
	c &= d->dataMask;
	if (d->lastCode==-1) k=c;
	else k=_lzwFindMotif(d,c,d->lastCode);
	if (k!=-1)
	{
		d->lastCode=k;
		return;
	}
	_lzwPrintWord(d,d->lastCode);
	_lzwAddToDict(d, c, d->lastCode);
	if (d->i >= d->iMax)
	{
		_lzwPrintWord(d,d->CLEARCODE);
		_lzwResetDict(d);
	}
	d->lastCode=c;
}

//--------------------------Decode
// output a full sequence from a word
void _lzwPrintFromWord(Dict* d, int word)
{
	char* p = d->buffer;
	do
	{
		*(p++) = d->tape[word].data;
		word = d->tape[word].parent;
	} while (word != -1);

	while(p!=d->buffer) _lzwPrintChar(d, *(--p));
}

int _lzwGetRoot(Dict* d, int i)
{
	while (d->tape[i].parent != -1) i = d->tape[i].parent;
	return i;
}

// decode a word from the bitstream
int _lzwDecodeWord(Dict* d,int word)
{
	int root;
	if (word == d->EOI) return LZW_DONE;
	if (word==d->CLEARCODE)
	{
		_lzwResetDict(d);
		return LZW_ONGOING;
	}
	if ((word < 0) || (word > d->i))
	{
//		PRINTF(LOG_DEV,"word is out of range %d/%d\n", word, d->i);
		return LZW_ERR;
	}
	if (d->lastCode!=-1)
	{
		root = _lzwGetRoot(d, (word == d->i) ? d->lastCode: word);
		if (d->i >= d->iMax)
		{
//			PRINTF(LOG_DEV,"%d exceeds Max number of patterns %d\n", d->i, d->iMax);
			return LZW_ERR;
		}
		_lzwAddToDict(d,d->tape[root].data,d->lastCode);
	}
	_lzwPrintFromWord(d, word);
	d->lastCode=word;
	return LZW_ONGOING;
}

void _lzwDecodeStream(Dict* d,char* p, LINT len)
{
	LINT i = 0;
	while (d->done== LZW_ONGOING)
	{
		int word;

		while ((d->nbits< MAX_BIT_LENGTH)&&(d->i >= (1 << d->nbits))) d->nbits++;
		while (d->inputBitSize < d->nbits)
		{
			int c;
			if (i >= len)
			{
//				PRINTF(LOG_DEV,"reach end of file %x / %x\n", i, len);
				return;
			}
			c = p[i++] & 255;
//			PRINTF(LOG_DEV,"b:%x.", c);
			d->inputStream = (c << d->inputBitSize) + d->inputStream;
			d->inputBitSize += 8;
		}
		word = d->inputStream & LZW_MASKS[d->nbits];
		d->inputBitSize -= d->nbits;
		d->inputStream >>= d->nbits;

//		PRINTF(LOG_DEV,"c:%x nbits:%d\n", word, d->nbits);
//		if (word == d->EOI) PRINTF(LOG_DEV,"found EOI at %x / %x\n", i, len);
//		if (word == d->CLEARCODE) PRINTF(LOG_DEV,"found CLEARCODE at %x / %x\n", i, len);

		d->done = _lzwDecodeWord(d, word);
	}
}

void _lzwInit(Dict* d, LINT dataBitSize)
{
	d->output = NULL;
	d->th = NULL;
	d->done = LZW_ONGOING;
	d->inputBitSize = 0;
	d->outputBitSize = 0;
	d->iMax = MAX_WORDS;
	d->inputStream = 0;
	d->outputStream = 0;

	d->dataBitSize = (int)dataBitSize;
	d->dataMask = (1 << d->dataBitSize) - 1;
	d->CLEARCODE = d->dataMask + 1;
	d->EOI = d->CLEARCODE + 1;
	_lzwResetDict(d);
}
//---------------------------------------------------
int fun_lzwCreate(Thread* th)
{
	Dict* d;

	LINT dataBitSize = STACK_PULL_INT(th);
	if ((dataBitSize<0)||(dataBitSize> MAX_BIT_LENGTH)) FUN_RETURN_NIL;
	if (dataBitSize == 0) dataBitSize = 8;
	d = (Dict*)memoryAllocExt(th, sizeof(Dict), DBG_BIN, NULL, NULL); if (!d) return EXEC_OM;
	_lzwInit(d, dataBitSize);

	FUN_RETURN_PNT((LB*)d);
}


MTHREAD_START _lzwDeflate(Thread* th)
{
	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT index = STACK_INT(th, 1);
	LB* src = STACK_PNT(th, 2);
	Buffer* b = (Buffer*)STACK_PNT(th, 3);
	Dict* d = (Dict*)STACK_PNT(th, 4);
	if ((!b)||(!d)||(d->done!= LZW_ONGOING)) return workerDoneNil(th);
	d->th = th;
	d->output = b;
	if (src)
	{
		char* p;
		LINT i;
		WORKER_SUBSTR(src, index, len, lenIsNil, STR_LENGTH(src));
		p = STR_START(src) + index;
		for (i = 0; i < len; i++) _lzwEncodeChar(d, p[i]);
	}
	else
	{
		_lzwEncodeLastChar(d);
		d->done = LZW_DONE;
	}
	return workerDonePnt(th, MM._true);
}

int fun_lzwDeflate(Thread* th) { return workerStart(th, 5, _lzwDeflate); }

MTHREAD_START _lzwInflate(Thread* th)
{
	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT index = STACK_INT(th, 1);
	LB* src = STACK_PNT(th, 2);
	Buffer* b = (Buffer*)STACK_PNT(th, 3);
	Dict* d = (Dict*)STACK_PNT(th, 4);
	if ((!b)||(!d)||(d->done!= LZW_ONGOING)) return workerDoneNil(th);
	d->th = th;
	d->output = b;
	WORKER_SUBSTR(src, index, len, lenIsNil, STR_LENGTH(src));
	_lzwDecodeStream(d, STR_START(src) + index, len);
	if (d->done == LZW_DONE) return workerDonePnt(th, MM._true);
	if (d->done == LZW_ERR) return workerDonePnt(th, MM._false);
	return workerDoneNil(th);
}
int fun_lzwInflate(Thread* th) { return workerStart(th, 5, _lzwInflate); }

int coreLzwInit(Thread* th, Pkg* system)
{
	Def* Lzw = pkgAddType(th, system, "_Lzw");
	Type* fun_I_Lzw = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.Int, Lzw->type);
	Type* fun_Lzw_Buf_S_I_I_B = typeAlloc(th, TYPECODE_FUN, NULL, 6,
		Lzw->type, MM.Buffer, MM.Str, MM.Int, MM.Int, MM.Boolean);

	pkgAddFun(th, system, "_lzwCreate", fun_lzwCreate, fun_I_Lzw);
	pkgAddFun(th, system, "_lzwDeflate", fun_lzwDeflate, fun_Lzw_Buf_S_I_I_B);
	pkgAddFun(th, system, "_lzwInflate", fun_lzwInflate, fun_Lzw_Buf_S_I_I_B);

	return 0;
}
