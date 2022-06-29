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
#ifndef _CORE_MEMORY_
#define _CORE_MEMORY_

#ifdef ATOMIC_32
#define LSHIFT 2
typedef long* LW;
typedef int LINT;
typedef float LFLOAT;

#define LSX "%x"
#define LSD "%d"
#else
#define LSHIFT 3
typedef long* LW;
typedef long long LINT;
typedef double LFLOAT;

#define LSX "%llX"
#define LSD "%lld"
#endif

#define LWLEN sizeof(LW)

#define VLSB true

#ifdef VLSB
#define LSBW(x) ((x)&0xffff)
#define MSBW(x) ((((x)>>8)&255)+(((x)&255)<<8))
#define LSB24(x) ((x)&0xffffff)
#define MSB24(x) ((((x)>>16)&0xff)+((x)&0xff00)+(((x)&0xff)<<16))
#define LSBL(x) ((x)&0xffffffff)
#define MSBL(x) ((((x)>>24)&0xff)+((((x)>>8)&0xff00)+(((x)&0xff00)<<8))+(((x)&0xff)<<24))
#else
#define LSBW(x) ((((x)>>8)&255)+(((x)&255)<<8))
#define MSBW(x) ((x)&0xffff)
#define LSB24(x) ((((x)>>16)&0xff)+((x)&0xff00)+(((x)&0xff)<<16))
#define MSB24(x) ((x)&0xffffff)
#define LSBL(x) ((((x)>>24)&0xff)+((((x)>>8)&0xff00)+(((x)&0xff00)<<8))+(((x)&0xff)<<24))
#define MSBL(x) ((x)&0xffffffff)
#endif


typedef struct Hashmap Hashmap;
typedef struct Pkg Pkg;
typedef struct Thread Thread;
typedef struct Ref Ref;
typedef struct Buffer Buffer;
typedef struct Type Type;
typedef struct LB LB;
typedef struct Mem Mem;
typedef struct Locals Locals;
typedef struct Compiler Compiler;
typedef struct Imports Imports;

typedef int (*FORGET)(LB*);
typedef void (*MARK)(LB*);

struct LB
{
	LINT sizetype;
	LB *lifo;
	LB *nextBlock;
	Mem* mem;
	LW data[1];
};

struct Mem
{
	LB header;
	FORGET forget;
	MARK mark;

	LB* name;
	LINT count;	// reference counter of blocks (threads and other)
	LINT bytes;
	LINT maxBytes;
	Mem* listNext;
};


typedef struct {
	MTHREAD_ID mainThread;
	Thread* scheduler;
	Thread* tmpStack;
	LB* USEFUL;
	LB* USELESS;

	LINT fastAlloc;
	LINT step;	// GC phasis (0/1/2)
	LB *lifo;	// list of blocks to mark (phasis 0)
	LB *listBlocks;
	LB *listCheck;
	LB *listFast;

	Mem* mem;	// default mem structure for allocation
	MLOCK lock;

	Thread* listThreads;
	Mem* listMems;
	Pkg* listPkgs;

	LINT blocs_nb;
	LINT blocs_length;

	LINT gc_period;
	LINT gc_nb0;
	LINT gc_length0;
	LINT gc_time;
	LINT gc_free;
	LINT gc_count;

	Pkg* system;
	LW trueRef;
	LW falseRef;
	Buffer* userBuffer;
	Buffer* errBuffer;
	Buffer* tmpBuffer;
	LB* args;

	Type* I;
	Type* F;
	Type* S;
	Type* Bytes;
	Type* Boolean;
	Type* BigNum;
	Type* Bitmap;
	Type* Buffer;
	Type* Pkg;
	Type* Exception;
	Type* Type;
	Type* Thread;
	LW MemoryException;
	Type* fun_u0_list_u0_list_u0;
	Type* fun_array_u0_I_u0;

	Ref* bigAdd;
	Ref* bigSub;
	Ref* bigMul;
	Ref* bigDiv;
	Ref* bigExp;
	Ref* bigAddMod;
	Ref* bigSubMod;
	Ref* bigMulMod;
	Ref* bigDivMod;
	Ref* bigExpMod;

	Ref* bigMulModBarrett;
	Ref* bigDivModBarrett;
	Ref* bigExpModBarrett;
	Ref* bigModBarrett;

	Ref* bigMod;
	Ref* bigNeg;
	Ref* bigNegMod;
	Ref* bigGT;
	Ref* bigGE;
	Ref* bigLT;
	Ref* bigLE;

	LB* funStart;
	int gcConsole;
	int reboot;
	int OM;
}Memory;

extern Memory MM;
extern int gcTRON;

typedef int (*NATIVE)(Thread*);


#define MEM_BY_TID_SIZE (1<<8)	// should be a power of 2
#define MEM_LIST_NB 3
#define MEM_LIST_KEY 0
#define MEM_LIST_VAL 1
#define MEM_LIST_NEXT 2

#define GC_PERIOD 20

#define NIL (LW)(1)	// nil
#define ISVALPNT(v) (((LINT)v)&1)
#define ISVALINT(v) ((((LINT)v)&3)==2)
#define ISVALFLOAT(v) ((((LINT)v)&3)==0)


#define TYPE_BINARY 0
#define TYPE_TAB 1
#define TYPE_NATIVE 2
#define TYPE_STRING 3

#define _USEFUL (LB*)(2)
#define _USELESS (LB*)(3)

#ifdef DBG_MEM_CHECK
void memoryAlert(LB* from, LB* to);
extern LB* MemoryCheck;
#define MEMORYMARK(from,p) {if ((p) && (p)==MemoryCheck) memoryAlert(from,p); if ((p) && ((p)->lifo==MM.USELESS)) {(p)->lifo=MM.lifo; MM.lifo=(p); } }
#else
#define MEMORYMARK(from,p) {if ((p) && ((p)->lifo==MM.USELESS)) {(p)->lifo=MM.lifo; MM.lifo=(p); } }
#endif

#define MEMORYSET(dst,i,v) \
{ \
	LW memVal=v; \
	dst->data[i+1]=memVal; \
	if (ISVALPNT(memVal)) MEMORYMARK(dst,VALTOPNT(memVal)); \
}

#define NATIVE_FORGET 1
#define NATIVE_MARK 2


// structure for a list
#define LIST_LENGTH 2
#define LIST_VAL 0
#define LIST_NXT 1

#define HEADER_TYPE(p) (((p)->sizetype)&3)
#define HEADER_SIZE(p) (((p)->sizetype)>>2)
#define HEADERSETSIZETYPE(p,size,type) ((p)->sizetype=((size<<2)+(type&3)))
#define HEADER_DBG(p) ((p)->data[0])

#define PNTTOVAL(p) ((LW)((LINT)(1+((LINT)(p)))))
#define INTTOVAL(i) ((LW)((LINT)(((i)<<2)|2)))
#define FLOATTOVAL(v) ((LW)((LINT)(((~3)&(*(LINT*)(&(v)))))))

#define VALTOPNT(v) ((LB*)(((LINT)v)-1))
#define VALTOINT(v) (((LINT)((LINT)v))>>2)
#define VALTOFLOAT(v) (*((LFLOAT*)(&(v))))

#define TABGET(p,i)	((p)->data[1+i])
#define TABGETINT(p,i)	(VALTOINT(TABGET(p,i)))
#define TABSET(p,i,val) MEMORYSET(p,i,val)	// set the i-th value of a table
#define TABSETINT(p,i,val) ((p)->data[1+i]=INTTOVAL(val))	// set the i-th value of a table
#define TABSETFLOAT(p,i,val) ((p)->data[1+i]=FLOATTOVAL(val))	// set the i-th value of a table
#define TABSETNIL(p,i) ((p)->data[1+i]=NIL)	// set the i-th value of a table
#define TABSETSAFE(p,i,val) ((p)->data[1+i]=(val))	// idem TABSET, use only when val is not a pointer (NIL or integer or float)
#define TABSTART(p) (&((p)->data[1]))
#define TABLEN(p) (HEADER_SIZE(p)>>LSHIFT)

#define BINSTART(p) ((char*)(&((p)->data[1])))
#define BINLEN(p) HEADER_SIZE(p)	

#define STRSTART(p) ((char*)&((p)->data[1]))
#define STRLEN(p) (HEADER_SIZE(p)-1)

#define DBG_STACK INTTOVAL(0)
#define DBG_THREAD INTTOVAL(1)
#define DBG_LIST INTTOVAL(2)
#define DBG_TUPLE INTTOVAL(3)
#define DBG_S INTTOVAL(4)
#define DBG_BUFFER INTTOVAL(5)
#define DBG_BIN INTTOVAL(6)
#define DBG_HASHSET INTTOVAL(7)
#define DBG_ARRAY INTTOVAL(8)
#define DBG_HASH_LIST_LIST INTTOVAL(9)
#define DBG_TYPE INTTOVAL(10)
#define DBG_REF INTTOVAL(11)
#define DBG_PKG INTTOVAL(12)
#define DBG_LOCALS INTTOVAL(13)
#define DBG_BYTECODE INTTOVAL(14)
#define DBG_LAMBDA INTTOVAL(15)
#define DBG_IMPORTS INTTOVAL(16)
#define DBG_B INTTOVAL(17)
#define DBG_FIFO INTTOVAL(18)
#define DBG_FUN INTTOVAL(19)
#define DBG_SOCKET INTTOVAL(20)
#define DBG_MEMCOUNT INTTOVAL(21)

void memoryInit(int argc, char** argv);
int memoryEnd(void);
void memoryUserBufferize(int val);
void memoryErrBufferize(int val);
LB* memoryPrintBuffer(Thread* th, Buffer* buffer);
void memoryGC(LINT period);
void memoryFullGC(void);
void memoryCheck(LB* p);

Mem* memCreate(Thread* th, Mem* parent, LINT maxBytes, LB* name);
void memCountDecrement(Mem* mem);

#define NEED_FAST_ALLOC(th) if (!memoryGetFast(th)) memoryEnterFast(th);
void memoryEnterFast(Thread * th); // after this, newly allocated blocks cannot be GCized until memoryLeaveFast
void memoryLeaveFast(Thread * th);
LINT memoryGetFast(Thread * th);

LINT memoryTest(Mem * mem, LINT total);
LB* memoryAllocTable(Thread * th, LINT size, LW dbg);
LB* memoryAllocExt(Thread * th, LINT sizeofExt,LW dbg,FORGET forget,MARK mark);
LB* memoryAllocStr(Thread * th, char* src, LINT size);
LB* memoryAllocBin(Thread * th, char* src, LINT size, LW dbg);
LB* memoryAllocFromBuffer(Thread * th, Buffer * b);

void memoryTake(Mem * m, LB * p);
int memoryIsMainThread(void);

char* errorName(int err);
#endif
