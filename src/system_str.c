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
int fun_strConcat(Thread* th)
{
	char* dst;

	LB* b=STACKPNT(th,0);
	LB* a=STACKPNT(th,1);
	if ((!b)||(!a)) FUN_RETURN_PNT(a?a:b);
	FUN_PUSH_STR(NULL,STRLEN(a)+STRLEN(b));
	dst=STRSTART(STACKPNT(th, 0));
	memcpy(dst,STRSTART(a),STRLEN(a));
	memcpy(dst+STRLEN(a),STRSTART(b),STRLEN(b));
	return 0;
}
int fun_strSlice(Thread* th)
{
	int lenIsNil = STACKISNIL(th,0);
	LINT len = STACKINT(th, 0);
	LINT start=STACKINT(th,1);
	LB* p=STACKPNT(th,2);
	FUN_SUBSTR(p, start,len,lenIsNil,STRLEN(p));

	if ((start == 0) && (len == STRLEN(p))) FUN_RETURN_PNT(p);
	FUN_RETURN_STR(STRSTART(p) + start, len);
}

int fun_strLeft(Thread* th)
{
	LINT len=STACKINT(th,0);
	LB* src=STACKPNT(th,1);
	FUN_CROP_LENGTH(src, len, STRLEN(src));
	if (len >= STRLEN(src)) FUN_RETURN_PNT(src);
	FUN_RETURN_STR(STRSTART(src), len);
}

int fun_strRight(Thread* th)
{
	LINT len =STACKINT(th, 0);
	LB* src=STACKPNT(th,1);
	FUN_CROP_LENGTH(src, len, STRLEN(src));
	if (len >= STRLEN(src)) FUN_RETURN_PNT(src);
	FUN_RETURN_STR(STRSTART(src) + STRLEN(src) - len, len);
}

int fun_strLength(Thread* th)
{
	LB* a=STACKPNT(th,0);
	STACKSETINT(th,0,a?STRLEN(a):0);
	return 0;
}

int fun_strCmp(Thread* th)
{
	LINT sza,szb;

	LB* b=STACKPNT(th,0);
	LB* a=STACKPNT(th,1);
	if ((!a)||(!b))
	{
		LINT v=-1;		// to prevent from -Wshift-negative-value warning
		if ((!b)&&(!a)) FUN_RETURN_INT(0);
		if (!a) FUN_RETURN_INT(v);
		FUN_RETURN_INT(1);
	}
	sza=STRLEN(a);
	szb=STRLEN(b);
	FUN_RETURN_INT(memcmp(STRSTART(a),STRSTART(b),(sza>szb)?sza:szb));
}

int fun_strBuild(Thread* th)
{
	if (STACKISNIL(th,0)) FUN_RETURN_NIL;

	bufferReinit(MM.tmpBuffer);
	if (bufferItem(th, MM.tmpBuffer, STACKGET(th, 0),STACKTYPE(th, 0), NULL)) return EXEC_OM;
	FUN_RETURN_BUFFER(MM.tmpBuffer);
}

int fun_strJoin(Thread* th)
{
	LB* p_join = STACKPNT(th, 1);
	if (STACKISNIL(th,0)) FUN_RETURN_NIL;

	bufferReinit(MM.tmpBuffer);
	if (bufferItem(th, MM.tmpBuffer, STACKGET(th, 0),STACKTYPE(th, 0), p_join)) return EXEC_OM;
	FUN_RETURN_BUFFER(MM.tmpBuffer);
}

int fun_strSplit(Thread* th)
{
	char *sa,*sb;
	LINT i=0;
	LINT start=0;
	LINT n=0;

	LB* a=STACKPNT(th,0);
	LB* b=STACKPNT(th,1);
	if ((!a)||(!b)||(!STRLEN(b))) FUN_RETURN_NIL;

	sa=STRSTART(a);
	sb=STRSTART(b);
	for(i=0;i<=STRLEN(a)-STRLEN(b);i++)
	{
		if (!memcmp(sa+i,sb,STRLEN(b)))
		{
			FUN_PUSH_STR(sa+start,i-start);
			n++;
			start=i+STRLEN(b);
			i=start-1;	// because there will be i++ before the next loop
		}
	}
	FUN_PUSH_STR(sa+start,STRLEN(a)-start);
	n++;
	FUN_PUSH_NIL;
	while(n--) FUN_MAKE_TABLE( LIST_LENGTH,DBG_LIST);
	return 0;
}
int fun_strLines(Thread* th)
{
	char* sa;
	LINT i = 0;
	LINT start = 0;
	LINT n = 0;

	LB* a = STACKPNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	sa = STRSTART(a);
	for (i = 0; i < STRLEN(a); i++)
	{
		if ((sa[i] == 13)||(sa[i] == 10))
		{
			FUN_PUSH_STR( sa + start, i - start);
			n++;
			if ((sa[i] == 13) && (sa[i + 1] == 10)) i++;
			start = i + 1;
		}
	}
	FUN_PUSH_STR( sa + start, STRLEN(a) - start);
	n++;
	FUN_PUSH_NIL;
	while (n--) FUN_MAKE_TABLE( LIST_LENGTH, DBG_LIST);
	return 0;
}
int fun_strReplace(Thread* th)
{
	LINT finalsize,i,lc;
	char *dst,*sa,*sb,*sc;

	LB* c=STACKPNT(th,0);
	LB* b=STACKPNT(th,1);
	LB* a=STACKPNT(th,2);
	if ((!a)||(!b)) FUN_RETURN_NIL;
	sa=STRSTART(a);
	sb=STRSTART(b);
	sc=c?STRSTART(c):"";
	lc = c?STRLEN(c):0;
	if (STRLEN(b)==0) FUN_RETURN_PNT(a);

	finalsize=0;
	for(i=0;i<=STRLEN(a)-STRLEN(b);i++) if (!memcmp(sa+i,sb,STRLEN(b)))
	{
		finalsize++;
		i+=STRLEN(b)-1;
	}
	if (!finalsize) FUN_RETURN_PNT(a);

	finalsize=STRLEN(a)+finalsize*(lc-STRLEN(b));
	FUN_PUSH_STR(NULL,finalsize);
	dst=STRSTART(STACKPNT(th, 0));
	for(i=0;i<STRLEN(a);i++) if ((i<=STRLEN(a)-STRLEN(b))&&(!memcmp(sa+i,sb,STRLEN(b))))
	{
		memcpy(dst,sc,lc);
		dst+=lc;
		i+=STRLEN(b)-1;
	}
	else *dst++=sa[i];
	return 0;
}

int fun_strGet(Thread* th)
{
	LINT i=STACKINT(th,0);
	LB* a=STACKPNT(th,1);
	FUN_CHECK_CONTAINS(a, i, 1, STRLEN(a));
	FUN_RETURN_INT(255&((LINT)(STRSTART(a))[i]));
}

int fun_strCharPos(Thread* th)
{
	char* sa;

	LINT vi = STACKINT(th, 0);
	char c = (char)STACKINT(th, 1);
	LB* a = STACKPNT(th, 2);
	FUN_CHECK_CONTAINS(a, vi, 1, STRLEN(a));

	sa = STRSTART(a);
	for (; vi < STRLEN(a); vi++) if (sa[vi]==c) FUN_RETURN_INT(vi);
	FUN_RETURN_NIL;
}

int fun_strCheckPos(Thread* th)
{
	LINT i=STACKINT(th,0);
	LB* b=STACKPNT(th,1);
	LB* a=STACKPNT(th,2);
	if ((!a)||(!b)) FUN_RETURN_NIL;
	if ((i<0)||(i+STRLEN(b)>STRLEN(a))) FUN_RETURN_PNT(MM._false);
	if (!memcmp(STRSTART(a) + i, STRSTART(b), STRLEN(b))) FUN_RETURN_PNT(MM._true);
	FUN_RETURN_PNT(MM._false);
}

int fun_strCharPosRev(Thread* th)
{
	char* sa;

	int viIsNil = STACKISNIL(th,0);
	LINT vi = STACKINT(th, 0);
	char c = (char)STACKINT(th, 1);
	LB* a = STACKPNT(th, 2);
	if (!a) FUN_RETURN_NIL;
	if (viIsNil) vi = STRLEN(a)-1;
	FUN_CHECK_CONTAINS(a, vi, 1, STRLEN(a));
	sa = STRSTART(a);
	for (; vi >= 0; vi--) if (sa[vi] == c) FUN_RETURN_INT(vi);
	FUN_RETURN_NIL;
}
int fun_strPos(Thread* th)
{
	char* sa, * sb;
	char first;

	LINT vi = STACKINT(th, 0);
	LB* b = STACKPNT(th, 1);
	LB* a = STACKPNT(th, 2);
	if ((!b)||(!STRLEN(b))) FUN_RETURN_NIL;
	FUN_CHECK_CONTAINS(a, vi, STRLEN(b), STRLEN(a));
	sa = STRSTART(a);
	sb = STRSTART(b);
	first = sb[0];
	for (; vi <= STRLEN(a) - STRLEN(b); vi++)
		if ((sa[vi] == first) && !memcmp(sa + vi, sb, STRLEN(b))) FUN_RETURN_INT(vi);
	FUN_RETURN_NIL;
}
int fun_strPosRev(Thread* th)
{
	char* sa, * sb;
	char first;

	int viIsNil = STACKISNIL(th,0);
	LINT vi = STACKINT(th, 0);
	LB* b = STACKPNT(th, 1);
	LB* a = STACKPNT(th, 2);
	if ((!b) || (!STRLEN(b))) FUN_RETURN_NIL;
	if (a && viIsNil) vi = STRLEN(a) - STRLEN(b);
	FUN_CHECK_CONTAINS(a, vi, STRLEN(b), STRLEN(a));

	sa = STRSTART(a);
	sb = STRSTART(b);
	first = sb[0];
	for (; vi >= 0; vi--) if ((sa[vi] == first) && !memcmp(sa + vi, sb, STRLEN(b))) FUN_RETURN_INT(vi);
	FUN_RETURN_NIL;
}

int fun_strFromChar(Thread* th)
{
	char c = (char)STACKINT(th, 0);
	if (STACKISNIL(th,0)) FUN_RETURN_NIL;
	FUN_RETURN_STR(&c, 1);
}

int fun_strRand(Thread* th)
{
	LINT len=STACKINT(th,0);
	if (len<=0) FUN_RETURN_NIL;
	FUN_PUSH_STR(NULL,len);
	hwRandomBytes(STRSTART(STACKPNT(th, 0)),len);
	return 0;
}

int coreStrInit(Thread* th, Pkg *system)
{
	Type* u0=typeAllocUndef(th);
	Type* list_S=typeAlloc(th,TYPECODE_LIST,NULL,1,MM.S);
	Type* fun_I_S = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.I, MM.S);
	Type* fun_S_I=typeAlloc(th,TYPECODE_FUN,NULL,2,MM.S,MM.I);
	Type* fun_S_S_I=typeAlloc(th,TYPECODE_FUN,NULL,3,MM.S,MM.S,MM.I);
	Type* fun_S_I_S=typeAlloc(th,TYPECODE_FUN,NULL,3,MM.S,MM.I,MM.S);
	Type* fun_S_I_I_I =typeAlloc(th,TYPECODE_FUN,NULL,4,MM.S,MM.I,MM.I,MM.I);
	Type* fun_S_S_I_I=typeAlloc(th,TYPECODE_FUN,NULL,4,MM.S,MM.S,MM.I,MM.I);
	Type* fun_S_S_I_B=typeAlloc(th,TYPECODE_FUN,NULL,4,MM.S,MM.S,MM.I,MM.Boolean);
	Type* fun_S_S_S_S=typeAlloc(th,TYPECODE_FUN,NULL,4,MM.S,MM.S,MM.S,MM.S);
	Type* fun_S_I_I_S=typeAlloc(th,TYPECODE_FUN,NULL,4,MM.S,MM.I,MM.I,MM.S);

	Type* fun_S_I_I = typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.S, MM.I, MM.I);

	pkgAddFun(th, system,"strConcat",fun_strConcat,typeAlloc(th,TYPECODE_FUN,NULL,3,MM.S,MM.S,MM.S));
	pkgAddFun(th, system,"strLength",fun_strLength,fun_S_I);
	pkgAddFun(th, system,"strCmp",fun_strCmp,fun_S_S_I);
	pkgAddFun(th, system,"strSlice",fun_strSlice,fun_S_I_I_S);
	pkgAddFun(th, system,"strLeft",fun_strLeft,fun_S_I_S);
	pkgAddFun(th, system,"strRight",fun_strRight,fun_S_I_S);

	pkgAddFun(th, system,"strGet",fun_strGet, fun_S_I_I);
	pkgAddFun(th, system,"strCharPos", fun_strCharPos, fun_S_I_I_I);
	pkgAddFun(th, system,"strPos",fun_strPos,fun_S_S_I_I);
	pkgAddFun(th, system,"strCharPosRev", fun_strCharPosRev, fun_S_I_I_I);
	pkgAddFun(th, system,"strPosRev",fun_strPosRev, fun_S_S_I_I);
	pkgAddFun(th, system, "strCheckPos", fun_strCheckPos, fun_S_S_I_B);

	pkgAddFun(th, system, "bytesBuild", fun_strBuild, typeAlloc(th, TYPECODE_FUN, NULL, 2, u0, MM.Bytes));
	pkgAddFun(th, system,"strBuild",fun_strBuild,typeAlloc(th,TYPECODE_FUN,NULL,2,u0,MM.S));
	pkgAddFun(th, system,"strListConcat",fun_strBuild, typeAlloc(th,TYPECODE_FUN, NULL, 2, list_S, MM.S));	// idem strBuild, but with a different type
	pkgAddFun(th, system,"strJoin",fun_strJoin,typeAlloc(th,TYPECODE_FUN,NULL,3,MM.S, u0, MM.S));
	pkgAddFun(th, system,"strSplit",fun_strSplit,typeAlloc(th,TYPECODE_FUN,NULL,3,MM.S,MM.S,list_S));
	pkgAddFun(th, system,"strLines",fun_strLines,typeAlloc(th,TYPECODE_FUN,NULL,2,MM.S,list_S));
		
	pkgAddFun(th, system,"strReplace",fun_strReplace,fun_S_S_S_S);

	pkgAddFun(th, system, "strFromChar", fun_strFromChar, fun_I_S);
	pkgAddFun(th, system,"strRand",fun_strRand,fun_I_S);


	return 0;
}


