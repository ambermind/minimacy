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

#define USE_ALL_BITS

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
typedef long long LW;
typedef long long LINT;
typedef unsigned long long LUINT;
typedef double LFLOAT;

#define LSX "%llx"
#define LSD "%lld"
#endif

#ifdef USE_MEMORY_C
void cMallocInit();
void* bmmMalloc(LINT size);
void bmmFree(void* block);
void bmmRebuildTree(void);
void bmmDump(void);
void bmmMayday(void);
LINT bmmReservedMem(void);

void bmmSetTotalSize(LINT size);
char* bmmAllocForEver(LINT size);
extern LINT BmmTotalSize;
extern LINT BmmMaxSize;
extern LINT BmmTotalFree;
LINT bmmCompact(void);
#define VM_MALLOC bmmMalloc
#define VM_FREE bmmFree

typedef struct BMM BMM;

struct BMM {
	LINT sizeAndType;
	BMM* left;
	BMM* right;
	BMM* next;
};

#else
#ifdef ON_ESP32
void* VM_MALLOC(long long x);
void VM_FREE(void* x);
#else
#define VM_MALLOC malloc
#define VM_FREE free
#endif
#endif

#define LWLEN sizeof(LW)
#define LWLEN_MASK ((LWLEN)-1)

#define VLSB true

#ifdef VLSB
#define LSBW(x) ((x)&0xffff)
#define MSBW(x) ((((x)>>8)&255)+(((x)&255)<<8))
#define LSB24(x) ((x)&0xffffff)
#define MSB24(x) ((((x)>>16)&0xff)+((x)&0xff00)+(((x)&0xff)<<16))
#define LSBL(x) ((x)&0xffffffff)
#define MSBL(x) ((((x)>>24)&0xff)+((((x)>>8)&0xff00)+(((x)&0xff00)<<8))+(((x)&0xff)<<24))
#define LSB64(x) (x)
#define MSB64(x) (((x&0xff)<<56)|((x&0xff00)<<40)|((x&0xff0000)<<24)|((x&0xff000000)<<8)|((x>>8)&0xff000000)|((x>>24)&0xff0000)|((x>>40)&0xff00)|((x>>56)&0xff))
#else
#define LSBW(x) ((((x)>>8)&255)+(((x)&255)<<8))
#define MSBW(x) ((x)&0xffff)
#define LSB24(x) ((((x)>>16)&0xff)+((x)&0xff00)+(((x)&0xff)<<16))
#define MSB24(x) ((x)&0xffffff)
#define LSBL(x) ((((x)>>24)&0xff)+((((x)>>8)&0xff00)+(((x)&0xff00)<<8))+(((x)&0xff)<<24))
#define MSBL(x) ((x)&0xffffffff)
#define LSB64(x) (((x&0xff)<<56)|((x&0xff00)<<40)|((x&0xff0000)<<24)|((x&0xff000000)<<8)|((x>>8)&0xff000000)|((x>>24)&0xff0000)|((x>>40)&0xff00)|((x>>56)&0xff))
#define MSB64(x) (x)
#endif


typedef struct HashSlots HashSlots;
typedef struct Pkg Pkg;
typedef struct Thread Thread;
typedef struct Def Def;
typedef struct Buffer Buffer;
typedef struct Type Type;
typedef struct LB LB;
typedef struct Oblivion Oblivion;

typedef struct Mem Mem;
typedef struct Locals Locals;
typedef struct Compiler Compiler;
typedef struct Imports Imports;

typedef int (*FORGET)(LB*);
typedef void (*MARK)(LB*);

struct LB
{
	LINT sizeAndType;
	LB *lifo;
	LB *nextBlock;
	Pkg* pkg;
	LW data[1];
};

struct Oblivion
{
	LB header;
	FORGET forget;
	MARK mark;

	LB* f;	// function to call when oblivioned
	Oblivion* listNext;	// list to watch by GC, no mark
	Oblivion* popNext;	// list of oblivioned function to call
};

typedef struct {
	MTHREAD_ID mainThread;
	Thread* scheduler;
	Thread* tmpStack;
	MLOCK lock;
	LB* USEFUL;
	LB* USELESS;
	LB* roots;
	LB* tmpRoot;
	Oblivion* popOblivions;
	Oblivion* listOblivions;

	LINT fastAlloc;
	LINT step;	// GC step (0/1/2)
	LB *lifo;	// list of blocks to mark (step 0)
	LB *listBlocks;
	LB *listCheck;
	LB *listFast;
	LINT updating;

	Thread* listThreads;
	Pkg* listPkgs;
	Pkg* currentPkg;

	LINT blocs_nb;
	LINT blocs_length;

	LINT gc_nb0;
	LINT gc_free;
	LINT gc_count;
	LINT gc_period;
	LINT gc_period_counter;
	LINT gc_period_time;

	Pkg* system;
	LB * _true;
	LB * _false;
	LB* _loopMark;
	LB* _throwMark;

	LB* ansiVolume;
	LB* romdiskVolume;
	LB* partitionsFS;

	Buffer* tmpBuffer;
	LB* args;

	Type* Int;
	Type* Float;
	Type* Str;
	Type* Bytes;
	Type* Boolean;
	Type* BigNum;
	Type* Package;
	Type* Exception;
	Type* Type;

	Type* fun_u0_list_u0_list_u0;
	Type* fun_array_u0_I_u0;

	LINT bigAdd;
	LINT bigSub;
	LINT bigMul;
	LINT bigDiv;
	LINT bigExp;
	LINT bigAddMod;
	LINT bigSubMod;
	LINT bigMulMod;
	LINT bigDivMod;
	LINT bigExpMod;

	LINT bigMulModBarrett;
	LINT bigDivModBarrett;
	LINT bigExpModBarrett;
	LINT bigModBarrett;

	LINT bigMod;
	LINT bigNeg;
	LINT bigNegMod;
	LINT bigGT;
	LINT bigGE;
	LINT bigLT;
	LINT bigLE;

	LB* funStart;
	int gcTrace;
	int reboot;
	int OM;
}Memory;

extern Memory MM;

typedef int (*NATIVE)(Thread*);

#define GC_PERIOD_START 20
#define GC_PERIOD_COUNT 1000

#ifndef MEMORY_SAFE_SIZE
#define MEMORY_SAFE_SIZE (1024*1024*16)
#endif

#define TYPE_FREE 0
#define TYPE_ARRAY 1
#define TYPE_NATIVE 2
#define TYPE_BINARY 3

#define _USEFUL (LB*)(2)
#define _USELESS (LB*)(3)

#define BLOCK_MARK(p) { if ((p) && (((LB*)(p))->lifo==MM.USELESS)) {((LB*)(p))->lifo=MM.lifo; MM.lifo=(LB*)(p); } }

#ifdef USE_MEMORY_C
int checkPointer(LINT p);
LINT relativePointer(LB* p);
int checkListPointer(char* title, LB* p);

#define MEMORY_MARK(p) { if (p) {	\
		if (MM.updating) *(LB**)(&(p)) = ((LB*)(p))->lifo; \
		else if (((LB*)(p))->lifo == MM.USELESS) {((LB*)(p))->lifo = MM.lifo; MM.lifo = (LB*)(p); } \
} }
#else
#define MEMORY_MARK(p) { if (p) {	\
		if (MM.updating) *(LB**)(&(p)) = ((LB*)(p))->lifo; \
		else if (((LB*)(p))->lifo == MM.USELESS) {((LB*)(p))->lifo = MM.lifo; MM.lifo = (LB*)(p); } \
} }
#endif

#define NATIVE_FORGET 0
#define NATIVE_MARK 1


// structure for a list
#define LIST_LENGTH 2
#define LIST_VAL 0
#define LIST_NXT 1

#define HEADER_TYPE(p) (((p)->sizeAndType)&3)
#define HEADER_SIZE(p) (((p)->sizeAndType)>>2)
#define HEADER_SET_SIZE_AND_TYPE(p,size,type) ((p)->sizeAndType=(((size)<<2)+((type)&3)))
#define HEADER_DBG(p) ((p)->data[0])
#ifdef USE_ALL_BITS
#define BLOCK_TOTAL_MEMORY(type,size) \
	(sizeof(LB) + (((size)+ LWLEN_MASK)&~LWLEN_MASK)+\
	(((type) != TYPE_ARRAY)?0:(((size) >> LSHIFT) + LWLEN_MASK) &~LWLEN_MASK))


#else
#define BLOCK_TOTAL_MEMORY(type,size) (sizeof(LB) + (((size)+ LWLEN_MASK)&~LWLEN_MASK))
#endif


#ifdef USE_ALL_BITS
#define NUMBER_RESERVED_BITS 0
#define DBG_IS_PNT(v) (!(((LINT)(v))&1))

#define VAL_FROM_PNT(p) ((LW)(p))
#define VAL_FROM_INT(i) ((LW)(i))
#define VAL_FROM_FLOAT(v) ((LW)*(LINT*)(&(v)))

#define PNT_FROM_VAL(v) ((LB*)(v))
#define INT_FROM_VAL(v) ((LINT)(v))
#define FLOAT_FROM_VAL(v) (*((LFLOAT*)(&(v))))

#define VAL_TYPE_PNT 0
#define VAL_TYPE_INT 1
#define VAL_TYPE_FLOAT 2

#define VAL_FROM_DEBUG(x) VAL_FROM_INT((LINT)(2*(x)+1))
#define NIL ((LW)0)	// nil

#define ARRAY_TYPE(p,i) ((char*)p)[sizeof(LB)+HEADER_SIZE(p)+(i)]
#define ARRAY_SET_TYPE(p,i,v,type) \
{ \
	LW memVal=(LW)v; \
	LINT memI=i;	\
	char memType=type;	\
	((char*)p)[sizeof(LB)+HEADER_SIZE(p)+ memI]= memType; \
	(p)->data[1+ memI]=memVal; \
 	if (memType ==VAL_TYPE_PNT) BLOCK_MARK(PNT_FROM_VAL(memVal)); \
}
#define ARRAY_SET_TYPE_NO_MARK(p,i,v,type) \
{ \
	LINT memI=i;	\
	((char*)p)[sizeof(LB)+HEADER_SIZE(p)+ memI]= type; \
	(p)->data[1+ memI]=v; \
}
#else
#define NUMBER_RESERVED_BITS 2
#define DBG_IS_PNT(v) (((LINT)v)&1)

#define VAL_FROM_PNT(p) ((LW)((LINT)(1+((LINT)(p)))))
#define VAL_FROM_INT(i) ((LW)((LINT)(((i)<<2)|2)))
#define VAL_FROM_FLOAT(v) ((LW)((LINT)(((~3)&(*(LINT*)(&(v)))))))

#define PNT_FROM_VAL(v) ((LB*)(((LINT)v)-1))
#define INT_FROM_VAL(v) (((LINT)((LINT)v))>>2)
#define FLOAT_FROM_VAL(v) (*((LFLOAT*)(&(v))))

#define VAL_TYPE_FLOAT 0
#define VAL_TYPE_PNT 1
#define VAL_TYPE_INT 2

#define VAL_FROM_DEBUG(x) VAL_FROM_INT(x)
#define NIL ((LW)1)	// nil

#define ARRAY_TYPE(p,i) (((LINT)(p)->data[1+(i)])&3)
#define ARRAY_SET_TYPE(p,i,v,type) \
{ \
	LINT memVal=(LINT)v; \
	int memType=type; \
	memVal=(memVal&-4)|type;	\
	(p)->data[1+(i)]=(LW)memVal; \
 	if (memType==VAL_TYPE_PNT) BLOCK_MARK(PNT_FROM_VAL((LW)memVal)); \
}
#define ARRAY_SET_TYPE_NO_MARK(p,i,v,type) \
{ \
	(p)->data[1+(i)]=v; \
}

#endif


#define ARRAY_SET_PNT(dst,i,v) ARRAY_SET_TYPE(dst,i,VAL_FROM_PNT(v),VAL_TYPE_PNT)
#define ARRAY_SET_NIL(dst,i) ARRAY_SET_TYPE_NO_MARK(dst,i,VAL_FROM_PNT(NULL),VAL_TYPE_PNT)

#define ARRAY_IS_PNT(p,i) (ARRAY_TYPE(p,i)==VAL_TYPE_PNT)
#define ARRAY_IS_NIL(p,i) (ARRAY_IS_PNT(p,i) && !ARRAY_PNT(p,i))
#define ARRAY_LENGTH(p) (HEADER_SIZE(p)>>LSHIFT)
#define ARRAY_GET(p,i)	((p)->data[1+(i)])
#define ARRAY_INT(p,i)	INT_FROM_VAL(ARRAY_GET(p,i))
#define ARRAY_PNT(p,i)	PNT_FROM_VAL(ARRAY_GET(p,i))
#define ARRAY_FLOAT(p,i) FLOAT_FROM_VAL(ARRAY_GET(p,i))
#define ARRAY_SET_INT(p,i,val) ARRAY_SET_TYPE_NO_MARK(p,i,VAL_FROM_INT(val),VAL_TYPE_INT)
#define ARRAY_SET_FLOAT(p,i,val) ARRAY_SET_TYPE_NO_MARK(p,i,VAL_FROM_FLOAT(val),VAL_TYPE_FLOAT)
#define ARRAY_SET_BOOL(p,i,val) ARRAY_SET_TYPE_NO_MARK(p,i,VAL_FROM_PNT((val)?MM._true:MM._false),VAL_TYPE_PNT)
#define ARRAY_COPY(p,i,q,j) {LINT memJ=j; ARRAY_SET_TYPE(p,i,ARRAY_GET(q,memJ),ARRAY_TYPE(q,memJ))}
#define ARRAY_INTERNAL_COPY(p,i,j) {LINT memJ=j; ARRAY_SET_TYPE_NO_MARK(p,i,ARRAY_GET(p,memJ),ARRAY_TYPE(p,memJ))}
#define BIN_START(p) ((char*)(&((p)->data[1])))
#define BIN_LENGTH(p) HEADER_SIZE(p)

#define STR_START(p) BIN_START(p)
#define STR_LENGTH(p) (HEADER_SIZE(p)-1)

#define DBG_STACK VAL_FROM_DEBUG(0)
#define DBG_THREAD VAL_FROM_DEBUG(1)
#define DBG_LIST VAL_FROM_DEBUG(2)
#define DBG_TUPLE VAL_FROM_DEBUG(3)
#define DBG_S VAL_FROM_DEBUG(4)
#define DBG_BUFFER VAL_FROM_DEBUG(5)
#define DBG_BIN VAL_FROM_DEBUG(6)
#define DBG_HASHMAP VAL_FROM_DEBUG(7)
#define DBG_ARRAY VAL_FROM_DEBUG(8)
#define DBG_HASH_LIST_LIST VAL_FROM_DEBUG(9)
#define DBG_TYPE VAL_FROM_DEBUG(10)
#define DBG_DEF VAL_FROM_DEBUG(11)
#define DBG_PKG VAL_FROM_DEBUG(12)
#define DBG_LOCALS VAL_FROM_DEBUG(13)
#define DBG_BYTECODE VAL_FROM_DEBUG(14)
#define DBG_LAMBDA VAL_FROM_DEBUG(15)
#define DBG_IMPORTS VAL_FROM_DEBUG(16)
#define DBG_B VAL_FROM_DEBUG(17)
#define DBG_FIFO VAL_FROM_DEBUG(18)
#define DBG_FUN VAL_FROM_DEBUG(19)
#define DBG_SOCKET VAL_FROM_DEBUG(20)
#define DBG_HASHSET VAL_FROM_DEBUG(21)
#define DBG_OBLIVION VAL_FROM_DEBUG(22)

void memoryInit(int argc, const char** argv);
int memoryEnd(void);
LB* memoryPrintBuffer(Thread* th, Buffer* buffer);
void memoryGC(LINT period);
void memoryFinalizeGC(void);
void memoryRecount(void);

void memoryEnterFast(void); // after this, newly allocated blocks cannot be GCized until memoryLeaveFast
void memoryLeaveFast(void);
LINT memoryGetFast(void);

int memoryAddRoot(LB * root);
void memorySetTmpRoot(LB* p);

LB* memoryAllocArray(LINT size, LW dbg);
LB* memoryAllocExt(LINT sizeofExt,LW dbg,FORGET forget,MARK mark);
LB* memoryAllocStr(char* src, LINT size);
LB* memoryAllocBin(char* src, LINT size, LW dbg);
LB* memoryAllocFromBuffer(Buffer * b);

int memoryTry(LINT size);

int memoryIsMainThread(void);

char* errorName(int err);
#endif
