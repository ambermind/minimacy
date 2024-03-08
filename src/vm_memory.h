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

#define FULL64

#ifdef USE_MEMORY_C
void cMallocInit();
void* bmmMalloc(long long size);
void bmmFree(void* block);
#define VM_MALLOC bmmMalloc
#define VM_FREE bmmFree
#else
#define VM_MALLOC malloc
#define VM_FREE free
#endif

#ifdef ATOMIC_32
#define LSHIFT 2
typedef char* LW;
typedef int LINT;
typedef unsigned int LUINT;
typedef float LFLOAT;

#define LSX "%x"
#define LSD "%d"
#else
#define LSHIFT 3
#ifdef DBG_MEM
typedef char* LW;
#else
typedef long long LW;
#endif
//typedef long* LW;
typedef long long LINT;
typedef unsigned long long LUINT;
typedef double LFLOAT;

#define LSX "%llx"
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


typedef struct HashSlots HashSlots;
typedef struct Pkg Pkg;
typedef struct Thread Thread;
typedef struct Def Def;
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
	LB* roots;

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
	LINT gc_totalCount;

	Pkg* system;
	LB * _true;
	LB * _false;

	LB* ansiVolume;
	LB* romdiskVolume;
	LB* uefiVolume;
	LB* partitionsFS;

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
	Type* VolumeType;
	LW MemoryException;
	Type* fun_u0_list_u0_list_u0;
	Type* fun_array_u0_I_u0;

	Def* bigAdd;
	Def* bigSub;
	Def* bigMul;
	Def* bigDiv;
	Def* bigExp;
	Def* bigAddMod;
	Def* bigSubMod;
	Def* bigMulMod;
	Def* bigDivMod;
	Def* bigExpMod;

	Def* bigMulModBarrett;
	Def* bigDivModBarrett;
	Def* bigExpModBarrett;
	Def* bigModBarrett;

	Def* bigMod;
	Def* bigNeg;
	Def* bigNegMod;
	Def* bigGT;
	Def* bigGE;
	Def* bigLT;
	Def* bigLE;

	LB* funStart;
	int gcTrace;
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

#define TYPE_BINARY 0
#define TYPE_TAB 1
#define TYPE_NATIVE 2
#define TYPE_STRING 3

#define _USEFUL (LB*)(2)
#define _USELESS (LB*)(3)

#define MEMORYMARK(from,p) {if ((p) && ((p)->lifo==MM.USELESS)) {(p)->lifo=MM.lifo; MM.lifo=(p); } }

#define NATIVE_FORGET 0
#define NATIVE_MARK 1


// structure for a list
#define LIST_LENGTH 2
#define LIST_VAL 0
#define LIST_NXT 1

#define HEADER_TYPE(p) (((p)->sizetype)&3)
#define HEADER_SIZE(p) (((p)->sizetype)>>2)
#define HEADERSETSIZETYPE(p,size,type) ((p)->sizetype=((size<<2)+(type&3)))
#define HEADER_DBG(p) ((p)->data[0])

#ifdef DBG_MEM
int DBGISPNT(LW v);
LW PNTTOVAL(LB* p);
LW INTTOVAL(LINT i);
LW FLOATTOVAL(LFLOAT v);

LB* VALTOPNT(LW v);
LINT VALTOINT(LW v);
LFLOAT VALTOFLOAT(LW v);
#else
#ifdef FULL64
#define DBGISPNT(v) (!(((LINT)(v))&1))

#define PNTTOVAL(p) ((LW)(p))
#define INTTOVAL(i) ((LW)(i))
#define FLOATTOVAL(v) ((LW)*(LINT*)(&(v)))

#define VALTOPNT(v) ((LB*)(v))
#define VALTOINT(v) ((LINT)(v))
#define VALTOFLOAT(v) (*((LFLOAT*)(&(v))))
#else
#define DBGISPNT(v) (((LINT)v)&1)

#define PNTTOVAL(p) ((LW)((LINT)(1+((LINT)(p)))))
#define INTTOVAL(i) ((LW)((LINT)(((i)<<2)|2)))
#define FLOATTOVAL(v) ((LW)((LINT)(((~3)&(*(LINT*)(&(v)))))))

#define VALTOPNT(v) ((LB*)(((LINT)v)-1))
#define VALTOINT(v) (((LINT)((LINT)v))>>2)
#define VALTOFLOAT(v) (*((LFLOAT*)(&(v))))
#endif
#endif

#ifdef FULL64
#define VAL_TYPE_PNT 0
#define VAL_TYPE_INT 1
#define VAL_TYPE_FLOAT 2

#define DBGTOVAL(x) INTTOVAL((LINT)(2*(x)+1))
#define NIL ((LW)0)	// nil

#define TABTYPE(p,i) ((char*)p)[sizeof(LB)+HEADER_SIZE(p)+(i)]
#define TABSETTYPE(p,i,v,type) \
{ \
	LW memVal=(LW)v; \
	LINT memI=i;	\
	char memType=type;	\
	((char*)p)[sizeof(LB)+HEADER_SIZE(p)+ memI]= memType; \
	(p)->data[1+ memI]=memVal; \
 	if (memType ==VAL_TYPE_PNT) MEMORYMARK(p,VALTOPNT(memVal)); \
}
#define TABSETTYPE_NOMARK(p,i,v,type) \
{ \
	LINT memI=i;	\
	((char*)p)[sizeof(LB)+HEADER_SIZE(p)+ memI]= type; \
	(p)->data[1+ memI]=v; \
}
#else
#define VAL_TYPE_FLOAT 0
#define VAL_TYPE_PNT 1
#define VAL_TYPE_INT 2

#define DBGTOVAL(x) INTTOVAL(x)
#define NIL ((LW)1)	// nil

#define TABTYPE(p,i) (((LINT)(p)->data[1+(i)])&3)
#define TABSETTYPE(p,i,v,type) \
{ \
	LINT memVal=(LINT)v; \
	int memType=type; \
	memVal=(memVal&-4)|type;	\
	(p)->data[1+(i)]=(LW)memVal; \
 	if (memType==VAL_TYPE_PNT) MEMORYMARK(p,VALTOPNT((LW)memVal)); \
}
#define TABSETTYPE_NOMARK(p,i,v,type) \
{ \
	(p)->data[1+(i)]=v; \
}
#endif

#define TABSETPNT(dst,i,v) TABSETTYPE(dst,i,PNTTOVAL(v),VAL_TYPE_PNT)
#define TABSETNIL(dst,i) TABSETTYPE_NOMARK(dst,i,PNTTOVAL(NULL),VAL_TYPE_PNT)

#define TABISPNT(p,i) (TABTYPE(p,i)==VAL_TYPE_PNT)
#define TABISNIL(p,i) (TABISPNT(p,i) && !TABPNT(p,i))
#define TABLEN(p) (HEADER_SIZE(p)>>LSHIFT)
#define TABGET(p,i)	((p)->data[1+(i)])
#define TABINT(p,i)	VALTOINT(TABGET(p,i))
#define TABPNT(p,i)	VALTOPNT(TABGET(p,i))
#define TABFLOAT(p,i) VALTOFLOAT(TABGET(p,i))
#define TABSETINT(p,i,val) TABSETTYPE_NOMARK(p,i,INTTOVAL(val),VAL_TYPE_INT)
#define TABSETFLOAT(p,i,val) TABSETTYPE_NOMARK(p,i,FLOATTOVAL(val),VAL_TYPE_FLOAT)
#define TABSETBOOL(p,i,val) TABSETTYPE_NOMARK(p,i,PNTTOVAL((val)?MM._true:MM._false),VAL_TYPE_PNT)
#define TABCOPY(p,i,q,j) {LINT memJ=j; TABSETTYPE(p,i,TABGET(q,memJ),TABTYPE(q,memJ))}
#define TABINTERNALCOPY(p,i,j) {LINT memJ=j; TABSETTYPE_NOMARK(p,i,TABGET(p,memJ),TABTYPE(p,memJ))}
#define BINSTART(p) ((char*)(&((p)->data[1])))
#define BINLEN(p) HEADER_SIZE(p)

#define STRSTART(p) BINSTART(p)
#define STRLEN(p) (HEADER_SIZE(p)-1)

#define DBG_STACK DBGTOVAL(0)
#define DBG_THREAD DBGTOVAL(1)
#define DBG_LIST DBGTOVAL(2)
#define DBG_TUPLE DBGTOVAL(3)
#define DBG_S DBGTOVAL(4)
#define DBG_BUFFER DBGTOVAL(5)
#define DBG_BIN DBGTOVAL(6)
#define DBG_HASHMAP DBGTOVAL(7)
#define DBG_ARRAY DBGTOVAL(8)
#define DBG_HASH_LIST_LIST DBGTOVAL(9)
#define DBG_TYPE DBGTOVAL(10)
#define DBG_DEF DBGTOVAL(11)
#define DBG_PKG DBGTOVAL(12)
#define DBG_LOCALS DBGTOVAL(13)
#define DBG_BYTECODE DBGTOVAL(14)
#define DBG_LAMBDA DBGTOVAL(15)
#define DBG_IMPORTS DBGTOVAL(16)
#define DBG_B DBGTOVAL(17)
#define DBG_FIFO DBGTOVAL(18)
#define DBG_FUN DBGTOVAL(19)
#define DBG_SOCKET DBGTOVAL(20)
#define DBG_MEMCOUNT DBGTOVAL(21)
#define DBG_HASHSET DBGTOVAL(22)

void memoryInit(int argc, char** argv);
int memoryEnd(void);
LB* memoryPrintBuffer(Thread* th, Buffer* buffer);
void memoryGC(LINT period);
void memoryFullGC(void);
void memoryCheck(LB* p);

Mem* memCreate(Thread* th, Mem* parent, LINT maxBytes, LB* name);
void memCountDecrement(Mem* mem);

#define NEED_FAST_ALLOC(th) if (!memoryGetFast()) memoryEnterFast();
void memoryEnterFast(void); // after this, newly allocated blocks cannot be GCized until memoryLeaveFast
void memoryLeaveFast(void);
LINT memoryGetFast(void);

int memoryAddRoot(LB * root);

LB* memoryAllocTable(Thread * th, LINT size, LW dbg);
LB* memoryAllocExt(Thread * th, LINT sizeofExt,LW dbg,FORGET forget,MARK mark);
LB* memoryAllocStr(Thread * th, char* src, LINT size);
LB* memoryAllocBin(Thread * th, char* src, LINT size, LW dbg);
LB* memoryAllocFromBuffer(Thread * th, Buffer * b);

void memoryTake(Mem * m, LB * p);
int memoryIsMainThread(void);

char* errorName(int err);
#endif
