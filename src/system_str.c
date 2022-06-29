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
int fun_strCat(Thread* th)
{
	char* dst;
	LINT NDROP=2-1;
	LW result=NIL;

	LB* b=VALTOPNT(STACKGET(th,0));
	LB* a=VALTOPNT(STACKGET(th,1));
	if ((!b)||(!a))
	{
		result=PNTTOVAL(a?a:b);
		goto cleanup;
	}
	if (stackPushStr(th,NULL,STRLEN(a)+STRLEN(b))) return EXEC_OM;
	NDROP++;
	result = STACKGET(th, 0);
	dst=STRSTART(VALTOPNT(result));
	memcpy(dst,STRSTART(a),STRLEN(a));
	memcpy(dst+STRLEN(a),STRSTART(b),STRLEN(b));
cleanup:
	STACKSET(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}
int fun_strSub(Thread* th)
{
	LB* p;
	LINT len;
	LINT NDROP=3-1;
	LW result=NIL;

	LW vlen=STACKGET(th,0);
	LINT start=VALTOINT(STACKGET(th,1));
	LB* a=VALTOPNT(STACKGET(th,2));
	if (!a) goto cleanup;
	if (start < 0) start += STRLEN(a);
	if (start < 0 || start >= STRLEN(a)) goto cleanup;

	len=(vlen==NIL)?STRLEN(a):VALTOINT(vlen);
	if ((start + len) > STRLEN(a)) len = STRLEN(a) - start;
	if ((start == 0) && (len == STRLEN(a)))
	{
		STACKDROPN(th, 2);
		return 0;
	}
	p = memoryAllocStr(th, STRSTART(a) + start, len); if (!p) return EXEC_OM;
	result=PNTTOVAL(p);
cleanup:
	STACKSET(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}
int fun_strLeft(Thread* th)
{
	LINT i;
	LINT NDROP=2-1;
	LW result=NIL;

	LW vi=STACKGET(th,0);
	LB* a=VALTOPNT(STACKGET(th,1));
	if ((vi==NIL)||(!a)) goto cleanup;
	i=VALTOINT(vi);
	if (i >= STRLEN(a))
	{
		result = STACKGET(th, 1);
		goto cleanup;
	}
	if (i < 0) i += STRLEN(a);
	if (i<=0) goto cleanup;
	if (stackPushStr(th,STRSTART(a),i)) return EXEC_OM;
	NDROP++;
	result = STACKGET(th, 0);

cleanup:
	STACKSET(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}

int fun_strRight(Thread* th)
{
	LINT i;
	LINT NDROP=2-1;
	LW result=NIL;

	LW vi=STACKGET(th,0);
	LB* a=VALTOPNT(STACKGET(th,1));
	if ((vi==NIL)||(!a)) goto cleanup;
	i=VALTOINT(vi);
	if (i >= STRLEN(a))
	{
		result = STACKGET(th, 1);
		goto cleanup;
	}
	if (i< 0) i+=STRLEN(a);
	if (i<=0) goto cleanup;
	if (stackPushStr(th,STRSTART(a)+STRLEN(a)-i,i)) return EXEC_OM;
	NDROP++;
	result = STACKGET(th, 0);

cleanup:
	STACKSET(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}

int fun_strLength(Thread* th)
{
	LB* a=VALTOPNT(STACKGET(th,0));
	if (!a) STACKSETINT(th, 0, (0));
	else STACKSETINT(th,0,(STRLEN(a)));
	return 0;
}

int fun_strCmp(Thread* th)
{
	LINT sza,szb;
	LINT NDROP=2-1;
	LW result=NIL;

	LB* b=VALTOPNT(STACKGET(th,0));
	LB* a=VALTOPNT(STACKGET(th,1));
	if ((!a)||(!b))
	{
		LINT v=-1;		// to prevent from -Wshift-negative-value warning
		if ((!b)&&(!a)) result=INTTOVAL(0);
		else if (!a) result=INTTOVAL(v);
		else result=INTTOVAL(1);
		goto cleanup;
	}
	sza=STRLEN(a);
	szb=STRLEN(b);
	result=INTTOVAL(memcmp(STRSTART(a),STRSTART(b),(sza>szb)?sza:szb));
cleanup:
	STACKSETSAFE(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}

int fun_strBuild(Thread* th)
{
	int k;
	LB* p;
	LINT NDROP=1-1;
	LW result=NIL;

	LW a=STACKGET(th,0);
	if (a==NIL) goto cleanup;
	if ((ISVALPNT(a)) && (HEADER_DBG(VALTOPNT(a)) == DBG_S)) return 0;

	bufferReinit(MM.tmpBuffer);
	if ((k = bufferItem(th, MM.tmpBuffer, a, NULL, 0, 0))) return k;
	p = memoryAllocFromBuffer(th, MM.tmpBuffer); if (!p) return EXEC_OM;
	result = PNTTOVAL(p);
cleanup:
	STACKSET(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}

int fun_strJoin(Thread* th)
{
	int k;
	LB* p;
	LINT NDROP = 2 - 1;
	LW result = NIL;

	LB* p_join = VALTOPNT(STACKGET(th, 1));
	LW a = STACKGET(th, 0);
	if (a == NIL) goto cleanup;
	if ((ISVALPNT(a)) && (HEADER_DBG(VALTOPNT(a)) == DBG_S)) return 0;

	bufferReinit(MM.tmpBuffer);
	if ((k = bufferItem(th, MM.tmpBuffer, a, p_join, 0, 0))) return k;
	p = memoryAllocFromBuffer(th, MM.tmpBuffer); if (!p) return EXEC_OM;
	result = PNTTOVAL(p);
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}

int fun_strSplit(Thread* th)
{
	char *sa,*sb;
	LINT i=0;
	LINT start=0;
	LINT n=0;
	LINT NDROP=2-1;
	LW result=NIL;

	LB* a=VALTOPNT(STACKGET(th,0));
	LB* b=VALTOPNT(STACKGET(th,1));
	if ((!a)||(!b)) goto cleanup;
	if (!STRLEN(b)) goto cleanup;
	sa=STRSTART(a);
	sb=STRSTART(b);

	for(i=0;i<=STRLEN(a)-STRLEN(b);i++)
	{
		if (!memcmp(sa+i,sb,STRLEN(b)))
		{
			if (stackPushStr(th,sa+start,i-start)) return EXEC_OM;
			n++;
			start=i+STRLEN(b);
			i=start-1;	// because there will be i++ before the next loop
		}
	}
	if (stackPushStr(th,sa+start,STRLEN(a)-start)) return EXEC_OM;
	n++;
	result = STACKGET(th, 0);

	STACKPUSH_OM(th, NIL,EXEC_OM);
	while(n--) if (DEFTAB(th, LIST_LENGTH,DBG_LIST)) return EXEC_OM;
	result = STACKGET(th, 0);
	NDROP++;
cleanup:
	STACKSET(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}

int fun_strReplace(Thread* th)
{
	LINT finalsize,i,lc;
	char *dst,*sa,*sb,*sc;
	LINT NDROP=3-1;
	LW result=NIL;

	LB* c=VALTOPNT(STACKGET(th,0));
	LB* b=VALTOPNT(STACKGET(th,1));
	LB* a=VALTOPNT(STACKGET(th,2));
	if ((!a)||(!b)) goto cleanup;
	sa=STRSTART(a);
	sb=STRSTART(b);
	sc=c?STRSTART(c):"";
	lc = c?STRLEN(c):0;
	result=PNTTOVAL(a);
	if (STRLEN(b)==0) goto cleanup;

	finalsize=0;
	for(i=0;i<=STRLEN(a)-STRLEN(b);i++) if (!memcmp(sa+i,sb,STRLEN(b)))
	{
		finalsize++;
		i+=STRLEN(b)-1;
	}
	if (!finalsize) goto cleanup;

	finalsize=STRLEN(a)+finalsize*(lc-STRLEN(b));
	if (stackPushStr(th,NULL,finalsize)) return EXEC_OM;
	NDROP++;
	result = STACKGET(th, 0);

	dst=STRSTART(VALTOPNT(result));

	for(i=0;i<STRLEN(a);i++) if ((i<=STRLEN(a)-STRLEN(b))&&(!memcmp(sa+i,sb,STRLEN(b))))
	{
		memcpy(dst,sc,lc);
		dst+=lc;
		i+=STRLEN(b)-1;
	}
	else *dst++=sa[i];

cleanup:
	STACKSET(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}

int fun_strGet(Thread* th)
{
	LINT i;
	LINT NDROP=2-1;
	LW result=NIL;

	LW vi=STACKGET(th,0);
	LB* a=VALTOPNT(STACKGET(th,1));
	if ((vi==NIL)||(!a)) goto cleanup;
	i=VALTOINT(vi);
	if (i < 0) i += STRLEN(a);
	if ((i<0)||(i>=STRLEN(a))) goto cleanup;
	result=INTTOVAL(255&((LINT)(STRSTART(a))[i]));
cleanup:
	STACKSETSAFE(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}

int fun_strCharPos(Thread* th)
{
	LINT i, vi;
	char* sa;
	LINT NDROP = 3 - 1;
	LW result = NIL;

	LW vvi = STACKGET(th, 0);
	char c = (char)(VALTOINT(STACKGET(th, 1)));
	LB* a = VALTOPNT(STACKGET(th, 2));
	if ((!a) || (vvi == NIL)) goto cleanup;
	vi = VALTOINT(vvi);
	if (vi < 0) vi += STRLEN(a);
	if ((vi < 0) || (vi >= STRLEN(a))) goto cleanup;
	sa = STRSTART(a);
	for (i = vi; i < STRLEN(a); i++) if (sa[i]==c)
	{
		result = INTTOVAL(i);
		goto cleanup;
	}
cleanup:
	STACKSETSAFE(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}

int fun_strPos(Thread* th)
{
	LINT i,vi;
	char *sa,*sb;
	LINT NDROP=3-1;
	LW result=NIL;

	LW vvi=STACKGET(th,0);
	LB* b=VALTOPNT(STACKGET(th,1));
	LB* a=VALTOPNT(STACKGET(th,2));
	if ((!a)||(!b)||(vvi==NIL)) goto cleanup;
	vi=VALTOINT(vvi);
	if (vi < 0) vi += STRLEN(a);
	if ((vi<0)||(vi+STRLEN(b)>STRLEN(a))) goto cleanup;
	sa=STRSTART(a);
	sb=STRSTART(b);
	for(i=vi;i<=STRLEN(a)-STRLEN(b);i++) if (!memcmp(sa+i,sb,STRLEN(b)))
	{
		result=INTTOVAL(i);
		goto cleanup;
	}
cleanup:
	STACKSETSAFE(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}
int fun_strCheckPos(Thread* th)
{
	LINT NDROP=3-1;
	LW result= MM.falseRef;

	LINT i=VALTOINT(STACKGET(th,0));
	LB* b=VALTOPNT(STACKGET(th,1));
	LB* a=VALTOPNT(STACKGET(th,2));
	if ((!a)||(!b)) goto cleanup;
	if ((i<0)||(i+STRLEN(b)>STRLEN(a))) goto cleanup;
	if (!memcmp(STRSTART(a) + i, STRSTART(b), STRLEN(b))) result = MM.trueRef;
cleanup:
	STACKSETSAFE(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}

int fun_strCharPosRev(Thread* th)
{
	LINT i, vi;
	char* sa;
	LINT NDROP = 3 - 1;
	LW result = NIL;

	LW vvi = STACKGET(th, 0);
	char c = (char)(VALTOINT(STACKGET(th, 1)));
	LB* a = VALTOPNT(STACKGET(th, 2));
	if (!a) goto cleanup;
	vi = (vvi == NIL) ? STRLEN(a)-1 : VALTOINT(vvi);
	if (vi < 0) vi += STRLEN(a);
	if ((vi < 0) || (vi >= STRLEN(a))) goto cleanup;
	sa = STRSTART(a);
	for (i = vi; i >= 0; i--) if (sa[i] == c)
	{
		result = INTTOVAL(i);
		goto cleanup;
	}
cleanup:
	STACKSETSAFE(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}
int fun_strPosRev(Thread* th)
{
	LINT i, vi;
	char* sa, * sb;
	LINT NDROP = 3 - 1;
	LW result = NIL;

	LW vvi = STACKGET(th, 0);
	LB* b = VALTOPNT(STACKGET(th, 1));
	LB* a = VALTOPNT(STACKGET(th, 2));
	if ((!a) || (!b)) goto cleanup;
	vi = (vvi == NIL) ? STRLEN(a)- STRLEN(b) : VALTOINT(vvi);
	if (vi < 0) vi += STRLEN(a);
	if ((vi < 0) || (vi + STRLEN(b) > STRLEN(a))) goto cleanup;

	sa = STRSTART(a);
	sb = STRSTART(b);
	for (i = vi; i >= 0; i--) if (!memcmp(sa + i, sb, STRLEN(b)))
	{
		result = INTTOVAL(i);
		goto cleanup;
	}
cleanup:
	STACKSETSAFE(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}

int fun_charToStr(Thread* th)
{
	char c;
	LW va = STACKGET(th, 0);
	if (va == NIL) return 0;

	c = (char)VALTOINT(va);
	STACKDROP(th);
	if (stackPushStr(th, &c, 1)) return EXEC_OM;
	return 0;
}

int fun_strRand(Thread* th)
{
	LINT NDROP=1-1;
	LW result=NIL;

	LW va=STACKGET(th,0);
	if (va==NIL) goto cleanup;
	if (stackPushStr(th,NULL,VALTOINT(va))) return EXEC_OM;
	NDROP++;
	result = STACKGET(th, 0);
	lsRand(STRSTART(VALTOPNT(result)),VALTOINT(va));
cleanup:
	STACKSET(th,NDROP,result);
	STACKDROPN(th,NDROP);
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

	pkgAddFun(th, system,"strCat",fun_strCat,typeAlloc(th,TYPECODE_FUN,NULL,3,MM.S,MM.S,MM.S));
	pkgAddFun(th, system,"strLength",fun_strLength,fun_S_I);
	pkgAddFun(th, system,"strCmp",fun_strCmp,fun_S_S_I);
	pkgAddFun(th, system,"strSub",fun_strSub,fun_S_I_I_S);
	pkgAddFun(th, system,"strLeft",fun_strLeft,fun_S_I_S);
	pkgAddFun(th, system,"strRight",fun_strRight,fun_S_I_S);

	pkgAddFun(th, system,"strGet",fun_strGet, fun_S_I_I);
	pkgAddFun(th, system,"strCharPos", fun_strCharPos, fun_S_I_I_I);
	pkgAddFun(th, system,"strPos",fun_strPos,fun_S_S_I_I);
	pkgAddFun(th, system,"strCharPosRev", fun_strCharPosRev, fun_S_I_I_I);
	pkgAddFun(th, system,"strPosRev",fun_strPosRev, fun_S_S_I_I);
	pkgAddFun(th, system, "strCheckPos", fun_strCheckPos, fun_S_S_I_B);

	pkgAddFun(th, system,"strBuild",fun_strBuild,typeAlloc(th,TYPECODE_FUN,NULL,2,u0,MM.S));
	pkgAddFun(th, system,"strListCat",fun_strBuild, typeAlloc(th,TYPECODE_FUN, NULL, 2, list_S, MM.S));	// idem strBuild, but with another type
	pkgAddFun(th, system,"strJoin",fun_strJoin,typeAlloc(th,TYPECODE_FUN,NULL,3,MM.S, u0, MM.S));
	pkgAddFun(th, system,"strSplit",fun_strSplit,typeAlloc(th,TYPECODE_FUN,NULL,3,MM.S,MM.S,list_S));
		
	pkgAddFun(th, system,"strReplace",fun_strReplace,fun_S_S_S_S);

	pkgAddFun(th, system, "charToStr", fun_charToStr, fun_I_S);
	pkgAddFun(th, system,"strRand",fun_strRand,fun_I_S);


	return 0;
}
/* 
pour memoire dans metal v2:
 "wordextr","wordbuild","strsign","strsigni","zip",
 "unzip","strswap"
};
*/

