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

int fun_bufferCreate(Thread* th)
{
	Buffer* p= bufferCreate(th); if (!p) return EXEC_OM;
	return STACKPUSH(th, (PNTTOVAL(p)));
}
int fun_bufferCreateWithSize(Thread* th)
{
	Buffer* p= bufferCreateWithSize(th,VALTOINT(STACKGET(th,0))); if (!p) return EXEC_OM;
	STACKSET(th, 0, (PNTTOVAL(p)));
	return 0;
}
int fun_bufferToStr(Thread* th)
{
	LB* p;
	Buffer* b = (Buffer*)VALTOPNT(STACKGET(th, 0));
	if (!b) return 0;
	p = memoryAllocFromBuffer(th, b); if (!p) return EXEC_OM;
	STACKSET(th, 0, PNTTOVAL(p));
	return 0;
}
int fun_bufferLength(Thread* th)
{
	Buffer* b = (Buffer*)VALTOPNT(STACKGET(th, 0));
	if (b) STACKSETINT(th, 0, bufferSize(b));
	return 0;
}
int fun_bufferReset(Thread* th)
{
	Buffer* b = (Buffer*)VALTOPNT(STACKGET(th, 0));
	if (b) bufferReinit(b);
	return 0;
}


int fun_bufferAppend(Thread* th)
{
	LW a = STACKGET(th, 0);
	Buffer* b = (Buffer*)VALTOPNT(STACKGET(th, 1));
	if (a == NIL || !b) goto cleanup;
	if (bufferItem(th,b,a,NULL,0,bufferSize(b))) return EXEC_OM;
cleanup:
	STACKDROP(th);
	return 0;
}
int fun_bufferAppendJoin(Thread* th)
{
	LW a = STACKGET(th, 0);
	LB* join = VALTOPNT(STACKGET(th, 1));
	Buffer* b = (Buffer*)VALTOPNT(STACKGET(th, 2));
	if (a == NIL || !b ||!join) goto cleanup;

	if (bufferItem(th, b, a, join, 0, bufferSize(b))) return EXEC_OM;
cleanup:
	STACKDROPN(th,2);
	return 0;
}

int coreBufferInit(Thread* th, Pkg *system)
{
	Type* u0 = typeAllocUndef(th);

	pkgAddFun(th, system,"bufferCreate",fun_bufferCreate,typeAlloc(th,TYPECODE_FUN,NULL,1,MM.Buffer));
	pkgAddFun(th, system,"bufferCreateWithSize",fun_bufferCreateWithSize,typeAlloc(th,TYPECODE_FUN,NULL,2,MM.I,MM.Buffer));
	pkgAddFun(th, system,"bufferAppend", fun_bufferAppend,typeAlloc(th,TYPECODE_FUN,NULL,3, MM.Buffer,u0, MM.Buffer));
	pkgAddFun(th, system,"bufferAppendJoin", fun_bufferAppendJoin,typeAlloc(th,TYPECODE_FUN,NULL,4, MM.Buffer,MM.S,u0, MM.Buffer));
	pkgAddFun(th, system,"bufferLength", fun_bufferLength, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Buffer, MM.I));
	pkgAddFun(th, system,"bufferToStr", fun_bufferToStr, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Buffer, MM.S));
	pkgAddFun(th, system,"bufferToBytes", fun_bufferToStr, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Buffer, MM.Bytes));
	pkgAddFun(th, system,"bufferReset", fun_bufferReset, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Buffer, MM.Buffer));

	return 0;
}
