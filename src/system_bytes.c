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
	LINT n;
	LINT NDROP=2-1;
	LW result=NIL;

	LINT val=VALTOINT(STACKGET(th,0));
	LW wn=STACKGET(th,1);
	if (wn == NIL) goto cleanup;
	n=VALTOINT(wn);
	if (n<0) goto cleanup;
	p=memoryAllocStr(th, NULL,n); if (!p) return EXEC_OM;
	memset(STRSTART(p),(int)val,n);
	result=PNTTOVAL(p);
cleanup:
	STACKSET(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}

int fun_bytesSet(Thread* th)
{
	LINT NDROP=3-1;
	LW result=NIL;

	LINT val=VALTOINT(STACKGET(th,0));
	LINT i=VALTOINT(STACKGET(th,1));
	LB* a=VALTOPNT(STACKGET(th,2));
	if (!a) goto cleanup;
	if ((i<0)||(i>=STRLEN(a))) goto cleanup;
	(STRSTART(a))[i]=(char)val;
	result=STACKGET(th,2);
cleanup:
	STACKSET(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}

int fun_bytesClear(Thread* th)
{
	LINT NDROP=2-1;
	LW result=NIL;

	LINT val=VALTOINT(STACKGET(th,0));
	LB* a=VALTOPNT(STACKGET(th,1));
	if (!a) goto cleanup;
	memset(STRSTART(a),(int)val,STRLEN(a));
	result=STACKGET(th,1);
cleanup:
	STACKSET(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}

int fun_strToBytes(Thread* th)
{
	LB* p;
	LINT len;
	LINT NDROP = 3 - 1;
	LW result = NIL;

	LW vlen = STACKGET(th, 0);
	LINT start = VALTOINT(STACKGET(th, 1));
	LB* a = VALTOPNT(STACKGET(th, 2));
	if (!a) goto cleanup;
	if (start < 0) start += STRLEN(a);
	if (start < 0 || start >= STRLEN(a)) goto cleanup;

	len = (vlen == NIL) ? STRLEN(a): VALTOINT(vlen);
	if ((start + len) > STRLEN(a)) len = STRLEN(a) - start;
	p = memoryAllocStr(th, STRSTART(a) + start, len); if (!p) return EXEC_OM;
	result = PNTTOVAL(p);
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}

int fun_bytesLSL1(Thread* th)
{
	char* p;
	LINT i;
	LINT NDROP = 2 - 1;
	LW result = NIL;

	LINT cc = VALTOINT(STACKGET(th, 0))&1;
	LB* a = VALTOPNT(STACKGET(th, 1));
	if (!a) goto cleanup;
	p = STRSTART(a);
	for (i = STRLEN(a) - 1; i >= 0; i--)
	{
		cc += (((LINT)p[i]) & 255) << 1;
		p[i] = (char)cc;
		cc >>= 8;
	}
	result = INTTOVAL(cc);
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}


int fun_bytesCopy(Thread* th)
{
	LINT len;
	LINT NDROP=5-1;
	LW result=NIL;

	LW wlen=STACKGET(th,0);
	LINT start=VALTOINT(STACKGET(th,1));
	LB* q=VALTOPNT(STACKGET(th,2));
	LINT index=VALTOINT(STACKGET(th,3));
	LB* p=VALTOPNT(STACKGET(th,4));
	result=STACKGET(th,4);
	if ((!q)||(!p)) goto cleanup;

	if (index < 0) index += STRLEN(p);
	if (start < 0) start += STRLEN(q);
	if ((index < 0) || (index >= STRLEN(p))) goto cleanup;
	if ((start < 0) || (start >= STRLEN(q))) goto cleanup;
	len=(wlen==NIL)?STRLEN(q):VALTOINT(wlen);
	if (index + len > STRLEN(p)) len = STRLEN(p) - index;
	if (start + len > STRLEN(q)) len = STRLEN(q) - start;
	memcpy(STRSTART(p) + index, STRSTART(q) + start, len);
cleanup:
	STACKSET(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}

int fun_bytesXor(Thread* th)
{
	char* pp;
	char* qq;
	LINT i;
	LINT len;
	LINT NDROP = 5 - 1;
	LW result = NIL;

	LW wlen = STACKGET(th, 0);
	LINT start = VALTOINT(STACKGET(th, 1));
	LB* q = VALTOPNT(STACKGET(th, 2));
	LINT index = VALTOINT(STACKGET(th, 3));
	LB* p = VALTOPNT(STACKGET(th, 4));
	result = STACKGET(th, 4);
	if ((!q) || (!p)) goto cleanup;

	if (index < 0) index += STRLEN(p);
	if (start < 0) start += STRLEN(q);
	if ((index < 0) || (index >= STRLEN(p))) goto cleanup;
	if ((start < 0) || (start >= STRLEN(q))) goto cleanup;
	len = (wlen == NIL) ? STRLEN(q) : VALTOINT(wlen);
	if (index + len > STRLEN(p)) len = STRLEN(p) - index;
	if (start + len > STRLEN(q)) len = STRLEN(q) - start;

	pp = STRSTART(p) + index;
	qq = STRSTART(q) + start;
	for (i = 0; i < len; i++) *(pp++) ^= *(qq++);
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}
int fun_bytesRand(Thread* th)
{
	LINT len;
	LINT NDROP = 3 - 1;
	LW result = NIL;

	LW wlen = STACKGET(th, 0);
	LINT index = VALTOINT(STACKGET(th, 1));
	LB* p = VALTOPNT(STACKGET(th, 2));
	result = STACKGET(th, 2);
	if (!p) goto cleanup;
	if (index < 0) index += STRLEN(p);
	if (index<0 || index>=STRLEN(p)) goto cleanup;

	len = (wlen == NIL) ? STRLEN(p) : VALTOINT(wlen);
	if ((index + len) > STRLEN(p)) len = STRLEN(p) - index;
	lsRand(STRSTART(p), len);
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}


int coreBytesInit(Thread* th, Pkg *system)
{
	Type* fun_B_I_I = typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.Bytes, MM.I, MM.I);
	Type* fun_B_I_I_B = typeAlloc(th,TYPECODE_FUN, NULL, 4, MM.Bytes, MM.I, MM.I, MM.Bytes);

	pkgAddFun(th, system,"bytesCreate",fun_bytesCreate,typeAlloc(th,TYPECODE_FUN,NULL,3,MM.I,MM.I,MM.Bytes));
	pkgAddFun(th, system,"bytesGet",fun_strGet, fun_B_I_I);
	pkgAddFun(th, system,"bytesSet",fun_bytesSet, fun_B_I_I_B);
	pkgAddFun(th, system,"bytesCopy",fun_bytesCopy,typeAlloc(th,TYPECODE_FUN,NULL,6,MM.Bytes,MM.I,MM.S,MM.I,MM.I,MM.Bytes));
	pkgAddFun(th, system,"bytesXor",fun_bytesXor,typeAlloc(th,TYPECODE_FUN,NULL,6,MM.Bytes,MM.I,MM.S,MM.I,MM.I,MM.Bytes));
	pkgAddFun(th, system,"bytesLSL1", fun_bytesLSL1, fun_B_I_I);
	pkgAddFun(th, system,"bytesClear", fun_bytesClear, typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.Bytes, MM.I, MM.Bytes));
	pkgAddOpcode(th, system,"bytesAsStr",OPnop,typeAlloc(th,TYPECODE_FUN,NULL,2,MM.Bytes,MM.S));
	pkgAddFun(th, system,"bytesFromStr",fun_strToBytes,typeAlloc(th,TYPECODE_FUN,NULL,4,MM.S,MM.I,MM.I,MM.Bytes));
	pkgAddFun(th, system,"bytesLength",fun_strLength,typeAlloc(th,TYPECODE_FUN,NULL,2,MM.Bytes,MM.I));
	pkgAddFun(th, system,"bytesRand", fun_bytesRand, fun_B_I_I_B);

	return 0;
}

