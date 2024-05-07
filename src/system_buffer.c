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
	FUN_RETURN_PNT((LB*)p);
}
int fun_bufferCreateWithSize(Thread* th)
{
	Buffer* p= bufferCreateWithSize(th,STACK_INT(th,0)); if (!p) return EXEC_OM;
	FUN_RETURN_PNT((LB*)p);
}
int fun_strFromBuffer(Thread* th)
{
	Buffer* b = (Buffer*)STACK_PNT(th, 0);
	if (!b) FUN_RETURN_NIL;
	FUN_RETURN_BUFFER(b);
}
int fun_bufferLength(Thread* th)
{
	Buffer* b = (Buffer*)STACK_PNT(th, 0);
	if (!b) FUN_RETURN_NIL;
	FUN_RETURN_INT(bufferSize(b));
}
int fun_bufferReset(Thread* th)
{
	Buffer* b = (Buffer*)STACK_PNT(th, 0);
	if (b) bufferReinit(b);
	return 0;
}
int fun_bufferRemove(Thread* th)
{
	LINT delta = STACK_PULL_INT(th);
	Buffer* b = (Buffer*)STACK_PNT(th, 0);
	if (b) bufferRemove(b,(int)delta);
	return 0;
}
int fun_bufferGet(Thread* th)
{
	LINT index = STACK_PULL_INT(th);
	Buffer* b = (Buffer*)STACK_PNT(th, 0);
	FUN_CHECK_CONTAINS(b,index,1,bufferSize(b));
	FUN_RETURN_INT(255& bufferGetChar(b,index));
}
int fun_bufferAppendChar(Thread* th)
{
	LINT c = STACK_PULL_INT(th);
	Buffer* b = (Buffer*)STACK_PNT(th, 0);
	if (b&&bufferAddChar(th,b,(char)c)) return EXEC_OM;
	return 0;
}
int fun_bufferSliceOfStr(Thread* th)
{
	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT start = STACK_INT(th,1);
	Buffer* b = (Buffer*)STACK_PNT(th, 2);
	FUN_SUBSTR(b,start,len,lenIsNil,bufferSize(b));

	FUN_RETURN_STR(bufferStart(b)+start, len);
}
int fun_bufferAppend(Thread* th)
{
	Buffer* b = (Buffer*)STACK_PNT(th, 1);
	if (STACK_IS_NIL(th, 0) || !b) FUN_RETURN_NIL;

	if (bufferItem(th,b,STACK_GET(th, 0),STACK_TYPE(th, 0),NULL)) return EXEC_OM;
	STACK_DROP(th);
	return 0;
}
int fun_bufferAppendJoin(Thread* th)
{
	LB* join = STACK_PNT(th, 1);
	Buffer* b = (Buffer*)STACK_PNT(th, 2);
	if (STACK_IS_NIL(th, 0) || !b ||!join) FUN_RETURN_NIL;

	if (bufferItem(th, b, STACK_GET(th, 0), STACK_TYPE(th, 0), join)) return EXEC_OM;
	STACK_DROPN(th,2);
	return 0;
}

int coreBufferInit(Thread* th, Pkg *system)
{
	Type* u0 = typeAllocUndef(th);

	pkgAddFun(th, system,"bufferCreate",fun_bufferCreate,typeAlloc(th,TYPECODE_FUN,NULL,1,MM.Buffer));
	pkgAddFun(th, system,"bufferCreateWithSize",fun_bufferCreateWithSize,typeAlloc(th,TYPECODE_FUN,NULL,2,MM.Int,MM.Buffer));
	pkgAddFun(th, system,"bufferAppend", fun_bufferAppend,typeAlloc(th,TYPECODE_FUN,NULL,3, MM.Buffer,u0, MM.Buffer));
	pkgAddFun(th, system,"bufferAppendJoin", fun_bufferAppendJoin,typeAlloc(th,TYPECODE_FUN,NULL,4, MM.Buffer,MM.Str,u0, MM.Buffer));
	pkgAddFun(th, system,"bufferLength", fun_bufferLength, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Buffer, MM.Int));
	pkgAddFun(th, system,"strFromBuffer", fun_strFromBuffer, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Buffer, MM.Str));
	pkgAddFun(th, system,"bytesFromBuffer", fun_strFromBuffer, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Buffer, MM.Bytes));
	pkgAddFun(th, system,"bufferReset", fun_bufferReset, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Buffer, MM.Buffer));
	pkgAddFun(th, system, "bufferRemove", fun_bufferRemove, typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.Buffer, MM.Int, MM.Buffer));
	pkgAddFun(th, system, "bufferGet", fun_bufferGet, typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.Buffer, MM.Int, MM.Int));
	pkgAddFun(th, system, "bufferAppendChar", fun_bufferAppendChar, typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.Buffer, MM.Int, MM.Buffer));
	pkgAddFun(th, system, "bufferSliceOfStr", fun_bufferSliceOfStr, typeAlloc(th, TYPECODE_FUN, NULL, 4, MM.Buffer, MM.Int, MM.Int, MM.Str));
	pkgAddFun(th, system, "bufferSliceOfBytes", fun_bufferSliceOfStr, typeAlloc(th, TYPECODE_FUN, NULL, 4, MM.Buffer, MM.Int, MM.Int, MM.Bytes));

	return 0;
}
