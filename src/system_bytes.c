// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

int fun_strGet(Thread* th);
int fun_strLength(Thread* th);
int fun_strCmp(Thread* th);

int fun_bytesCreate(Thread* th)
{
	LB* p;

	LINT val=STACK_INT(th,0);
	LINT n=STACK_INT(th,1);
	if (STACK_IS_NIL(th,1)) FUN_RETURN_NIL;
	if (n<0) FUN_RETURN_NIL;
	p=memoryAllocStr(NULL,n); if (!p) return EXEC_OM;
	memset(STR_START(p),(int)val,n);
	FUN_RETURN_PNT(p)
}

int fun_bytesSet(Thread* th)
{
	LINT val=STACK_INT(th,0);
	LINT i=STACK_INT(th,1);
	LB* a=STACK_PNT(th,2);
	FUN_CHECK_CONTAINS(a, i, 1, STR_LENGTH(a));
	(STR_START(a))[i]=(char)val;
	FUN_RETURN_PNT(STACK_PNT(th,2))
}

int fun_bytesClear(Thread* th)
{
	LINT val=STACK_INT(th,0);
	LB* a=STACK_PNT(th,1);
	if (!a) FUN_RETURN_NIL;
	memset(STR_START(a),(int)val,STR_LENGTH(a));
	FUN_RETURN_PNT(STACK_PNT(th,1))
}

int fun_strSliceForceCopy(Thread* th)
{
	int lenIsNil = STACK_IS_NIL(th, 0);
	LINT len = STACK_INT(th, 0);
	LINT start = STACK_INT(th, 1);
	LB* p = STACK_PNT(th, 2);
	FUN_SUBSTR(p,start,len,lenIsNil,STR_LENGTH(p));

	FUN_RETURN_STR(STR_START(p) + start, len);
}

int fun_bytesLSL1(Thread* th)
{
	char* p;
	LINT i;

	LINT cc = STACK_INT(th, 0)&1;
	LB* a = STACK_PNT(th, 1);
	if (!a) FUN_RETURN_NIL;
	p = STR_START(a);
	for (i = STR_LENGTH(a) - 1; i >= 0; i--)
	{
		cc += (((LINT)p[i]) & 255) << 1;
		p[i] = (char)cc;
		cc >>= 8;
	}
	FUN_RETURN_INT(cc)
}

int fun_bytesCopy(Thread* th)
{
	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT start=STACK_INT(th,1);
	LB* src=STACK_PNT(th,2);
	LINT index=STACK_INT(th,3);
	LB* dst=STACK_PNT(th,4);
	FUN_COPY_CROP(dst, index, STR_LENGTH(dst), src, start, len, lenIsNil, STR_LENGTH(src));

	memcpy(STR_START(dst) + index, STR_START(src) + start, len);
	FUN_RETURN_PNT(dst);
}

int fun_bytesXor(Thread* th)
{
	char* pp;
	char* qq;

	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT start = STACK_INT(th, 1);
	LB* src = STACK_PNT(th, 2);
	LINT index = STACK_INT(th, 3);
	LB* dst = STACK_PNT(th, 4);
	FUN_COPY_CROP(dst, index, STR_LENGTH(dst), src, start, len, lenIsNil, STR_LENGTH(src));

	pp = STR_START(src) + start;
	qq = STR_START(dst) + index;
	while(len--) *(qq++) ^= *(pp++);
	FUN_RETURN_PNT(dst)
}

int fun_bytesRand(Thread* th)
{
	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT start = STACK_INT(th, 1);
	LB* p = STACK_PNT(th, 2);
	FUN_SUBSTR(p,start,len,lenIsNil,STR_LENGTH(p));
	hwRandomBytes(STR_START(p)+start, len);
	FUN_RETURN_PNT(p);
}
int fun_strCopy(Thread* th)
{
	LB* a = STACK_PNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	FUN_RETURN_STR(STR_START(a), STR_LENGTH(a));
}

int fun_bytesLeft(Thread* th)
{
	LINT len = STACK_INT(th, 0);
	LB* src = STACK_PNT(th, 1);
	FUN_CROP_LENGTH(src, len, STR_LENGTH(src));
	FUN_RETURN_STR(STR_START(src), len);
}
int fun_bytesRight(Thread* th)
{
	LINT len = STACK_INT(th, 0);
	LB* src = STACK_PNT(th, 1);
	FUN_CROP_LENGTH(src, len, STR_LENGTH(src));
	FUN_RETURN_STR(STR_START(src) + STR_LENGTH(src) - len, len);
}

int systemBytesInit(Pkg *system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "bytesCreate", fun_bytesCreate, "fun Int Int -> Bytes"},
		{ NATIVE_FUN, "strCreate", fun_bytesCreate, "fun Int Int -> Str" },
		{ NATIVE_FUN, "bytesGet", fun_strGet, "fun Bytes Int -> Int" },
		{ NATIVE_FUN, "bytesSet", fun_bytesSet, "fun Bytes Int Int -> Bytes" },
		{ NATIVE_FUN, "bytesCopy", fun_bytesCopy, "fun Bytes Int Str Int Int -> Bytes" },
		{ NATIVE_FUN, "bytesCopyBytes", fun_bytesCopy, "fun Bytes Int Bytes Int Int -> Bytes" },
		{ NATIVE_FUN, "bytesXor", fun_bytesXor, "fun Bytes Int Str Int Int -> Bytes" },
		{ NATIVE_FUN, "bytesXorBytes", fun_bytesXor, "fun Bytes Int Bytes Int Int -> Bytes" },
		{ NATIVE_FUN, "bytesLSL1", fun_bytesLSL1, "fun Bytes Int -> Int" },
		{ NATIVE_FUN, "bytesClear", fun_bytesClear, "fun Bytes Int -> Bytes" },
		{ NATIVE_OPCODE, "_bytesAsStr", (void*)OPnop, "fun Bytes -> Str" },
		{ NATIVE_FUN, "bytesLeft", fun_bytesLeft, "fun Bytes Int -> Bytes" },
		{ NATIVE_FUN, "strLeftBytes", fun_bytesLeft, "fun Bytes Int -> Str" },
		{ NATIVE_FUN, "bytesRight", fun_bytesRight, "fun Bytes Int -> Bytes" },
		{ NATIVE_FUN, "bytesSlice", fun_strSliceForceCopy, "fun Bytes Int Int -> Bytes" },
		{ NATIVE_FUN, "strSliceOfBytes", fun_strSliceForceCopy, "fun Str Int Int -> Bytes" },
		{ NATIVE_FUN, "bytesSliceOfStr", fun_strSliceForceCopy, "fun Bytes Int Int -> Str" },
		{ NATIVE_FUN, "strFromBytes", fun_strCopy, "fun Bytes -> Str" },
		{ NATIVE_FUN, "bytesFromStr", fun_strCopy, "fun Str -> Bytes" },
		{ NATIVE_FUN, "bytesLength", fun_strLength, "fun Bytes -> Int" },
		{ NATIVE_FUN, "bytesCmp", fun_strCmp, "fun Bytes Bytes -> Int" },
		{ NATIVE_FUN, "bytesRand", fun_bytesRand, "fun Bytes Int Int -> Bytes" },
	};
	NATIVE_DEF(nativeDefs);

	return 0;
}

