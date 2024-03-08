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
#include"minimacy.h"

//#define DOUBLE_FORMAT "%.10g"
#define DOUBLE_FORMAT "%g"

int _bufferBiggerFinalize(Thread* th, Buffer* b, LINT newsize)
{
	LB* newbuffer = memoryAllocBin(th, NULL, newsize + 1, DBG_BIN);
	if (!newbuffer) return EXEC_OM;
	memcpy(BINSTART(newbuffer), b->buffer, b->size + 1);
	b->bloc = newbuffer;
	b->buffer = BINSTART(b->bloc);
	b->size = newsize;
	return 0;
}
int _bufferBigger(Thread* th,Buffer* b,LINT neededSize)
{
	LINT newsize = b->size;
	neededSize += b->index;
	while (newsize <= neededSize)
	{
		if (newsize < BUFFER_DOUBLE_UNTIL) newsize *= 2;
		else newsize = neededSize + BUFFER_FINAL_INCREMENT;
	}
//	PRINTF(LOG_DEV,"bigger buffer %lld -> %lld (mainthread? %d)\n", b->size, newsize, memoryIsMainThread());
	if (memoryIsMainThread()) return _bufferBiggerFinalize(th, b, newsize);

	if (workerBiggerBuffer(th, b, newsize)) return EXEC_OM;
	return 0;
}

void bufferMark(LB* user)
{
	Buffer* buffer=(Buffer*)user;
	MEMORYMARK(user,buffer->bloc);
}

Buffer* bufferCreateWithSize(Thread* th, LINT size)
{
	Buffer* b;
	if (size < BUFFER_SIZE0) size = BUFFER_SIZE0;
	memoryEnterFast();
	b=(Buffer*)memoryAllocExt(th, sizeof(Buffer),DBG_BUFFER,NULL,bufferMark); if (!b) return NULL;
	b->index=0;
	b->size=size;
	b->bloc = NULL;	// required because the following memoryAlloc may fire a GC
	b->bloc= memoryAllocBin(th, NULL,size+1,DBG_BIN);
	if (!b->bloc) return NULL;
	b->buffer=BINSTART(b->bloc);
	memoryLeaveFast();
	return b;
}
Buffer* bufferCreate(Thread* th)
{
	return bufferCreateWithSize(th, 0);
}
void bufferReinit(Buffer* b)
{
	b->index=0;
}
int bufferAddChar(Thread* th, Buffer* b,char c)
{
	int k;
	if ((b->index>=b->size)&&(k= _bufferBigger(th,b,0))) return k;
	b->buffer[b->index++]=c;
	return 0;
}
int bufferAddZero(Thread* th, Buffer* b,LINT n)
{
	int k;
	if ((b->index+n>= b->size) && (k = _bufferBigger(th,b, n))) return k;
	while(n--) b->buffer[b->index++]=0;
	return 0;
}

int bufferAddInt(Thread* th, Buffer* b,LINT i)
{
	int k;
	int j;
	for(j=0;j<sizeof(LINT);j++)
	{
		if ((k = bufferAddChar(th, b, (char)i))) return k;
		i>>=8;
	}
	return 0;
}
int bufferAddIntN(Thread* th, Buffer* b,LINT i,LINT n)
{
	int k;
	while(n--)
	{
		if((k=bufferAddChar(th, b,(char)i))) return k;
		i>>=8;
	}
	return 0;
}
void bufferSetChar(Buffer* b,LINT index,char c)
{
	b->buffer[index]=c;
}
char bufferGetChar(Buffer* b, LINT index)
{
	if (index < 0) index += b->index;
	if (index >= 0 && index < b->index) return b->buffer[index];
	return 0;
}
void bufferDelete(Buffer* b, LINT index, LINT remove)
{
	LINT i;
	if (index + remove >= b->index)
	{
		b->index = index;
		return;
	}
	for (i = index + remove; i < b->index; i++) b->buffer[i - remove] = b->buffer[i];
	b->index -= remove;
}
void bufferSetInt(Buffer* b,LINT index,LINT i)
{
	int j;
	for(j=0;j<sizeof(LINT);j++)
	{
		bufferSetChar(b,index++,(char)i);
		i>>=8;
	}
}
void bufferSetIntN(Buffer* b,LINT index,LINT i,LINT n)
{
	while(n--)
	{
		bufferSetChar(b,index++,(char)i);
		i>>=8;
	}
}
char* bufferSkip(Thread* th, Buffer* b, LINT len)
{
	char* result;
	if (len < 0) return NULL;
	while (b->index + len >= b->size) if (_bufferBigger(th,b, len)) return NULL;
	result = b->buffer + b->index;
	b->index += len;
	return result;
}

int bufferAddBin(Thread* th, Buffer* b,char *src,LINT len)
{
	int k;
	if (len < 0) len = strlen(src);
	while(b->index+len>=b->size) if ((k=_bufferBigger(th,b,len))) return k;
	memcpy(b->buffer+b->index,src,len);
	b->index+=len;
	return 0;
}

int bufferAddStr(Thread* th, Buffer* b, char* src)
{
	return bufferAddBin(th, b, src, strlen(src));
}

int bufferVPrintf(Thread* th, Buffer* b,char *format, va_list arglist)
{
	LINT sizeout;
	va_list copy;

	va_copy(copy,arglist);
	// vsnprintf : arg=max bytes, 0 included | return needed bytes, 0 NOT included
	while(((sizeout=vsnprintf(b->buffer+b->index,b->size-b->index+1,format,copy))<0)||(sizeout>b->size-b->index))
	{
		int k=_bufferBigger(th,b,sizeout);
		if (k) return k;
		va_end(copy);
		va_copy(copy, arglist);
	}
	va_end(copy);
	b->index+=sizeout;
	return 0;
}
int bufferPrintf(Thread* th, Buffer* b, char* format, ...)
{
	va_list arglist;
	int k;
	va_start(arglist, format);
	k=bufferVPrintf(th, b, format, arglist);
	va_end(arglist);
	return k;
}
void bufferCut(Buffer* b,LINT len)
{
	if ((len>=0)&&(len<b->index)) b->index=len;
}
LINT bufferSize(Buffer* b)
{
	return b->index;
}
char* bufferStart(Buffer* b)
{
	b->buffer[b->index] = 0;
	return b->buffer;
}

int _bufferJoin(Thread* th, Buffer* b, LB* join, int* first)
{
	if (!join) return 0;
	if (*first) {
		*first = 0;
		return 0;
	}
	return bufferAddBin(th, b, STRSTART(join), STRLEN(join));
}
int _bufferItem(Thread* th, Buffer* b, LW v, int type, LB* join, int rec, int* first)
{
	int k;
	if (type==VAL_TYPE_PNT)
	{
		LW dbg;
		LB* p = VALTOPNT(v);
		if (!p) return 0;
		dbg = HEADER_DBG(p);
		if (DBGISPNT(dbg))
		{
			Def* defType = (Def*)VALTOPNT(dbg);
			if (defType->code == DEF_CODE_CONS0) return _bufferJoin(th, b,join, first)||bufferAddStr(th, b,STRSTART(defType->name));
			return 0;
		}
		if (dbg == DBG_S) return _bufferJoin(th, b, join, first) || bufferAddBin(th, b, STRSTART(p), STRLEN(p));
		if (dbg == DBG_B)
		{
			int len= bignumToStr(th, (bignum)p,NULL);
			char* dst;
			if ((k=_bufferJoin(th, b, join, first))) return k;
			dst= bufferSkip(th, b, len); if (!dst) return EXEC_OM;
			bignumToStr(th, (bignum)p, dst);
			return 0;
		}
		if (dbg == DBG_TUPLE)
		{
			LINT j;
			LINT nb = TABLEN(p);
			for (j = 0; j < nb; j++) if ((k = _bufferItem(th, b, TABGET(p, j), TABTYPE(p, j), join, rec, first))) return k;
			return 0;
		}
		if (dbg == DBG_ARRAY)
		{
			LINT i,j;
			LINT nb = TABLEN(p);
			for (i = 0; i < rec; i++) if (STACKPNT(MM.tmpStack, i) == p) return bufferAddStr(th, b, "[loop]");

			STACKPUSHPNT_ERR(MM.tmpStack, p, EXEC_OM);	// this is done to check the loops
			for (j = 0; j < nb; j++) if ((k = _bufferItem(th, b, TABGET(p, j), TABTYPE(p, j), join, rec + 1, first))) return k;
			STACKDROP(MM.tmpStack);

			return 0;
		}
		if (dbg == DBG_DEF)
		{
			Def* d = (Def*)p;
			if (d->code == DEF_CODE_CONS0) return _bufferJoin(th, b, join, first) || bufferAddStr(th, b, STRSTART(d->name));
			return 0;
		}
		if (dbg == DBG_LIST)
		{
			while (p)
			{
				if ((k = _bufferItem(th, b, TABGET(p, LIST_VAL), TABTYPE(p, LIST_VAL), join, rec, first))) return k;
				p = (TABPNT(p, LIST_NXT));
			}
			return 0;
		}
		if (dbg == DBG_FIFO)
		{
			p = (TABPNT(p, FIFO_START));
			while (p)
			{
				if ((k = _bufferItem(th, b, TABGET(p, LIST_VAL), TABTYPE(p, LIST_VAL), join, rec, first))) return k;
				p = (TABPNT(p, LIST_NXT));
			}
			return 0;
		}
		if (dbg == DBG_TYPE)
		{
			Type* t = (Type*)p;
			return _bufferJoin(th, b, join, first) || typeBuffer(th, b, t);
		}
		if (p == MM._true) return _bufferJoin(th, b, join, first) || bufferAddStr(th, b, "true");
		if (p == MM._false) return _bufferJoin(th, b, join, first) || bufferAddStr(th, b, "false");
		return 0;
	}
	if (type==VAL_TYPE_INT) return _bufferJoin(th, b, join, first) || bufferPrintf(th, b,LSD, VALTOINT(v));
	if (type==VAL_TYPE_FLOAT) return _bufferJoin(th, b, join, first) || bufferPrintf(th, b, DOUBLE_FORMAT, VALTOFLOAT(v));
	return 0;
}
int bufferItem(Thread* th, Buffer* b, LW v, int type, LB* join)
{
	int first = 1;
	return _bufferItem(th, b, v, type, join, 0, &first);
}

int bufferFormat(Buffer* b, Thread* th, LINT argc)
{
	LINT i,fLen;
	char* f;

	LB* format = STACKPNT(th, argc);
	if (!format) FUN_RETURN_NIL;
	f = STRSTART(format);
	fLen = STRLEN(format);
	bufferReinit(b);
	for (i = 0; i < fLen; i++)
	{
		if (f[i] == '*')
		{
			int k;
			argc--;
			if ((argc>=0)&&((k=bufferItem(th, b, STACKGET(th, argc), STACKTYPE(th, argc),NULL)))) return k;
		} 
		else bufferAddChar(th, b, f[i]);
	}
	FUN_RETURN_BUFFER(b);
}