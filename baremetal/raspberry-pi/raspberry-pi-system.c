// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#include"../../src/minimacy.h"
#include"raspberry-pi.h"

void cacheInvalidate (u64 addr, int len)
{
	while (len>0)
	{
		asm volatile ("dc civac, %0" : : "r" (addr) : "memory");
		addr += 64;
		len  -= 64;
	}
	asm volatile ("dsb sy" ::: "memory");
}

int fun_read8(Thread* th)
{
	LINT val;
	LINT addr=STACK_INT(th,0);
	addr&=0xffffffff;
	val= *(u8 volatile *) addr;
	FUN_RETURN_INT(val)
}

int fun_write8(Thread* th)
{
	LINT val=STACK_INT(th,0);
	LINT addr=STACK_INT(th,1);
	addr&=0xffffffff;
	*(u8 volatile *) addr = (u8)val;
	return 0;
}

int fun_read16(Thread* th)
{
	LINT val;
	LINT addr=STACK_INT(th,0);
	addr&=0xffffffff;
	if (addr&1) FUN_RETURN_NIL;	// on arm, address must be aligned on dwords
	val= *(u16 volatile *) addr;
	FUN_RETURN_INT(val)
}

int fun_write16(Thread* th)
{
	LINT val=STACK_INT(th,0);
	LINT addr=STACK_INT(th,1);
	addr&=0xffffffff;
	if (addr&1) FUN_RETURN_NIL;	// on arm, address must be aligned on dwords
	*(u16 volatile *) addr = (u16)val;
	return 0;
}

int fun_read32(Thread* th)
{
	LINT val;
	LINT addr=STACK_INT(th,0);
	addr&=0xffffffff;
	if (addr&3) FUN_RETURN_NIL;	// on arm, address must be aligned on dwords
	val= *(u32 volatile *) addr;
	FUN_RETURN_INT(val)
}

int fun_write32(Thread* th)
{
	LINT val=STACK_INT(th,0);
	LINT addr=STACK_INT(th,1);
	addr&=0xffffffff;
	if (addr&3) FUN_RETURN_NIL;	// on arm, address must be aligned on dwords
	*(u32 volatile *) addr = (u32)val;
	return 0;
}

int fun_read8X(Thread* th)
{
	LINT val;
	LUINT addrL=STACK_INT(th,0);
	LUINT addrH=STACK_INT(th,1);
	addrL&=0xffffffff;
	addrL+=addrH<<32;
	val= *(u8 volatile *) addrL;
	FUN_RETURN_INT(val)
}
int fun_read16X(Thread* th)
{
	LINT val;
	LUINT addrL=STACK_INT(th,0);
	LUINT addrH=STACK_INT(th,1);
	addrL&=0xffffffff;
	if (addrL&1) FUN_RETURN_NIL;	// on arm, address must be aligned on dwords
	addrL+=addrH<<32;
	val= *(u16 volatile *) addrL;
	FUN_RETURN_INT(val)
}
int fun_read32X(Thread* th)
{
	LINT val;
	LUINT addrL=STACK_INT(th,0);
	LUINT addrH=STACK_INT(th,1);
	addrL&=0xffffffff;
	if (addrL&3) FUN_RETURN_NIL;	// on arm, address must be aligned on dwords
	addrL+=addrH<<32;
	val= *(u32 volatile *) addrL;
	FUN_RETURN_INT(val)
}

int fun_write32X(Thread* th)
{
	LINT val=STACK_INT(th,0);
	LUINT addrL=STACK_INT(th,1);
	LUINT addrH=STACK_INT(th,2);
	addrL&=0xffffffff;
	if (addrL&3) FUN_RETURN_NIL;	// on arm, address must be aligned on dwords
	addrL+=addrH<<32;
	*(u32 volatile *) addrL = (u32)val;
	return 0;
}

int fun_read32Repeat(Thread* th)
{
	char* p;
	u32* q;
	int nb=STACK_INT(th,0);
	int start=STACK_INT(th,1);
	LB* str=STACK_PNT(th,2);
	LINT addr=STACK_INT(th,3);
	addr&=0xffffffff;
	if (addr&3) FUN_RETURN_NIL;	// on arm, address must be aligned on dwords
	if (start&3) FUN_RETURN_NIL;	// on arm, address must be aligned on dwords
	if (!str) FUN_RETURN_NIL;
	if (start<0 || nb<=0 || (start+nb*4>STR_LENGTH(str))) FUN_RETURN_NIL;
	p=STR_START(str)+start;
	q=(u32*)p;
	while((nb--)>0) *(q++)=*(u32 volatile *) addr;
	FUN_RETURN_BOOL(1);
}
int fun_write32Repeat(Thread* th)
{
	char* p;
	u32* q;
	int nb=STACK_INT(th,0);
	int start=STACK_INT(th,1);
	LB* str=STACK_PNT(th,2);
	LINT addr=STACK_INT(th,3);
	addr&=0xffffffff;
	if (addr&3) FUN_RETURN_NIL;	// on arm, address must be aligned on dwords
	if (start&3) FUN_RETURN_NIL;	// on arm, address must be aligned on dwords
	if (!str) FUN_RETURN_NIL;
	if (start<0 || nb<=0 || (start+nb*4>STR_LENGTH(str))) FUN_RETURN_NIL;
	p=STR_START(str)+start;
	q=(u32*)p;
	while((nb--)>0) *(u32 volatile *)addr = *(q++);
	FUN_RETURN_BOOL(1);
}

int fun_dataSyncBarrier(Thread* th)
{
	asm volatile ("dsb sy" ::: "memory");
	FUN_RETURN_BOOL(1);
}

int fun_dataMemBarrier(Thread* th)
{
	asm volatile ("dmb sy" ::: "memory");
	FUN_RETURN_BOOL(1);
}

int fun_cacheInvalidate(Thread* th)
{
	LINT len=STACK_INT(th,0);
	LUINT start=STACK_INT(th,1);
	start&=0xffffffff;
	cacheInvalidate((u64)start, len);
	FUN_RETURN_BOOL(1);
}
int fun_bytesCacheInvalidate(Thread* th)
{
	char* p;
	int len=STACK_INT(th,0);
	int start=STACK_INT(th,1);
	LB* str=STACK_PNT(th,2);
	if (!str) FUN_RETURN_NIL;
	if (start<0 || (start+len>STR_LENGTH(str))) FUN_RETURN_NIL;
	p=STR_START(str)+start;
	cacheInvalidate((u64)p, len);
	FUN_RETURN_BOOL(1);
}
int fun_bytesStart(Thread* th)
{
	LB* str=STACK_PNT(th,0);
	if (!str) FUN_RETURN_NIL;
	FUN_RETURN_INT((LINT)STR_START(str));
}

#ifdef ON_RPI5
#define GRAPHIC_MODES_NB 1
int GraphicModes[GRAPHIC_MODES_NB*3]={
	1920,1080,32
};
#else
#define GRAPHIC_MODES_NB 5
int GraphicModes[GRAPHIC_MODES_NB*3]={
	640,480,32,
	800,600,32,
	1024,768,32,
	1280,720,32,
	1920,1080,32
};
#endif
extern unsigned int fbWidth, fbHeight, fbWpl;
extern int *fbStart;
int fun_graphicModes(Thread* th)
{
	int i;
	for(i=0;i<GRAPHIC_MODES_NB;i++){
		FUN_PUSH_INT((LINT)i);
		FUN_PUSH_INT((LINT)GraphicModes[i*3]);
		FUN_PUSH_INT((LINT)GraphicModes[i*3+1]);
		FUN_PUSH_INT((LINT)GraphicModes[i*3+2]);
		FUN_MAKE_ARRAY(4, DBG_TUPLE);
	}
	FUN_PUSH_NIL;
	while (i--) FUN_MAKE_ARRAY(LIST_LENGTH, DBG_LIST);
	return 0;
}
int fun_graphicSetMode(Thread* th)
{
	int result;
	int mode=STACK_INT(th,0);
	if (mode<0 || mode>=GRAPHIC_MODES_NB) FUN_RETURN_NIL;
	result=mboxFrameBuffer(GraphicModes[mode*3],GraphicModes[mode*3+1]);
	if(!result) FUN_RETURN_NIL;
	FUN_RETURN_INT((LINT)mode)
}
int fun_graphicW(Thread* th)
{
	if (!fbStart) FUN_RETURN_NIL;
	FUN_RETURN_INT((LINT)fbWidth);
}
int fun_graphicH(Thread* th)
{
	if (!fbStart) FUN_RETURN_NIL;
	FUN_RETURN_INT((LINT)fbHeight);
}
int fun_graphicBlit(Thread* th)
{
	int* dst;
	int* src;

	int hIsNil= STACK_IS_NIL(th,0);
	LINT h = STACK_INT(th,0);
	int wIsNil= STACK_IS_NIL(th,1);
	LINT w = STACK_INT(th,1);
	LINT ysrc = STACK_INT(th,2);
	LINT xsrc = STACK_INT(th,3);
	LBitmap* a = (LBitmap*)STACK_PNT(th,4);
	LINT ydst = STACK_INT(th,5);
	LINT xdst = STACK_INT(th,6);
	if (!fbStart) FUN_RETURN_NIL;
	if (!a) FUN_RETURN_NIL;
	if (wIsNil) w= a->w - xsrc;
	if (hIsNil) h= a->h - ysrc;
	
	LINT dx, dy;
	if (_clip1D(0, a->w, &xsrc, &w, &dx)) FUN_RETURN_NIL;
	if (_clip1D(0, a->h, &ysrc, &h, &dy)) FUN_RETURN_NIL;
	xdst += dx; ydst += dy;
	if (_clip1D(0, fbWidth, &xdst, &w, &dx)) FUN_RETURN_NIL;
	if (_clip1D(0, fbHeight, &ydst, &h, &dy)) FUN_RETURN_NIL;
	xsrc += dx; ysrc += dy;

	dst=fbStart + xdst + ydst*fbWpl;
	src=a->start32 + xsrc + ysrc*a->next32;

	while((h--)>0) {
		LINT k;
		for(k=0;k<w;k++) dst[k]=src[k];
		dst+=fbWpl;
		src+=a->next32;
	}
	FUN_RETURN_BOOL(1);
}

//----------DTB copy
char* DTBcopyStart=NULL;
int DTBcopyLength=0;

void hwDtbInit()
{
	unsigned int dtbAddr=*(int*)(long long)0xf8;
	int* dtb = (int*)(long long)dtbAddr;
	int magic=MSBL(dtb[0]);
	int size=MSBL(dtb[1]);

//	PRINTF(LOG_SYS,"dtb start = %x\n",dtbAddr);
//	PRINTF(LOG_SYS,"dtb magic = %x, size= %x\n",magic,size);
	if (magic!=0xd00dfeed) {
		PRINTF(LOG_SYS,"DTB not found\n");
		return;
	}
	DTBcopyLength=size;
	DTBcopyStart=bmmAllocForEver(DTBcopyLength);
	memcpy(DTBcopyStart,(char*)dtb,DTBcopyLength);
	PRINTF(LOG_SYS,"DTB found  : %d bytes\n",DTBcopyLength);
}

int fun_dtb(Thread* th)
{
	if (!DTBcopyLength) FUN_RETURN_NIL;
	FUN_RETURN_STR(DTBcopyStart,DTBcopyLength);
}

int fun_clockRate(Thread* th)
{
	LINT id = STACK_INT(th,0);
	if (!id) FUN_RETURN_NIL;
	FUN_RETURN_INT(mboxGetClockRate(id));
}

int hostOnlyFunctionsInit(Pkg* system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "mbox", fun_mbox, "fun Int array Int Int -> array Int"},
		{ NATIVE_FUN, "dataSyncBarrier", fun_dataSyncBarrier, "fun -> Bool"},
		{ NATIVE_FUN, "dataMemBarrier", fun_dataMemBarrier, "fun -> Bool"},
		{ NATIVE_FUN, "cacheInvalidate", fun_cacheInvalidate, "fun Int Int -> Bool"},
		//{ NATIVE_FUN, "bytesCacheInvalidate", fun_bytesCacheInvalidate, "fun Bytes Int Int -> Bool"},
		//{ NATIVE_FUN, "bytesStart", fun_bytesStart, "fun Bytes -> Int"},
		{ NATIVE_FUN, "read8", fun_read8, "fun Int -> Int"},
		{ NATIVE_FUN, "write8", fun_write8, "fun Int Int -> Int"},
		{ NATIVE_FUN, "read16", fun_read16, "fun Int -> Int"},
		{ NATIVE_FUN, "write16", fun_write16, "fun Int Int -> Int"},
		{ NATIVE_FUN, "read32", fun_read32, "fun Int -> Int"},
		{ NATIVE_FUN, "write32", fun_write32, "fun Int Int -> Int"},
		{ NATIVE_FUN, "read8X", fun_read8X, "fun Int Int -> Int"},
		{ NATIVE_FUN, "read16X", fun_read16X, "fun Int Int -> Int"},
		{ NATIVE_FUN, "read32X", fun_read32X, "fun Int Int -> Int"},
		{ NATIVE_FUN, "write32X", fun_write32X, "fun Int Int Int -> Int"},
		{ NATIVE_FUN, "read32Repeat", fun_read32Repeat, "fun Int Bytes Int Int -> Bool"},
		{ NATIVE_FUN, "write32Repeat", fun_write32Repeat, "fun Int Bytes Int Int -> Bool"},
		{ NATIVE_FUN, "graphicModes", fun_graphicModes, "fun -> list [Int Int Int Int]"},
		{ NATIVE_FUN, "graphicSetMode", fun_graphicSetMode, "fun Int -> Int"},
		{ NATIVE_FUN, "graphicW", fun_graphicW, "fun -> Int"},
		{ NATIVE_FUN, "graphicH", fun_graphicH, "fun -> Int"},
		{ NATIVE_FUN, "graphicBlit", fun_graphicBlit, "fun Int Int Bitmap Int Int Int Int -> Bool"},
		{ NATIVE_FUN, "dtb", fun_dtb, "fun -> Str"},
		{ NATIVE_FUN, "clockRate", fun_clockRate, "fun Int -> Int"},
	};
	NATIVE_DEF(nativeDefs);
	systemSharedMemInit(system);

	fbWidth=fbHeight=fbWpl=0;
	fbStart=0;
	
	return 0;
}
