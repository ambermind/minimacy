// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

int fun_bufferCreate(Thread* th)
{
	Buffer* p= bufferCreate(); if (!p) return EXEC_OM;
	FUN_RETURN_PNT((LB*)p);
}
int fun_bufferCreateWithSize(Thread* th)
{
	Buffer* p= bufferCreateWithSize(STACK_INT(th,0)); if (!p) return EXEC_OM;
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
	if (b&&bufferAddChar(b,(char)c)) return EXEC_OM;
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

	if (bufferItem(b,STACK_GET(th, 0),STACK_TYPE(th, 0),NULL)) return EXEC_OM;
	STACK_DROP(th);
	return 0;
}
int fun_bufferAppendJoin(Thread* th)
{
	LB* join = STACK_PNT(th, 1);
	Buffer* b = (Buffer*)STACK_PNT(th, 2);
	if (STACK_IS_NIL(th, 0) || !b ||!join) FUN_RETURN_NIL;

	if (bufferItem(b, STACK_GET(th, 0), STACK_TYPE(th, 0), join)) return EXEC_OM;
	STACK_DROPN(th,2);
	return 0;
}

int systemBufferInit(Pkg *system)
{
	static const Native nativeDefs[] = { 
		{ NATIVE_FUN, "bufferCreate", fun_bufferCreate, "fun -> Buffer"},
		{ NATIVE_FUN, "bufferCreateWithSize", fun_bufferCreateWithSize, "fun Int -> Buffer" },
		{ NATIVE_FUN, "bufferAppend", fun_bufferAppend, "fun Buffer a1 -> Buffer" },
		{ NATIVE_FUN, "bufferAppendJoin", fun_bufferAppendJoin, "fun Buffer Str a1 -> Buffer" },
		{ NATIVE_FUN, "bufferLength", fun_bufferLength, "fun Buffer -> Int" },
		{ NATIVE_FUN, "strFromBuffer", fun_strFromBuffer, "fun Buffer -> Str" },
		{ NATIVE_FUN, "bytesFromBuffer", fun_strFromBuffer, "fun Buffer -> Bytes" },
		{ NATIVE_FUN, "bufferReset", fun_bufferReset, "fun Buffer -> Buffer" },
		{ NATIVE_FUN, "bufferRemove", fun_bufferRemove, "fun Buffer Int -> Buffer" },
		{ NATIVE_FUN, "bufferGet", fun_bufferGet, "fun Buffer Int -> Int" },
		{ NATIVE_FUN, "bufferAppendChar", fun_bufferAppendChar, "fun Buffer Int -> Buffer" },
		{ NATIVE_FUN, "bufferSliceOfStr", fun_bufferSliceOfStr, "fun Buffer Int Int -> Str" },
		{ NATIVE_FUN, "bufferSliceOfBytes", fun_bufferSliceOfStr, "fun Buffer Int Int -> Bytes" },
	};
	NATIVE_DEF(nativeDefs);
	return 0;
}
