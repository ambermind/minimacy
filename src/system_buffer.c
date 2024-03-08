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
	Buffer* p= bufferCreateWithSize(th,STACKINT(th,0)); if (!p) return EXEC_OM;
	FUN_RETURN_PNT((LB*)p);
}
int fun_strFromBuffer(Thread* th)
{
	Buffer* b = (Buffer*)STACKPNT(th, 0);
	if (!b) FUN_RETURN_NIL;
	FUN_RETURN_BUFFER(b);
}
int fun_bufferLength(Thread* th)
{
	Buffer* b = (Buffer*)STACKPNT(th, 0);
	if (!b) FUN_RETURN_NIL;
	FUN_RETURN_INT(bufferSize(b));
}
int fun_bufferReset(Thread* th)
{
	Buffer* b = (Buffer*)STACKPNT(th, 0);
	if (b) bufferReinit(b);
	return 0;
}
int fun_bufferGet(Thread* th)
{
	LINT index = STACKPULLINT(th);
	Buffer* b = (Buffer*)STACKPNT(th, 0);
	FUN_CHECK_CONTAINS(b,index,1,bufferSize(b));
	FUN_RETURN_INT(255& bufferGetChar(b,index));
}
int fun_bufferAppendChar(Thread* th)
{
	LINT c = STACKPULLINT(th);
	Buffer* b = (Buffer*)STACKPNT(th, 0);
	if (b&&bufferAddChar(th,b,(char)c)) return EXEC_OM;
	return 0;
}
int fun_bufferSliceOfStr(Thread* th)
{
	int lenIsNil = STACKISNIL(th,0);
	LINT len = STACKINT(th, 0);
	LINT start = STACKINT(th,1);
	Buffer* b = (Buffer*)STACKPNT(th, 2);
	FUN_SUBSTR(b,start,len,lenIsNil,bufferSize(b));

	FUN_RETURN_STR(bufferStart(b)+start, len);
}
int fun_bufferAppend(Thread* th)
{
	Buffer* b = (Buffer*)STACKPNT(th, 1);
	if (STACKISNIL(th, 0) || !b) FUN_RETURN_NIL;

	if (bufferItem(th,b,STACKGET(th, 0),STACKTYPE(th, 0),NULL)) return EXEC_OM;
	STACKDROP(th);
	return 0;
}
int fun_bufferAppendJoin(Thread* th)
{
	LB* join = STACKPNT(th, 1);
	Buffer* b = (Buffer*)STACKPNT(th, 2);
	if (STACKISNIL(th, 0) || !b ||!join) FUN_RETURN_NIL;

	if (bufferItem(th, b, STACKGET(th, 0), STACKTYPE(th, 0), join)) return EXEC_OM;
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
	pkgAddFun(th, system,"strFromBuffer", fun_strFromBuffer, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Buffer, MM.S));
	pkgAddFun(th, system,"bytesFromBuffer", fun_strFromBuffer, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Buffer, MM.Bytes));
	pkgAddFun(th, system,"bufferReset", fun_bufferReset, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Buffer, MM.Buffer));
	pkgAddFun(th, system, "bufferGet", fun_bufferGet, typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.Buffer, MM.I, MM.I));
	pkgAddFun(th, system, "bufferAppendChar", fun_bufferAppendChar, typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.Buffer, MM.I, MM.Buffer));
	pkgAddFun(th, system, "bufferSliceOfStr", fun_bufferSliceOfStr, typeAlloc(th, TYPECODE_FUN, NULL, 4, MM.Buffer, MM.I, MM.I, MM.S));
	pkgAddFun(th, system, "bufferSliceOfBytes", fun_bufferSliceOfStr, typeAlloc(th, TYPECODE_FUN, NULL, 4, MM.Buffer, MM.I, MM.I, MM.Bytes));

	return 0;
}
