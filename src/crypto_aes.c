// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"
#include "crypto_aes.h"

// https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.197.pdf

#define Nb 4

static const uint8_t sbox[256] = {
0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

static const uint8_t rsbox[256] = {
0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

static const uint8_t Rcon[11] = {
0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

static const uint8_t Xtime[256] = {
0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30,
32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62,
64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94,
96, 98, 100, 102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126,
128, 130, 132, 134, 136, 138, 140, 142, 144, 146, 148, 150, 152, 154, 156, 158,
160, 162, 164, 166, 168, 170, 172, 174, 176, 178, 180, 182, 184, 186, 188, 190,
192, 194, 196, 198, 200, 202, 204, 206, 208, 210, 212, 214, 216, 218, 220, 222,
224, 226, 228, 230, 232, 234, 236, 238, 240, 242, 244, 246, 248, 250, 252, 254,
27, 25, 31, 29, 19, 17, 23, 21, 11, 9, 15, 13, 3, 1, 7, 5,
59, 57, 63, 61, 51, 49, 55, 53, 43, 41, 47, 45, 35, 33, 39, 37,
91, 89, 95, 93, 83, 81, 87, 85, 75, 73, 79, 77, 67, 65, 71, 69,
123, 121, 127, 125, 115, 113, 119, 117, 107, 105, 111, 109, 99, 97, 103, 101,
155, 153, 159, 157, 147, 145, 151, 149, 139, 137, 143, 141, 131, 129, 135, 133,
187, 185, 191, 189, 179, 177, 183, 181, 171, 169, 175, 173, 163, 161, 167, 165,
219, 217, 223, 221, 211, 209, 215, 213, 203, 201, 207, 205, 195, 193, 199, 197,
251, 249, 255, 253, 243, 241, 247, 245, 235, 233, 239, 237, 227, 225, 231, 229,
};

// SHR(b,x) assumes x is unsigned
#define SHL(b,x)    ((x) << (b))
#define SHR(b,x)    ((x) >> (b))
#define ROTL32(b,x) (SHL(b,x) | SHR(32-(b), x))
#define ROTR32(b,x) (SHR(b,x) | SHL(32-(b), x))

#define _SUBWORD(box,v) ((box[(v>>24)&255]<<24)|(box[(v>>16)&255]<<16)|(box[(v>>8)&255]<<8)|(box[v&255]))
#define SUBWORD(v)    _SUBWORD(sbox,  (v))
#define SUBWORDINV(v) _SUBWORD(rsbox, (v))

void AESCreate(AesCtx* ctx, char* key, int len)
{
	unsigned int i, j, rcIndex, T;
	unsigned int Nk = len / 4;
	unsigned int Nr = Nk+6;
	unsigned int* RK=ctx->RK;
	unsigned int iEndLoop = Nb * (Nr + 1);
	ctx->Nr = Nr;

	memcpy(RK, key, 4 * Nk);
	for (i = 0; i < Nk; i++) RK[i] = LSBL(RK[i]);

	T = RK[Nk - 1];
	for (i = Nk, j=0, rcIndex=1; i < iEndLoop; i++, j++)
	{
		if (j >= Nk) { j = 0; rcIndex++; }
		if (j == 0)
		{
			T = ROTR32(8, T);
			T = SUBWORD(T) ^ Rcon[rcIndex];
		}
		if ((Nk == 8) && (j == 4))	T = SUBWORD(T); // AES 256 only
		T ^= RK[i - Nk];
		RK[i] = T;
	}
	memset(ctx->W, 0, AES_BLOCKLEN);
}

#define AddRoundKey(round) { for (i = 0; i < 4; i++) ctx->W[i] ^= ctx->RK[round * 4 + i];}

#define SubBytes() { for (i = 0; i < 4; i++) ctx->W[i] = SUBWORD(ctx->W[i]);}
#define InvSubBytes() { for (i = 0; i < 4; i++) ctx->W[i] = SUBWORDINV(ctx->W[i]); }

static void ShiftRows(AesCtx* ctx)
{
	unsigned int W0, W1, W2, W3;
	W0 = ctx->W[0]; W1 = ctx->W[1]; W2 = ctx->W[2]; W3 = ctx->W[3];

	ctx->W[0] = (W0 & 0xff) | (W1 & 0xff00) | (W2 & 0xff0000) | (W3 & 0xff000000);
	ctx->W[1] = (W1 & 0xff) | (W2 & 0xff00) | (W3 & 0xff0000) | (W0 & 0xff000000);
	ctx->W[2] = (W2 & 0xff) | (W3 & 0xff00) | (W0 & 0xff0000) | (W1 & 0xff000000);
	ctx->W[3] = (W3 & 0xff) | (W0 & 0xff00) | (W1 & 0xff0000) | (W2 & 0xff000000);
}
static void InvShiftRows(AesCtx* ctx)
{
	unsigned int W0, W1, W2, W3;
	W0 = ctx->W[0]; W1 = ctx->W[1]; W2 = ctx->W[2]; W3 = ctx->W[3];

	ctx->W[0] = (W0 & 0xff) | (W3 & 0xff00) | (W2 & 0xff0000) | (W1 & 0xff000000);
	ctx->W[1] = (W1 & 0xff) | (W0 & 0xff00) | (W3 & 0xff0000) | (W2 & 0xff000000);
	ctx->W[2] = (W2 & 0xff) | (W1 & 0xff00) | (W0 & 0xff0000) | (W3 & 0xff000000);
	ctx->W[3] = (W3 & 0xff) | (W2 & 0xff00) | (W1 & 0xff0000) | (W0 & 0xff000000);
}


#define xtime(x) Xtime[x]

static void MixColumns(AesCtx* ctx)
{
	int i;
	for (i = 0; i < 4; i++)
	{
		unsigned int W = ctx->W[i];
		unsigned int X = 0x01010101 * (255 & ((W >> 24) ^ (W >> 16) ^ (W >> 8) ^ (W)));
		W ^= ROTR32(8, W);
		W = (xtime((W >> 24) & 255) << 24) | (xtime((W >> 16) & 255) << 16) | (xtime((W >> 8) & 255) << 8) | xtime((W) & 255);
		ctx->W[i] ^= W ^ X;
	}
}

// Multiply is used to multiply numbers in the field GF(2^8)
#define Multiply(x, y) \
	(((y & 1) * x) ^ \
	((y>>1 & 1) * xtime(x)) ^ \
	((y>>2 & 1) * xtime(xtime(x))) ^ \
	((y>>3 & 1) * xtime(xtime(xtime(x)))) ^ \
	((y>>4 & 1) * xtime(xtime(xtime(xtime(x))))))

#define MultiplyRow(a, b, c, d) (Multiply(a, 0x0e) ^ Multiply(b, 0x0b) ^ Multiply(c, 0x0d) ^ Multiply(d, 0x09))

static void InvMixColumns(AesCtx* ctx)
{
	int i;
	uint8_t a, b, c, d;
	for (i = 0; i < 4; i++)
	{
		unsigned int W = ctx->W[i];
		a = W; b = W >> 8; c = W >> 16; d = W >> 24;
		ctx->W[i] = (MultiplyRow(a, b, c, d)) | (MultiplyRow(b, c, d, a) << 8) | (MultiplyRow(c, d, a, b) << 16) | (MultiplyRow(d, a, b, c) << 24);
	}
}

void AESEncrypt(AesCtx* ctx, char* data) 
{
	int i;
	int round = 0;
	memcpy(ctx->W, data, AES_BLOCKLEN);

	AddRoundKey(0);

	for (round = 1; round < ctx->Nr; round++)
	{
		SubBytes();
		ShiftRows(ctx);
		MixColumns(ctx);
		AddRoundKey(round);
	}

	SubBytes();
	ShiftRows(ctx);
	AddRoundKey(ctx->Nr);
}

void AESDecrypt(AesCtx* ctx, char* data)
{
	int i;
	int round = 0;
	memcpy(ctx->W, data, AES_BLOCKLEN);

	AddRoundKey(ctx->Nr);

	for (round = (ctx->Nr - 1); round > 0; round--)
	{
		InvShiftRows(ctx);
		InvSubBytes();
		AddRoundKey(round);
		InvMixColumns(ctx);
	}

	InvShiftRows(ctx);
	InvSubBytes();
	AddRoundKey(0);
}

void AESOutput(AesCtx* ctx, char* output)
{
	memcpy(output, ctx->W, AES_BLOCKLEN);
}

// For GCM operation mode, we need multiplications in F2[x]/x^128+x^7+x^2+x+1
// Sums and substractions are the same (?!): a simple XOR.
// In such a polynom the degree 0 is at the most left (bit 127 in the usual order).
// Therefore, the polynom x^7+x^2+x+1 is 0xe1000000000000000000000000000000
// the Minimacy version is :
// 
//const _R=bigFromHex("e1000000000000000000000000000000");;
//fun mulH(V, H)=
//	let bigFromInt(0) -> Z in
//	(
//		for i=0; i<128 do
//		(
//			if 0<>bigBit(H, 127-i) then set Z=bigXor(Z, V);
//			set V=
//				if 0<>bigBit(V, 0) then bigXor(_R, bigASR1(V))
//				else bigASR1(V)
//		);
//		Z
//	);;
//
// In order to speed it up, we precompute a table of 512 16-bytes vectors,
// so that the multiplication is only 32 XOR operations on 16 bytes vectors.
// Everything is done in constant time

void _aesClear(unsigned int* a)
{
	a[0] = 
	a[1] = 
	a[2] = 
	a[3] = 0;
}
void _aesCopy(unsigned int* a, unsigned int* b)	// a= b
{
	a[0] = b[0];
	a[1] = b[1];
	a[2] = b[2];
	a[3] = b[3];
}
void _aesXor(unsigned int* a, unsigned int* b)	// a= a^b
{
	a[0] ^= b[0];
	a[1] ^= b[1];
	a[2] ^= b[2];
	a[3] ^= b[3];
}
void _aesASR1(unsigned char* b)
{
	int i;
	for(i=15;i>0;i--) b[i]= (b[i] >> 1) | ((b[i-1] & 1) << 7);
	b[0]>>=1;
}
void _aesGcmMul4bits(unsigned char* H, int b4, unsigned char* Z)
{
	int i;
	unsigned char V[16];
	_aesClear((unsigned int*)Z);
	_aesCopy((unsigned int*)V, (unsigned int*)H);
	for (i = 8; i > 0; i >>= 1) {
		unsigned char alt;
		unsigned char c = (V[15] & 1)-1;
		if (b4 & i) _aesXor((unsigned int*)Z, (unsigned int*)V);
		_aesASR1(V);
		alt = V[0] ^ 0xe1;
		V[0] = (alt & ~c) | (V[0] & c);	// constant time
	}
}
void _aesGcmMulx4(unsigned char* src, unsigned char* V)
{
	int i;
	_aesCopy((unsigned int*)V, (unsigned int*)src);
	for (i = 0; i < 4; i++) {
		unsigned char alt;
		unsigned char c = (V[15] & 1) - 1;
		_aesASR1(V);
		alt = V[0] ^ 0xe1;	// constant time
		V[0] = (alt & ~c) | (V[0] & c);
	}
}
void aesGcmPrecompute(unsigned char* H, unsigned char* dst)
{
	int i;
	for (i = 0; i < 16; i++) _aesGcmMul4bits(H, i, dst + (i << 4));
	for (;i<512;i++) _aesGcmMulx4(dst + ((i-16) << 4), dst + (i << 4));
}

void aesGcmMul(unsigned char* V, unsigned char* tAll, unsigned char* dst)
{
	int i;
	unsigned char* src = tAll;
	_aesClear((unsigned int*)dst);
	for (i = 0; i < 16; i++) {
		unsigned char c = V[i];
		_aesXor((unsigned int*)dst, (unsigned int*)&src[(15 & (c >> 4)) << 4]);
		_aesXor((unsigned int*)dst, (unsigned int*)&src[(16 + (15 & c)) << 4]);
		src += 32 * 16;
	}
}
void bytesMsbInc(unsigned char* V, int len)
{
	int i = len-1;
	while (i >= 0) {
		if (++V[i]) break;
		i--;
	}
}

void aesGcmGhash(char* src, int len0, unsigned char* tAll, unsigned char* Y)
{
	int i;
	int len = len0 & (~15);
	unsigned char R[16];
	for (i = 0; i < len; i += 16) {
		_aesXor((unsigned int*)Y, (unsigned int*)&src[i]);
		aesGcmMul(Y, tAll, R);
		_aesCopy((unsigned int*)Y, (unsigned int*)R);
	}
	if (len != len0) {
		char srcTail[16];
		_aesClear((unsigned int*)srcTail);
		memcpy(srcTail, src + len, len0 - len);
		_aesXor((unsigned int*)Y, (unsigned int*)srcTail);
		aesGcmMul(Y, tAll, R);
		_aesCopy((unsigned int*)Y, (unsigned int*)R);
	}
}