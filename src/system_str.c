// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"
int fun_strConcat(Thread* th)
{
	char* dst;

	LB* b=STACK_PNT(th,0);
	LB* a=STACK_PNT(th,1);
	if ((!b)||(!a)) FUN_RETURN_PNT(a?a:b);
	FUN_PUSH_STR(NULL,STR_LENGTH(a)+STR_LENGTH(b));
	dst=STR_START(STACK_PNT(th, 0));
	memcpy(dst,STR_START(a),STR_LENGTH(a));
	memcpy(dst+STR_LENGTH(a),STR_START(b),STR_LENGTH(b));
	return 0;
}
int fun_strSlice(Thread* th)
{
	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT start=STACK_INT(th,1);
	LB* p=STACK_PNT(th,2);
	FUN_SUBSTR(p, start,len,lenIsNil,STR_LENGTH(p));

	if ((start == 0) && (len == STR_LENGTH(p))) FUN_RETURN_PNT(p);
	FUN_RETURN_STR(STR_START(p) + start, len);
}
int fun_strTail(Thread* th)
{
	int lenIsNil = 1;
	LINT len = 0;
	LINT start=STACK_INT(th,0);
	LB* p=STACK_PNT(th,1);
	FUN_SUBSTR(p, start,len,lenIsNil,STR_LENGTH(p));

	if ((start == 0) && (len == STR_LENGTH(p))) FUN_RETURN_PNT(p);
	FUN_RETURN_STR(STR_START(p) + start, len);
}

int fun_strLeft(Thread* th)
{
	LINT len=STACK_INT(th,0);
	LB* src=STACK_PNT(th,1);
	FUN_CROP_LENGTH(src, len, STR_LENGTH(src));
	if (len >= STR_LENGTH(src)) FUN_RETURN_PNT(src);
	FUN_RETURN_STR(STR_START(src), len);
}

int fun_strRight(Thread* th)
{
	LINT len =STACK_INT(th, 0);
	LB* src=STACK_PNT(th,1);
	FUN_CROP_LENGTH(src, len, STR_LENGTH(src));
	if (len >= STR_LENGTH(src)) FUN_RETURN_PNT(src);
	FUN_RETURN_STR(STR_START(src) + STR_LENGTH(src) - len, len);
}

int fun_strLength(Thread* th)
{
	LB* a=STACK_PNT(th,0);
	STACK_SET_INT(th,0,a?STR_LENGTH(a):0);
	return 0;
}

int fun_strCmp(Thread* th)
{
	LINT sza,szb;

	LB* b=STACK_PNT(th,0);
	LB* a=STACK_PNT(th,1);
	if ((!a)||(!b))
	{
		LINT v=-1;		// to prevent from -Wshift-negative-value warning
		if ((!b)&&(!a)) FUN_RETURN_INT(0);
		if (!a) FUN_RETURN_INT(v);
		FUN_RETURN_INT(1);
	}
	sza=STR_LENGTH(a);
	szb=STR_LENGTH(b);
	FUN_RETURN_INT(memcmp(STR_START(a),STR_START(b),(sza>szb)?sza:szb));
}

int fun_strBuild(Thread* th)
{
	if (STACK_IS_NIL(th,0)) FUN_RETURN_NIL;

	bufferReinit(MM.tmpBuffer);
	if (bufferItem(MM.tmpBuffer, STACK_GET(th, 0),STACK_TYPE(th, 0), NULL)) return EXEC_OM;
	FUN_RETURN_BUFFER(MM.tmpBuffer);
}

int fun_strJoin(Thread* th)
{
	LB* p_join = STACK_PNT(th, 1);
	if (STACK_IS_NIL(th,0)) FUN_RETURN_NIL;

	bufferReinit(MM.tmpBuffer);
	if (bufferItem(MM.tmpBuffer, STACK_GET(th, 0),STACK_TYPE(th, 0), p_join)) return EXEC_OM;
	FUN_RETURN_BUFFER(MM.tmpBuffer);
}

int fun_strSplit(Thread* th)
{
	char *sa,*sb;
	LINT i=0;
	LINT start=0;
	LINT n=0;

	LB* a=STACK_PNT(th,0);
	LB* b=STACK_PNT(th,1);
	if ((!a)||(!b)||(!STR_LENGTH(b))) FUN_RETURN_NIL;

	sa=STR_START(a);
	sb=STR_START(b);
	for(i=0;i<=STR_LENGTH(a)-STR_LENGTH(b);i++)
	{
		if (!memcmp(sa+i,sb,STR_LENGTH(b)))
		{
			FUN_PUSH_STR(sa+start,i-start);
			n++;
			start=i+STR_LENGTH(b);
			i=start-1;	// because there will be i++ before the next loop
		}
	}
	FUN_PUSH_STR(sa+start,STR_LENGTH(a)-start);
	n++;
	FUN_PUSH_NIL;
	while(n--) FUN_MAKE_ARRAY( LIST_LENGTH,DBG_LIST);
	return 0;
}
int fun_strLines(Thread* th)
{
	char* sa;
	LINT i = 0;
	LINT start = 0;
	LINT n = 0;

	LB* a = STACK_PNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	sa = STR_START(a);
	for (i = 0; i < STR_LENGTH(a); i++)
	{
		if ((sa[i] == 13)||(sa[i] == 10))
		{
			FUN_PUSH_STR( sa + start, i - start);
			n++;
			if ((sa[i] == 13) && (sa[i + 1] == 10)) i++;
			start = i + 1;
		}
	}
	FUN_PUSH_STR( sa + start, STR_LENGTH(a) - start);
	n++;
	FUN_PUSH_NIL;
	while (n--) FUN_MAKE_ARRAY( LIST_LENGTH, DBG_LIST);
	return 0;
}
int fun_strReplace(Thread* th)
{
	LINT finalsize,i,lc;
	char *dst,*sa,*sb,*sc;

	LB* c=STACK_PNT(th,0);
	LB* b=STACK_PNT(th,1);
	LB* a=STACK_PNT(th,2);
	if ((!a)||(!b)) FUN_RETURN_NIL;
	sa=STR_START(a);
	sb=STR_START(b);
	sc=c?STR_START(c):"";
	lc = c?STR_LENGTH(c):0;
	if (STR_LENGTH(b)==0) FUN_RETURN_PNT(a);

	finalsize=0;
	for(i=0;i<=STR_LENGTH(a)-STR_LENGTH(b);i++) if (!memcmp(sa+i,sb,STR_LENGTH(b)))
	{
		finalsize++;
		i+=STR_LENGTH(b)-1;
	}
	if (!finalsize) FUN_RETURN_PNT(a);

	finalsize=STR_LENGTH(a)+finalsize*(lc-STR_LENGTH(b));
	FUN_PUSH_STR(NULL,finalsize);
	dst=STR_START(STACK_PNT(th, 0));
	for(i=0;i<STR_LENGTH(a);i++) if ((i<=STR_LENGTH(a)-STR_LENGTH(b))&&(!memcmp(sa+i,sb,STR_LENGTH(b))))
	{
		memcpy(dst,sc,lc);
		dst+=lc;
		i+=STR_LENGTH(b)-1;
	}
	else *dst++=sa[i];
	return 0;
}

int fun_strGet(Thread* th)
{
	LINT i=STACK_INT(th,0);
	LB* a=STACK_PNT(th,1);
	FUN_CHECK_CONTAINS(a, i, 1, STR_LENGTH(a));
	FUN_RETURN_INT(255&((LINT)(STR_START(a))[i]));
}

int fun_strCharPos(Thread* th)
{
	char* sa;

	LINT vi = STACK_INT(th, 0);
	char c = (char)STACK_INT(th, 1);
	LB* a = STACK_PNT(th, 2);
	FUN_CHECK_CONTAINS(a, vi, 1, STR_LENGTH(a));

	sa = STR_START(a);
	for (; vi < STR_LENGTH(a); vi++) if (sa[vi]==c) FUN_RETURN_INT(vi);
	FUN_RETURN_NIL;
}

int fun_strCheckPos(Thread* th)
{
	LINT i=STACK_INT(th,0);
	LB* b=STACK_PNT(th,1);
	LB* a=STACK_PNT(th,2);
	if ((!a)||(!b)) FUN_RETURN_NIL;
	if ((i<0)||(i+STR_LENGTH(b)>STR_LENGTH(a))) FUN_RETURN_PNT(MM._false);
	if (!memcmp(STR_START(a) + i, STR_START(b), STR_LENGTH(b))) FUN_RETURN_PNT(MM._true);
	FUN_RETURN_PNT(MM._false);
}

int fun_strCharPosRev(Thread* th)
{
	char* sa;

	int viIsNil = STACK_IS_NIL(th,0);
	LINT vi = STACK_INT(th, 0);
	char c = (char)STACK_INT(th, 1);
	LB* a = STACK_PNT(th, 2);
	if (!a) FUN_RETURN_NIL;
	if (viIsNil) vi = STR_LENGTH(a)-1;
	FUN_CHECK_CONTAINS(a, vi, 1, STR_LENGTH(a));
	sa = STR_START(a);
	for (; vi >= 0; vi--) if (sa[vi] == c) FUN_RETURN_INT(vi);
	FUN_RETURN_NIL;
}
int fun_strPos(Thread* th)
{
	char* sa, * sb;
	char first;

	LINT vi = STACK_INT(th, 0);
	LB* b = STACK_PNT(th, 1);
	LB* a = STACK_PNT(th, 2);
	if ((!b)||(!STR_LENGTH(b))) FUN_RETURN_NIL;
	FUN_CHECK_CONTAINS(a, vi, STR_LENGTH(b), STR_LENGTH(a));
	sa = STR_START(a);
	sb = STR_START(b);
	first = sb[0];
	for (; vi <= STR_LENGTH(a) - STR_LENGTH(b); vi++)
		if ((sa[vi] == first) && !memcmp(sa + vi, sb, STR_LENGTH(b))) FUN_RETURN_INT(vi);
	FUN_RETURN_NIL;
}
int fun_strPosRev(Thread* th)
{
	char* sa, * sb;
	char first;

	int viIsNil = STACK_IS_NIL(th,0);
	LINT vi = STACK_INT(th, 0);
	LB* b = STACK_PNT(th, 1);
	LB* a = STACK_PNT(th, 2);
	if ((!b) || (!STR_LENGTH(b))) FUN_RETURN_NIL;
	if (a && viIsNil) vi = STR_LENGTH(a) - STR_LENGTH(b);
	FUN_CHECK_CONTAINS(a, vi, STR_LENGTH(b), STR_LENGTH(a));

	sa = STR_START(a);
	sb = STR_START(b);
	first = sb[0];
	for (; vi >= 0; vi--) if ((sa[vi] == first) && !memcmp(sa + vi, sb, STR_LENGTH(b))) FUN_RETURN_INT(vi);
	FUN_RETURN_NIL;
}

int fun_strFromChar(Thread* th)
{
	char c = (char)STACK_INT(th, 0);
	if (STACK_IS_NIL(th,0)) FUN_RETURN_NIL;
	FUN_RETURN_STR(&c, 1);
}

int fun_strRand(Thread* th)
{
	LINT len=STACK_INT(th,0);
	if (len<=0) FUN_RETURN_NIL;
	FUN_PUSH_STR(NULL,len);
	hwRandomBytes(STR_START(STACK_PNT(th, 0)),len);
	return 0;
}

int systemStrInit(Pkg *system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "strConcat", fun_strConcat, "fun Str Str -> Str"},
		{ NATIVE_FUN, "strLength", fun_strLength, "fun Str -> Int"},
		{ NATIVE_FUN, "strCmp", fun_strCmp, "fun Str Str -> Int"},
		{ NATIVE_FUN, "strSlice", fun_strSlice, "fun Str Int Int -> Str"},
		{ NATIVE_FUN, "strTail", fun_strTail, "fun Str Int -> Str"},
		{ NATIVE_FUN, "strLeft", fun_strLeft, "fun Str Int -> Str"},
		{ NATIVE_FUN, "strRight", fun_strRight, "fun Str Int -> Str"},
		{ NATIVE_FUN, "strGet", fun_strGet, "fun Str Int -> Int"},
		{ NATIVE_FUN, "strCharPos", fun_strCharPos, "fun Str Int Int -> Int"},
		{ NATIVE_FUN, "strPos", fun_strPos, "fun Str Str Int -> Int"},
		{ NATIVE_FUN, "strCharPosRev", fun_strCharPosRev, "fun Str Int Int -> Int"},
		{ NATIVE_FUN, "strPosRev", fun_strPosRev, "fun Str Str Int -> Int"},
		{ NATIVE_FUN, "strCheckPos", fun_strCheckPos, "fun Str Str Int -> Bool"},
		{ NATIVE_FUN, "bytesBuild", fun_strBuild, "fun a1 -> Bytes"},
		{ NATIVE_FUN, "strBuild", fun_strBuild, "fun a1 -> Str"},
		{ NATIVE_FUN, "strListConcat", fun_strBuild, "fun list Str -> Str"},
		{ NATIVE_FUN, "strJoin", fun_strJoin, "fun Str a1 -> Str"},
		{ NATIVE_FUN, "strSplit", fun_strSplit, "fun Str Str -> list Str"},
		{ NATIVE_FUN, "strLines", fun_strLines, "fun Str -> list Str"},
		{ NATIVE_FUN, "strReplace", fun_strReplace, "fun Str Str Str -> Str"},
		{ NATIVE_FUN, "strFromChar", fun_strFromChar, "fun Int -> Str"},
		{ NATIVE_FUN, "strRand", fun_strRand, "fun Int -> Str"},
	};
	NATIVE_DEF(nativeDefs);

	return 0;
}


