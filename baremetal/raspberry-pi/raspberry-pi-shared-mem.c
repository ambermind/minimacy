// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#include"../../src/minimacy.h"
#include"raspberry-pi.h"

LINT SharedMemStart;

int fun_sharedMemClear(Thread* th)
{
	LINT len=STACK_INT(th,0);
	LINT start=STACK_INT(th,1);
	memset((char*)start,0,len);
	FUN_RETURN_TRUE;
}
int fun_sharedMemCopy(Thread* th)
{
	LINT i;
	int* pSrc;
	int* pDst;
	LINT len=STACK_INT(th,0);
	LINT src=STACK_INT(th,1);
	LINT dst=STACK_INT(th,2);
	pSrc=(int*)src;
	pDst=(int*)dst;
	for(i=0;i<len;i+=4) *(pDst++)=*(pSrc++);
	FUN_RETURN_TRUE;
}
int fun_sharedMemExport(Thread* th)
{
	LINT len=STACK_INT(th,0);
	LINT start=STACK_INT(th,1);
	FUN_RETURN_STR((char*)start,len);
}
int fun_sharedMemImport(Thread* th)
{
	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT start=STACK_INT(th,1);
	LB* p=STACK_PNT(th,2);
	LINT addr=STACK_INT(th,3);
	FUN_SUBSTR(p, start,len,lenIsNil,STR_LENGTH(p));
	memcpy((char*)addr,STR_START(p) + start, len);

	FUN_RETURN_TRUE;
}

int fun_sharedMemRead8(Thread* th)
{
	LINT val;
	LINT addr=STACK_INT(th,0) + STACK_INT(th,1);
	val= *(u8 volatile *) addr;
	FUN_RETURN_INT(val)
}

int fun_sharedMemWrite8(Thread* th)
{
	LINT val=STACK_INT(th,0);
	LINT addr=STACK_INT(th,1) + STACK_INT(th,2);
	*(u8 volatile *) addr = (u8)val;
	return 0;
}

int fun_sharedMemRead16(Thread* th)
{
	LINT val;
	LINT addr=STACK_INT(th,0) + STACK_INT(th,1);
	if (addr&1) FUN_RETURN_NIL;	// on arm, address must be aligned on dwords
	val= *(u16 volatile *) addr;
	FUN_RETURN_INT(val)
}

int fun_sharedMemWrite16(Thread* th)
{
	LINT val=STACK_INT(th,0);
	LINT addr=STACK_INT(th,1) + STACK_INT(th,2);
	if (addr&1) FUN_RETURN_NIL;	// on arm, address must be aligned on dwords
	*(u16 volatile *) addr = (u16)val;
	return 0;
}

int fun_sharedMemRead32(Thread* th)
{
	LINT val;
	LINT addr=STACK_INT(th,0) + STACK_INT(th,1);
	if (addr&3) FUN_RETURN_NIL;	// on arm, address must be aligned on dwords
	val= *(u32 volatile *) addr;
	FUN_RETURN_INT(val)
}

int fun_sharedMemWrite32(Thread* th)
{
	LINT val=STACK_INT(th,0);
	LINT addr=STACK_INT(th,1) + STACK_INT(th,2);
	if (addr&3) FUN_RETURN_NIL;	// on arm, address must be aligned on dwords
	*(u32 volatile *) addr = (u32)val;
	return 0;
}

int _sharedMemAllocFinalize(Thread* th, LINT aligned, LINT size)
{
	SharedMemStart = aligned+size;
	if (SharedMemStart>=SHARED_MEM_END) {
		PRINTF(LOG_SYS,"> Out of Shared Memory\n");
		return EXEC_OM;
	}
	memset((char*)aligned,0,size);
	FUN_RETURN_INT(aligned);
}
int fun_sharedMemAllocateAlign(Thread* th)
{
	LINT aligned;
	LINT align=STACK_INT(th,0);
	LINT size=STACK_INT(th,1);

	aligned= (SharedMemStart+align-1)&~(align-1);
	return _sharedMemAllocFinalize(th, aligned, size);
}
int fun_sharedMemAllocateBoundaries(Thread* th)
{
	LINT aligned;
	LINT boundaries=STACK_INT(th,0);
	LINT align=STACK_INT(th,1);
	LINT size=STACK_INT(th,2);

	aligned= (SharedMemStart+align-1)&~(align-1);
	if ((aligned&~(boundaries-1))!=((aligned+size-1)&~(boundaries-1))) {
		aligned=(SharedMemStart+boundaries-1)&~(boundaries-1);
	}
	return _sharedMemAllocFinalize(th, aligned, size);
}

int systemSharedMemInit(Pkg* system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "sharedMemAllocateAlign", fun_sharedMemAllocateAlign, "fun Int Int -> Int"},
		{ NATIVE_FUN, "sharedMemAllocateBoundaries", fun_sharedMemAllocateBoundaries, "fun Int Int Int -> Int"},
		{ NATIVE_FUN, "sharedMemClear", fun_sharedMemClear, "fun Int Int -> Bool"},
		{ NATIVE_FUN, "sharedMemCopy", fun_sharedMemCopy, "fun Int Int Int -> Bool"},
		{ NATIVE_FUN, "sharedMemExport", fun_sharedMemExport, "fun Int Int -> Str"},
		{ NATIVE_FUN, "sharedMemImport", fun_sharedMemImport, "fun Int Str Int Int -> Bool"},
		{ NATIVE_FUN, "sharedMemRead8", fun_sharedMemRead8, "fun Int Int -> Int"},
		{ NATIVE_FUN, "sharedMemWrite8", fun_sharedMemWrite8, "fun Int Int Int -> Int"},
		{ NATIVE_FUN, "sharedMemRead16", fun_sharedMemRead16, "fun Int Int -> Int"},
		{ NATIVE_FUN, "sharedMemWrite16", fun_sharedMemWrite16, "fun Int Int Int -> Int"},
		{ NATIVE_FUN, "sharedMemRead32", fun_sharedMemRead32, "fun Int Int -> Int"},
		{ NATIVE_FUN, "sharedMemWrite32", fun_sharedMemWrite32, "fun Int Int Int -> Int"},
	};
	NATIVE_DEF(nativeDefs);

	SharedMemStart=SHARED_MEM_START;

	return 0;
}
