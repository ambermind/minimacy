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

int fun_strGet(Thread* th);
int fun_strLength(Thread* th);

int fun_bytesCreate(Thread* th)
{
	LB* p;

	LINT val=STACK_INT(th,0);
	LINT n=STACK_INT(th,1);
	if (STACK_IS_NIL(th,1)) FUN_RETURN_NIL;
	if (n<0) FUN_RETURN_NIL;
	p=memoryAllocStr(th, NULL,n); if (!p) return EXEC_OM;
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

int coreBytesInit(Thread* th, Pkg *system)
{
	Type* fun_B_I_I = typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.Bytes, MM.Int, MM.Int);
	Type* fun_B_I_B = typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.Bytes, MM.Int, MM.Bytes);
	Type* fun_B_I_S = typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.Bytes, MM.Int, MM.Str);
	Type* fun_B_I_I_B = typeAlloc(th,TYPECODE_FUN, NULL, 4, MM.Bytes, MM.Int, MM.Int, MM.Bytes);

	pkgAddFun(th, system,"bytesCreate",fun_bytesCreate,typeAlloc(th,TYPECODE_FUN,NULL,3,MM.Int,MM.Int,MM.Bytes));
	pkgAddFun(th, system,"strCreate",fun_bytesCreate,typeAlloc(th,TYPECODE_FUN,NULL,3,MM.Int,MM.Int,MM.Str));
	pkgAddFun(th, system,"bytesGet",fun_strGet, fun_B_I_I);
	pkgAddFun(th, system,"bytesSet",fun_bytesSet, fun_B_I_I_B);
	pkgAddFun(th, system,"bytesCopy",fun_bytesCopy,typeAlloc(th,TYPECODE_FUN,NULL,6,MM.Bytes,MM.Int,MM.Str,MM.Int,MM.Int,MM.Bytes));
	pkgAddFun(th, system,"bytesCopyBytes",fun_bytesCopy,typeAlloc(th,TYPECODE_FUN,NULL,6,MM.Bytes,MM.Int,MM.Bytes,MM.Int,MM.Int,MM.Bytes));
	pkgAddFun(th, system,"bytesXor",fun_bytesXor,typeAlloc(th,TYPECODE_FUN,NULL,6,MM.Bytes,MM.Int,MM.Str,MM.Int,MM.Int,MM.Bytes));
	pkgAddFun(th, system,"bytesXorBytes",fun_bytesXor,typeAlloc(th,TYPECODE_FUN,NULL,6,MM.Bytes,MM.Int,MM.Bytes,MM.Int,MM.Int,MM.Bytes));
	pkgAddFun(th, system,"bytesLSL1", fun_bytesLSL1, fun_B_I_I);
	pkgAddFun(th, system,"bytesClear", fun_bytesClear, fun_B_I_B);
	pkgAddOpcode(th, system,"_bytesAsStr",OPnop,typeAlloc(th,TYPECODE_FUN,NULL,2,MM.Bytes,MM.Str));
	pkgAddFun(th, system, "bytesLeft", fun_bytesLeft, fun_B_I_B);
	pkgAddFun(th, system, "strLeftBytes", fun_bytesLeft, fun_B_I_S);
	pkgAddFun(th, system, "bytesRight", fun_bytesRight, fun_B_I_B);
	pkgAddFun(th, system,"bytesSlice",fun_strSliceForceCopy,typeAlloc(th,TYPECODE_FUN,NULL,4,MM.Bytes,MM.Int,MM.Int,MM.Bytes));
	pkgAddFun(th, system,"strSliceOfBytes",fun_strSliceForceCopy,typeAlloc(th,TYPECODE_FUN,NULL,4,MM.Str,MM.Int,MM.Int,MM.Bytes));
	pkgAddFun(th, system,"bytesSliceOfStr",fun_strSliceForceCopy,typeAlloc(th,TYPECODE_FUN,NULL,4,MM.Bytes,MM.Int,MM.Int,MM.Str));
	pkgAddFun(th, system,"strFromBytes",fun_strCopy,typeAlloc(th,TYPECODE_FUN,NULL,2,MM.Bytes,MM.Str));
	pkgAddFun(th, system,"bytesFromStr",fun_strCopy,typeAlloc(th,TYPECODE_FUN,NULL,2,MM.Str,MM.Bytes));
	pkgAddFun(th, system,"bytesLength",fun_strLength,typeAlloc(th,TYPECODE_FUN,NULL,2,MM.Bytes,MM.Int));
	pkgAddFun(th, system,"bytesRand", fun_bytesRand, fun_B_I_I_B);

	return 0;
}

