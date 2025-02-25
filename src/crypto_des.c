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
#include"crypto_des.h"

// https://en.wikipedia.org/wiki/Data_Encryption_Standard

static const char PC1[56] = {
63, 55, 47, 39, 31, 23, 15,  7,
62, 54, 46, 38, 30, 22, 14,  6,
61, 53, 45, 37, 29, 21, 13,  5,
60, 52, 44, 36,
57, 49, 41, 33, 25, 17,  9,  1,
58, 50, 42, 34, 26, 18, 10,  2,
59, 51, 43, 35, 27, 19, 11,  3,
28, 20, 12,  4
};

static const char PC2[48] = {
42, 39, 45, 32, 55, 51,
53, 28, 41, 50, 35, 46,
33, 37, 44, 52, 30, 48,
40, 49, 29, 36, 43, 54,
15,  4, 25, 19,  9,  1,
26, 16,  5, 11, 23,  8,
12,  7, 17,  0, 22,  3,
10, 14,  6, 20, 27, 24
};

static const char KEY_shift[] = {
1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1
};

static const char IP[] = {
62, 54, 46, 38, 30, 22, 14,  6,
60, 52, 44, 36, 28, 20, 12,  4,
58, 50, 42, 34, 26, 18, 10,  2,
56, 48, 40, 32, 24, 16,  8,  0,
63, 55, 47, 39, 31, 23, 15,  7,
61, 53, 45, 37, 29, 21, 13,  5,
59, 51, 43, 35, 27, 19, 11,  3,
57, 49, 41, 33, 25, 17,  9,  1
};

static const char FP[] = {
31, 63, 23, 55, 15, 47, 7, 39, 30, 62, 22, 54, 14, 46, 6, 38,
29, 61, 21, 53, 13, 45, 5, 37, 28, 60, 20, 52, 12, 44, 4, 36,
27, 59, 19, 51, 11, 43, 3, 35, 26, 58, 18, 50, 10, 42, 2, 34,
25, 57, 17, 49, 9, 41, 1, 33, 24, 56, 16, 48, 8, 40, 0, 32,
};

static const char E[] = {
 0, 31, 30, 29, 28, 27,
28, 27, 26, 25, 24, 23,
24, 23, 22, 21, 20, 19,
20, 19, 18, 17, 16, 15,
16, 15, 14, 13, 12, 11,
12, 11, 10,  9,  8,  7,
 8,  7,  6,  5,  4,  3,
 4,  3,  2,  1,  0, 31};

static const char P[] = {
16, 25, 12, 11,
 3, 20,  4, 15,
31, 17,  9,  6,
27, 14,  1, 22,
30, 24,  8, 18,
 0,  5, 29, 23,
13, 19,  2, 26,
10, 21, 28,  7 
};

static const char S[8][64] = {
{ // S8
13,1,2,15,8,13,4,8,6,10,15,3,11,7,1,4,10,12,9,5,3,6,14,11,5,0,0,14,12,9,7,2,7,2,11,1,4,14,1,7,9,4,12,10,14,8,2,13,0,15,6,12,10,9,13,0,15,3,3,5,5,6,8,11
},{ // S7
4,13,11,0,2,11,14,7,15,4,0,9,8,1,13,10,3,14,12,3,9,5,7,12,5,2,10,15,6,8,1,6,1,6,4,11,11,13,13,8,12,1,3,4,7,10,14,7,10,9,15,5,6,0,8,15,0,14,5,2,9,3,2,12
},{ // S6
12,10,1,15,10,4,15,2,9,7,2,12,6,9,8,5,0,6,13,1,3,13,4,14,14,0,7,11,5,3,11,8,9,4,14,3,15,2,5,12,2,9,8,5,12,15,3,10,7,11,0,14,4,1,10,7,1,6,13,0,11,8,6,13
},{ // S5
2,14,12,11,4,2,1,12,7,4,10,7,11,13,6,1,8,5,5,0,3,15,15,10,13,3,0,9,14,8,9,6,4,11,2,8,1,12,11,7,10,1,13,14,7,2,8,13,15,6,9,15,12,0,5,9,6,10,3,4,0,5,14,3
},{ // S4
7,13,13,8,14,11,3,5,0,6,6,15,9,0,10,3,1,4,2,7,8,2,5,12,11,1,12,10,4,14,15,9,10,3,6,15,9,0,0,6,12,10,11,1,7,13,13,8,15,9,1,4,3,5,14,11,5,12,2,7,8,2,4,14
},{ // S3
10,13,0,7,9,0,14,9,6,3,3,4,15,6,5,10,1,2,13,8,12,5,7,14,11,12,4,11,2,15,8,1,13,1,6,10,4,13,9,0,8,6,15,9,3,8,0,7,11,4,1,15,2,14,12,3,5,11,10,5,14,2,7,12
},{ // S2
15,3,1,13,8,4,14,7,6,15,11,2,3,8,4,14,9,12,7,0,2,1,13,10,12,6,0,9,5,11,10,5,0,13,14,8,7,10,11,1,10,3,4,15,13,4,1,2,5,11,8,6,12,7,6,12,9,0,3,5,2,14,15,9
},{ // S1
14,0,4,15,13,7,1,4,2,14,15,2,11,13,8,1,3,10,10,6,6,12,12,11,5,9,9,5,0,3,7,8,4,15,1,12,14,8,8,2,13,4,6,9,2,1,11,7,15,5,12,11,9,3,7,14,3,10,10,0,5,6,0,13
}
};

void DESCreate(DesCtx* ctx, char* key)
{
	int i,j;
	long long CD = 0;
	long long keyLong;
	memcpy(&keyLong, key, 8);
	keyLong = LSB64(keyLong);

	for (i = 0; i < 56; i++) CD = (CD<<1) | ((keyLong >> PC1[i]) & 1);

	for (i = 0; i < 16; i++) {
		for (j = 0; j < KEY_shift[i]; j++) CD = (0x00ffffffeffffffe & (CD << 1)) | (0x0000000010000001 & (CD >> 27));
		ctx->sub_key[i] = 0;
		for (j = 0; j < 48; j++) ctx->sub_key[i] = (ctx->sub_key[i] << 1) | ((CD >> PC2[j]) & 1);
	}
}

void DESProcess(DesCtx* ctx, char* data, int enc) { // block is always 8 bytes
	int i, j;
	unsigned int L32, R32, F32, S32;
	unsigned long long reg48, reg64;
	long long inputLong;
	memcpy(&inputLong, data, 8);
	inputLong = LSB64(inputLong);

	// initial permutation
	reg64 = 0;
	for (i = 0; i < 64; i++) reg64 = (reg64 << 1) | ((inputLong >> IP[i]) & 1);
	 
	// split i
	R32 = (unsigned int)reg64; reg64 >>= 32;
	L32 = (unsigned int)reg64;
	for (i = 0; i < 16; i++) {
		// Feistel expansion
		reg48 = 0;
		for (j = 0; j < 48; j++) reg48 = (reg48 << 1) | ((R32 >>E[j]) & 1);
		// Key merging
		reg48 = reg48 ^ ctx->sub_key[enc?i:(15 - i)];
		// Substitution
		S32 = 0;
		for (j = 0; j < 8; j++) {
			int index = reg48 & 0x3f;
			S32 |= (S[j][index]) << (j << 2);
			reg48 >>= 6;
		}
		// Permutation
		F32 = 0;
		for (j = 0; j < 32; j++) F32 = (F32 << 1) | ((S32 >> P[j]) & 1);
		//Swapping
		F32 ^= L32;
		L32 = R32;
		R32 = F32;
	}
	reg64 = (((long long)R32) << 32) | L32;

	// final permutation
	ctx->result = 0;
	for (i = 0; i < 64; i++) ctx->result = (ctx->result << 1) | ((reg64 >> FP[i]) & 1);
}

void DESOutput(DesCtx* ctx, char* output)
{
	ctx->result = LSB64(ctx->result);
	memcpy(output, &ctx->result, 8);
}
