// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
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
	LINT sp;
	LINT allocSize;
	LINT w;
	LINT h;
	MSEM* sem;
	Buffer* buffer;
	LW result;
	int type;
	int OM;
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
	LINT forceOpcode;

	volatile Thread** link;

	Thread* listNext;

	Worker worker;

	LB* user;
	LB* error;

	LINT sp;
	LB* stack;
	char atomic;
};

Thread* threadCreate(LINT stackLen);
int threadBigger(Thread* th);
void stackReset(Thread* th);

int stackPushEmptyArray(Thread* th,LINT size,LW dbg);
int stackPushFilledArray(Thread* th,LINT size,LW dbg);
int stackPushStr(Thread* th,char* src,LINT size);
int stackSetStr(Thread * th, LINT i, char* src, LINT size);
int stackSetBuffer(Thread * th, LINT i, Buffer * b);

void threadPrintCallstack(Thread* t);

int systemThreadInit(Pkg *system);

#define THREAD_STACK_LENGTH0 256
#define THREAD_STACK_STEP (256*16)

#define THREAD_OPCODE_NONE (-1)

#define EXEC_IDLE 0
#define EXEC_PREEMPTION 1
#define EXEC_WAIT 2
#define EXEC_EXIT 3
#define EXEC_OM 4

#define EXEC_FORMAT 5


#define STACK_BLOCK(th) ((th)->stack)
#define STACK_INDEX(th,i) ((th)->sp-(i))
#define STACK_TYPE(th,i) ARRAY_TYPE(STACK_BLOCK(th), STACK_INDEX(th,i))
#define STACK_GET(th,i) ARRAY_GET(STACK_BLOCK(th), STACK_INDEX(th,i))
#define STACK_IS_NIL(th,i) ARRAY_IS_NIL(STACK_BLOCK(th), STACK_INDEX(th,i))
#define STACK_IS_PNT(th,i) ARRAY_IS_PNT(STACK_BLOCK(th), STACK_INDEX(th,i))
#define STACK_INT(th,i) ARRAY_INT(STACK_BLOCK(th), STACK_INDEX(th,i))
#define STACK_PNT(th,i) ARRAY_PNT(STACK_BLOCK(th), STACK_INDEX(th,i))
#define STACK_BOOL(th,i) ((ARRAY_PNT(STACK_BLOCK(th), STACK_INDEX(th,i))==MM._true)?1:0)
#define STACK_FLOAT(th,i) ARRAY_FLOAT(STACK_BLOCK(th), STACK_INDEX(th,i))
#define STACK_SET_TYPE(th,i,val,type) ARRAY_SET_TYPE(STACK_BLOCK(th), STACK_INDEX(th,i),val,type)
#define STACK_SET_PNT(th,i,val) ARRAY_SET_PNT(STACK_BLOCK(th), STACK_INDEX(th,i),val)
#define STACK_SET_INT(th,i,val) ARRAY_SET_INT(STACK_BLOCK(th), STACK_INDEX(th,i),val)
#define STACK_SET_FLOAT(th,i,val) ARRAY_SET_FLOAT(STACK_BLOCK(th), STACK_INDEX(th,i),val)
#define STACK_SET_BOOL(th,i,val) ARRAY_SET_PNT(STACK_BLOCK(th), STACK_INDEX(th,i),(val)?MM._true:MM._false)
#define STACK_SET_NIL(th,i) ARRAY_SET_NIL(STACK_BLOCK(th), STACK_INDEX(th,i))

#define STACK_COPY(th,i,thFrom,iFrom) ARRAY_COPY(STACK_BLOCK(th), STACK_INDEX(th,i), STACK_BLOCK(thFrom), STACK_INDEX(thFrom,iFrom))
#define STACK_STORE(p,i,thFrom,iFrom) ARRAY_COPY(p,i, STACK_BLOCK(thFrom), STACK_INDEX(thFrom,iFrom))
#define STACK_LOAD(th,i,tbFrom,iFrom) ARRAY_COPY(STACK_BLOCK(th), STACK_INDEX(th,i),tbFrom,iFrom)
#define STACK_INTERNAL_COPY(th,i,iFrom) ARRAY_INTERNAL_COPY(STACK_BLOCK(th), STACK_INDEX(th,i), STACK_INDEX(th,iFrom))
#define STACK_SKIP(th,n) if (n) { STACK_INTERNAL_COPY(th, n, 0);	(th)->sp -= (n);}


#define STACK_PULL_INT(th) ARRAY_INT(STACK_BLOCK(th),(th)->sp--)
#define STACK_PULL_FLOAT(th) ARRAY_FLOAT(STACK_BLOCK(th),(th)->sp--)
#define STACK_PULL_PNT(th) ARRAY_PNT(STACK_BLOCK(th),(th)->sp--)
#define STACK_DROP(th) ((th)->sp--)
#define STACK_DROPN(th,n) ((th)->sp-=(n))

#define STACK_REF(th) ((th)->sp)
#define STACK_DEF_INDEX(def,i) ((def)-(i))
#define STACK_REF_INT(th,def,i) ARRAY_INT(STACK_BLOCK(th), STACK_DEF_INDEX(def,i))
#define STACK_REF_PNT(th,def,i) ARRAY_PNT(STACK_BLOCK(th), STACK_DEF_INDEX(def,i))
#define STACK_REF_SET_PNT(th,def,i,val) ARRAY_SET_PNT(STACK_BLOCK(th), STACK_DEF_INDEX(def,i), val)
#define STACK_COPY_FROM_REF(th,j,def,i) ARRAY_INTERNAL_COPY(STACK_BLOCK(th), STACK_INDEX(th,j), STACK_DEF_INDEX(def,i))
#define STACK_COPY_TO_REF(th,def,i,j) ARRAY_INTERNAL_COPY(STACK_BLOCK(th), STACK_DEF_INDEX(def,i), STACK_INDEX(th,j))

#define STACK_PUSH_NIL_ERR(th,err) { \
	ARRAY_SET_NIL(STACK_BLOCK(th),(++(th)->sp)); \
	if (((th)->sp >= ARRAY_LENGTH(STACK_BLOCK(th)) - 1)&&threadBigger(th)) return err; \
}
#define _STACK_PUSH_ERR(arraySet, th,val,err) { \
	arraySet(STACK_BLOCK(th),(++(th)->sp),val); \
	if (((th)->sp >= ARRAY_LENGTH(STACK_BLOCK(th)) - 1)&&threadBigger(th)) return err; \
}
#define STACK_PUSH_INT_ERR(th,val,err) _STACK_PUSH_ERR(ARRAY_SET_INT,th,val,err)
#define STACK_PUSH_FLOAT_ERR(th,val,err) _STACK_PUSH_ERR(ARRAY_SET_FLOAT, th,val,err)
#define STACK_PUSH_PNT_ERR(th,val,err) _STACK_PUSH_ERR(ARRAY_SET_PNT, th,val,err)
#define STACK_PUSH_BOOL_ERR(th,val,err) _STACK_PUSH_ERR(ARRAY_SET_BOOL, th,val,err)

#define STACK_PUSH_EMPTY_ARRAY_ERR(th,size,dbg,err) if (stackPushEmptyArray(th,size,dbg)) return err;
#define STACK_PUSH_FILLED_ARRAY_ERR(th,size,dbg,err) if (stackPushFilledArray(th,size,dbg)) return err;
#define STACK_PUSH_STR_ERR(th,src,size,err) if (stackPushStr(th,src,size)) return err;

#define FUN_PUSH_NIL STACK_PUSH_NIL_ERR(th,EXEC_OM)
#define FUN_PUSH_INT(val) STACK_PUSH_INT_ERR(th,val,EXEC_OM)
#define FUN_PUSH_FLOAT(val) STACK_PUSH_FLOAT_ERR(th,val,EXEC_OM)
#define FUN_PUSH_PNT(val) STACK_PUSH_PNT_ERR(th,val,EXEC_OM)
#define FUN_PUSH_BOOL(val) STACK_PUSH_BOOL_ERR(th,val,EXEC_OM)
#define FUN_MAKE_ARRAY(size,dbg) STACK_PUSH_FILLED_ARRAY_ERR(th,size,dbg,EXEC_OM)
#define FUN_PUSH_STR(src,size) STACK_PUSH_STR_ERR(th,src,size,EXEC_OM)

#define FUN_RETURN_NIL { STACK_SET_NIL(th, 0); return 0;}
#define FUN_RETURN_INT(value) { STACK_SET_INT(th, 0,value); return 0;}
#define FUN_RETURN_FLOAT(value) { STACK_SET_FLOAT(th, 0,value); return 0;}
#define FUN_RETURN_PNT(value) { STACK_SET_PNT(th, 0,value); return 0;}
#define FUN_RETURN_TRUE { STACK_SET_PNT(th, 0,MM._true); return 0;}
#define FUN_RETURN_FALSE { STACK_SET_PNT(th, 0,MM._false); return 0;}
#define FUN_RETURN_BOOL(value) { STACK_SET_BOOL(th, 0,value); return 0;}
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

#define TMP_PUSH(p,err) if(MM.tmpStack) STACK_PUSH_PNT_ERR(MM.tmpStack,(LB*)(p),err)
#define TMP_PULL() if(MM.tmpStack) STACK_DROP(MM.tmpStack)