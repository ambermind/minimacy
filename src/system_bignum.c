// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

#define _bigGet(p,i) (bignum)STACK_PNT(p,i)

#define FUN_RETURN_BIG(bigSrc) \
{	LINT bigLen; LB* bigResult; \
	bignum big=bigSrc;	\
	if (!big) FUN_RETURN_NIL;	\
	bigLen=sizeof(Bignum) - sizeof(LB) + sizeof(bigword) * (((LINT)big->len));	\
	bigResult=memoryAllocBin((char*)&big->len, bigLen, DBG_B); if (!bigResult) return EXEC_OM;	\
	bigRelease(big);	\
	FUN_RETURN_PNT(bigResult);	\
}

#define bigOpeI_B(name,ope)	\
int name(Thread* th)	\
{	\
	int vIsNil=STACK_IS_NIL(th,0); \
	LINT v=STACK_INT(th,0); \
	if (vIsNil) FUN_RETURN_NIL;	\
	FUN_RETURN_BIG(ope(v));	\
}

#define bigOpeIBool_B(name,ope)	\
int name(Thread* th)	\
{	\
	LB* v=STACK_PNT(th,0); \
	int wIsNil=STACK_IS_NIL(th,1); \
	LINT w=STACK_INT(th,1); \
	if ((!v)||wIsNil) FUN_RETURN_NIL;	\
	FUN_RETURN_BIG(ope(w,v));	\
}

#define bigOpeB_B(name,ope)	\
int name(Thread* th)	\
{	\
	bignum a=_bigGet(th,0);	\
	if (!a) FUN_RETURN_NIL;	\
	FUN_RETURN_BIG(ope(a));	\
}

#define bigOpeB_BOOL(name,ope)	\
int name(Thread* th)	\
{	\
	bignum b=_bigGet(th,0);	\
	if (!b) FUN_RETURN_NIL;	\
	FUN_RETURN_PNT((ope(b))?MM._true:MM._false);	\
}

#define bigOpeB_I(name,ope)	\
int name(Thread* th)	\
{	\
	bignum b=_bigGet(th,0);	\
	if (!b) FUN_RETURN_NIL;	\
	FUN_RETURN_INT(ope(b));	\
}

#define bigOpeBI_B(name,ope)	\
int name(Thread* th)	\
{	\
	int vIsNil=STACK_IS_NIL(th,0); \
	LINT v=STACK_INT(th,0); \
	bignum a=_bigGet(th,1);	\
	if ((!a)||vIsNil) FUN_RETURN_NIL;	\
	FUN_RETURN_BIG(ope(a,v));	\
}

#define bigOpeBB_B(name,ope)	\
int name(Thread* th)	\
{	\
	bignum b=_bigGet(th,0);	\
	bignum a=(_bigGet(th,1));	\
	if ((!a)||(!b)) FUN_RETURN_NIL;	\
	FUN_RETURN_BIG(ope(a,b));	\
}

#define bigOpeBB_BOOL(name,ope)	\
int name(Thread* th)	\
{	\
	bignum b=_bigGet(th,0);	\
	bignum a=_bigGet(th,1);	\
	if ((!a)||(!b)) FUN_RETURN_NIL;	\
	FUN_RETURN_PNT((ope(a,b))?MM._true:MM._false);	\
}

#define bigOpeBB_I(name,ope)	\
int name(Thread* th)	\
{	\
	bignum b=_bigGet(th,0);	\
	bignum a=_bigGet(th,1);	\
	if ((!a)||(!b)) FUN_RETURN_NIL;	\
	FUN_RETURN_INT(ope(a,b));	\
}

#define bigOpeBI_I(name,ope)	\
int name(Thread* th)	\
{	\
	int vIsNil=STACK_IS_NIL(th,0); \
	LINT v=STACK_INT(th,0); \
	bignum a=_bigGet(th,1);	\
	if ((!a)||vIsNil) FUN_RETURN_NIL;	\
	FUN_RETURN_INT(ope(a,v));	\
}

#define bigOpeBBI_B(name,ope)	\
int name(Thread* th)	\
{	\
	LINT v=STACK_INT(th,0); \
	bignum b=_bigGet(th,1);	\
	bignum a=_bigGet(th,2);	\
	if ((!a)||(!b)) FUN_RETURN_NIL;	\
	FUN_RETURN_BIG(ope(a,b,v));	\
}

#define bigOpeBBB_B(name,ope)	\
int name(Thread* th)	\
{	\
	bignum c=_bigGet(th,0);	\
	bignum b=_bigGet(th,1);	\
	bignum a=_bigGet(th,2);	\
	if ((!a)||(!b)||(!c)) FUN_RETURN_NIL;	\
	FUN_RETURN_BIG(ope(a,b,c));	\
}
#define bigOpeBBBB_B(name,ope)	\
int name(Thread* th)	\
{	\
	bignum d=_bigGet(th,0);	\
	bignum c=_bigGet(th,1);	\
	bignum b=_bigGet(th,2);	\
	bignum a=_bigGet(th,3);	\
	if ((!a)||(!b)||(!c)||(!d)) FUN_RETURN_NIL;	\
	FUN_RETURN_BIG(ope(a,b,c,d));	\
}

#define bigOpeBBBBBB_B(name,ope)	\
int name(Thread* th)	\
{	\
	bignum b1=_bigGet(th,0);	\
	bignum a1=_bigGet(th,1);	\
	bignum d0=_bigGet(th,2);	\
	bignum c0=_bigGet(th,3);	\
	bignum b0=_bigGet(th,4);	\
	bignum a0=_bigGet(th,5);	\
	if ((!a0)||(!b0)||(!c0)||(!d0)) FUN_RETURN_NIL;	\
	if ((!a1)||(!b1)) FUN_RETURN_NIL;	\
	FUN_RETURN_BIG(ope(a0,b0,c0,d0,a1,b1));	\
}

#define bigOpeSBB_S(name,ope)	\
int name(Thread* th)	\
{	\
	LB* p;	\
	LINT len;	\
	bignum mod=_bigGet(th,0);	\
	bignum exp=_bigGet(th,1);	\
	MBLOC* data=(STACK_PNT(th,2));	\
	if ((!mod)||(!exp)||(!data)) FUN_RETURN_NIL;	\
	len=ope(mod,exp,STR_START(data),STR_LENGTH(data),NULL);	\
	if (!len) FUN_RETURN_NIL;	\
	p=memoryAllocStr(NULL,len); if(!p) return EXEC_OM;	\
	len=ope(mod,exp,STR_START(data),STR_LENGTH(data),STR_START(p));	\
	if (!len) FUN_RETURN_NIL;	\
	FUN_RETURN_PNT(p);	\
}
LINT _bightoc(LINT c)
{
	if ((c >= '0') && (c <= '9')) return c - '0';
	else if ((c >= 'A') && (c <= 'F')) return c - 'A' + 10;
	else if ((c >= 'a') && (c <= 'f')) return c - 'a' + 10;
	return 0;
}

bignum bigFromHex(char* src)
{
	char* bin;
	bignum res;
	LINT len = strlen(src);
	LINT start;
	LINT k = 0;
	LINT i;
	LINT n = 0;
	LINT sign = 0;
	LB* p;;
	if ((*src) == '-')
	{
		sign = 1;
		src++;
		len--;
	}
	start = len & 1;
	p = memoryAllocStr(NULL, (len + 1) / 2); if (!p) return NULL;
	bin = STR_START(p);
	for (i = start; i < len + start; i++)
	{
		LINT c = _bightoc(*(src++));
		k = (k << 4) + c;
		if (i & 1) bin[n++] = (char)k;
	}
	res = bigFromBin(bin, n);
	if (sign) bigSignSet(res, 1);
	return res;
}

LINT charMul4Daa(char* buf, LINT c, LINT len)
{
	LINT i, x;

	for (i = 0; i < len; i++)
	{
		x = buf[i] * 4 + c;
		c = 0;
		while (x > 9)
		{
			c++;
			x -= 10;
		}
		buf[i] = (char)x;
	}
	return 0;
}

int bigDecToBuffer(bignum b, Buffer* buffer)
{
	int k;
	char* buf;
	LINT i, l;
	LINT len = bigLen32(b)* BIG_WORD_BITS * 10;
	LB* p_buf = memoryAllocStr(NULL, len); if (!p_buf) return EXEC_OM;
	TMP_PUSH(p_buf, EXEC_OM);
	buf = STR_START(p_buf);
	if (bigSign(b))
	{
		if ((k = bufferAddChar(buffer, '-'))) return k;
	}
	for (i = 0; i < len; i++)buf[i] = 0;
	for (i = bigLen(b) - 1; i >= 0; i--)
	{
		LINT j;
		bigword x = bigGet(b, i);
		for (j = BIG_WORD_BITS-4; j >= 0; j -= 4)
		{
			charMul4Daa(buf, 0, len);
			charMul4Daa(buf, (x >> j) & 15, len);
		}
	}
	l = len - 1;
	while ((l > 0) && (buf[l] == 0)) l--;
	while (l >= 0) if ((k = bufferAddChar(buffer, buf[l--] + 48))) return k;
	TMP_PULL();
	return 0;
}
//----------------------------------------
LB* bigAlloc(bignum b0)
{
	LINT len;
	LB* b;
	if (!b0) return NULL;
	len = sizeof(Bignum) - sizeof(LB) + sizeof(bigword) * (((LINT)b0->len));	// keep one more bigword than necessary to enable 64 bits operations on 64 bits cpus
	b = memoryAllocBin((char*)&b0->len, len, DBG_B); if (!b) return NULL;
	bigRelease(b0);
	return b;
}
int bigPush(Thread* th, bignum b0)
{
	LINT len;
	LB* b;
	if (!b0) {
		FUN_PUSH_NIL;
		return 0;
	}
	len = sizeof(Bignum) - sizeof(LB) + sizeof(bigword) * (((LINT)b0->len));	// keep one more bigword than necessary to enable 64 bits operations on 64 bits cpus

	b = memoryAllocBin((char*)&b0->len, len, DBG_B); if (!b) return EXEC_OM;
	FUN_PUSH_PNT(b);
	bigRelease(b0);
	return 0;
}

int fun_bigDeserialize(Thread* th)
{
	LB* p = STACK_PNT(th, 0);
	if (!p) FUN_RETURN_NIL;
	FUN_RETURN_BIG(bigFromBin(STR_START(p), STR_LENGTH(p)));
}
int fun_bigSerialize(Thread* th)
{
	LB* p;
	LINT size;

	LINT len = STACK_INT(th, 0);
	bignum b = _bigGet(th, 1);
	if (!b) FUN_RETURN_NIL;

	size = bigStringBin(b, len, NULL);
	if (size < 0) FUN_RETURN_NIL;
	p = memoryAllocStr(NULL, size); if (!p) return EXEC_OM;
	bigStringBin(b, len, STR_START(p));
	FUN_RETURN_PNT(p);
}
int fun_signedBigDeserialize(Thread* th)
{
	LB* p = STACK_PNT(th, 0);
	if (!p) FUN_RETURN_NIL;
	FUN_RETURN_BIG(bigFromSignedBin(STR_START(p), STR_LENGTH(p)));
}
int fun_signedBigSerialize(Thread* th)
{
	LB* p;
	LINT size;

	bignum b = _bigGet(th, 0);
	if (!b) FUN_RETURN_NIL;

	size = bigStringSignedBin(b, NULL);
	if (size < 0) FUN_RETURN_NIL;
	p = memoryAllocStr(NULL, size); if (!p) return EXEC_OM;
	bigStringSignedBin(b, STR_START(p));
	FUN_RETURN_PNT(p);
}
int fun_bigFromStr(Thread* th)
{
	LB* p = STACK_PNT(th, 0);
	if (!p) FUN_RETURN_NIL;
	FUN_RETURN_BIG(bigFromDec(STR_START(p)));
}

int fun_strFromBig(Thread* th)
{
	int k;

	bignum b = _bigGet(th, 0);
	if (!b) FUN_RETURN_NIL;
	bufferReinit(MM.tmpBuffer);
	if ((k = bigDecToBuffer((bignum)b, MM.tmpBuffer))) return k;
	FUN_RETURN_BUFFER(MM.tmpBuffer);
}

int fun_bigFromHex(Thread* th)
{
	LB* p = STACK_PNT(th, 0);
	if (!p) FUN_RETURN_NIL;
	FUN_RETURN_BIG(bigFromHex(STR_START(p)));
}
int fun_hexFromBig(Thread* th)
{
	LB* p;
	LINT size;

	bignum b = _bigGet(th, 0);
	if (!b) FUN_RETURN_NIL;
	size = bigStringHex(b, NULL);
	if (size < 0) FUN_RETURN_NIL;
	p = memoryAllocStr(NULL, size); if (!p) return EXEC_OM;
	bigStringHex(b, STR_START(p));
	FUN_RETURN_PNT(p);
}

int fun_bigDivRemainder(Thread* th)
{
	bignum q, r;

	bignum b = _bigGet(th, 0);
	bignum a = _bigGet(th, 1);
	if ((!a) || (!b)) FUN_RETURN_NIL;
	q = bigDivRemainder(a, b, &r);
	if (!q) FUN_RETURN_NIL;
	if (bigPush(th, q)) return EXEC_OM;
	if (bigPush(th, r)) return EXEC_OM;
	FUN_MAKE_ARRAY(2, DBG_TUPLE);
	return 0;
}
int fun_bigEuclid(Thread* th)
{
	bignum u, v, pgcd;

	bignum b = _bigGet(th, 0);
	bignum a = _bigGet(th, 1);
	if ((!a) || (!b)) FUN_RETURN_NIL;
	if (bigEuclide(a, b, &u, &v, &pgcd)) FUN_RETURN_NIL;
	if (bigPush(th, u)) return EXEC_OM;
	if (bigPush(th, v)) return EXEC_OM;
	if (bigPush(th, pgcd)) return EXEC_OM;
	FUN_MAKE_ARRAY(3, DBG_TUPLE);
	return 0;
}

int fun_bigX25519(Thread* th)
{
	bignum u = _bigGet(th, 0);
	bignum k = _bigGet(th, 1);
	bignum One = _bigGet(th, 2);
	bignum A24 = _bigGet(th, 3);
	bignum Pminus2 = _bigGet(th, 4);
	bignum Pprime = _bigGet(th, 5);
	bignum P = _bigGet(th, 6);
	if (!u || !k || !P || !Pprime || !Pminus2 || !A24 || !One) FUN_RETURN_NIL;

	FUN_RETURN_BIG(bigX25519(P, Pprime, Pminus2, A24, One, k, u));
	return 0;
}

int fun_bigEd25519Add(Thread* th)
{
	bignum x1, y1, z1, t1, x2, y2, z2, t2;

	LB* p2 = STACK_PNT(th, 0);
	LB* p1 = STACK_PNT(th, 1);
	bignum D = _bigGet(th, 2);
	bignum Pprime = _bigGet(th, 3);
	bignum P = _bigGet(th, 4);
	if (!p1 || !p2 || !D || !Pprime || !P) FUN_RETURN_NIL;

	x1 = (bignum)ARRAY_PNT(p1, 0);
	y1 = (bignum)ARRAY_PNT(p1, 1);
	z1 = (bignum)ARRAY_PNT(p1, 2);
	t1 = (bignum)ARRAY_PNT(p1, 3);
	if (!x1 || !y1 || !z1 || !t1) FUN_RETURN_NIL;
	x2 = (bignum)ARRAY_PNT(p2, 0);
	y2 = (bignum)ARRAY_PNT(p2, 1);
	z2 = (bignum)ARRAY_PNT(p2, 2);
	t2 = (bignum)ARRAY_PNT(p2, 3);
	if (!x2 || !y2 || !z2 || !t2) FUN_RETURN_NIL;
	if (bigEd25519Add(P, Pprime, D, x1, y1, z1, t1, x2, y2, z2, t2, &x1, &y1, &z1, &t1)) FUN_RETURN_NIL;
	if (bigPush(th, x1)) return EXEC_OM;
	if (bigPush(th, y1)) return EXEC_OM;
	if (bigPush(th, z1)) return EXEC_OM;
	if (bigPush(th, t1)) return EXEC_OM;
	FUN_MAKE_ARRAY(4, DBG_TUPLE);
	return 0;
}

int fun_bigEd25519Mul(Thread* th)
{
	bignum px, py, pz, pt;

	LB* p = STACK_PNT(th, 0);
	bignum s = _bigGet(th, 1);
	bignum One = _bigGet(th, 2);
	bignum D = _bigGet(th, 3);
	bignum Pprime = _bigGet(th, 4);
	bignum P = _bigGet(th, 5);
	LB* constantTime = STACK_PNT(th, 6);
	if (!p || !s || !P || !Pprime || !D || !One) FUN_RETURN_NIL;

	px = (bignum)ARRAY_PNT(p, 0);
	py = (bignum)ARRAY_PNT(p, 1);
	pz = (bignum)ARRAY_PNT(p, 2);
	pt = (bignum)ARRAY_PNT(p, 3);
	if (!px || !py || !pz || !pt) FUN_RETURN_NIL;
	if (bigEd25519Mul((constantTime == MM._true) ? 1 : 0, P, Pprime, D, One, s, px, py, pz, pt, &px, &py, &pz, &pt)) FUN_RETURN_NIL;
	if (bigPush(th, px)) return EXEC_OM;
	if (bigPush(th, py)) return EXEC_OM;
	if (bigPush(th, pz)) return EXEC_OM;
	if (bigPush(th, pt)) return EXEC_OM;
	FUN_MAKE_ARRAY(4, DBG_TUPLE);
	return 0;
}

int fun_bigWeierstrassA0Add(Thread* th)
{
	bignum x1, y1, z1, x2, y2, z2;

	LB* p2 = STACK_PNT(th, 0);
	LB* p1 = STACK_PNT(th, 1);
	bignum b3= _bigGet(th, 2);
	bignum Pprime= _bigGet(th, 3);
	bignum P= _bigGet(th, 4);
	if (!p1 || !p2 || !b3 || !Pprime || !P) FUN_RETURN_NIL;

	x1 = (bignum)ARRAY_PNT(p1, 0);
	y1 = (bignum)ARRAY_PNT(p1, 1);
	z1 = (bignum)ARRAY_PNT(p1, 2);
	if (!x1 || !y1 || !z1) FUN_RETURN_NIL;
	x2 = (bignum)ARRAY_PNT(p2, 0);
	y2 = (bignum)ARRAY_PNT(p2, 1);
	z2 = (bignum)ARRAY_PNT(p2, 2);
	if (!x2 || !y2 || !z2) FUN_RETURN_NIL;
	if (bigWeierstrassA0Add(P, Pprime, b3, x1, y1, z1, x2, y2, z2, &x1, &y1, &z1)) FUN_RETURN_NIL;
	if (bigIsNull(z1)) {
		bigRelease(x1);
		bigRelease(y1);
		bigRelease(z1);
		FUN_RETURN_NIL;
	}
	if (bigPush(th, x1)) return EXEC_OM;
	if (bigPush(th, y1)) return EXEC_OM;
	if (bigPush(th, z1)) return EXEC_OM;
	FUN_MAKE_ARRAY(3, DBG_TUPLE);
	return 0;
}

int fun_bigWeierstrassA0Mul(Thread* th)
{
	bignum px, py, pz;

	LB* p = STACK_PNT(th, 0);
	bignum s = _bigGet(th, 1);
	bignum b3 = _bigGet(th, 2);
	bignum Pprime = _bigGet(th, 3);
	bignum P = _bigGet(th, 4);
	LB* constantTime = STACK_PNT(th, 5);
	if (!p || !s || !b3 || !Pprime || !P) FUN_RETURN_NIL;

	px = (bignum)ARRAY_PNT(p, 0);
	py = (bignum)ARRAY_PNT(p, 1);
	pz = (bignum)ARRAY_PNT(p, 2);
	if (!px || !py || !pz) FUN_RETURN_NIL;
	if (bigWeierstrassA0Mul(P, Pprime, b3, (constantTime == MM._true) ? 1 : 0, s, px, py, pz, &px, &py, &pz)) FUN_RETURN_NIL;
	if (bigIsNull(pz)) {
		bigRelease(px);
		bigRelease(py);
		bigRelease(pz);
		FUN_RETURN_NIL;
	}
	if (bigPush(th, px)) return EXEC_OM;
	if (bigPush(th, py)) return EXEC_OM;
	if (bigPush(th, pz)) return EXEC_OM;
	FUN_MAKE_ARRAY(3, DBG_TUPLE);
	return 0;
}

int fun_bigWeierstrassA3Add(Thread* th)
{
	bignum x1, y1, z1, x2, y2, z2;

	LB* p2 = STACK_PNT(th, 0);
	LB* p1 = STACK_PNT(th, 1);
	bignum b = _bigGet(th, 2);
	bignum Pprime = _bigGet(th, 3);
	bignum P = _bigGet(th, 4);
	if (!p1 || !p2 || !b || !Pprime || !P) FUN_RETURN_NIL;

	x1 = (bignum)ARRAY_PNT(p1, 0);
	y1 = (bignum)ARRAY_PNT(p1, 1);
	z1 = (bignum)ARRAY_PNT(p1, 2);
	if (!x1 || !y1 || !z1) FUN_RETURN_NIL;
	x2 = (bignum)ARRAY_PNT(p2, 0);
	y2 = (bignum)ARRAY_PNT(p2, 1);
	z2 = (bignum)ARRAY_PNT(p2, 2);
	if (!x2 || !y2 || !z2) FUN_RETURN_NIL;
	if (bigWeierstrassA3Add(P, Pprime, b, x1, y1, z1, x2, y2, z2, &x1, &y1, &z1)) FUN_RETURN_NIL;
	if (bigIsNull(z1)) {
		bigRelease(x1);
		bigRelease(y1);
		bigRelease(z1);
		FUN_RETURN_NIL;
	}
	if (bigPush(th, x1)) return EXEC_OM;
	if (bigPush(th, y1)) return EXEC_OM;
	if (bigPush(th, z1)) return EXEC_OM;
	FUN_MAKE_ARRAY(3, DBG_TUPLE);
	return 0;
}

int fun_bigWeierstrassA3Mul(Thread* th)
{
	bignum px, py, pz;

	LB* p = STACK_PNT(th, 0);
	bignum s = _bigGet(th, 1);
	bignum b = _bigGet(th, 2);
	bignum Pprime = _bigGet(th, 3);
	bignum P = _bigGet(th, 4);
	LB* constantTime = STACK_PNT(th, 5);
	if (!p || !s || !b || !Pprime || !P) FUN_RETURN_NIL;

	px = (bignum)ARRAY_PNT(p, 0);
	py = (bignum)ARRAY_PNT(p, 1);
	pz = (bignum)ARRAY_PNT(p, 2);
	if (!px || !py || !pz) FUN_RETURN_NIL;
	if (bigWeierstrassA3Mul(P, Pprime, b, (constantTime == MM._true) ? 1 : 0, s, px, py, pz, &px, &py, &pz)) FUN_RETURN_NIL;
	if (bigIsNull(px)) {
		bigRelease(px);
		bigRelease(py);
		bigRelease(pz);
		FUN_RETURN_NIL;
	}
	if (bigPush(th, px)) return EXEC_OM;
	if (bigPush(th, py)) return EXEC_OM;
	if (bigPush(th, pz)) return EXEC_OM;
	FUN_MAKE_ARRAY(3, DBG_TUPLE);
	return 0;
}

bigOpeI_B(fun_bigFromInt, bigFromInt)
bigOpeI_B(fun_bigPower2, bigPower2)

bigOpeIBool_B(fun_bigRand, bigRand)

bigOpeB_B(fun_bigASR1, bigASR1)
bigOpeB_B(fun_bigASL1, bigASL1)
bigOpeB_B(fun_bigAbs, bigAbs)
bigOpeB_B(fun_bigNeg, bigNeg)
bigOpeB_B(fun_bigBarrett, bigBarrett)
bigOpeB_B(fun_bigSquare, bigSquare)
bigOpeB_B(fun_bigMontgomery, bigMontgomery)

bigOpeB_I(fun_intFromBig, bigToInt)
bigOpeB_I(fun_bigNbits, bigNbits)
bigOpeB_I(fun_bigLowestBit, bigLowestBit)
bigOpeB_I(fun_bigCheckFirstFactors, bigCheckFirstFactors)

bigOpeB_BOOL(fun_bigPositive, bigPositive)
bigOpeB_BOOL(fun_bigIsNull, bigIsNull)
bigOpeB_BOOL(fun_bigIsOne, bigIsOne)
bigOpeB_BOOL(fun_bigIsEven, bigIsEven)
bigOpeB_BOOL(fun_bigIsOdd, bigIsOdd)

bigOpeBI_B(fun_bigModPower2, bigModPower2)
bigOpeBI_B(fun_bigASR, bigASR)
bigOpeBI_B(fun_bigASL, bigASL)

bigOpeBI_I(fun_bigBit, bigBit)

bigOpeBB_B(fun_bigXor, bigXor)
bigOpeBB_B(fun_bigAdd, bigAdd)
bigOpeBB_B(fun_bigSub, bigSub)
bigOpeBB_B(fun_bigMul, bigMul)
bigOpeBB_B(fun_bigExp, bigExp)
bigOpeBB_B(fun_bigDiv, bigDiv)
bigOpeBB_B(fun_bigMod, bigMod)
bigOpeBB_B(fun_bigSquareMod, bigSquareMod)
bigOpeBB_B(fun_bigNegMod, bigNegMod)
bigOpeBB_B(fun_bigInv, bigInv)
bigOpeBB_B(fun_bigGcd, bigGcd)
bigOpeBB_B(fun_bigMontgomeryForm, bigMontgomeryForm)

bigOpeBB_I(fun_bigCmp, bigCmp)
bigOpeBB_BOOL(fun_bigEquals, bigEquals)
bigOpeBB_BOOL(fun_bigGreater, bigGreater)
bigOpeBB_BOOL(fun_bigGreaterEquals, bigGreaterEqual)
bigOpeBB_BOOL(fun_bigLower, bigLower)
bigOpeBB_BOOL(fun_bigLowerEquals, bigLowerEqual)

bigOpeBBI_B(fun_bigMulPower2, bigModPower2Mul)

bigOpeBBB_B(fun_bigAddMod, bigAddMod)
bigOpeBBB_B(fun_bigSubMod, bigSubMod)
bigOpeBBB_B(fun_bigDivMod, bigDivMod)
bigOpeBBB_B(fun_bigMulMod, bigMulMod)
bigOpeBBB_B(fun_bigExpMod, bigExpMod)
bigOpeBBB_B(fun_bigModBarrett, bigModBarrett)
bigOpeBBB_B(fun_bigSquareModBarrett, bigSquareModBarrett)
bigOpeBBB_B(fun_bigSquareModMontgomery, bigSquareModMontgomery)
bigOpeBBB_B(fun_bigRedc, bigMontgomeryRedc)

bigOpeBBBB_B(fun_bigMulModBarrett, bigMulModBarrett)
bigOpeBBBB_B(fun_bigDivModBarrett, bigDivModBarrett)
bigOpeBBBB_B(fun_bigExpModBarrett, bigExpModBarrett)
bigOpeBBBB_B(fun_bigExpModBarrettFast, bigExpModBarrettFast)
bigOpeBBBB_B(fun_bigMulModMontgomery, bigMulModMontgomery)
bigOpeBBBB_B(fun_bigDivModMontgomery, bigDivModMontgomery)
bigOpeBBBB_B(fun_bigExpModMontgomery, bigExpModMontgomery)
bigOpeBBBB_B(fun_bigExpModMontgomeryFast, bigExpModMontgomeryFast)

bigOpeBBBBBB_B(fun_bigExpChinese5, bigExpChinese5)

int systemBignumInit(Pkg* system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "bigSerialize", fun_bigSerialize, "fun BigNum Int -> Str" },
		{ NATIVE_FUN, "bigDeserialize", fun_bigDeserialize, "fun Str -> BigNum" },
		{ NATIVE_FUN, "bigDeserializeBytes", fun_bigDeserialize, "fun Bytes -> BigNum" },
		{ NATIVE_FUN, "signedBigSerialize", fun_signedBigSerialize, "fun BigNum -> Str" },
		{ NATIVE_FUN, "signedBigDeserialize", fun_signedBigDeserialize, "fun Str -> BigNum" },
		{ NATIVE_FUN, "signedBigDeserializeBytes", fun_signedBigDeserialize, "fun Bytes -> BigNum" },
		{ NATIVE_FUN, "strFromBig", fun_strFromBig, "fun BigNum -> Str" },
		{ NATIVE_FUN, "hexFromBig", fun_hexFromBig, "fun BigNum -> Str" },
		{ NATIVE_FUN, "intFromBig", fun_intFromBig, "fun BigNum -> Int" },
		{ NATIVE_FUN, "bigFromStr", fun_bigFromStr, "fun Str -> BigNum" },
		{ NATIVE_FUN, "bigFromHex", fun_bigFromHex, "fun Str -> BigNum" },
		{ NATIVE_FUN, "bigFromInt", fun_bigFromInt, "fun Int -> BigNum" },
		{ NATIVE_FUN, "bigPower2", fun_bigPower2, "fun Int -> BigNum" },
		{ NATIVE_FUN, "bigRand", fun_bigRand, "fun Int Bool -> BigNum" },

		{ NATIVE_FUN, "bigEquals", fun_bigEquals, "fun BigNum BigNum -> Bool" },
		{ NATIVE_FUN, "bigGreater", fun_bigGreater, "fun BigNum BigNum -> Bool" },
		{ NATIVE_FUN, "bigGreaterEquals", fun_bigGreaterEquals, "fun BigNum BigNum -> Bool" },
		{ NATIVE_FUN, "bigLower", fun_bigLower, "fun BigNum BigNum -> Bool" },
		{ NATIVE_FUN, "bigLowerEquals", fun_bigLowerEquals, "fun BigNum BigNum -> Bool" },
		{ NATIVE_FUN, "bigCmp", fun_bigCmp, "fun BigNum BigNum -> Int" },

		{ NATIVE_FUN, "bigIsEven", fun_bigIsEven, "fun BigNum -> Bool" },
		{ NATIVE_FUN, "bigIsOdd", fun_bigIsOdd, "fun BigNum -> Bool" },
		{ NATIVE_FUN, "bigIsNull", fun_bigIsNull, "fun BigNum -> Bool" },
		{ NATIVE_FUN, "bigIsOne", fun_bigIsOne, "fun BigNum -> Bool" },
		{ NATIVE_FUN, "bigPositive", fun_bigPositive, "fun BigNum -> Bool" },
		{ NATIVE_FUN, "bigNbits", fun_bigNbits, "fun BigNum -> Int" },
		{ NATIVE_FUN, "bigLowestBit", fun_bigLowestBit, "fun BigNum -> Int" },
		{ NATIVE_FUN, "bigBit", fun_bigBit, "fun BigNum Int -> Int" },

		{ NATIVE_FUN, "bigAbs", fun_bigAbs, "fun BigNum -> BigNum"},
		{ NATIVE_FUN, "bigNeg", fun_bigNeg, "fun BigNum -> BigNum" },
		{ NATIVE_FUN, "bigAdd", fun_bigAdd, "fun BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigSub", fun_bigSub, "fun BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigXor", fun_bigXor, "fun BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigASR1", fun_bigASR1, "fun BigNum -> BigNum" },
		{ NATIVE_FUN, "bigASR", fun_bigASR, "fun BigNum Int -> BigNum" },
		{ NATIVE_FUN, "bigASL1", fun_bigASL1, "fun BigNum -> BigNum" },
		{ NATIVE_FUN, "bigASL", fun_bigASL, "fun BigNum Int -> BigNum" },

		{ NATIVE_FUN, "bigMul", fun_bigMul, "fun BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigSquare", fun_bigSquare, "fun BigNum -> BigNum" },
		{ NATIVE_FUN, "bigExp", fun_bigExp, "fun BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigDiv", fun_bigDiv, "fun BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigDivRemainder", fun_bigDivRemainder, "fun BigNum BigNum -> [BigNum BigNum]" },
		{ NATIVE_FUN, "bigEuclid", fun_bigEuclid, "fun BigNum BigNum -> [BigNum BigNum BigNum]" },
		{ NATIVE_FUN, "bigInv", fun_bigInv, "fun BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigGcd", fun_bigGcd, "fun BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigCheckFirstFactors", fun_bigCheckFirstFactors, "fun BigNum -> Int" },

		{ NATIVE_FUN, "bigMod", fun_bigMod, "fun BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigNegMod", fun_bigNegMod, "fun BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigModPower2", fun_bigModPower2, "fun BigNum Int -> BigNum" },
		{ NATIVE_FUN, "bigModPower2Mul", fun_bigMulPower2, "fun BigNum BigNum Int -> BigNum" },
		{ NATIVE_FUN, "bigAddMod", fun_bigAddMod, "fun BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigSubMod", fun_bigSubMod, "fun BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigMulMod", fun_bigMulMod, "fun BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigSquareMod", fun_bigSquareMod, "fun BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigDivMod", fun_bigDivMod, "fun BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigExpMod", fun_bigExpMod, "fun BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigExpChinese5", fun_bigExpChinese5, "fun BigNum BigNum BigNum BigNum BigNum BigNum -> BigNum" },

		{ NATIVE_FUN, "bigBarrett", fun_bigBarrett, "fun BigNum -> BigNum" },
		{ NATIVE_FUN, "bigModBarrett", fun_bigModBarrett, "fun BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigMulModBarrett", fun_bigMulModBarrett, "fun BigNum BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigSquareModBarrett", fun_bigSquareModBarrett, "fun BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigDivModBarrett", fun_bigDivModBarrett, "fun BigNum BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigExpModBarrett", fun_bigExpModBarrett, "fun BigNum BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigExpModBarrettFast", fun_bigExpModBarrettFast, "fun BigNum BigNum BigNum BigNum -> BigNum" },

		{ NATIVE_FUN, "bigMontgomery", fun_bigMontgomery, "fun BigNum -> BigNum" },
		{ NATIVE_FUN, "bigMontgomeryForm", fun_bigMontgomeryForm, "fun BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigMontgomeryRedc", fun_bigRedc, "fun BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigMulModMontgomery", fun_bigMulModMontgomery, "fun BigNum BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigSquareModMontgomery", fun_bigSquareModMontgomery, "fun BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigDivModMontgomery", fun_bigDivModMontgomery, "fun BigNum BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigExpModMontgomery", fun_bigExpModMontgomery, "fun BigNum BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigExpModMontgomeryFast", fun_bigExpModMontgomeryFast, "fun BigNum BigNum BigNum BigNum -> BigNum" },

		{ NATIVE_FUN, "bigX25519", fun_bigX25519, "fun BigNum BigNum BigNum BigNum BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigEd25519Add", fun_bigEd25519Add, "fun BigNum BigNum BigNum [BigNum BigNum BigNum BigNum] [BigNum BigNum BigNum BigNum] -> [BigNum BigNum BigNum BigNum]" },
		{ NATIVE_FUN, "bigEd25519Mul", fun_bigEd25519Mul, "fun Bool BigNum BigNum BigNum BigNum BigNum [BigNum BigNum BigNum BigNum] -> [BigNum BigNum BigNum BigNum]" },
		{ NATIVE_FUN, "bigWeierstrassA0Add", fun_bigWeierstrassA0Add, "fun BigNum BigNum BigNum [BigNum BigNum BigNum] [BigNum BigNum BigNum] -> [BigNum BigNum BigNum]" },
		{ NATIVE_FUN, "bigWeierstrassA0Mul", fun_bigWeierstrassA0Mul, "fun Bool BigNum BigNum BigNum BigNum [BigNum BigNum BigNum] -> [BigNum BigNum BigNum]" },
		{ NATIVE_FUN, "bigWeierstrassA3Add", fun_bigWeierstrassA3Add, "fun BigNum BigNum BigNum [BigNum BigNum BigNum] [BigNum BigNum BigNum] -> [BigNum BigNum BigNum]" },
		{ NATIVE_FUN, "bigWeierstrassA3Mul", fun_bigWeierstrassA3Mul, "fun Bool BigNum BigNum BigNum BigNum [BigNum BigNum BigNum] -> [BigNum BigNum BigNum]" },
	};
	NATIVE_DEF(nativeDefs);
	MM.bigGT = nativeOpcode("bigGreater", 2);
	MM.bigGE = nativeOpcode("bigGreaterEquals", 2);
	MM.bigLT = nativeOpcode("bigLower", 2);
	MM.bigLE = nativeOpcode("bigLowerEquals", 2);

	MM.bigNeg = nativeOpcode("bigNeg", 1);
	MM.bigAdd = nativeOpcode("bigAdd", 2);
	MM.bigSub = nativeOpcode("bigSub", 2);

	MM.bigMul = nativeOpcode("bigMul", 2);
	MM.bigSquare = nativeOpcode("bigSquare", 1);
	MM.bigExp = nativeOpcode("bigExp", 2);
	MM.bigDiv = nativeOpcode("bigDiv", 2);

	MM.bigMod = nativeOpcode("bigMod", 2);
	MM.bigNegMod = nativeOpcode("bigNegMod", 2);
	MM.bigAddMod = nativeOpcode("bigAddMod", 3);
	MM.bigSubMod = nativeOpcode("bigSubMod", 3);
	MM.bigMulMod = nativeOpcode("bigMulMod", 3);
	MM.bigSquareMod = nativeOpcode("bigSquareMod", 2);
	MM.bigDivMod = nativeOpcode("bigDivMod", 3);
	MM.bigExpMod = nativeOpcode("bigExpMod", 3);

	MM.bigModBarrett = nativeOpcode("bigModBarrett", 3);
	MM.bigMulModBarrett = nativeOpcode("bigMulModBarrett", 4);
	MM.bigSquareModBarrett = nativeOpcode("bigSquareModBarrett", 3);
	MM.bigDivModBarrett = nativeOpcode("bigDivModBarrett", 4);
	MM.bigExpModBarrett = nativeOpcode("bigExpModBarrett", 4);

	MM.bigMontgomeryRedc = nativeOpcode("bigMontgomeryRedc", 3);
	MM.bigMontgomeryForm = nativeOpcode("bigMontgomeryForm", 2);

	MM.bigMulModMontgomery = nativeOpcode("bigMulModMontgomery", 4);
	MM.bigSquareModMontgomery = nativeOpcode("bigSquareModMontgomery", 3);
	MM.bigDivModMontgomery = nativeOpcode("bigDivModMontgomery", 4);
	MM.bigExpModMontgomery = nativeOpcode("bigExpModMontgomery", 4);

	bigInit();
	return 0;
}
