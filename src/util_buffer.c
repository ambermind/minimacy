// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

//#define DOUBLE_FORMAT "%.10g"
#define DOUBLE_FORMAT "%g"

int _bufferBiggerFinalize(Buffer* b, LINT newsize)
{
	LB* newbuffer = memoryAllocBin(NULL, newsize + 1, DBG_BIN);
	if (!newbuffer) return EXEC_OM;
	memcpy(BIN_START(newbuffer), b->buffer, b->size + 1);
	b->bloc = newbuffer;
	b->buffer = BIN_START(b->bloc);
	b->size = newsize;
	return 0;
}
int _bufferBigger(Buffer* b,LINT neededSize)
{
	LINT newsize = b->size;
	neededSize += b->index;
//	PRINTF(LOG_DEV, "_bufferBigger "LSX" current "LSD" needs "LSD" \n", (void*)b, b->size, neededSize);
	while (newsize <= neededSize)
	{
		if (newsize < BUFFER_DOUBLE_UNTIL) newsize = newsize * 3/2;
		else newsize = neededSize + BUFFER_FINAL_INCREMENT;
	}
//	PRINTF(LOG_DEV,"bigger buffer %lld -> %lld (mainthread? %d)\n", b->size, newsize, memoryIsMainThread());
	if (memoryIsMainThread()) return _bufferBiggerFinalize(b, newsize);
	if (workerBiggerBuffer(b->pth, b, newsize)) return EXEC_OM;
	return 0;
}

void bufferMark(LB* user)
{
	Buffer* b=(Buffer*)user;
	MARK_OR_MOVE(b->bloc);
	if (MOVING_BLOCKS) {
		b->buffer = BIN_START(b->bloc);
		if (b->link) *b->link = (Buffer*)b->header.listMark;
	}
}

Buffer* bufferCreateWithSize(LINT size)
{
	Buffer* b;
	LB* p;
	if (size < BUFFER_SIZE0) size = BUFFER_SIZE0;
	memoryEnterSafe();
	b=(Buffer*)memoryAllocNative(sizeof(Buffer),DBG_BUFFER,NULL,bufferMark); if (!b) return NULL;
	b->index=0;
	b->size=size;
	b->pth=NULL;
	b->link = NULL;
	b->bloc = NULL;	// required because the following memoryAlloc may fire a GC
	p= memoryAllocBin(NULL,size+1,DBG_BIN); if (!p) return NULL;
	b->bloc = p;
	b->buffer=BIN_START(b->bloc);
//	PRINTF(LOG_DEV, "_create Buffer "LSX" size "LSD"\n", (void*)b, b->size);
	memoryLeaveSafe();
	return b;
}
Buffer* bufferCreate(void)
{
	return bufferCreateWithSize(0);
}
void bufferSetWorkerThread(volatile Buffer** pb, volatile Thread** pth)
{
	(*pb)->link = pb;
	(*pb)->pth = pth;
	(*pth)->link = pth;
}
void bufferUnsetWorkerThread(volatile Buffer** pb, volatile Thread** pth)
{
	(*pb)->link = NULL;
	(*pb)->pth = NULL;
	(*pth)->link = NULL;
}

void bufferReinit(Buffer* b)
{
	b->index=0;
}
void bufferRemove(Buffer* b, int delta)
{
	if (delta<=0) return;
	b->index-=delta;
	if (b->index<0) b->index=0;
}
int bufferAddCharWorker(volatile Buffer** pb, char c)
{
	int k;
	Buffer* b = (Buffer*)(*pb);
	if ((b->index >= b->size) && (k = _bufferBigger(b, 1))) return k;
	b = (Buffer*)(*pb);
	b->buffer[b->index++] = c;
	return 0;
}

int bufferAddChar(Buffer* b,char c)
{
	int k;
	if ((b->index>=b->size)&&(k= _bufferBigger(b,1))) return k;
	b->buffer[b->index++]=c;
	return 0;
}
int bufferAddZero(Buffer* b,LINT n)
{
	int k;
	if ((b->index+n>= b->size) && (k = _bufferBigger(b, n))) return k;
	while(n--) b->buffer[b->index++]=0;
	return 0;
}

int bufferAddInt(Buffer* b,LINT i)
{
	int k;
	int j;
	for(j=0;j<sizeof(LINT);j++)
	{
		if ((k = bufferAddChar(b, (char)i))) return k;
		i>>=8;
	}
	return 0;
}
int bufferAddIntN(Buffer* b,LINT i,LINT n)
{
	int k;
	while(n--)
	{
		if((k=bufferAddChar(b,(char)i))) return k;
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

LINT bufferGetIntN(Buffer* b, LINT index, LINT n)
{
	LINT result = 0;
	while (n--) result = (result << 8) + ((unsigned char)b->buffer[index + n]);
	return result;
}

char* bufferRequireWorker(volatile Buffer** pb, LINT len)
{
	char* result;
	Buffer* b = (Buffer*)(*pb);
	if (len < 0) return NULL;
	while (b->index + len >= b->size) {
		if (_bufferBigger(b, len)) return NULL;
		b = (Buffer*)(*pb);
	}
	result = b->buffer + b->index;
	b->index += len;
	return result;
}

char* bufferRequire(Buffer* b, LINT len)
{
	char* result;
	if (len < 0) return NULL;
	while (b->index + len >= b->size) if (_bufferBigger(b, len)) return NULL;
	result = b->buffer + b->index;
	b->index += len;
	return result;
}

int bufferAddBinWorker(volatile Buffer** pb, char* src, LINT len)
{
	int k;
	Buffer* b = (Buffer*)(*pb);
	if (len < 0) len = strlen(src);
	while (b->index + len >= b->size) {
		if ((k = _bufferBigger(b, len))) return k;
		b = (Buffer*)(*pb);
	}
	memcpy(b->buffer + b->index, src, len);
	b->index += len;
	return 0;
}
int bufferAddBin(Buffer* b,char *src,LINT len)
{
	int k;
	if (len < 0) len = strlen(src);
	while(b->index+len>=b->size) if ((k=_bufferBigger(b,len))) return k;
	memcpy(b->buffer+b->index,src,len);
	b->index+=len;
	return 0;
}

int bufferAddStr(Buffer* b, char* src)
{
	return bufferAddBin(b, src, strlen(src));
}

int bufferVPrintf(Buffer* b,char *format, va_list arglist)
{
	LINT sizeout;
	va_list copy;

	va_copy(copy,arglist);
	// vsnprintf : arg=max bytes, 0 included | return needed bytes, 0 NOT included
	while(((sizeout=vsnprintf(b->buffer+b->index,b->size-b->index+1,format,copy))<0)||(sizeout>b->size-b->index))
	{
		int k=_bufferBigger(b,sizeout);
		if (k) return k;
		va_end(copy);
		va_copy(copy, arglist);
	}
	va_end(copy);
	b->index+=sizeout;
	return 0;
}
int bufferPrintf(Buffer* b, char* format, ...)
{
	va_list arglist;
	int k;
	va_start(arglist, format);
	k=bufferVPrintf(b, format, arglist);
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

int _bufferJoin(Buffer* b, LB* join, int* first)
{
	if (!join) return 0;
	if (*first) {
		*first = 0;
		return 0;
	}
	return bufferAddBin(b, STR_START(join), STR_LENGTH(join));
}
int _bufferItem(Buffer* b, LW v, int type, LB* join, int rec, int* first)
{
	int k;
	if (type==VAL_TYPE_PNT)
	{
		LW dbg;
		LB* p = PNT_FROM_VAL(v);
		if (!p) return 0;
		dbg = HEADER_DBG(p);
		if (DBG_IS_PNT(dbg))
		{
			Def* defType = (Def*)PNT_FROM_VAL(dbg);
			if (defType->code == DEF_CODE_CONS0) return _bufferJoin(b,join, first)||bufferAddStr(b,STR_START(defType->name));
			return 0;
		}
		if (dbg == DBG_S) return _bufferJoin(b, join, first) || bufferAddBin(b, STR_START(p), STR_LENGTH(p));
		if (dbg == DBG_B)
		{
			TMP_PUSH(p,EXEC_OM);
			if ((k=_bufferJoin(b, join, first))) return k;
			if ((k = bignumDecToBuffer((bignum)p, b))) return k;
			TMP_PULL();
			return 0;
		}
		if (dbg == DBG_TUPLE)
		{
			LINT j;
			LINT nb = ARRAY_LENGTH(p);
			for (j = 0; j < nb; j++) if ((k = _bufferItem(b, ARRAY_GET(p, j), ARRAY_TYPE(p, j), join, rec, first))) return k;
			return 0;
		}
		if (dbg == DBG_ARRAY)
		{
			LINT i,j;
			LINT nb = ARRAY_LENGTH(p);
			for (i = 0; i < rec; i++) if (STACK_PNT(MM.tmpStack, i) == p) return bufferAddStr(b, "[loop]");

			STACK_PUSH_PNT_ERR(MM.tmpStack, p, EXEC_OM);	// this is done to check the loops
			for (j = 0; j < nb; j++) if ((k = _bufferItem(b, ARRAY_GET(p, j), ARRAY_TYPE(p, j), join, rec + 1, first))) return k;
			STACK_DROP(MM.tmpStack);

			return 0;
		}
		if (dbg == DBG_DEF)
		{
			Def* d = (Def*)p;
			if (d->code == DEF_CODE_CONS0) return _bufferJoin(b, join, first) || bufferAddStr(b, STR_START(d->name));
			return 0;
		}
		if (dbg == DBG_LIST)
		{
			while (p)
			{
				if ((k = _bufferItem(b, ARRAY_GET(p, LIST_VAL), ARRAY_TYPE(p, LIST_VAL), join, rec, first))) return k;
				p = (ARRAY_PNT(p, LIST_NXT));
			}
			return 0;
		}
		if (dbg == DBG_FIFO)
		{
			p = (ARRAY_PNT(p, FIFO_START));
			while (p)
			{
				if ((k = _bufferItem(b, ARRAY_GET(p, LIST_VAL), ARRAY_TYPE(p, LIST_VAL), join, rec, first))) return k;
				p = (ARRAY_PNT(p, LIST_NXT));
			}
			return 0;
		}
		if (dbg == DBG_TYPE)
		{
			Type* t = (Type*)p;
			return _bufferJoin(b, join, first) || typeBuffer(b, t);
		}
		if (p == MM._true) return _bufferJoin(b, join, first) || bufferAddStr(b, "true");
		if (p == MM._false) return _bufferJoin(b, join, first) || bufferAddStr(b, "false");
		return 0;
	}
	if (type==VAL_TYPE_INT) return _bufferJoin(b, join, first) || bufferPrintf(b,LSD, INT_FROM_VAL(v));
	if (type==VAL_TYPE_FLOAT) return _bufferJoin(b, join, first) || bufferPrintf(b, DOUBLE_FORMAT, FLOAT_FROM_VAL(v));
	return 0;
}
int bufferItem(Buffer* b, LW v, int type, LB* join)
{
	int first = 1;
	return _bufferItem(b, v, type, join, 0, &first);
}

int bufferFormat(Buffer* b, Thread* th, LINT argc)
{
	LINT i,fLen;
	char* f;

	LB* format = STACK_PNT(th, argc);
	if (!format) FUN_RETURN_NIL;
	f = STR_START(format);
	fLen = STR_LENGTH(format);
	bufferReinit(b);
	for (i = 0; i < fLen; i++)
	{
		int k;
		if (f[i] == '*')
		{
			argc--;
			if ((argc>=0)&&((k=bufferItem(b, STACK_GET(th, argc), STACK_TYPE(th, argc),NULL)))) return k;
		} 
		else if ((k=bufferAddChar(b, f[i]))) return k;
	}
	FUN_RETURN_BUFFER(b);
}