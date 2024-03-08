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

#define _bignumGet(p,i) (bignum)STACKPNT(p,i)

#define FUN_RETURN_BIG(bigSrc) \
{	LINT bigLen; LB* bigResult; \
	bignum big=bigSrc;	\
	if (!big) return EXEC_OM;	\
	bigLen=sizeof(Bignum) - sizeof(LB) + sizeof(uint) * (((LINT)big->len) - 1);	\
	bigResult=memoryAllocBin(th, (char*)&big->len, bigLen, DBG_B); if (!bigResult) return EXEC_OM;	\
	bignumRelease(big);	\
	FUN_RETURN_PNT(bigResult);	\
}

#define bigOpeI_B(name,ope)	\
int name(Thread* th)	\
{	\
	int vIsNil=STACKISNIL(th,0); \
	LINT v=STACKINT(th,0); \
	if (vIsNil) FUN_RETURN_NIL;	\
	FUN_RETURN_BIG(ope(v));	\
}

#define bigOpeIBool_B(name,ope)	\
int name(Thread* th)	\
{	\
	LB* v=STACKPNT(th,0); \
	int wIsNil=STACKISNIL(th,1); \
	LINT w=STACKINT(th,1); \
	if ((!v)||wIsNil) FUN_RETURN_NIL;	\
	FUN_RETURN_BIG(ope(w,v));	\
}

#define bigOpeB_B(name,ope)	\
int name(Thread* th)	\
{	\
	bignum a=_bignumGet(th,0);	\
	if (!a) FUN_RETURN_NIL;	\
	FUN_RETURN_BIG(ope(a));	\
}

#define bigOpeB_BOOL(name,ope)	\
int name(Thread* th)	\
{	\
	bignum b=_bignumGet(th,0);	\
	if (!b) FUN_RETURN_NIL;	\
	FUN_RETURN_PNT((ope(b))?MM._true:MM._false);	\
}

#define bigOpeB_I(name,ope)	\
int name(Thread* th)	\
{	\
	bignum b=_bignumGet(th,0);	\
	if (!b) FUN_RETURN_NIL;	\
	FUN_RETURN_INT(ope(b));	\
}

#define bigOpeBI_B(name,ope)	\
int name(Thread* th)	\
{	\
	int vIsNil=STACKISNIL(th,0); \
	LINT v=STACKINT(th,0); \
	bignum a=_bignumGet(th,1);	\
	if ((!a)||vIsNil) FUN_RETURN_NIL;	\
	FUN_RETURN_BIG(ope(a,v));	\
}

#define bigOpeBB_B(name,ope)	\
int name(Thread* th)	\
{	\
	bignum b=_bignumGet(th,0);	\
	bignum a=(_bignumGet(th,1));	\
	if ((!a)||(!b)) FUN_RETURN_NIL;	\
	FUN_RETURN_BIG(ope(a,b));	\
}

#define bigOpeBB_BOOL(name,ope)	\
int name(Thread* th)	\
{	\
	bignum b=_bignumGet(th,0);	\
	bignum a=_bignumGet(th,1);	\
	if ((!a)||(!b)) FUN_RETURN_NIL;	\
	FUN_RETURN_PNT((ope(a,b))?MM._true:MM._false);	\
}

#define bigOpeBB_I(name,ope)	\
int name(Thread* th)	\
{	\
	bignum b=_bignumGet(th,0);	\
	bignum a=_bignumGet(th,1);	\
	if ((!a)||(!b)) FUN_RETURN_NIL;	\
	FUN_RETURN_INT(ope(a,b));	\
}

#define bigOpeBI_I(name,ope)	\
int name(Thread* th)	\
{	\
	int vIsNil=STACKISNIL(th,0); \
	LINT v=STACKINT(th,0); \
	bignum a=_bignumGet(th,1);	\
	if ((!a)||vIsNil) FUN_RETURN_NIL;	\
	FUN_RETURN_INT(ope(a,v));	\
}

#define bigOpeBBB_B(name,ope)	\
int name(Thread* th)	\
{	\
	bignum c=_bignumGet(th,0);	\
	bignum b=_bignumGet(th,1);	\
	bignum a=_bignumGet(th,2);	\
	if ((!a)||(!b)||(!c)) FUN_RETURN_NIL;	\
	FUN_RETURN_BIG(ope(a,b,c));	\
}
#define bigOpeBBBB_B(name,ope)	\
int name(Thread* th)	\
{	\
	bignum d=_bignumGet(th,0);	\
	bignum c=_bignumGet(th,1);	\
	bignum b=_bignumGet(th,2);	\
	bignum a=_bignumGet(th,3);	\
	if ((!a)||(!b)||(!c)||(!d)) FUN_RETURN_NIL;	\
	FUN_RETURN_BIG(ope(a,b,c,d));	\
}

#define bigOpeBBBBBB_B(name,ope)	\
int name(Thread* th)	\
{	\
	bignum b1=_bignumGet(th,0);	\
	bignum a1=_bignumGet(th,1);	\
	bignum d0=_bignumGet(th,2);	\
	bignum c0=_bignumGet(th,3);	\
	bignum b0=_bignumGet(th,4);	\
	bignum a0=_bignumGet(th,5);	\
	if ((!a0)||(!b0)||(!c0)||(!d0)) FUN_RETURN_NIL;	\
	if ((!a1)||(!b1)) FUN_RETURN_NIL;	\
	FUN_RETURN_BIG(ope(a0,b0,c0,d0,a1,b1));	\
}
#define bigOpeBBBBBBBB_B(name,ope)	\
int name(Thread* th)	\
{	\
	bignum d1=_bignumGet(th,0);	\
	bignum c1=_bignumGet(th,1);	\
	bignum b1=_bignumGet(th,2);	\
	bignum a1=_bignumGet(th,3);	\
	bignum d0=_bignumGet(th,4);	\
	bignum c0=_bignumGet(th,5);	\
	bignum b0=_bignumGet(th,6);	\
	bignum a0=_bignumGet(th,7);	\
	if ((!a0)||(!b0)||(!c0)||(!d0)) FUN_RETURN_NIL;	\
	if ((!a1)||(!b1)||(!c1)||(!d1)) FUN_RETURN_NIL;	\
	FUN_RETURN_BIG(ope(a0,b0,c0,d0,a1,b1,c1,d1));	\
}

#define bigOpeSBB_S(name,ope)	\
int name(Thread* th)	\
{	\
	LB* p;	\
	LINT len;	\
	bignum mod=_bignumGet(th,0);	\
	bignum exp=_bignumGet(th,1);	\
	MBLOC* data=(STACKPNT(th,2));	\
	if ((!mod)||(!exp)||(!data)) FUN_RETURN_NIL;	\
	len=ope(mod,exp,STRSTART(data),STRLEN(data),NULL);	\
	if (!len) FUN_RETURN_NIL;	\
	p=memoryAllocStr(th, NULL,len); if(!p) return EXEC_OM;	\
	len=ope(mod,exp,STRSTART(data),STRLEN(data),STRSTART(p));	\
	if (!len) FUN_RETURN_NIL;	\
	FUN_RETURN_PNT(p);	\
}

void coreBignumReset(void);
int coreBignumInit(Thread* th, Pkg *system);


#endif
