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
#ifndef _THREAD_
#define _THREAD_

extern LINT ThreadCounter;

#define WORKER_READY 0
#define WORKER_RUN 1
#define WORKER_DONE 2
#define WORKER_ALLOC_EXT 3
#define WORKER_BIGGER_BUFFER 4

typedef struct {
	volatile LINT state;
	LINT pp;
	LINT allocSize;
	LINT w;
	LINT h;
	MSEM sem;
	Buffer* buffer;
	LW result;
	int type;
	LW dbg;
	FORGET forget;
	MARK mark;
} Worker;

struct Thread
{
	LB header;
	FORGET forget;
	MARK mark;

	LINT uid;

	LINT count;
	LINT callstack;
	LINT pc;
	LB* fun;
	char forceOpcode;
	char OM;	// 0 until Out of Memory error
	char atomic;

	Mem* memDelegate;

	Thread* listNext;

	Worker worker;

	LB* user;

	LINT pp;
	LB* stack;
};

Thread* threadCreate(Thread* th, Mem* mem);
int threadBigger(Thread* th);
void stackReset(Thread* th);

int stackPushTable(Thread* th,LINT size,LW dbg);
int stackMakeTable(Thread* th,LINT size,LW dbg);
int stackPushStr(Thread* th,char* src,LINT size);
int stackSetStr(Thread * th, LINT i, char* src, LINT size);
int stackSetBuffer(Thread * th, LINT i, Buffer * b);

void threadPrintCallstack(Thread* t);

int coreThreadInit(Thread* th, Pkg *system);

#define THREAD_STACK_LENGTH0 1024
#define THREAD_STACK_STEP (1024*16)

#define THREAD_OPCODE_NONE ((char)-1)

#define EXEC_IDLE 0
#define EXEC_PREEMPTION 1
#define EXEC_WAIT 2
#define EXEC_EXIT 3
#define EXEC_OM 4

#define EXEC_FORMAT 5


#define STACKTAB(th) ((th)->stack)
#define STACKINDEX(th,i) ((th)->pp-(i))
#define STACKTYPE(th,i) TABTYPE(STACKTAB(th), STACKINDEX(th,i))
#define STACKGET(th,i) TABGET(STACKTAB(th), STACKINDEX(th,i))
#define STACKISNIL(th,i) TABISNIL(STACKTAB(th), STACKINDEX(th,i))
#define STACKISPNT(th,i) TABISPNT(STACKTAB(th), STACKINDEX(th,i))
#define STACKINT(th,i) TABINT(STACKTAB(th), STACKINDEX(th,i))
#define STACKPNT(th,i) TABPNT(STACKTAB(th), STACKINDEX(th,i))
#define STACKBOOL(th,i) ((TABPNT(STACKTAB(th), STACKINDEX(th,i))==MM._true)?1:0)
#define STACKFLOAT(th,i) TABFLOAT(STACKTAB(th), STACKINDEX(th,i))
#define STACKSETTYPE(th,i,val,type) TABSETTYPE(STACKTAB(th), STACKINDEX(th,i),val,type)
#define STACKSETPNT(th,i,val) TABSETPNT(STACKTAB(th), STACKINDEX(th,i),val)
#define STACKSETINT(th,i,val) TABSETINT(STACKTAB(th), STACKINDEX(th,i),val)
#define STACKSETFLOAT(th,i,val) TABSETFLOAT(STACKTAB(th), STACKINDEX(th,i),val)
#define STACKSETBOOL(th,i,val) TABSETPNT(STACKTAB(th), STACKINDEX(th,i),(val)?MM._true:MM._false)
#define STACKSETNIL(th,i) TABSETNIL(STACKTAB(th), STACKINDEX(th,i))

#define STACKCOPY(th,i,thFrom,iFrom) TABCOPY(STACKTAB(th), STACKINDEX(th,i), STACKTAB(thFrom), STACKINDEX(thFrom,iFrom))
#define STACKSTORE(p,i,thFrom,iFrom) TABCOPY(p,i, STACKTAB(thFrom), STACKINDEX(thFrom,iFrom))
#define STACKLOAD(th,i,tbFrom,iFrom) TABCOPY(STACKTAB(th), STACKINDEX(th,i),tbFrom,iFrom)
#define STACKINTERNALCOPY(th,i,iFrom) TABINTERNALCOPY(STACKTAB(th), STACKINDEX(th,i), STACKINDEX(th,iFrom))
#define STACKSKIP(th,n) if (n) { STACKINTERNALCOPY(th, n, 0);	(th)->pp -= (n);}


#define STACKPULLINT(th) TABINT(STACKTAB(th),(th)->pp--)
#define STACKPULLFLOAT(th) TABFLOAT(STACKTAB(th),(th)->pp--)
#define STACKPULLPNT(th) TABPNT(STACKTAB(th),(th)->pp--)
#define STACKDROP(th) ((th)->pp--)
#define STACKDROPN(th,n) ((th)->pp-=(n))

#define STACKREF(th) ((th)->pp)
#define STACKDEF_INDEX(def,i) ((def)-(i))
#define STACKREFINT(th,def,i) TABINT(STACKTAB(th), STACKDEF_INDEX(def,i))
#define STACKREFPNT(th,def,i) TABPNT(STACKTAB(th), STACKDEF_INDEX(def,i))
#define STACKREFSETPNT(th,def,i,val) TABSETPNT(STACKTAB(th), STACKDEF_INDEX(def,i), val)
#define STACKCOPYFROMREF(th,j,def,i) TABINTERNALCOPY(STACKTAB(th), STACKINDEX(th,j), STACKDEF_INDEX(def,i))
#define STACKCOPYTOREF(th,def,i,j) TABINTERNALCOPY(STACKTAB(th), STACKDEF_INDEX(def,i), STACKINDEX(th,j))

#define STACKPUSHNIL_ERR(th,err) { \
	TABSETNIL(STACKTAB(th),(++(th)->pp)); \
	if (((th)->pp >= TABLEN(STACKTAB(th)) - 1)&&threadBigger(th)) return err; \
}
#define _STACKPUSH_ERR(tabset, th,val,err) { \
	tabset(STACKTAB(th),(++(th)->pp),val); \
	if (((th)->pp >= TABLEN(STACKTAB(th)) - 1)&&threadBigger(th)) return err; \
}
#define STACKPUSHINT_ERR(th,val,err) _STACKPUSH_ERR(TABSETINT,th,val,err)
#define STACKPUSHFLOAT_ERR(th,val,err) _STACKPUSH_ERR(TABSETFLOAT, th,val,err)
#define STACKPUSHPNT_ERR(th,val,err) _STACKPUSH_ERR(TABSETPNT, th,val,err)
#define STACKPUSHBOOL_ERR(th,val,err) _STACKPUSH_ERR(TABSETBOOL, th,val,err)

#define STACKPUSHTABLE_ERR(th,size,dbg,err) if (stackPushTable(th,size,dbg)) return err;
#define STACKMAKETABLE_ERR(th,size,dbg,err) if (stackMakeTable(th,size,dbg)) return err;
#define STACKPUSHSTR_ERR(th,src,size,err) if (stackPushStr(th,src,size)) return err;

#define FUN_PUSH_NIL STACKPUSHNIL_ERR(th,EXEC_OM)
#define FUN_PUSH_INT(val) STACKPUSHINT_ERR(th,val,EXEC_OM)
#define FUN_PUSH_FLOAT(val) STACKPUSHFLOAT_ERR(th,val,EXEC_OM)
#define FUN_PUSH_PNT(val) STACKPUSHPNT_ERR(th,val,EXEC_OM)
#define FUN_PUSH_BOOL(val) STACKPUSHBOOL_ERR(th,val,EXEC_OM)
#define FUN_MAKE_TABLE(size,dbg) STACKMAKETABLE_ERR(th,size,dbg,EXEC_OM)
#define FUN_PUSH_STR(src,size) STACKPUSHSTR_ERR(th,src,size,EXEC_OM)

#define FUN_RETURN_NIL { STACKSETNIL(th, 0); return 0;}
#define FUN_RETURN_INT(value) { STACKSETINT(th, 0,value); return 0;}
#define FUN_RETURN_FLOAT(value) { STACKSETFLOAT(th, 0,value); return 0;}
#define FUN_RETURN_PNT(value) { STACKSETPNT(th, 0,value); return 0;}
#define FUN_RETURN_TRUE { STACKSETPNT(th, 0,MM._true); return 0;}
#define FUN_RETURN_FALSE { STACKSETPNT(th, 0,MM._false); return 0;}
#define FUN_RETURN_BOOL(value) { STACKSETBOOL(th, 0,value); return 0;}
#define FUN_RETURN_STR(p,len) { return stackSetStr(th, 0, p, len);}
#define FUN_RETURN_BUFFER(b) { return stackSetBuffer(th, 0, b);}

#define _COMMON_SUBSTR(src,start,len,lenIsNil,srcLen,abort) \
	if (!src) abort;	\
	if (start < 0) start += srcLen;	\
	if (len < 0) len += srcLen;	\
	if (lenIsNil) len = srcLen;	\
	if (start + len > srcLen) len = srcLen - start;	\
	if ((start < 0) || (len < 0)) abort;

#define FUN_SUBSTR(src,start,len,lenIsNil,srcLen) _COMMON_SUBSTR(src,start,len,lenIsNil,srcLen,FUN_RETURN_NIL)
#define WORKER_SUBSTR(src,start,len,lenIsNil,srcLen) _COMMON_SUBSTR(src,start,len,lenIsNil,srcLen,return workerDoneNil(th))

#define FUN_CHECK_CONTAINS(p,start,len,srcLen) \
	if (!p) FUN_RETURN_NIL;	\
	if (start < 0) start += srcLen;	\
	if ((start < 0) || (start + len > srcLen)) FUN_RETURN_NIL;

#define FUN_CROP_LENGTH(p,len,srcLen) \
	if (!p) FUN_RETURN_NIL;	\
	if (len < 0) len += srcLen;	\
	if (len < 0) FUN_RETURN_NIL;	\
	if (len >= srcLen) len = srcLen;

#define FUN_COPY_CROP(dst,index,dstLen, src,start,len,lenIsNil,srcLen) \
	if ((!src)||(!dst)) FUN_RETURN_PNT(dst);	\
	if (start < 0) start += srcLen;	\
	if (len < 0) len += srcLen;	\
	if (lenIsNil) len = srcLen;	\
	if (start + len > srcLen) len = srcLen - start;	\
	if (index < 0) index += dstLen;	\
	if (index + len > dstLen) len = dstLen - index;	\
	if ((index < 0) || (start < 0) || (len <= 0)) FUN_RETURN_PNT(dst);


#endif
