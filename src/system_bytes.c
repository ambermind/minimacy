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

	LINT val=STACKINT(th,0);
	LINT n=STACKINT(th,1);
	if (STACKISNIL(th,1)) FUN_RETURN_NIL;
	if (n<0) FUN_RETURN_NIL;
	p=memoryAllocStr(th, NULL,n); if (!p) return EXEC_OM;
	memset(STRSTART(p),(int)val,n);
	FUN_RETURN_PNT(p)
}

int fun_bytesSet(Thread* th)
{
	LINT val=STACKINT(th,0);
	LINT i=STACKINT(th,1);
	LB* a=STACKPNT(th,2);
	FUN_CHECK_CONTAINS(a, i, 1, STRLEN(a));
	(STRSTART(a))[i]=(char)val;
	FUN_RETURN_PNT(STACKPNT(th,2))
}

int fun_bytesClear(Thread* th)
{
	LINT val=STACKINT(th,0);
	LB* a=STACKPNT(th,1);
	if (!a) FUN_RETURN_NIL;
	memset(STRSTART(a),(int)val,STRLEN(a));
	FUN_RETURN_PNT(STACKPNT(th,1))
}

int fun_strSliceForceCopy(Thread* th)
{
	int lenIsNil = STACKISNIL(th, 0);
	LINT len = STACKINT(th, 0);
	LINT start = STACKINT(th, 1);
	LB* p = STACKPNT(th, 2);
	FUN_SUBSTR(p,start,len,lenIsNil,STRLEN(p));

	FUN_RETURN_STR(STRSTART(p) + start, len);
}

int fun_bytesLSL1(Thread* th)
{
	char* p;
	LINT i;

	LINT cc = STACKINT(th, 0)&1;
	LB* a = STACKPNT(th, 1);
	if (!a) FUN_RETURN_NIL;
	p = STRSTART(a);
	for (i = STRLEN(a) - 1; i >= 0; i--)
	{
		cc += (((LINT)p[i]) & 255) << 1;
		p[i] = (char)cc;
		cc >>= 8;
	}
	FUN_RETURN_INT(cc)
}

int fun_bytesCopy(Thread* th)
{
	int lenIsNil = STACKISNIL(th,0);
	LINT len = STACKINT(th, 0);
	LINT start=STACKINT(th,1);
	LB* src=STACKPNT(th,2);
	LINT index=STACKINT(th,3);
	LB* dst=STACKPNT(th,4);
	FUN_COPY_CROP(dst, index, STRLEN(dst), src, start, len, lenIsNil, STRLEN(src));

	memcpy(STRSTART(dst) + index, STRSTART(src) + start, len);
	FUN_RETURN_PNT(dst);
}

int fun_bytesXor(Thread* th)
{
	char* pp;
	char* qq;

	int lenIsNil = STACKISNIL(th,0);
	LINT len = STACKINT(th, 0);
	LINT start = STACKINT(th, 1);
	LB* src = STACKPNT(th, 2);
	LINT index = STACKINT(th, 3);
	LB* dst = STACKPNT(th, 4);
	FUN_COPY_CROP(dst, index, STRLEN(dst), src, start, len, lenIsNil, STRLEN(src));

	pp = STRSTART(src) + start;
	qq = STRSTART(dst) + index;
	while(len--) *(qq++) ^= *(pp++);
	FUN_RETURN_PNT(dst)
}

int fun_bytesRand(Thread* th)
{
	int lenIsNil = STACKISNIL(th,0);
	LINT len = STACKINT(th, 0);
	LINT start = STACKINT(th, 1);
	LB* p = STACKPNT(th, 2);
	FUN_SUBSTR(p,start,len,lenIsNil,STRLEN(p));
	hwRandomBytes(STRSTART(p)+start, len);
	FUN_RETURN_PNT(p);
}
int fun_strCopy(Thread* th)
{
	LB* a = STACKPNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	FUN_RETURN_STR(STRSTART(a), STRLEN(a));
}

int fun_bytesLeft(Thread* th)
{
	LINT len = STACKINT(th, 0);
	LB* src = STACKPNT(th, 1);
	FUN_CROP_LENGTH(src, len, STRLEN(src));
	FUN_RETURN_STR(STRSTART(src), len);
}
int fun_bytesRight(Thread* th)
{
	LINT len = STACKINT(th, 0);
	LB* src = STACKPNT(th, 1);
	FUN_CROP_LENGTH(src, len, STRLEN(src));
	FUN_RETURN_STR(STRSTART(src) + STRLEN(src) - len, len);
}

int coreBytesInit(Thread* th, Pkg *system)
{
	Type* fun_B_I_I = typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.Bytes, MM.I, MM.I);
	Type* fun_B_I_B = typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.Bytes, MM.I, MM.Bytes);
	Type* fun_B_I_S = typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.Bytes, MM.I, MM.S);
	Type* fun_B_I_I_B = typeAlloc(th,TYPECODE_FUN, NULL, 4, MM.Bytes, MM.I, MM.I, MM.Bytes);

	pkgAddFun(th, system,"bytesCreate",fun_bytesCreate,typeAlloc(th,TYPECODE_FUN,NULL,3,MM.I,MM.I,MM.Bytes));
	pkgAddFun(th, system,"strCreate",fun_bytesCreate,typeAlloc(th,TYPECODE_FUN,NULL,3,MM.I,MM.I,MM.S));
	pkgAddFun(th, system,"bytesGet",fun_strGet, fun_B_I_I);
	pkgAddFun(th, system,"bytesSet",fun_bytesSet, fun_B_I_I_B);
	pkgAddFun(th, system,"bytesCopy",fun_bytesCopy,typeAlloc(th,TYPECODE_FUN,NULL,6,MM.Bytes,MM.I,MM.S,MM.I,MM.I,MM.Bytes));
	pkgAddFun(th, system,"bytesCopyBytes",fun_bytesCopy,typeAlloc(th,TYPECODE_FUN,NULL,6,MM.Bytes,MM.I,MM.Bytes,MM.I,MM.I,MM.Bytes));
	pkgAddFun(th, system,"bytesXor",fun_bytesXor,typeAlloc(th,TYPECODE_FUN,NULL,6,MM.Bytes,MM.I,MM.S,MM.I,MM.I,MM.Bytes));
	pkgAddFun(th, system,"bytesXorBytes",fun_bytesXor,typeAlloc(th,TYPECODE_FUN,NULL,6,MM.Bytes,MM.I,MM.Bytes,MM.I,MM.I,MM.Bytes));
	pkgAddFun(th, system,"bytesLSL1", fun_bytesLSL1, fun_B_I_I);
	pkgAddFun(th, system,"bytesClear", fun_bytesClear, fun_B_I_B);
	pkgAddOpcode(th, system,"_bytesAsStr",OPnop,typeAlloc(th,TYPECODE_FUN,NULL,2,MM.Bytes,MM.S));
	pkgAddFun(th, system, "bytesLeft", fun_bytesLeft, fun_B_I_B);
	pkgAddFun(th, system, "strLeftBytes", fun_bytesLeft, fun_B_I_S);
	pkgAddFun(th, system, "bytesRight", fun_bytesRight, fun_B_I_B);
	pkgAddFun(th, system,"bytesSlice",fun_strSliceForceCopy,typeAlloc(th,TYPECODE_FUN,NULL,4,MM.Bytes,MM.I,MM.I,MM.Bytes));
	pkgAddFun(th, system,"strSliceOfBytes",fun_strSliceForceCopy,typeAlloc(th,TYPECODE_FUN,NULL,4,MM.S,MM.I,MM.I,MM.Bytes));
	pkgAddFun(th, system,"bytesSliceOfStr",fun_strSliceForceCopy,typeAlloc(th,TYPECODE_FUN,NULL,4,MM.Bytes,MM.I,MM.I,MM.S));
	pkgAddFun(th, system,"strFromBytes",fun_strCopy,typeAlloc(th,TYPECODE_FUN,NULL,2,MM.Bytes,MM.S));
	pkgAddFun(th, system,"bytesFromStr",fun_strCopy,typeAlloc(th,TYPECODE_FUN,NULL,2,MM.S,MM.Bytes));
	pkgAddFun(th, system,"bytesLength",fun_strLength,typeAlloc(th,TYPECODE_FUN,NULL,2,MM.Bytes,MM.I));
	pkgAddFun(th, system,"bytesRand", fun_bytesRand, fun_B_I_I_B);

	return 0;
}

