// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _UTIL_BIGNUM_
#define _UTIL_BIGNUM_

#include"minimacy.h"

#ifdef ON_WINDOWS
#ifdef ATOMIC_32
#else
#define USE_MSVC_X86_64
#define USE_BIG_64
#endif
#else
#ifdef ATOMIC_32
#define USE_ARM32
#else
#define USE_GCC_64
#define USE_BIG_64
#endif
#endif

typedef unsigned int big32;
typedef unsigned long long big64;

#ifdef USE_BIG_64
typedef big64 bigword;
#define BIG_WORD_BITS_LOG2 6
#define BIG_WORD_ONE 1ULL
#define BIG_WORD_FORMAT "%llx"
#define BIG_WORD_FORMAT_ZEROS "%016llx"
#define bigRegister32(n) bigRegister(((n)+1)>>1)
#else
typedef big32 bigword;
#define BIG_WORD_BITS_LOG2 5
#define BIG_WORD_ONE 1
#define BIG_WORD_FORMAT "%x"
#define BIG_WORD_FORMAT_ZEROS "%08x"
#define bigRegister32(n) bigRegister(n)

#endif

#define BIG_WORD_BITS              (1<<BIG_WORD_BITS_LOG2)
#define BIG_WORD_BIT_MASK          (BIG_WORD_BITS-1)
#define BIG_WORDS_FROM_BITS(bits)  (((bits)+BIG_WORD_BIT_MASK)>>BIG_WORD_BITS_LOG2)
#define BIG_WORD_FROM_BIT(bit)     ((bit)>>BIG_WORD_BITS_LOG2)
#define BIG_WORD_BIT_FROM_BIT(bit) (bigword)(BIG_WORD_ONE<<((bit)&BIG_WORD_BIT_MASK))

#define BIG_WORD_LAST_BIT(bits)    (bigword)(BIG_WORD_ONE<<(((bits)-1)&BIG_WORD_BIT_MASK))
#define BIG_WORD_LAST_MASK(bits)   (bigword)(((BIG_WORD_ONE<<1)<<(((bits)-1)&BIG_WORD_BIT_MASK))-1)


#define BIGNUM_MAXWORDS (8320/(8*sizeof(bigword)))	// minimum to handle 4096bits keys/modulo

typedef struct
{
	LB header;

	int len;
	int sign;
	bigword data[1];
}Bignum;
typedef Bignum* bignum;

typedef struct
{
	LB header;

	int len;
	int sign;
	bigword data[BIGNUM_MAXWORDS];
}BignumRegister;

#define LEN_32(len) ((len)<<(BIG_WORD_BITS_LOG2-5))

#define BIGNUM_IS_NULL(a) (((bigLen(a) == 1) && (!bigGet(a, 0)))?1:0)
#define BIGNUM_IS_ONE(a) ((bigLen(a)==1)&&(bigSign(a)==0)&&(bigGet(a,0)==1))
#define BIGNUM_EQUALS(a,b) ((bigSign(a)==bigSign(b))&&(bigLen(a)==bigLen(b))&&(!memcmp(a->data, b->data, bigLen(a) << 2)))
#define BIGNUM_GREATER(a,b) ((bigSign(a)^bigSign(b))?(bigSign(a)?0:1):(bigSign(a)?bigGabs(b,a,0):bigGabs(a,b,0)))
#define BIGNUM_GREATER_EQUAL(a,b) ((bigSign(a)^bigSign(b))?(bigSign(a)?0:1):(bigSign(a)?bigGabs(b,a,1):bigGabs(a,b,1)))
#define BIGNUM_LOWER(a,b) ((bigSign(a) ^ bigSign(b))?(bigSign(a) ? 0 : 1):(bigSign(a) ? bigGabs(b, a, 0) : bigGabs(a, b, 0)))
#define BIGNUM_LOWER_EQUAL(a,b) ((bigSign(a) ^ bigSign(b))?(bigSign(a) ? 0 : 1):(bigSign(a) ? bigGabs(b, a, 1) : bigGabs(a, b, 1)))

#define bigSign(b) (b)->sign
#define bigSignSet(b,s) (b)->sign=(int)(s)
#define bigLen(b) (b)->len
#define bigLenSet(b,l) (b)->len=(int)(l)
#define bigGet(b,n) (b)->data[n]
#define bigSet(b,n,v) (b)->data[n]=(bigword)(v)
#define bigStart32(b) ((big32*)(b)->data)
#define bigLen32(b) LEN_32(bigLen(b))

#define bigRegNeg(a) bigSignSet(a,1-bigSign(a))


LINT bigEquals(bignum a, bignum b);
LINT bigStringHex(bignum b,char* dst);
bignum bigFromDec(char* src);
bignum bigFromHex(char* src);

bignum bigAbs(bignum a);;
bignum bigAdd(bignum a, bignum b);
bignum bigAddMod(bignum a, bignum b, bignum n);
bignum bigAddSub(bignum a, bignum b, LINT sub);
bignum bigASL(bignum a, LINT n);
bignum bigASL1(bignum a);
bignum bigASR(bignum a, LINT n);
bignum bigASR1(bignum a);
bignum bigBarrett(bignum n);
bignum bigCopy(bignum a);
bignum bigRegister(LINT nword);
bignum bigDiv(bignum p, bignum q);
bignum bigDivMod(bignum a, bignum b, bignum n);
bignum bigDivModBarrett(bignum a, bignum b, bignum n, bignum mu);
bignum bigDivRemainder(bignum p, bignum q, bignum* r);
bignum bigExp(bignum a, bignum e);
bignum bigExpChinese5(bignum c, bignum p1, bignum p2, bignum e1, bignum e2, bignum coef);
bignum bigExpMod(bignum p, bignum e, bignum n);
bignum bigExpModBarrett(bignum p, bignum e, bignum n, bignum mu);
bignum bigExpModBarrettFast(bignum p, bignum e, bignum n, bignum mu);
bignum bigMontgomeryRedc(bignum ab, bignum n, bignum nPrime);
bignum bigMontgomeryForm(bignum a, bignum n);
bignum bigExpModMontgomery(bignum a, bignum e, bignum n, bignum nPrime);
bignum bigExpModMontgomeryFast(bignum a, bignum e, bignum n, bignum nPrime);
bignum bigFromBin(char* src, LINT n);
bignum bigFromByte(char v0);
bignum bigFromDec(char* src);
bignum bigFromInt(LINT v0);
bignum bigFromSignedBin(char* src, LINT n);
bignum bigGcd(bignum p, bignum q);
bignum bigInv(bignum p, bignum q);
bignum bigMod(bignum p, bignum q);
bignum bigModBarrett(bignum x, bignum n, bignum mu);
bignum bigModPower2(bignum a, LINT n);
bignum bigMontgomery(bignum n);
bignum bigMulModMontgomery(bignum p, bignum q, bignum n, bignum nPrime);
bignum bigSquareModMontgomery(bignum p, bignum n, bignum nPrime);
bignum bigDivModMontgomery(bignum A, bignum B, bignum n, bignum nPrime);
bignum bigMul(bignum a, bignum b);
bignum bigMulMod(bignum p, bignum q, bignum n);
bignum bigSquareMod(bignum p, bignum n);
bignum bigMulModBarrett(bignum p, bignum q, bignum n, bignum mu);
bignum bigSquareModBarrett(bignum p,bignum n, bignum mu);
bignum bigModPower2Mul(bignum a, bignum b, LINT nBits);
bignum bigNeg(bignum a);
bignum bigNegMod(bignum a, bignum n);
bignum bigPower2(LINT n);
bignum bigRand(LINT nbits, LB* exact);
bignum bigReplace(bignum x, bignum y);
bignum bigSquare(bignum a);
bignum bigSub(bignum a, bignum b);
bignum bigSubMod(bignum a, bignum b, bignum n);
bignum bigXor(bignum a, bignum b);
int bigEuclide(bignum p, bignum q, bignum* a, bignum* b, bignum* pgcd); // ap+bq=pgcd
int bigRegASL(bignum a, LINT n);
LINT bigBit(bignum a, LINT i);
LINT bigCheckFirstFactors(bignum a);
LINT bigCmp(bignum a, bignum b);
LINT bigEquals(bignum a, bignum b);
LINT bigGabs(bignum a, bignum b, LINT ifequal);
LINT bigGreater(bignum a, bignum b);
LINT bigGreaterEqual(bignum a, bignum b);
LINT bigIsEven(bignum a);
LINT bigIsOdd(bignum a);
LINT bigIsNull(bignum a);
LINT bigIsOne(bignum a);
LINT bigLower(bignum b, bignum a);
LINT bigLowerEqual(bignum b, bignum a);
LINT bigLowestBit(bignum b);
LINT bigNbits(bignum b);
LINT bigPositive(bignum a);
LINT bigRelease(bignum b);
bignum bigSelectOrRelease(LINT which, bignum x, bignum y);
void bigKeepOrSwapConstantTime(LINT which, bignum* x, bignum* y);
LINT bigStringBin(bignum b, LINT outlen, char* p);
LINT bigStringHex(bignum b, char* dst);
LINT bigStringSignedBin(bignum b, char* p);
LINT bigToInt(bignum b);
void bigDump(char* label, bignum a);
void bigOptimize(bignum b);
void bigRegASL1(bignum a);
void bigRegASL1(bignum a);
void bigRegASR1(bignum a);
void bigRegASR1(bignum a);
void bigRegSub(bignum a, bignum b);
void bigReset(void);
void bigInit(void);

bignum bigX25519(bignum _P, bignum _Pprime, bignum _Pminus2, bignum _A24, bignum _One, bignum k, bignum u0);

int bigEd25519Add(bignum _P, bignum _Pprime, bignum _D, bignum x1, bignum y1, bignum z1, bignum t1,
	bignum x2, bignum y2, bignum z2, bignum t2,
	bignum* x, bignum* y, bignum* z, bignum* t);
int bigEd25519Mul(int constantTime, bignum _P, bignum _Pprime, bignum _D, bignum _One, bignum s, bignum px, bignum py, bignum pz, bignum pt, bignum* x, bignum* y, bignum* z, bignum* t);

int bigWeierstrassA0Add(bignum _P, bignum _Pprime, bignum b3,
	bignum X1, bignum Y1, bignum Z1,
	bignum X2, bignum Y2, bignum Z2,
	bignum* X, bignum* Y, bignum* Z);
int bigWeierstrassA0Mul(bignum _P, bignum _Pprime, bignum b3, int constantTime, bignum s, bignum px, bignum py, bignum pz, bignum* x, bignum* y, bignum* z);

int bigWeierstrassA3Add(bignum _P, bignum _Pprime, bignum b,
	bignum X1, bignum Y1, bignum Z1,
	bignum X2, bignum Y2, bignum Z2,
	bignum* X, bignum* Y, bignum* Z);
int bigWeierstrassA3Mul(bignum _P, bignum _Pprime, bignum b, int constantTime, bignum s, bignum px, bignum py, bignum pz, bignum* x, bignum* y, bignum* z);

#endif
