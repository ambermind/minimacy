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
#ifndef _CORE_BIGNUM_
#define _CORE_BIGNUM_

#define BIGNUM_MAXWORDS 512

typedef unsigned int uint;
typedef unsigned long long ulonglong;

typedef struct
{
	LB header;

	int len;
	int sign;
	uint data[1];
}Bignum;
typedef Bignum* bignum;

typedef struct
{
	LB header;

	int len;
	int sign;
	uint data[BIGNUM_MAXWORDS];
}BignumRegister;


#define bignumSign(b) (b)->sign
#define bignumSignSet(b,s) (b)->sign=(int)(s)
#define bignumLen(b) (b)->len
#define bignumLenSet(b,l) (b)->len=(int)(l)
#define bignumGet(b,n) (b)->data[n]
#define bignumSet(b,n,v) (b)->data[n]=(uint)(v)

LINT bignumEquals(bignum a, bignum b);
LINT bignumStringHex(bignum b,char* dst);
bignum bignumFromDec(char* src);
bignum bignumFromHex(Thread* th, char* src);

int bignumToBuffer(Thread* th, bignum b, Buffer* buffer);
int bignumToStr(Thread* th, bignum b, char* dest);

LB* bigAlloc(Thread* th, bignum b0);
//----------------------------------------
//#define bigPush(th,b) STACKPUSH(th,PNTTOVAL((LB*)b))
#define _bignumGet(b) (bignum)VALTOPNT(b)

#define bigOpeI_B(name,ope)	\
int name(Thread* th)	\
{	\
	bignum b;	\
	LINT NDROP=1-1;	\
	LW result=NIL;	\
	LW v=STACKGET(th,0); \
	if (v==NIL) goto cleanup;	\
	b=ope(VALTOINT(v)); if (!b) return EXEC_OM; \
	if (bigPush(th,b)) return EXEC_OM; \
	NDROP++;	\
	result = STACKGET(th, 0); \
cleanup:	\
	STACKSET(th,NDROP,result);	\
	STACKDROPN(th,NDROP);	\
	return 0;	\
}

#define bigOpeIBool_B(name,ope)	\
int name(Thread* th)	\
{	\
	bignum b;	\
	LINT NDROP=2-1;	\
	LW result=NIL;	\
	LW v=STACKGET(th,0); \
	LW w=STACKGET(th,1); \
	if ((v==NIL)||(w==NIL)) goto cleanup;	\
	b=ope(VALTOINT(w),v); if (!b) return EXEC_OM; \
	if (bigPush(th,b)) return EXEC_OM; \
	NDROP++;	\
	result = STACKGET(th, 0); \
cleanup:	\
	STACKSET(th,NDROP,result);	\
	STACKDROPN(th,NDROP);	\
	return 0;	\
}

#define bigOpeB_B(name,ope)	\
int name(Thread* th)	\
{	\
	bignum b;	\
	LINT NDROP=1-1;	\
	LW result=NIL;	\
	bignum a=_bignumGet(STACKGET(th,0));	\
	if (!a) goto cleanup;	\
	b=ope(a); if (!b) return EXEC_OM; \
	if (bigPush(th,b)) return EXEC_OM; \
	NDROP++;	\
	result = STACKGET(th, 0); \
cleanup:	\
	STACKSET(th,NDROP,result);	\
	STACKDROPN(th,NDROP);	\
	return 0;	\
}

#define bigOpeB_BOOL(name,ope)	\
int name(Thread* th)	\
{	\
	LINT NDROP=1-1;	\
	LW result=NIL;	\
	bignum b=_bignumGet(STACKGET(th,0));	\
	if (!b) goto cleanup;	\
	result=(ope(b))?MM.trueRef:MM.falseRef;	\
cleanup:	\
	STACKSETSAFE(th,NDROP,result);	\
	STACKDROPN(th,NDROP);	\
	return 0;	\
}

#define bigOpeB_I(name,ope)	\
int name(Thread* th)	\
{	\
	LINT NDROP=1-1;	\
	LW result=NIL;	\
	bignum b=_bignumGet(STACKGET(th,0));	\
	if (!b) goto cleanup;	\
	result=INTTOVAL(ope(b));	\
cleanup:	\
	STACKSETSAFE(th,NDROP,result);	\
	STACKDROPN(th,NDROP);	\
	return 0;	\
}

#define bigOpeBI_B(name,ope)	\
int name(Thread* th)	\
{	\
	bignum b;	\
	LINT NDROP=2-1;	\
	LW result=NIL;	\
	LW v=STACKGET(th,0); \
	bignum a=_bignumGet(STACKGET(th,1));	\
	if ((!a)||(v==NIL)) goto cleanup;	\
	b=ope(a,VALTOINT(v)); if (!b) return EXEC_OM; \
	if (bigPush(th,b)) return EXEC_OM; \
	NDROP++;	\
	result = STACKGET(th, 0); \
cleanup:	\
	STACKSET(th,NDROP,result);	\
	STACKDROPN(th,NDROP);	\
	return 0;	\
}

#define bigOpeBB_B(name,ope)	\
int name(Thread* th)	\
{	\
	bignum p;	\
	LINT NDROP=2-1;	\
	LW result=NIL;	\
	bignum b=_bignumGet(STACKGET(th,0));	\
	bignum a=_bignumGet(STACKGET(th,1));	\
	if ((!a)||(!b)) goto cleanup;	\
	p=ope(a,b); if (!p) return EXEC_OM; \
	if (bigPush(th,p)) return EXEC_OM; \
	NDROP++;	\
	result = STACKGET(th, 0); \
cleanup:	\
	STACKSET(th,NDROP,result);	\
	STACKDROPN(th,NDROP);	\
	return 0;	\
}

#define bigOpeBB_BOOL(name,ope)	\
int name(Thread* th)	\
{	\
	LINT NDROP=2-1;	\
	LW result=NIL;	\
	bignum b=_bignumGet(STACKGET(th,0));	\
	bignum a=_bignumGet(STACKGET(th,1));	\
	if ((!a)||(!b)) goto cleanup;	\
	result=(ope(a,b))?MM.trueRef:MM.falseRef;	\
cleanup:	\
	STACKSETSAFE(th,NDROP,result);	\
	STACKDROPN(th,NDROP);	\
	return 0;	\
}

#define bigOpeBB_I(name,ope)	\
int name(Thread* th)	\
{	\
	LINT NDROP=2-1;	\
	LW result=NIL;	\
	bignum b=_bignumGet(STACKGET(th,0));	\
	bignum a=_bignumGet(STACKGET(th,1));	\
	if ((!a)||(!b)) goto cleanup;	\
	result=INTTOVAL(ope(a,b));	\
cleanup:	\
	STACKSETSAFE(th,NDROP,result);	\
	STACKDROPN(th,NDROP);	\
	return 0;	\
}

#define bigOpeBI_I(name,ope)	\
int name(Thread* th)	\
{	\
	LINT NDROP=2-1;	\
	LW result=NIL;	\
	LW v=STACKGET(th,0); \
	bignum a=_bignumGet(STACKGET(th,1));	\
	if ((!a)||(v==NIL)) goto cleanup;	\
	result=INTTOVAL(ope(a,VALTOINT(v)));	\
cleanup:	\
	STACKSETSAFE(th,NDROP,result);	\
	STACKDROPN(th,NDROP);	\
	return 0;	\
}

#define bigOpeBBB_B(name,ope)	\
int name(Thread* th)	\
{	\
	bignum p;	\
	LINT NDROP=3-1;	\
	LW result=NIL;	\
	bignum c=_bignumGet(STACKGET(th,0));	\
	bignum b=_bignumGet(STACKGET(th,1));	\
	bignum a=_bignumGet(STACKGET(th,2));	\
	if ((!a)||(!b)||(!c)) goto cleanup;	\
	p=ope(a,b,c); if (!p) return EXEC_OM; \
	if (bigPush(th,p)) return EXEC_OM; \
	NDROP++;	\
	result = STACKGET(th, 0); \
cleanup:	\
	STACKSET(th,NDROP,result);	\
	STACKDROPN(th,NDROP);	\
	return 0;	\
}
#define bigOpeBBBB_B(name,ope)	\
int name(Thread* th)	\
{	\
	bignum p;	\
	LINT NDROP=4-1;	\
	LW result=NIL;	\
	bignum d=_bignumGet(STACKGET(th,0));	\
	bignum c=_bignumGet(STACKGET(th,1));	\
	bignum b=_bignumGet(STACKGET(th,2));	\
	bignum a=_bignumGet(STACKGET(th,3));	\
	if ((!a)||(!b)||(!c)||(!d)) goto cleanup;	\
	p=ope(a,b,c,d); if (!p) return EXEC_OM; \
	if (bigPush(th,p)) return EXEC_OM; \
	NDROP++;	\
	result = STACKGET(th, 0); \
cleanup:	\
	STACKSET(th,NDROP,result);	\
	STACKDROPN(th,NDROP);	\
	return 0;	\
}

#define bigOpeBBBBBB_B(name,ope)	\
int name(Thread* th)	\
{	\
	bignum p;	\
	LINT NDROP=6-1;	\
	LW result=NIL;	\
	bignum b1=_bignumGet(STACKGET(th,0));	\
	bignum a1=_bignumGet(STACKGET(th,1));	\
	bignum d0=_bignumGet(STACKGET(th,2));	\
	bignum c0=_bignumGet(STACKGET(th,3));	\
	bignum b0=_bignumGet(STACKGET(th,4));	\
	bignum a0=_bignumGet(STACKGET(th,5));	\
	if ((!a0)||(!b0)||(!c0)||(!d0)) goto cleanup;	\
	if ((!a1)||(!b1)) goto cleanup;	\
	p=ope(a0,b0,c0,d0,a1,b1); if (!p) return EXEC_OM; \
	if (bigPush(th,p)) return EXEC_OM; \
	NDROP++;	\
	result = STACKGET(th, 0); \
cleanup:	\
	STACKSET(th,NDROP,result);	\
	STACKDROPN(th,NDROP);	\
	return 0;	\
}
#define bigOpeBBBBBBBB_B(name,ope)	\
int name(Thread* th)	\
{	\
	bignum p;	\
	LINT NDROP=8-1;	\
	LW result=NIL;	\
	bignum d1=_bignumGet(STACKGET(th,0));	\
	bignum c1=_bignumGet(STACKGET(th,1));	\
	bignum b1=_bignumGet(STACKGET(th,2));	\
	bignum a1=_bignumGet(STACKGET(th,3));	\
	bignum d0=_bignumGet(STACKGET(th,4));	\
	bignum c0=_bignumGet(STACKGET(th,5));	\
	bignum b0=_bignumGet(STACKGET(th,6));	\
	bignum a0=_bignumGet(STACKGET(th,7));	\
	if ((!a0)||(!b0)||(!c0)||(!d0)) goto cleanup;	\
	if ((!a1)||(!b1)||(!c1)||(!d1)) goto cleanup;	\
	p=ope(a0,b0,c0,d0,a1,b1,c1,d1); if (!p) return EXEC_OM; \
	if (bigPush(th,p)) return EXEC_OM; \
	NDROP++;	\
	result = STACKGET(th, 0); \
cleanup:	\
	STACKSET(th,NDROP,result);	\
	STACKDROPN(th,NDROP);	\
	return 0;	\
}

#define bigOpeSBB_S(name,ope)	\
int name(Thread* th)	\
{	\
	LB* p;	\
	LINT len;	\
	LINT NDROP=3-1;	\
	LW result=NIL;	\
	bignum mod=_bignumGet(STACKGET(th,0));	\
	bignum exp=_bignumGet(STACKGET(th,1));	\
	MBLOC* data=VALTOPNT(STACKGET(th,2));	\
	if ((!mod)||(!exp)||(!data)) goto cleanup;	\
	len=ope(mod,exp,STRSTART(data),STRLEN(data),NULL);	\
	if (!len) goto cleanup;	\
	p=memoryAllocStr(th, NULL,len); if(!p) return EXEC_OM;	\
	len=ope(mod,exp,STRSTART(data),STRLEN(data),STRSTART(p));	\
	if (!len) goto cleanup;	\
	result=PNTTOVAL(p); NDROP++;	\
cleanup:	\
	STACKSET(th,NDROP,result);	\
	STACKDROPN(th,NDROP);	\
	return 0;	\
}

#define bigOpeSBBBB_S(name,ope)	\
int name(Thread* th)	\
{	\
	LB* p;	\
	LINT len;	\
	LINT NDROP=5-1;	\
	LW result=NIL;	\
	bignum q=_bignumGet(STACKGET(th,0));	\
	bignum pp=_bignumGet(STACKGET(th,1));	\
	bignum mod=_bignumGet(STACKGET(th,2));	\
	bignum exp=_bignumGet(STACKGET(th,3));	\
	MBLOC* data=VALTOPNT(STACKGET(th,4));	\
	if ((!mod)||(!exp)||(!pp)||(!q)||(!data)) goto cleanup;	\
	len=ope(mod,exp,pp,q,STRSTART(data),STRLEN(data),NULL);	\
	if (!len) goto cleanup;	\
	p=memoryAllocStr(th, NULL,len); if(!p) return EXEC_OM;	\
	len=ope(mod,exp,pp,q,STRSTART(data),STRLEN(data),STRSTART(p));	\
	if (!len) goto cleanup;	\
	result=PNTTOVAL(p); NDROP++;	\
cleanup:	\
	MDELETE(p);	\
	STACKSET(th,NDROP,result);	\
	STACKDROPN(th,NDROP);	\
	return 0;	\
}

void coreBignumReset(void);
int coreBignumInit(Thread* th, Pkg *system);


#endif
