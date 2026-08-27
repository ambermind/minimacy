// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"
#include"util_bignum.h"

#ifdef USE_MSVC_X86_64
#include <stdint.h>
bignum bigMul(bignum a, bignum b)	// big alloc 1
{
	LINT i, j, lenA, lenB;
	uint64_t* srcA;
	uint64_t* srcB;
	bignum r;
	if (!a || !b) return NULL;
	lenA = bigLen(a);
	lenB = bigLen(b);

	// we add 1 to the size to prevent memory overflow on the last dst[j]+=cc
	r = bigRegister(1 + lenA + lenB); if (!r) return NULL;
	srcA = (uint64_t*)a->data;
	srcB = (uint64_t*)b->data;
	for (i = 0; i < lenA; i++)
	{
		uint64_t cc = 0;
		uint64_t x = srcA[i];
		uint64_t* dst = ((uint64_t*)r->data) + i;
		for (j = 0; j < lenB; j++)
		{
			uint64_t hi;
			uint64_t lo = _umul128(x, srcB[j], &hi);
			hi += _addcarry_u64(0, lo, dst[j], &lo);
			hi += _addcarry_u64(0, lo, cc, &lo);
			dst[j] = lo;
			cc = hi;
		}
		dst[j] += cc;
	}
	bigSignSet(r, bigSign(a) ^ bigSign(b));
	bigOptimize(r);
	return r;
}
bignum bigModPower2Mul(bignum a, bignum b, LINT nbits)	// big alloc 1
{
	LINT i, j, lenA, lenB;
	uint64_t* srcA;
	uint64_t* srcB;
	uint64_t* dst0;
	uint64_t* dst = NULL;
	bignum r;
	LINT nWords;
	if (!a || !b || nbits <= 0) return NULL;
	nWords = (nbits + 63) >> 6;
	// we add 1 to the size to prevent memory overflow on the last dst[j]+=cc
	r = bigRegister(1 + nWords); if (!r) return NULL;

	lenA = bigLen(a);
	lenB = bigLen(b);

	srcA = (uint64_t*)a->data;
	srcB = (uint64_t*)b->data;
	dst0 = (uint64_t*)r->data;
	for (i = 0; i < lenA; i++)
	{
		uint64_t cc = 0;
		uint64_t x = srcA[i];
		LINT jmax = lenB;
		dst = ((uint64_t*)r->data) + i;
		if (i + jmax > nWords) jmax = nWords - i;
		for (j = 0; j < jmax; j++)
		{
			uint64_t hi;
			uint64_t lo = _umul128(x, srcB[j], &hi);
			hi += _addcarry_u64(0, lo, dst[j], &lo);
			hi += _addcarry_u64(0, lo, cc, &lo);
			dst[j] = lo;
			cc = hi;
		}
		dst[j] += cc;
	}
	if (dst) dst[j] = 0;
	dst0[nWords - 1] &= BIG_WORD_LAST_MASK(nbits);
	bigSignSet(r, bigSign(a) ^ bigSign(b));
	bigOptimize(r);
	return r;
}
bignum bigSquare(bignum a)	// big alloc 2
{
	LINT i, j, lenA;
	bignum r1, r2;
	uint64_t* srcA;
	uint64_t* dst0;
	if (!a) return NULL;
	lenA = bigLen(a);

	// we add 1 to the size to prevent memory overflow on the last dst[j]+=cc
	r1 = bigRegister(1 + lenA + lenA); if (!r1) return NULL;
	r2 = bigRegister(1 + lenA + lenA); if (!r2) return NULL;
	srcA = (uint64_t*)a->data;
	dst0 = (uint64_t*)r1->data;
	for (i = 0; i < lenA; i++) {
		uint64_t hi;
		uint64_t x = srcA[i];
		uint64_t lo = _umul128(x, x, &hi);
		*(dst0++) = lo;
		*(dst0++) = hi;
	}
	dst0 = (uint64_t*)r2->data;
	for (i = 0; i < lenA; i++)
	{
		uint64_t cc = 0;
		uint64_t x = srcA[i];
		uint64_t* dst = dst0 + i;
		for (j = i + 1; j < lenA; j++)
		{
			uint64_t hi;
			uint64_t lo = _umul128(x, srcA[j], &hi);
			hi += _addcarry_u64(0, lo, dst[j], &lo);
			hi += _addcarry_u64(0, lo, cc, &lo);
			dst[j] = lo;
			cc = hi;
		}
		dst[j] += cc;
	}
	bigRegASL1(r2);
	r1 = bigReplace(r1, bigAdd(r1, r2)); if (!r1) return NULL;
	bigRelease(r2);
	bigOptimize(r1);
	return r1;
}


#else
#ifdef USE_GCC_64
bignum bigMul(bignum a, bignum b)
{
	LINT i, j, lenA, lenB;
	uint64_t* srcA;
	uint64_t* srcB;
	bignum r;
	if (!a || !b) return NULL;
	lenA = bigLen(a);
	lenB = bigLen(b);

	// we add 1 to the size to prevent memory overflow on the last dst[j]+=cc
	r = bigRegister(1 + lenA + lenB); if (!r) return NULL;
	srcA = (uint64_t*)a->data;
	srcB = (uint64_t*)b->data;
	for (i = 0; i < lenA; i++)
	{
		unsigned __int128 cc = 0;
		uint64_t x = srcA[i];
		uint64_t* dst = ((uint64_t*)r->data) + i;
		for (j = 0; j < lenB; j++)
		{
			cc += (unsigned __int128)x * srcB[j] + dst[j];
			dst[j] = (uint64_t)cc;
			cc >>= 64;
		}
		dst[j] = (uint64_t)cc;
	}
	bigSignSet(r, bigSign(a) ^ bigSign(b));
	bigOptimize(r);
	return r;
}
bignum bigModPower2Mul(bignum a, bignum b, LINT nbits)
{
	LINT i, j, lenA, lenB;
	uint64_t* srcA;
	uint64_t* srcB;
	uint64_t* dst0;
	uint64_t* dst = NULL;
	bignum r;
	LINT nWords;
	if (!a || !b || nbits <= 0) return NULL;
	nWords = (nbits + 63) >> 6;
	// we add 1 to the size to prevent memory overflow on the last dst[j]+=cc
	r = bigRegister(1 + nWords); if (!r) return NULL;

	lenA = bigLen(a);
	lenB = bigLen(b);

	srcA = (uint64_t*)a->data;
	srcB = (uint64_t*)b->data;
	dst0 = (uint64_t*)r->data;
    j=0;    // prevent warning
	for (i = 0; i < lenA; i++)
	{
		unsigned __int128 cc = 0;
		uint64_t x = srcA[i];
		dst = ((uint64_t*)r->data) + i;
		LINT jmax = lenB;
		if (i + jmax > nWords) jmax = nWords - i;
		for (j = 0; j < jmax; j++)
		{
			cc += (unsigned __int128)x * srcB[j] + dst[j];
			dst[j] = (uint64_t)cc;
			cc >>= 64;
		}
		dst[j] += (uint64_t)cc;
	}
	if (dst) dst[j] = 0;
	dst0[nWords - 1] &= BIG_WORD_LAST_MASK(nbits);
	bigSignSet(r, bigSign(a) ^ bigSign(b));
	bigOptimize(r);
	return r;
}
bignum bigSquare(bignum a)
{
	LINT i, j, lenA;
	bignum r1, r2;
	uint64_t* srcA;
	uint64_t* dst0;
	if (!a) return NULL;
	lenA = bigLen(a);
	// we add 1 to the size to prevent memory overflow on the last dst[j]+=cc
	r1 = bigRegister(1 + lenA + lenA); if (!r1) return NULL;
	r2 = bigRegister(1 + lenA + lenA); if (!r2) return NULL;
	srcA = (uint64_t*)a->data;
	dst0 = (uint64_t*)r1->data;
	for (i = 0; i < lenA; i++) {
		uint64_t x = srcA[i];
		unsigned __int128 p = (unsigned __int128)x * x;
		*((unsigned __int128*)dst0) = p;
		dst0 += 2;
	}
	dst0 = (uint64_t*)r2->data;
	for (i = 0; i < lenA; i++)
	{
		unsigned __int128 cc = 0;
		uint64_t x = srcA[i];
		uint64_t* dst = dst0 + i;
		for (j = i + 1; j < lenA; j++)
		{
			cc += (unsigned __int128)x * srcA[j];
			cc += dst[j];
			dst[j] = (uint64_t)cc;
			cc >>= 64;
		}
		dst[j] += (uint64_t)cc;
	}
	bigRegASL1(r2);
	r1 = bigReplace(r1, bigAdd(r1, r2)); if (!r1) return NULL;
	bigRelease(r2);
	bigOptimize(r1);
	return r1;
}

#else
#ifdef USE_ARM32
bignum bigMul(bignum a, bignum b)
{
	LINT i, j, lenA, lenB;
	bigword* srcA;
	bignum r;
	if (!a || !b) return NULL;
	lenA = bigLen(a);
	lenB = bigLen(b);
	// we add 1 to the size to prevent memory overflow on the last dst[j]+=cc
	r = bigRegister(1 + lenA + lenB); if (!r) return NULL;
	srcA = a->data;
	for (i = 0; i < lenA; i++)
	{
		bigword cc = 0;
		bigword x = srcA[i];
		bigword* srcB = b->data;
		bigword* dst = r->data + i;
		for (j = 0; j < lenB; j++)
		{
			uint32_t lo = dst[j];
			__asm__("umaal %0, %1, %2, %3"
				: "+r"(lo), "+r"(cc)
				: "r"(x), "r"(srcB[j]));
			dst[j] = lo;
		}
		dst[j] += cc;
	}
	bigSignSet(r, bigSign(a) ^ bigSign(b));
	bigOptimize(r);
	return r;
}

bignum bigModPower2Mul(bignum a, bignum b, LINT nbits)
{
	LINT i, j, lenA, lenB;
	bigword* srcA;
	bigword* dst0;
	bigword* dst = NULL;
	bignum r;
	LINT nWords;
	if (!a || !b || nbits <= 0) return NULL;
	lenA = bigLen(a);
	lenB = bigLen(b);
	nWords = BIG_WORDS_FROM_BITS(nbits);
	// we add 1 to the size to prevent memory overflow on the last dst[j]+=cc
	r = bigRegister(1 + nWords); if (!r) return NULL;
	srcA = a->data;
	dst0 = r->data;
	for (i = 0; i < lenA; i++)
	{
		bigword cc = 0;
		bigword x = srcA[i];
		bigword* srcB = b->data;
		LINT jmax = bigLen(b);
		dst = r->data + i;
		if (i + jmax > nWords) jmax = nWords - i;
		for (j = 0; j < jmax; j++)
		{
			uint32_t lo = dst[j];
			__asm__("umaal %0, %1, %2, %3"
				: "+r"(lo), "+r"(cc)
				: "r"(x), "r"(srcB[j]));
			dst[j] = lo;
		}
		dst[j] += cc;
	}
	if (dst) dst[j] = 0;
	dst0[nWords - 1] &= BIG_WORD_LAST_MASK(nbits);
	bigSignSet(r, bigSign(a) ^ bigSign(b));
	bigOptimize(r);
	return r;
}

bignum bigSquare(bignum a)
{
	LINT i, j, lenA;
	bignum r1, r2;
	bigword* srcA;
	bigword* dst0;
	if (!a) return NULL;
	lenA = bigLen(a);
	// we add 1 to the size to prevent memory overflow on the last dst[j]+=cc
	r1 = bigRegister(1 + 2 * lenA); if (!r1) return NULL;
	r2 = bigRegister(1 + 2 * lenA); if (!r2) return NULL;
	srcA = a->data;
	dst0 = r1->data;
	for (i = 0; i < lenA; i++) {
		bigword x = srcA[i];
		bigword lo, hi;
		__asm__("umull %0, %1, %2, %3"
			: "=r"(lo), "=r"(hi)
			: "r"(x), "r"(x));
		dst0[i << 1] = lo;
		dst0[(i << 1) + 1] = hi;
	}

	dst0 = r2->data;
	for (i = 0; i < lenA; i++)
	{
		bigword cc = 0;
		bigword x = srcA[i];
		bigword* dst = dst0 + i;
		for (j = i + 1; j < lenA; j++)
		{
			uint32_t lo = dst[j];
			__asm__("umaal %0, %1, %2, %3"
				: "+r"(lo), "+r"(cc)
				: "r"(x), "r"(srcA[j]));
			dst[j] = lo;
		}
		dst[j] += cc;
	}
	bigRegASL1(r2);
	r1 = bigReplace(r1, bigAdd(r1, r2)); if (!r1) return NULL;
	bigRelease(r2);
	bigOptimize(r1);
	return r1;
}
#else
bignum bigMul(bignum a, bignum b)
{
	LINT i, j, lenA, lenB;
	big32* srcA;
	bignum r;
	if (!a || !b) return NULL;
	lenA = bigLen32(a);
	lenB = bigLen32(b);
	// we add 1 to the size to prevent memory overflow on the last dst[j]+=cc
	r = bigRegister32(1 + lenA + lenB); if (!r) return NULL;
	srcA = bigStart32(a);
	for (i = 0; i < lenA; i++)
	{
		big64 cc = 0;
		big64 x = srcA[i];
		big32* srcB = bigStart32(b);
		big32* dst = bigStart32(r) + i;
		for (j = 0; j < lenB; j++)
		{
			cc += (big64)x * srcB[j];
			cc += dst[j];
			dst[j] = (big32)cc;
			cc >>= 32;
		}
		dst[j] += (big32)cc;
	}
	bigSignSet(r, bigSign(a) ^ bigSign(b));
	bigOptimize(r);
	return r;
}
bignum bigModPower2Mul(bignum a, bignum b, LINT nbits)
{
	LINT i, j, lenA, lenB;
	big32* srcA;
	big32* dst0;
	big32* dst = NULL;
	bignum r;
	LINT nWords;
	if (!a || !b || nbits <= 0) return NULL;
	lenA = bigLen32(a);
	lenB = bigLen32(b);
	nWords = (nbits+31)>>5;
	// we add 1 to the size to prevent memory overflow on the last dst[j]+=cc
	r = bigRegister32(1 + nWords); if (!r) return NULL;
	srcA = bigStart32(a);
	dst0 = bigStart32(r);
	for (i = 0; i < lenA; i++)
	{
		big64 cc = 0;
		big64 x = srcA[i];
		big32* srcB = bigStart32(b);
		LINT jmax = lenB;
		dst = bigStart32(r) + i;
		if (i + jmax > nWords) jmax = nWords - i;
		for (j = 0; j < jmax; j++)
		{
			cc += (big64)x * srcB[j];
			cc += dst[j];
			dst[j] = (big32)cc;
			cc >>= 32;
		}
		dst[j] += (big32)cc;
	}
	if (dst) dst[j] = 0;
	dst0[nWords - 1] &= (2<<((nbits-1)&31))-1;

	bigSignSet(r, bigSign(a) ^ bigSign(b));
	bigOptimize(r);
	return r;
}
bignum bigSquare(bignum a)
{
	LINT i, j, lenA;
	bignum r1, r2;
	big32* srcA;
	big32* dst0;
	if (!a) return NULL;
	lenA = bigLen32(a);
	// we add 1 to the size to prevent memory overflow on the last dst[j]+=cc
	r1 = bigRegister32(1 + 2 * lenA); if (!r1) return NULL;
	r2 = bigRegister32(1 + 2 * lenA); if (!r2) return NULL;
	srcA = bigStart32(a);
	dst0 = bigStart32(r1);
	for (i = 0; i < lenA; i++) {
		big64 x = srcA[i];
		x *= x;
		dst0[i << 1] = (big32)x;
		x >>= 32;
		dst0[(i << 1) + 1] = (big32)x;
	}

	dst0 = bigStart32(r2);
	for (i = 0; i < lenA; i++)
	{
		big64 cc = 0;
		big64 x = srcA[i];
		big32* dst = dst0 + i;
		for (j = i + 1; j < lenA; j++)
		{
			big64 y = srcA[j];
			cc += x * y;
			cc += dst[j];
			dst[j] = (big32)cc;
			cc >>= 32;
		}
		dst[j] += (big32)cc;
	}
	bigRegASL1(r2);
	r1 = bigReplace(r1, bigAdd(r1, r2)); if (!r1) return NULL;
	bigRelease(r2);
	bigOptimize(r1);
	return r1;
}
#endif
#endif
#endif

#ifdef USE_MSVC_X86_64
void bigRegSub(bignum a, bignum b)
{
	LINT i, lenA, lenB;
	unsigned char cc = 0;
	big64* srcA = a->data;
	big64* srcB = b->data;
	big64 zero = 0;
	lenA = bigLen(a);
	lenB = bigLen(b);
	for (i = 0; i < lenB; i++) cc = _subborrow_u64(cc, srcA[i], srcB[i], &srcA[i]);
	for (; cc && i < lenA; i++) cc = _subborrow_u64(cc, srcA[i], zero, &srcA[i]);

	bigOptimize(a);
}

bignum bigAddSub(bignum a, bignum b, LINT sub)
{
	LINT i, signR, lenA, lenB;
	unsigned char cc = 0;
	bignum r, z;
	big64* srcA, * srcB, * dst;
	big64 zero = 0;

	if (bigGabs(a, b, 1)) signR = bigSign(a);
	else
	{
		z = a; a = b; b = z;
		signR = bigSign(a) ^ sub;
	}

	lenA = bigLen(a);
	lenB = bigLen(b);
	srcA = a->data;
	srcB = b->data;

	if (!(bigSign(a) ^ bigSign(b) ^ sub))
	{
		r = bigRegister(1 + lenA); if (!r) return NULL;
		dst = r->data;
		for (i = 0; i < lenB; i++) cc = _addcarry_u64(cc, srcA[i], srcB[i], &dst[i]);
		for (; i < lenA; i++) cc = _addcarry_u64(cc, srcA[i], zero, &dst[i]);
		dst[lenA] = (big64)cc;
	}
	else
	{
		r = bigRegister(lenA); if (!r) return NULL;
		dst = r->data;
		for (i = 0; i < lenB; i++) cc = _subborrow_u64(cc, srcA[i], srcB[i], &dst[i]);
		for (; i < lenA; i++) cc = _subborrow_u64(cc, srcA[i], zero, &dst[i]);
	}
	bigSignSet(r, signR);
	bigOptimize(r);
	return r;
}

#else
void bigRegSub(bignum a, bignum b)
{
	LINT i, lenA, lenB;
	big64 cc = 0;
	big32* srcA = bigStart32(a);
	big32* srcB = bigStart32(b);
	lenA = bigLen32(a);
	lenB = bigLen32(b);
	for (i = 0; i < lenB; i++)
	{
		cc = (big64)srcA[i] - srcB[i] - cc;
		srcA[i] = (big32)cc;
		cc = (cc >> 32) & 1;
	}
	for (; cc && i < lenA; i++)
	{
		cc = (big64)srcA[i] - cc;
		srcA[i] = (big32)cc;
		cc = (cc >> 32) & 1;
	}
	bigOptimize(a);
}

bignum bigAddSub(bignum a, bignum b, LINT sub)
{
	LINT i, signR, lenA, lenB;
	big64 cc = 0;
	bignum r, z;
	big32 *srcA, *srcB, *dst;

	if (bigGabs(a, b, 1)) signR = bigSign(a);
	else
	{
		z = a; a = b; b = z;
		signR = bigSign(a) ^ sub;
	}

	lenA= bigLen32(a);
	lenB= bigLen32(b);
	srcA=bigStart32(a);
	srcB=bigStart32(b);

	if (!(bigSign(a) ^ bigSign(b) ^ sub))
	{
		r = bigRegister32(1 + lenA); if (!r) return NULL;
		dst = bigStart32(r);
		for (i = 0; i < lenB; i++)
		{
			big64 x = srcA[i];
			big64 y = srcB[i];
			cc += x + y;
			dst[i]=(big32)cc;
			cc >>= 32;
		}
		for (; i < lenA; i++)
		{
			big64 x = srcA[i];
			cc += x;
			dst[i]=(big32)cc;
			cc >>= 32;
		}
		dst[lenA]=(big32)cc;
	}
	else
	{
		r = bigRegister32(lenA); if (!r) return NULL;
		dst = bigStart32(r);
		for (i = 0; i < lenB; i++)
		{
			cc = (big64)srcA[i] - srcB[i] - cc;
			dst[i] = (big32)cc;
			cc = (cc >> 32) & 1;
		}
		for (; i < lenA; i++)
		{
			cc = (big64)srcA[i] - cc;
			dst[i] = (big32)cc;
			cc = (cc >> 32) & 1;
		}
	}
	bigSignSet(r, signR);
	bigOptimize(r);
	return r;
}
#endif
