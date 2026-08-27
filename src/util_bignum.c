// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"
#include"util_bignum.h"

#define BIGREGISTERS 24
BignumRegister BigRegisters[BIGREGISTERS];
bignum BigList = NULL;
LINT BigCount;	// number of available registers, should be BIGREGISTERS when idle
//LINT BigCountMin = BIGREGISTERS;
const unsigned char BIT_RIGHT[256] = {
	0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
};
const unsigned char BIT_LEFT[256] = {
	0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};

void bigDump(char* label, bignum a)
{
	int i;
	if (label) PRINTF(LOG_DEV, "%s: ", label);
	if (bigSign(a)) PRINTF(LOG_DEV, "-");
	for (i = bigLen(a) - 1; i >= 0; i--) PRINTF(LOG_DEV, BIG_WORD_FORMAT_ZEROS, bigGet(a, i));
	PRINTF(LOG_DEV, "\n");
}

void bigOptimize(bignum b)
{
	int len= b->len - 1;
	bigword* src = b->data;
	if (src[len]) return;
	while ((len > 0) && !src[len]) len--;
	if (len == 0 && !src[0]) b->sign = 0; // zero is always positive
	b->len = len + 1;
}

bignum bigRegister(LINT nword)
{
	bignum b;
	if (!BigList)
	{
		PRINTF(LOG_SYS, "> Error: Out of bignum registers\n");
		return NULL;
	}

	if (nword < 1) nword = 1;
	if (nword > BIGNUM_MAXWORDS)
	{
		PRINTF(LOG_SYS, "> Error: Bignum too long (" LSD " bits, max is " LSD ")\n", nword * sizeof(bigword) * 8, BIGNUM_MAXWORDS * sizeof(bigword) * 8);
		bigReset();	// we assume that in such case, the caller aborts any processing on previously borrowed registers
		return NULL;
	}
	b = BigList;
	BigList = (bignum)BigList->header.nextBlock;
	BigCount--;
	//if (BigCountMin > BigCount) {
	//	BigCountMin = BigCount;
	//	printf(">>>>BigCountMin=%d\n", BigCountMin);
	//}
	b->len = (int)nword;
	b->sign = 0;
	memset((void*)b->data, 0, nword * sizeof(bigword));
	return b;
}
bignum bigCopy(bignum a)
{
	LINT nword = bigLen(a);
	bignum b;
	if (!BigList)
	{
		PRINTF(LOG_SYS, "> Error: Out of bignum registers\n");
		return NULL;
	}

	if (nword < 1) nword = 1;
	if (nword > BIGNUM_MAXWORDS)
	{
		PRINTF(LOG_SYS, "> Error: Bignum too long (" LSD " bits, max is " LSD ")\n", nword * sizeof(bigword) * 8, BIGNUM_MAXWORDS * sizeof(bigword) * 8);
		bigReset();	// we assume that in such case, the caller aborts any processing on previously borrowed registers
		return NULL;
	}
	b = BigList;
	BigList = (bignum)BigList->header.nextBlock;
	BigCount--;
	//if (BigCountMin > BigCount) {
	//	BigCountMin = BigCount;
	//	printf(">>>>BigCountMin=%d\n", BigCountMin);
	//}
	b->len = (int)nword;
	b->sign = a->sign;
	memset((void*)b->data, 0, nword * sizeof(bigword));
	memcpy((void*)b->data, (void*)a->data, nword * sizeof(bigword));
	return b;
}

LINT bigRelease(bignum b)
{
	if (b && BigCount< BIGREGISTERS) {
		b->header.nextBlock = (LB*)BigList;
		BigList = b;
		BigCount++;
	}
	return 1;
}

bignum bigReplace(bignum x, bignum y)
{
	if (x && BigCount < BIGREGISTERS) {
		x->header.nextBlock = (LB*)BigList;
		BigList = x;
		BigCount++;
	}
	return y;
}

bignum bigSelectOrRelease(LINT which, bignum x, bignum y)
{
	LINT mask = 0 - which;
	LINT invmask = ~mask;
	LINT keep   = (((LINT)x) & invmask) | (((LINT)y) & mask);
	LINT giveup = (((LINT)y) & invmask) | (((LINT)x) & mask);
	bigRelease((bignum)giveup);
	return (bignum)keep;
}

void bigKeepOrSwapConstantTime(LINT which, bignum *x, bignum *y)
{
	LINT mask = 0 - which;
	LINT invmask = ~mask;
	LINT newX   = (((LINT)*x) & invmask) | (((LINT)*y) & mask);
	LINT newY = (((LINT)*y) & invmask) | (((LINT)*x) & mask);
	*x = (bignum)newX;
	*y = (bignum)newY;
}

LINT bigIsNull(bignum a)
{
	return BIGNUM_IS_NULL(a);
}

bignum bigFromByte(char v0)	// big alloc 1
{
	bignum b;
	b = bigRegister(1);
	bigSet(b, 0, (bigword)v0);
	return b;
}
bignum bigFromInt(LINT v0)	// big alloc 1
{
	bignum b;
	LINT w0 = (v0 > 0) ? v0 : -v0;
	b = bigRegister(1);
	bigSet(b, 0, (bigword)w0);
	if (v0 < 0) bigSignSet(b, 1);
	return b;
}

LINT bigToInt(bignum b)	// return signed 32 bits	// big alloc 0
{
	int x = (int)bigGet(b, 0) & 0x7fffffff;
	if (bigSign(b)) x = -x;
	return (LINT)x;
}

bignum bigFromBin(char* src, LINT n)	// big alloc 1
{
	bignum b;
	LINT i;
	unsigned char* p;
	if (n <= 0) return bigFromByte(0);
	p = (unsigned char*)src;
	b = bigRegister(BIG_WORDS_FROM_BITS(n * 8)); if (!b) return NULL;
	for (i = 0; i < n; i++)
	{
		LINT ind = n - i - 1;
		LINT offset = BIG_WORD_FROM_BIT(ind * 8);
		bigword c = p[i] & 255;
		bigSet(b, offset, bigGet(b, offset) + (c << ((ind * 8) & BIG_WORD_BIT_MASK)));
	}
	bigOptimize(b);
	return b;
}

bignum bigFromSignedBin(char* src, LINT n)	// big alloc 1
{
	bignum b;
	LINT i;
	unsigned char sign;
	unsigned char* p;
	if (n <= 1) return bigFromByte(0);
	p = (unsigned char*)src;
	sign = *(p++); n--;
	b = bigRegister(BIG_WORDS_FROM_BITS(n * 8)); if (!b) return NULL;
	for (i = 0; i < n; i++)
	{
		LINT ind = n - i - 1;
		LINT offset = BIG_WORD_FROM_BIT(ind * 8);
		bigSet(b, offset, bigGet(b, offset) + ((p[i] & 255) << ((ind * 8) & BIG_WORD_BIT_MASK)));
	}
	if (sign) bigSignSet(b, 1);
	bigOptimize(b);
	return b;
}

LINT bigStringBin(bignum b, LINT outlen, char* p)	// big alloc 0
{
	LINT i;
	LINT zlen, vlen;
	LINT len = (bigNbits(b)+7)>>3;
	if (!len) len++;
	if (bigSign(b)) return -1;	// cannot export negative number
	if (outlen <= 0) outlen = len;
	if (!p) return outlen;
	zlen = (outlen > len) ? (outlen - len) : 0;
	vlen = (outlen > len) ? len : outlen;

	for (i = 0; i < zlen; i++) p[i] = 0;
	for (i = 0; i < vlen; i++)
	{
		bigword x = bigGet(b, BIG_WORD_FROM_BIT(i * 8));
		x >>= (i * 8) & BIG_WORD_BIT_MASK;
		p[outlen - 1 - i] = (char)x;
	}
	return len;
}

LINT bigStringSignedBin(bignum b, char* p)	// big alloc 0
{
	LINT i;
	LINT len = (bigNbits(b) + 7) >> 3;
	if (!len) len++;

	if (!p) return len + 1;

	*(p++) = bigSign(b) ? 1 : 0;
	for (i = 0; i < len; i++)
	{
		bigword x = bigGet(b, BIG_WORD_FROM_BIT(i * 8));
		x >>= (i * 8) & BIG_WORD_BIT_MASK;
		p[len - 1 - i] = (char)x;
	}
	return len;
}

bignum bigFromDec(char* src)	// big alloc 1
{
	bignum b;
	LINT lensrc = strlen(src);
	LINT len12bits = (lensrc + 2) / 3; // at least 12 bits to encode 3 decimal digits (999=0x3e7)
	LINT len = ((len12bits * 12)+31)>>5;
	if (!len) len = 1;
	b = bigRegister32(len); if (!b) return NULL;

	if ((*src) == '-')
	{
		bigSignSet(b, 1);
		src++;
	}
	while (((*src) >= '0') && ((*src) <= '9'))
	{
		LINT i;
		big64 cc = (*(src++)) - '0';
		big32* dst=bigStart32(b);
		for (i = 0; i < len; i++)
		{
			big64 x = dst[i];
			cc += x * 10;
			dst[i]=(big32)cc;
			cc >>= 32;
		}
	}
	bigOptimize(b);
	return b;
}

LINT bigStringHex(bignum b, char* dst)	// big alloc 0
{
	LINT i;
	LINT len;
	LINT buffer_len;
	char buffer[32];
	snprintf(buffer, 32, BIG_WORD_FORMAT, bigGet(b, bigLen(b) - 1));	// we need this to count the exact number of hex digits
	buffer_len = strlen(buffer);
	len = buffer_len + (bigLen(b) - 1) * (BIG_WORD_BITS >> 2) + ((bigSign(b)) ? 1 : 0);
	if (buffer_len & 1) len++;

	if (!dst) return len;
	if (bigSign(b)) *(dst++) = '-';

	if (buffer_len & 1) *(dst++) = '0';
	snprintf(dst, 32, BIG_WORD_FORMAT, bigGet(b, bigLen(b) - 1));
	dst += strlen(dst);

	for (i = 1; i < bigLen(b); i++)
	{
		snprintf(dst, 32, BIG_WORD_FORMAT_ZEROS, bigGet(b, bigLen(b) - 1 - i));
		dst += strlen(dst);
	}
	return len;
}


bignum bigRand(LINT nbits, LB* exact)	// big alloc 1
{
	bignum b;
	LINT i;
	bigword x;
	LINT len = BIG_WORDS_FROM_BITS(nbits);
	if (len < 1) len = 1;
	b = bigRegister(len); if (!b) return NULL;

	for (i = 0; i < len; i++) hwRandomBytes((char*)&bigGet(b, i), sizeof(bigword));

	x = bigGet(b, len - 1) & BIG_WORD_LAST_MASK(nbits);
	if (exact == MM._true) x |= BIG_WORD_LAST_BIT(nbits);
	bigSet(b, len - 1, x);
	bigOptimize(b);
	return b;
}

LINT bigNbits(bignum b)	// big alloc 0
{
	LINT n = bigLen(b);
	bigword x = bigGet(b, n - 1);
	n = (n - 1) * BIG_WORD_BITS;
#ifdef USE_BIG_64
	{
		bigword y = x >> 32;
		if (y) {
			if (y & 0xff000000) return n + 56 + BIT_LEFT[y >> 24];
			if (y & 0xff0000)   return n + 48 + BIT_LEFT[y >> 16];
			if (y & 0xff00)     return n + 40 + BIT_LEFT[y >> 8];
			if (y & 0xff)       return n + 32 + BIT_LEFT[y];
		}
	}
#endif
	if (x & 0xff000000) return n + 24 + BIT_LEFT[x >> 24];
	if (x & 0xff0000)   return n + 16 + BIT_LEFT[x >> 16];
	if (x & 0xff00)     return n + 8 + BIT_LEFT[x >> 8];
	return n + BIT_LEFT[x];
}
LINT bigLowestBit(bignum b)	// big alloc 0
{
	LINT i;
	if (BIGNUM_IS_NULL(b)) return 0;
	for (i = 0; i < bigLen(b); i++) {
		bigword x = bigGet(b, i);
		if (x) {
			LINT i0 = i * BIG_WORD_BITS;
			if (x & 0xffffffff) {
				if (x & 0xff)       return i0 + BIT_RIGHT[x & 0xff];
				if (x & 0xff00)     return i0 + 8 + BIT_RIGHT[(x >> 8) & 0xff];
				if (x & 0xff0000)   return i0 + 16 + BIT_RIGHT[(x >> 16) & 0xff];
				if (x & 0xff000000) return i0 + 24 + BIT_RIGHT[(x >> 24) & 0xff];
			}
#ifdef USE_BIG_64
			x >>= 32;
			if (x & 0xff)       return i0 + 32 + BIT_RIGHT[x & 0xff];
			if (x & 0xff00)     return i0 + 40 + BIT_RIGHT[(x >> 8) & 0xff];
			if (x & 0xff0000)   return i0 + 48 + BIT_RIGHT[(x >> 16) & 0xff];
			if (x & 0xff000000) return i0 + 56 + BIT_RIGHT[(x >> 24) & 0xff];
#endif
		}
	}
	return 0;	// should never happen
}
bignum bigAbs(bignum a)	// big alloc 1
{
	bignum b = bigCopy(a);
	bigSignSet(b, 0);
	return b;
}

bignum bigNeg(bignum a)	// big alloc 1
{
	bignum b = bigCopy(a);
	bigRegNeg(b);
	return b;
}
LINT bigPositive(bignum a)	// big alloc 0
{
	return bigSign(a) ? 0 : 1;
}

void bigRegASR1(bignum a)	// big alloc 0
{
	LINT i;
	bigword cc = 0;
	for (i = bigLen(a) - 1; i >= 0; i--)
	{
		bigword x = bigGet(a, i);
		bigSet(a, i, (x >> 1) + (cc << (BIG_WORD_BITS-1)));
		cc = x & 1;
	}
	bigOptimize(a);
}
void bigRegASL1(bignum a)	// big alloc 0
{
	LINT i, len;
	bigword cc = 0;
	bigword* src = a->data;
	len = bigLen(a);
	for (i = 0; i < len; i++)
	{
		bigword x = src[i];
		src[i] = (x << 1) + cc;
		cc = x >> (BIG_WORD_BITS-1);
	}
	if (!cc) return;
	src[i] = 1;
	a->len++;
}

bignum bigASR1(bignum a)	// big alloc 1
{
	bignum b = bigCopy(a);
	bigRegASR1(b);
	return b;
}
bignum bigASR(bignum a, LINT n)	// big alloc 1
{
	bignum b;
	LINT lenb, i;
	LINT na = bigNbits(a);
	LINT nb = na - n;
	LINT off = BIG_WORD_FROM_BIT(n);
	LINT shr = n & BIG_WORD_BIT_MASK;
	if (n < 0) return bigFromByte(0);
	if (n == 0) return bigCopy(a);
	if (nb <= 0) return bigFromByte(0);
	lenb = BIG_WORDS_FROM_BITS(nb);
	b = bigRegister(lenb);

	if (shr)
	{
		for (i = 0; i < lenb; i++)
		{
			if (off + i + 1 < bigLen(a)) bigSet(b, i, ((bigGet(a, off + i)) >> shr) + ((bigGet(a, off + i + 1)) << (BIG_WORD_BITS - shr)));
			else bigSet(b, i, (bigGet(a, off + i)) >> shr);
		}
	}
	else
	{
		for (i = 0; i < lenb; i++) bigSet(b, i, bigGet(a, off + i));
	}
	return b;
}
bignum bigASL(bignum a, LINT n)	// big alloc 1
{
	bignum b;
	LINT lenb, i;
	LINT na = bigNbits(a);
	LINT nb = na + n;
	LINT off = BIG_WORD_FROM_BIT(n);
	LINT shl = n & BIG_WORD_BIT_MASK;
	if (n < 0) return bigFromByte(0);
	if (n == 0) return bigCopy(a);
	if (nb <= 0) return bigFromByte(0);
	lenb = BIG_WORDS_FROM_BITS(nb);
	b = bigRegister(lenb); if (!b) return NULL;

	if (shl)
	{
		for (i = 0; i < bigLen(a); i++)
		{
			bigSet(b, i + off, bigGet(b, i + off) + (bigGet(a, i) << shl));
			if (i + off + 1 <= lenb) bigSet(b, i + off + 1, bigGet(b, i + off + 1) + (bigGet(a, i) >> (BIG_WORD_BITS - shl)));
		}
	}
	else
	{
		for (i = 0; i < bigLen(a); i++) bigSet(b, i + off, bigGet(a, i));
	}
	return b;
}
int bigRegASL(bignum a, LINT n)	// big alloc 0
{
	LINT lenResult, i;
	LINT shl = n & BIG_WORD_BIT_MASK;
	if (n == 0) return 0;
	if (n < 0) {
		a->data[0] = 0;
		a->len = 1;
		a->sign = 0;
		return 0;
	}
	lenResult = bigLen(a) + BIG_WORDS_FROM_BITS(n);
	if (lenResult > BIGNUM_MAXWORDS) return -1;
	if (shl)
	{
		for (i = bigLen(a); i < lenResult; i++) bigSet(a, i, 0);
		for (i = 0; i <= bigLen(a); i++) {
			bigword val = (i < bigLen(a)) ? (bigGet(a, bigLen(a) - 1 - i) >> (BIG_WORD_BITS - shl)) : 0;
			val |= i ? (bigGet(a, bigLen(a) - i) << shl) : 0;
			bigSet(a, lenResult - 1 - i, val);
		}
		for (; i < lenResult; i++) bigSet(a, lenResult - 1 - i, 0);
	}
	else
	{
		n = BIG_WORD_FROM_BIT(n);
		for (i = bigLen(a) - 1; i >= 0; i--) bigSet(a, i + n, bigGet(a, i));
		for (i = 0; i < n; i++) bigSet(a, i, 0);
	}
	a->len = (int)lenResult;
	bigOptimize(a);
	return 0;
}
bignum bigASL1(bignum a)	// big alloc 1
{
	LINT i;
	bigword cc = 0;
	bignum b = bigRegister(bigLen(a) + 1); if (!b) return NULL;

	for (i = 0; i < bigLen(a); i++)
	{
		bigword x = bigGet(a, i);
		bigSet(b, i, (x << 1) + cc);
		cc = (x >> (BIG_WORD_BITS - 1)) & 1;
	}
	bigSet(b, i, cc);
	bigOptimize(b);
	return b;
}

LINT bigEquals(bignum a, bignum b)	// big alloc 0
{
	return BIGNUM_EQUALS(a, b);
}
LINT bigGabs(bignum a, bignum b, LINT ifequal)	// big alloc 0
{
	LINT i;
	if (bigLen(a) < bigLen(b)) return 0;
	if (bigLen(a) > bigLen(b)) return 1;
	i = bigLen(a) - 1;
	while (i >= 0)
	{
		if (bigGet(a, i) < bigGet(b, i)) return 0;
		if (bigGet(a, i) > bigGet(b, i)) return 1;
		i--;
	}
	return ifequal;
}

LINT bigGreater(bignum a, bignum b)	// big alloc 0
{
	return BIGNUM_GREATER(a, b);
}
LINT bigGreaterEqual(bignum a, bignum b)	// big alloc 0
{
	return BIGNUM_GREATER_EQUAL(a, b);
}
LINT bigLower(bignum b, bignum a)	// big alloc 0
{
	return BIGNUM_LOWER(a, b);
}
LINT bigLowerEqual(bignum b, bignum a)	// big alloc 0
{
	return BIGNUM_LOWER_EQUAL(a, b);
}
LINT bigCmp(bignum a, bignum b)	// big alloc 0
{
	if (BIGNUM_EQUALS(a, b)) return 0;
	if (BIGNUM_GREATER(a, b)) return 1;
	return -1;
}

LINT bigIsOne(bignum a)	// big alloc 0
{
	return BIGNUM_IS_ONE(a) ? 1 : 0;
}
LINT bigIsEven(bignum a)	// big alloc 0
{
	return 1 - (bigGet(a, 0) & 1);
}
LINT bigIsOdd(bignum a)	// big alloc 0
{
	return (bigGet(a, 0) & 1);
}

#define FIRST_FACTORS_NB 7
#define FIRST_FACTORS_INTERVAL 33
#define FIRST_FACTORS_PRODUCT 4849845	// = 3*5*7*11*13*17*19
static const bigword FirstFactors[FIRST_FACTORS_NB] = { 3, 5, 7, 11, 13, 17, 19 };

LINT bigCheckFirstFactors(bignum a)	// return delta so that a+delta is not a multiple of 3, ..., 19	// big alloc 0
{
	bigword P = FIRST_FACTORS_PRODUCT;
	bigword sum = 0;
	bigword k = 1;
	bigword i, size;
	bigword factorResults[FIRST_FACTORS_NB];
	bigword iterate = 0;
	char crible[FIRST_FACTORS_INTERVAL];

	unsigned char* p = (unsigned char*)a->data;
	size = a->len * sizeof(bigword);
	for (i = 0; i < size; i++) {	// size<BIGNUM_MAXWORDS*4 (=1040)
		sum += (*(p++)) * k;
		k <<= 8;
		if (sum >= P) sum %= P;
		if (k >= P) k %= P;
	}
	// now sum<P
	for (i = 0; i < FIRST_FACTORS_NB; i++) {
		if (!(factorResults[i] = sum % FirstFactors[i])) iterate = 1;	// at least one multiple
	}
	if (!iterate) return 0;
	memset(crible, 0, FIRST_FACTORS_INTERVAL);
	for (i = 0; i < FIRST_FACTORS_NB; i++) {
		bigword f = FirstFactors[i];
		bigword j = f - factorResults[i];
		for (; j < FIRST_FACTORS_INTERVAL; j += f) crible[j] = 1;
	}
	for (i = 2; i < FIRST_FACTORS_INTERVAL; i += 2) if (!crible[i]) return i;
	return FIRST_FACTORS_INTERVAL;
}

LINT bigBit(bignum a, LINT i)	// big alloc 0
{
	bigword x;
	if ((i < 0) || (i >= bigLen(a) * BIG_WORD_BITS)) return 0;
	x = bigGet(a, BIG_WORD_FROM_BIT(i)) >> (i& BIG_WORD_BIT_MASK);
	return (LINT) x&1;
}
bignum bigModPower2(bignum a, LINT n)	// big alloc 1
{
	LINT len, k;
	bignum b;
	if (n <= 0) return bigFromByte(0);
	b = bigCopy(a);
	len = BIG_WORDS_FROM_BITS(n);
	k = bigLen(b) * BIG_WORD_BITS;

	if (n >= k) return b;
	bigLenSet(b, len);
	bigSet(b, len - 1, bigGet(b, len - 1) & BIG_WORD_LAST_MASK(n));
	bigOptimize(b);
	return b;
}
bignum bigPower2(LINT n)	// big alloc 1
{
	bignum b;
	if (n < 0) return bigFromByte(0);
	if (n == 0) return bigFromByte(1);
	b = bigRegister(BIG_WORDS_FROM_BITS(n + 1)); if (!b) return NULL;	// n=4096 => bigRegister(257)
	bigSet(b, BIG_WORD_FROM_BIT(n), BIG_WORD_BIT_FROM_BIT(n));
	return b;
}

bignum bigXor(bignum a, bignum b)	// big alloc 1
{
	LINT i, lenB;
	bignum c;
	bigword* srcB, * dst;
	if (bigLen(a) < bigLen(b)) {
		bignum z = a; a = b; b = z;
	}
	c = bigCopy(a);
	lenB = bigLen(b);
	srcB = b->data;
	dst = c->data;
	for (i = 0; i < lenB; i++) dst[i] ^= srcB[i];
	c->sign ^= b->sign;
	bigOptimize(c);
	return c;
}

bignum bigAdd(bignum a, bignum b)	// big alloc 1
{
	return bigAddSub(a, b, 0);
}
bignum bigSub(bignum a, bignum b)	// big alloc 1
{
	return bigAddSub(a, b, 1);
}

bignum bigExp(bignum a, bignum e)	// big alloc 3
{
	LINT i, n;
	bignum r;
	if ((bigSign(e)) || (BIGNUM_IS_NULL(a))) return bigFromByte(0);

	a = bigCopy(a);
	r = bigFromByte(1);
	n = bigNbits(e);
	for (i = 0; i < n; i++) {
		bignum r2 = bigMul(r, a); if (!r2) return NULL;
		r = bigSelectOrRelease(bigBit(e, i), r, r2);
		a = bigReplace(a, bigSquare(a)); if (!a) return NULL;
	}
	bigRelease(a);
	return r;
}

bignum bigDivRemainder(bignum p, bignum q, bignum* r)	// big alloc 4
{
	bignum w, v, d;
	LINT n;
	if (BIGNUM_IS_NULL(q))
	{
		if (r) *r = bigFromByte(0);
		return bigFromByte(0);
	}
	w = bigCopy(p);
	v = bigCopy(q);
	bigSignSet(w, 0);
	bigSignSet(v, 0);
	n = bigNbits(w) - bigNbits(v);
	d = bigFromByte(0);
	if (n >= 0) {
		if (n > 0) {
			bigRegASL(v, n);
			bigLenSet(d, BIG_WORDS_FROM_BITS(n + 1));
			memset(d->data, 0, d->len * sizeof(bigword));
		}
		while (n >= 0)
		{
			if (BIGNUM_GREATER_EQUAL(w, v))
			{
				bigRegSub(w, v);
				bigSet(d, BIG_WORD_FROM_BIT(n), bigGet(d, BIG_WORD_FROM_BIT(n)) | BIG_WORD_BIT_FROM_BIT(n));
			}
			bigRegASR1(v);
			n--;
		}
	}
	bigOptimize(d);
	if (bigSign(q)) bigRegNeg(d);
	if (bigSign(p)) bigRegNeg(d);
	if ((bigSign(p)) && (!BIGNUM_IS_NULL(w)))
	{
		bignum unit = bigFromByte(1);
		d = bigReplace(d, bigSign(q) ? bigAdd(d, unit) : bigSub(d, unit)); if (!d) return NULL;
		if (r && bigSign(q))
		{
			w = bigReplace(w, bigAdd(q, w)); if (!w) return NULL;
			bigRegNeg(w);
		}
		else
		{
			w = bigReplace(w, bigSub(q, w)); if (!w) return NULL;
		}
		bigRelease(unit);
	}
	bigOptimize(d);
	if (r) *r = w;
	else bigRelease(w);
	bigRelease(v);
	return d;
}
bignum bigDiv(bignum p, bignum q)	// big alloc 4
{
	return bigDivRemainder(p, q, NULL);
}

bignum bigMod(bignum p, bignum q)	// big alloc 2
{
	bignum w, v;
	LINT n = 1;
	if ((BIGNUM_IS_NULL(q)) || (bigSign(q))) return bigFromByte(0);
	if ((!bigSign(q)) && (!bigSign(p)) && (BIGNUM_GREATER(q, p))) return bigCopy(p);
	w = bigCopy(p);
	v = bigCopy(q);
	bigSignSet(w, 0);
	n = bigNbits(w) - bigNbits(v);
	if (n >= 0) {
		if (n > 0) bigRegASL(v, n);// bigReplace(v, bigASL(v, n));
		while (n >= 0)
		{
			if (BIGNUM_GREATER_EQUAL(w, v)) bigRegSub(w, v);
			bigRegASR1(v);
			n--;
		}
	}
	bigRelease(v);
	if ((bigSign(p)) && (!BIGNUM_IS_NULL(w))) w = bigReplace(w, bigSub(q, w));
	bigOptimize(w);
	return w;
}
bignum bigAddMod(bignum a, bignum b, bignum n)	// big alloc 2
{
	bignum v = bigAddSub(a, b, 0); if (!v) return NULL;
	return bigReplace(v, bigMod(v, n));
}
bignum bigSubMod(bignum a, bignum b, bignum n)	// big alloc 2
{
	bignum v = bigAddSub(a, b, 1); if (!v) return NULL;
	return bigReplace(v, bigMod(v, n));
}
bignum bigMulMod(bignum p, bignum q, bignum n)	// big alloc 2
{
	bignum r;
	if ((BIGNUM_IS_NULL(n)) || (bigSign(n))) return bigFromByte(0);
	r = bigMul(p, q); if (!r) return NULL;
	return bigReplace(r, bigMod(r, n));
}
bignum bigSquareMod(bignum p, bignum n)	// big alloc 3
{
	bignum r;
	if ((BIGNUM_IS_NULL(n)) || (bigSign(n))) return bigFromByte(0);
	r = bigSquare(p); if (!r) return NULL;
	return bigReplace(r, bigMod(r, n));
}

bignum bigGcd(bignum p, bignum q)	// big alloc 4
{
	bignum y;
	bignum x;
	if (BIGNUM_IS_NULL(p)) return bigCopy(q);
	if (BIGNUM_IS_NULL(q)) return bigCopy(p);
	x = bigCopy(p);
	y = bigCopy(q);
	bigSignSet(x, 0);
	bigSignSet(y, 0);
	while (!BIGNUM_IS_NULL(x))
	{
		bignum g = bigMod(y, x);
		bigRelease(y);
		y = x;
		x = g;
	}
	bigRelease(x);
	return y;
}

int bigEuclide(bignum p, bignum q, bignum* a, bignum* b, bignum* pgcd) // ap+bq=pgcd	// big alloc 10
{
	bignum a0, a1, b0, b1, x, y;
	if (BIGNUM_IS_NULL(p) || BIGNUM_IS_NULL(q))
	{
		if (a) *a = bigFromByte(0);
		if (b) *b = bigFromByte(0);
		if (pgcd) *pgcd = bigFromByte(0);
		return 0;
	}
	x = bigCopy(q);
	y = bigCopy(p);
	a0 = a1 = b0 = b1 = NULL;
	if (a)
	{
		a0 = bigFromByte(1);
		a1 = bigFromByte(0);
	}
	if (b)
	{
		b0 = bigFromByte(0);
		b1 = bigFromByte(1);
	}
	while (!BIGNUM_IS_NULL(x))
	{
		bignum z, r;
		bignum q = bigDivRemainder(y, x, &r); if (!q) return -1;

		if (a)
		{
			z = bigMul(q, a1); if (!z) return -1;
			z = bigReplace(z, bigSub(a0, z));
			bigRelease(a0);
			a0 = a1; a1 = z;
		}
		if (b)
		{
			z = bigMul(q, b1); if (!z) return -1;
			z = bigReplace(z, bigSub(b0, z));
			bigRelease(b0);
			b0 = b1; b1 = z;
		}
		bigRelease(y);
		y = x;
		x = r;
		bigRelease(q);
	}
	bigRelease(x);
	if (a) bigRelease(a1);
	if (b) bigRelease(b1);
	if (a) *a = a0;
	if (b) *b = b0;
	if (pgcd) *pgcd = y;
	else bigRelease(y);
	return 0;
}

bignum bigInv(bignum p, bignum q)	// big alloc 10
{
	bignum r;
	if ((BIGNUM_IS_NULL(q)) || (bigSign(q))) return bigFromByte(0);
	if (bigEuclide(p, q, &r, NULL, NULL)) return NULL;
	return bigReplace(r, bigMod(r, q));
}
bignum bigDivMod(bignum a, bignum b, bignum n)	// big alloc 10
{
	bignum v = bigInv(b, n); if (!v) return NULL;
	return bigReplace(v, bigMulMod(a, v, n));
}
bignum bigNegMod(bignum a, bignum n)	// big alloc 2
{
	bignum v = bigNeg(a);
	return bigReplace(v, bigMod(v, n));
}

bignum bigBarrett(bignum n)	// 2^2k / n 	// big alloc 5
{
	bignum radixk;
	LINT k;
	if ((BIGNUM_IS_NULL(n)) || (bigSign(n))) return NULL;
	k = bigNbits(n);
	radixk = bigPower2(2 * k); if (!radixk) return NULL;
	return bigReplace(radixk, bigDivRemainder(radixk, n, NULL));
}

bignum bigModBarrett(bignum x, bignum n, bignum mu)	// big alloc 3
{
	bignum r, xx;
	LINT N, sign;
	if ((BIGNUM_IS_NULL(n)) || (bigSign(n))) return NULL;
	if ((!bigSign(n)) && (!bigSign(x)) && (BIGNUM_GREATER(n, x))) return bigCopy(x);

	xx = bigCopy(x);
	sign = bigSign(x);
	if (sign) bigRegNeg(xx);

	N = bigNbits(n);
	r = bigASR(xx, N - 1);
	r = bigReplace(r, bigMul(r, mu)); if (!r) return NULL;
	r = bigReplace(r, bigASR(r, N + 1));
	r = bigReplace(r, bigMul(r, n)); if (!r) return NULL;
	r = bigReplace(r, bigSub(xx, r)); if (!r) return NULL;

	while (bigCmp(r, n) >= 0) bigRegSub(r, n);
	bigOptimize(r);
	if ((bigSign(x)) && (!BIGNUM_IS_NULL(r))) r = bigReplace(r, bigSub(n, r));
	bigRelease(xx);
	return r;
}

bignum bigMulModBarrett(bignum p, bignum q, bignum n, bignum mu)	// big alloc 6
{
	bignum pp, qq, r;
	if ((BIGNUM_IS_NULL(n)) || (bigSign(n))) return NULL;
	pp = bigMod(p, n);
	qq = bigMod(q, n);
	r = bigMul(pp, qq); if (!r) return NULL;
	r = bigReplace(r, bigModBarrett(r, n, mu));
	bigRelease(pp);
	bigRelease(qq);
	return r;
}
bignum bigSquareModBarrett(bignum p,bignum n, bignum mu)	// big alloc 6
{
	bignum pp, r;
	if ((BIGNUM_IS_NULL(n)) || (bigSign(n))) return NULL;
	pp = bigMod(p, n);
	r = bigSquare(pp); if (!r) return NULL;
	r = bigReplace(r, bigModBarrett(r, n, mu));
	bigRelease(pp);
	return r;
}
bignum bigDivModBarrett(bignum a, bignum b, bignum n, bignum mu)	// big alloc 10
{
	bignum v = bigInv(b, n); if (!v) return NULL;
	return bigReplace(v, bigMulModBarrett(a, v, n, mu));
}

bignum bigExpModBarrett(bignum p, bignum e, bignum n, bignum mu)
{
	LINT i, nbits;
	bignum th, r;

	if ((BIGNUM_IS_NULL(n)) || (bigSign(n))) return NULL;
	th = bigMod(p, n); if (!th) return NULL;
	r = bigFromByte(1); if (!r) return NULL;
	nbits = bigNbits(e);
	for (i = 0; i < nbits; i++)
	{
		// constant time:
		bignum r2 = bigMulModBarrett(r, th, n, mu); if (!r2) return NULL;
		r = bigSelectOrRelease(bigBit(e, i), r, r2);
		// faster yet not constant time:
		//if (bigBit(e, i)) {
		//	r = bigReplace(r, bigMulModBarrett(r, th, n, mu)); if (!r) return NULL;
		//}
		if (i != nbits - 1) {
			th = bigReplace(th, bigSquareModBarrett(th, n, mu)); if (!th) return NULL;
		}
	}
	bigRelease(th);
	if (bigSign(e)) r = bigReplace(r, bigInv(r, n));
	return r;
}
bignum bigExpModBarrettFast(bignum p, bignum e, bignum n, bignum mu)
{
	LINT i, nbits;
	bignum th, r;

	if ((BIGNUM_IS_NULL(n)) || (bigSign(n))) return NULL;
	th = bigMod(p, n); if (!th) return NULL;
	r = bigFromByte(1);
	nbits = bigNbits(e);
	for (i = 0; i < nbits; i++)
	{
		// faster yet not constant time:
		if (bigBit(e, i)) {
			r = bigReplace(r, bigMulModBarrett(r, th, n, mu)); if (!r) return NULL;
		}
		if (i != nbits - 1) {
			th = bigReplace(th, bigSquareModBarrett(th, n, mu)); if (!th) return NULL;
		}
	}
	bigRelease(th);
	if (bigSign(e)) r = bigReplace(r, bigInv(r, n));
	return r;
}

bignum bigMontgomery(bignum n)
{
	LINT i;
	LINT Rb = bigNbits(n);
	bignum R = bigPower2(Rb); if (!R) return NULL;
	bignum x = bigFromByte(1);
	bignum two = bigFromByte(2);
	for (i = 1; i < Rb;)
	{
		i <<= 1;
		bignum a = bigMul(n, x); if (!a) return NULL;
		a = bigReplace(a, bigSub(two, a)); if (!a) return NULL;
		a = bigReplace(a, bigMul(x, a)); if (!a) return NULL;
		x = bigReplace(x, bigModPower2(a, i));
		bigRelease(a);
	}
	bigRelease(two);
	x = bigReplace(x, bigModPower2(x, Rb));
	x->sign = 1 - x->sign;
	if (x->sign) x = bigReplace(x, bigAdd(x, R));
	bigRelease(R);
	return x;
}

bignum bigMontgomeryRedc(bignum ab, bignum n, bignum nPrime)	// where ab = (a.R) (b.R) mod n
{
	bignum m, p;
	LINT Rb = bigNbits(n);
	m = bigModPower2Mul(nPrime, ab, Rb); if (!m) return NULL;
	m = bigReplace(m, bigMul(m, n)); if (!m) return NULL;
	m = bigReplace(m, bigAdd(m, ab)); if (!m) return NULL;
	m = bigReplace(m, bigASR(m, Rb));
	p = bigSub(m, n);
	m = bigSelectOrRelease(bigSign(p), p, m);
	// returns a.b.R mod n
	return m;
}
bignum bigMontgomeryForm(bignum a, bignum n)
{
	LINT i;
	LINT Rb = bigNbits(n);
	bignum A = bigMod(a, n);
	for (i = 0; i < Rb; i++) {
		bigRegASL1(A);
		if (BIGNUM_GREATER_EQUAL(A, n)) bigRegSub(A, n);
	}
	return A;
}

bignum bigMulModMontgomery(bignum p, bignum q, bignum n, bignum nPrime)
{
	bignum r;
	if ((BIGNUM_IS_NULL(n)) || (bigSign(n))) return NULL;
	r = bigMul(p, q); if (!r) return NULL;
	r = bigReplace(r, bigMontgomeryRedc(r, n, nPrime));
	return r;
}
bignum bigSquareModMontgomery(bignum p, bignum n, bignum nPrime)
{
	bignum r;
	if ((BIGNUM_IS_NULL(n)) || (bigSign(n))) return NULL;
	r = bigSquare(p); if (!r) return NULL;
	r = bigReplace(r, bigMontgomeryRedc(r, n, nPrime));
	return r;
}
bignum bigDivModMontgomery(bignum A, bignum B, bignum n, bignum nPrime)
{
	// A=a.R, B=b.R, expected result is (a/b).R
	bignum r = bigMontgomeryRedc(B, n, nPrime); if (!r) return NULL; // r is 'b'
	r = bigReplace(r, bigMontgomeryRedc(r, n, nPrime)); if (!r) return NULL;	// r is now b.R-1
	r = bigInv(r, n); if (!r) return NULL;	// r is now (1/b).R
	r = bigReplace(r, bigMul(A, r)); if (!r) return NULL;	// r is now (a/b).R2
	return bigReplace(r, bigMontgomeryRedc(r, n, nPrime));	// r is now (a/b).R
}

bignum bigExpModMontgomery(bignum a, bignum e, bignum n, bignum nPrime)
{
	LINT i;
	LINT i0 = bigNbits(e) - 1;
	LINT Rb = bigNbits(n);
	bignum P = bigPower2(Rb); if (!P) return NULL;
	bigRegSub(P, n);

	for (i = i0; i >= 0; i--) {
		bignum P2;
		if (i != i0) {
			P = bigReplace(P, bigSquare(P)); if (!P) return NULL;
			P = bigReplace(P, bigMontgomeryRedc(P, n, nPrime)); if (!P) return NULL;
		}
		// constant time:
		P2 = bigMul(P, a); if (!P2) return NULL;
		P2 = bigReplace(P2, bigMontgomeryRedc(P2, n, nPrime)); if (!P2) return NULL;
		P = bigSelectOrRelease(bigBit(e, i), P, P2);
		// faster yet not constant time:
		//if (bigBit(e, i)) {
		//	P = bigReplace(P, bigMul(P, A)); if (!P) return NULL;
		//	P = bigReplace(P, bigMontgomeryRedc(P, n, nPrime)); if (!P) return NULL;
		//}
	}
	return P;
}
bignum bigExpModMontgomeryFast(bignum a, bignum e, bignum n, bignum nPrime)
{
	LINT i;
	LINT i0 = bigNbits(e) - 1;
	LINT Rb = bigNbits(n);
	bignum P = bigPower2(Rb); if (!P) return NULL;
	bigRegSub(P, n);

	for (i = i0; i >= 0; i--) {
		if (i != i0) {
			P = bigReplace(P, bigSquare(P)); if (!P) return NULL;
			P = bigReplace(P, bigMontgomeryRedc(P, n, nPrime)); if (!P) return NULL;
		}
		// faster yet not constant time:
		if (bigBit(e, i)) {
			P = bigReplace(P, bigMul(P, a)); if (!P) return NULL;
			P = bigReplace(P, bigMontgomeryRedc(P, n, nPrime)); if (!P) return NULL;
		}
	}
	return P;
}

bignum bigExpMod(bignum p, bignum e, bignum n)
{
	bignum k, r;
	if (bigSign(n) || BIGNUM_IS_NULL(p)) return bigFromByte(0);
	if (BIGNUM_IS_NULL(n)) return bigFromByte(0);
	if (BIGNUM_IS_NULL(e)) return bigFromByte(1);

	k = bigMontgomery(n); if (!k) return NULL;
	p = bigMontgomeryForm(p, n); if (!p) return NULL;
	r = bigExpModMontgomery(p, e, n, k); if (!r) return NULL;
	r = bigReplace(r, bigMontgomeryRedc(r, n, k)); if (!r) return NULL;
	//k = bigBarrett(n); if (!k) return NULL;
	//r = bigExpModBarrett(p, e, n, k);
	bigRelease(k);
	bigRelease(p);
	return r;
}
bignum bigExpChinese5(bignum c, bignum p1, bignum p2, bignum e1, bignum e2, bignum coef)
{
	bignum a, b;
	a = bigExpMod(c, e1, p1); if (!a) return NULL;
	b = bigExpMod(c, e2, p2); if (!b) return NULL;
	a = bigReplace(a, bigSub(a, b)); if (!a) return NULL;
	a = bigReplace(a, bigMod(a, p1));
	a = bigReplace(a, bigMulMod(coef, a, p1)); if (!a) return NULL;
	a = bigReplace(a, bigMul(a, p2)); if (!a) return NULL;
	a = bigReplace(a, bigAdd(b, a));
	bigRelease(b);
	return a;
}

void bigReset(void)
{
	int i;
	if (BigCount >= BIGREGISTERS) return;
	BigList = NULL;
	for (i = 0; i < BIGREGISTERS; i++)
	{
		BigRegisters[i].header.nextBlock = (LB*)BigList;
		BigList = (bignum)&BigRegisters[i];
	}
	BigCount = BIGREGISTERS;
}

void bigInit(void)
{
	BigCount = 0;
	bigReset();
}

