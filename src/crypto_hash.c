// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"
#include"crypto_hash.h"

// md5: https://www.ietf.org/rfc/rfc1321.txt
// sha: https://csrc.nist.gov/files/pubs/fips/180-3/final/docs/fips180-3_final.pdf
// sha384 and sha512 implementation limitation: the source length is limited to 2^61 bytes (2^64 bits)

// creation, process and output functions use a common pattern
// transform functions are less easy to mutualize, they are used by process and output functions, they are not exported

// SHR(b,x) assumes x is unsigned
#define SHL(b,x)    ((x) << (b))
#define SHR(b,x)    ((x) >> (b))
#define ROTL32(b,x) (SHL(b,x) | SHR(32-(b), x))
#define ROTR32(b,x) (SHR(b,x) | SHL(32-(b), x))
#define ROTR64(b,x) (SHR(b,x) | SHL(64-(b), x))

// -----------------------
// common Create functions
const static unsigned int HMD5[4] = {
0x67452301,
0xefcdab89,
0x98badcfe,
0x10325476
};

const static unsigned int H1[5] = {
0x67452301,
0xefcdab89,
0x98badcfe,
0x10325476,
0xc3d2e1f0,
};

const static unsigned int H256[8] = {
0x6a09e667,
0xbb67ae85,
0x3c6ef372,
0xa54ff53a,
0x510e527f,
0x9b05688c,
0x1f83d9ab,
0x5be0cd19
};

const static unsigned long long H384[8] = {
0xcbbb9d5dc1059ed8,
0x629a292a367cd507,
0x9159015a3070dd17,
0x152fecd8f70e5939,
0x67332667ffc00b31,
0x8eb44a8768581511,
0xdb0c2e0d64f98fa7,
0x47b5481dbefa4fa4
};

const static unsigned long long H512[8] = {
0x6a09e667f3bcc908,
0xbb67ae8584caa73b,
0x3c6ef372fe94f82b,
0xa54ff53a5f1d36f1,
0x510e527fade682d1,
0x9b05688c2b3e6c1f,
0x1f83d9abfb41bd6b,
0x5be0cd19137e2179
};

#define HASH_CREATE(name, typeCtx, initialHashValue, len) \
void name(typeCtx* ctx) { \
	memcpy(ctx->H, initialHashValue, len); \
	ctx->total = 0; \
}
HASH_CREATE(   md5Create,  MD5_CTX,  HMD5,    MD5_DIGEST_LENGTH)
HASH_CREATE(  sha1Create, SHA1_CTX,    H1,   SHA1_DIGEST_LENGTH)
HASH_CREATE(sha256Create, SHA2_CTX,  H256, SHA256_DIGEST_LENGTH)
HASH_CREATE(sha384Create, SHA2L_CTX, H384, SHA512_DIGEST_LENGTH)
HASH_CREATE(sha512Create, SHA2L_CTX, H512, SHA512_DIGEST_LENGTH)

// -------------------------------------
// Transform functions : MD5, SHA1, SHA2
#define LD(label,i)  label = ctx->H[i]
#define ADD(label,i) ctx->H[i] += label

// md5 Transform
#define F(x, y, z) (z ^ (x & (y ^ z)))
#define G(x, y, z) (y ^ (z & (x ^ y)))
#define H(x, y, z) (x ^ y ^ z)
#define I(x, y, z) (y ^ (x | ~z))

#define TRANSFORM(f, w, x, y, z, i, k, shift) { w += f(x, y, z) + i + k; w=ROTL32(shift, w) + x; }

void _md5Transform(MD5_CTX* ctx)
{
	unsigned int a, b, c, d;	// must be unsigned for w>>(32-shift) in ROTATE_LEFT
	unsigned int* W = ctx->W;

	LD(a, 0); LD(b, 1); LD(c, 2); LD(d, 3);

	TRANSFORM(F, a, b, c, d, W[0], 0xd76aa478, 7);
	TRANSFORM(F, d, a, b, c, W[1], 0xe8c7b756, 12);
	TRANSFORM(F, c, d, a, b, W[2], 0x242070db, 17);
	TRANSFORM(F, b, c, d, a, W[3], 0xc1bdceee, 22);
	TRANSFORM(F, a, b, c, d, W[4], 0xf57c0faf, 7);
	TRANSFORM(F, d, a, b, c, W[5], 0x4787c62a, 12);
	TRANSFORM(F, c, d, a, b, W[6], 0xa8304613, 17);
	TRANSFORM(F, b, c, d, a, W[7], 0xfd469501, 22);
	TRANSFORM(F, a, b, c, d, W[8], 0x698098d8, 7);
	TRANSFORM(F, d, a, b, c, W[9], 0x8b44f7af, 12);
	TRANSFORM(F, c, d, a, b, W[10], 0xffff5bb1, 17);
	TRANSFORM(F, b, c, d, a, W[11], 0x895cd7be, 22);
	TRANSFORM(F, a, b, c, d, W[12], 0x6b901122, 7);
	TRANSFORM(F, d, a, b, c, W[13], 0xfd987193, 12);
	TRANSFORM(F, c, d, a, b, W[14], 0xa679438e, 17);
	TRANSFORM(F, b, c, d, a, W[15], 0x49b40821, 22);

	TRANSFORM(G, a, b, c, d, W[1], 0xf61e2562, 5);
	TRANSFORM(G, d, a, b, c, W[6], 0xc040b340, 9);
	TRANSFORM(G, c, d, a, b, W[11], 0x265e5a51, 14);
	TRANSFORM(G, b, c, d, a, W[0], 0xe9b6c7aa, 20);
	TRANSFORM(G, a, b, c, d, W[5], 0xd62f105d, 5);
	TRANSFORM(G, d, a, b, c, W[10], 0x02441453, 9);
	TRANSFORM(G, c, d, a, b, W[15], 0xd8a1e681, 14);
	TRANSFORM(G, b, c, d, a, W[4], 0xe7d3fbc8, 20);
	TRANSFORM(G, a, b, c, d, W[9], 0x21e1cde6, 5);
	TRANSFORM(G, d, a, b, c, W[14], 0xc33707d6, 9);
	TRANSFORM(G, c, d, a, b, W[3], 0xf4d50d87, 14);
	TRANSFORM(G, b, c, d, a, W[8], 0x455a14ed, 20);
	TRANSFORM(G, a, b, c, d, W[13], 0xa9e3e905, 5);
	TRANSFORM(G, d, a, b, c, W[2], 0xfcefa3f8, 9);
	TRANSFORM(G, c, d, a, b, W[7], 0x676f02d9, 14);
	TRANSFORM(G, b, c, d, a, W[12], 0x8d2a4c8a, 20);

	TRANSFORM(H, a, b, c, d, W[5], 0xfffa3942, 4);
	TRANSFORM(H, d, a, b, c, W[8], 0x8771f681, 11);
	TRANSFORM(H, c, d, a, b, W[11], 0x6d9d6122, 16);
	TRANSFORM(H, b, c, d, a, W[14], 0xfde5380c, 23);
	TRANSFORM(H, a, b, c, d, W[1], 0xa4beea44, 4);
	TRANSFORM(H, d, a, b, c, W[4], 0x4bdecfa9, 11);
	TRANSFORM(H, c, d, a, b, W[7], 0xf6bb4b60, 16);
	TRANSFORM(H, b, c, d, a, W[10], 0xbebfbc70, 23);
	TRANSFORM(H, a, b, c, d, W[13], 0x289b7ec6, 4);
	TRANSFORM(H, d, a, b, c, W[0], 0xeaa127fa, 11);
	TRANSFORM(H, c, d, a, b, W[3], 0xd4ef3085, 16);
	TRANSFORM(H, b, c, d, a, W[6], 0x04881d05, 23);
	TRANSFORM(H, a, b, c, d, W[9], 0xd9d4d039, 4);
	TRANSFORM(H, d, a, b, c, W[12], 0xe6db99e5, 11);
	TRANSFORM(H, c, d, a, b, W[15], 0x1fa27cf8, 16);
	TRANSFORM(H, b, c, d, a, W[2], 0xc4ac5665, 23);

	TRANSFORM(I, a, b, c, d, W[0], 0xf4292244, 6);
	TRANSFORM(I, d, a, b, c, W[7], 0x432aff97, 10);
	TRANSFORM(I, c, d, a, b, W[14], 0xab9423a7, 15);
	TRANSFORM(I, b, c, d, a, W[5], 0xfc93a039, 21);
	TRANSFORM(I, a, b, c, d, W[12], 0x655b59c3, 6);
	TRANSFORM(I, d, a, b, c, W[3], 0x8f0ccc92, 10);
	TRANSFORM(I, c, d, a, b, W[10], 0xffeff47d, 15);
	TRANSFORM(I, b, c, d, a, W[1], 0x85845dd1, 21);
	TRANSFORM(I, a, b, c, d, W[8], 0x6fa87e4f, 6);
	TRANSFORM(I, d, a, b, c, W[15], 0xfe2ce6e0, 10);
	TRANSFORM(I, c, d, a, b, W[6], 0xa3014314, 15);
	TRANSFORM(I, b, c, d, a, W[13], 0x4e0811a1, 21);
	TRANSFORM(I, a, b, c, d, W[4], 0xf7537e82, 6);
	TRANSFORM(I, d, a, b, c, W[11], 0xbd3af235, 10);
	TRANSFORM(I, c, d, a, b, W[2], 0x2ad7d2bb, 15);
	TRANSFORM(I, b, c, d, a, W[9], 0xeb86d391, 21);

	ADD(a, 0); ADD(b, 1); ADD(c, 2); ADD(d, 3);
}

// sha1 Transform
#define SHA1LOOP(delta) { \
	T = ROTL32(5, a) + e + W[t] + (delta); \
	e = d; d = c; c = ROTL32(30, b); b = a; a = T; \
}
void _sha1Transform(SHA1_CTX* ctx)
{
	int t;
	unsigned int a, b, c, d, e, T;
	unsigned int* W = ctx->W;

	for (t = 0; t < 16; t++) W[t] = MSBL(W[t]);	// 512bits binary input is considered as 16 MSB 32bits integers

	for (t = 16; t <= 79; t++) W[t] = ROTL32(1, W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16]);

	LD(a, 0); LD(b, 1); LD(c, 2); LD(d, 3); LD(e, 4);

	for (t =  0; t < 20; t++) SHA1LOOP(0x5a827999 + (((c ^ d) & b) ^ d));
	for (t = 20; t < 40; t++) SHA1LOOP(0x6ed9eba1 + (b ^ c ^ d));
	for (t = 40; t < 60; t++) SHA1LOOP(0x8f1bbcdc + ((b & c) | (d & (b | c))));
	for (t = 60; t < 80; t++) SHA1LOOP(0xca62c1d6 + (b ^ c ^ d));

	ADD(a, 0); ADD(b, 1); ADD(c, 2); ADD(d, 3); ADD(e, 4);
}

// sha2 Transform
const static unsigned int K256[64] = {
0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

const static unsigned long long K512[80] = {
0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f, 0xe9b5dba58189dbbc,
0x3956c25bf348b538, 0x59f111f1b605d019, 0x923f82a4af194f9b, 0xab1c5ed5da6d8118,
0xd807aa98a3030242, 0x12835b0145706fbe, 0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2,
0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235, 0xc19bf174cf692694,
0xe49b69c19ef14ad2, 0xefbe4786384f25e3, 0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65,
0x2de92c6f592b0275, 0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5,
0x983e5152ee66dfab, 0xa831c66d2db43210, 0xb00327c898fb213f, 0xbf597fc7beef0ee4,
0xc6e00bf33da88fc2, 0xd5a79147930aa725, 0x06ca6351e003826f, 0x142929670a0e6e70,
0x27b70a8546d22ffc, 0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed, 0x53380d139d95b3df,
0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6, 0x92722c851482353b,
0xa2bfe8a14cf10364, 0xa81a664bbc423001, 0xc24b8b70d0f89791, 0xc76c51a30654be30,
0xd192e819d6ef5218, 0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8,
0x19a4c116b8d2d0c8, 0x1e376c085141ab53, 0x2748774cdf8eeb99, 0x34b0bcb5e19b48a8,
0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb, 0x5b9cca4f7763e373, 0x682e6ff3d6b2b8a3,
0x748f82ee5defb2fc, 0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec,
0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915, 0xc67178f2e372532b,
0xca273eceea26619c, 0xd186b8c721c0c207, 0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178,
0x06f067aa72176fba, 0x0a637dc5a2c898a6, 0x113f9804bef90dae, 0x1b710b35131c471b,
0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc, 0x431d67c49c100d4c,
0x4cc5d4becb3e42b6, 0x597f299cfc657e2a, 0x5fcb6fab3ad6faec, 0x6c44198c4a475817
};

#define Ch(x,y,z)  ((z) ^ ((x) & ((y) ^ (z))))
#define Maj(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define SIGMA0_256(x)	(ROTR32( 2, (x)) ^ ROTR32(13, (x)) ^ ROTR32(22, (x)))
#define SIGMA1_256(x)	(ROTR32( 6, (x)) ^ ROTR32(11, (x)) ^ ROTR32(25, (x)))
#define sigma0_256(x)	(ROTR32( 7, (x)) ^ ROTR32(18, (x)) ^ SHR(    3, (x)))
#define sigma1_256(x)	(ROTR32(17, (x)) ^ ROTR32(19, (x)) ^ SHR(   10, (x)))

#define SIGMA0_512(x)	(ROTR64(28, (x)) ^ ROTR64(34, (x)) ^ ROTR64(39, (x)))
#define SIGMA1_512(x)	(ROTR64(14, (x)) ^ ROTR64(18, (x)) ^ ROTR64(41, (x)))
#define sigma0_512(x)	(ROTR64( 1, (x)) ^ ROTR64( 8, (x)) ^ SHR(    7, (x)))
#define sigma1_512(x)	(ROTR64(19, (x)) ^ ROTR64(61, (x)) ^ SHR(    6, (x)))

#define SHA2TRANSFORM(name, typeCtx, typeUnit, convert, nbLoops, sigma0, sigma1, SIGMA0, SIGMA1, K) \
void name(typeCtx* ctx) \
{ \
	int t; \
	unsigned typeUnit	a, b, c, d, e, f, g, h, T1, T2; \
	unsigned typeUnit	*W = ctx->W; \
	for (t = 0; t < 16; t++) W[t] = convert(W[t]); \
 \
	LD(a, 0); LD(b, 1); LD(c, 2); LD(d, 3); LD(e, 4); LD(f, 5); LD(g, 6); LD(h, 7); \
 \
	for (t = 0; t < nbLoops; t++) { \
		if (t >= 16) W[t & 15] += W[(t + 9) & 15] + sigma0(W[(t + 1) & 15]) + sigma1(W[(t + 14) & 15]); \
		T1 = h + SIGMA1(e) + Ch(e, f, g) + K[t] + W[t & 15]; \
		T2 = SIGMA0(a) + Maj(a, b, c); \
		h = g; g = f; f = e; e = d; d = c; c = b; b = a; \
		e += T1; \
		a = T1 + T2; \
	} \
	ADD(a, 0); ADD(b, 1); ADD(c, 2); ADD(d, 3); ADD(e, 4); ADD(f, 5); ADD(g, 6); ADD(h, 7); \
}
SHA2TRANSFORM(_sha256Transform,  SHA2_CTX,       int,  MSBL, 64, sigma0_256, sigma1_256, SIGMA0_256, SIGMA1_256, K256)
SHA2TRANSFORM(_sha512Transform, SHA2L_CTX, long long, MSB64, 80, sigma0_512, sigma1_512, SIGMA0_512, SIGMA1_512, K512)

// ------------------------
// common Process functions
#define HASH_PROCESS(name, type, blockLen, transform) \
void name(type* ctx, char* data, LINT len) \
{ \
	int index = ctx->total & (blockLen - 1); \
	ctx->total += len; \
	while (index + len >= blockLen) { \
		int toProcess = blockLen - index; \
		memcpy(ctx->in + index, data, toProcess); \
		transform(ctx); \
		data += toProcess; \
		len -= toProcess; \
		index = 0; \
	} \
	if (len > 0) { \
		memcpy(ctx->in+index, data, len); \
		index += (int)len; \
	} \
}
HASH_PROCESS(   md5Process,   MD5_CTX,    MD5_BLOCK_LENGTH,    _md5Transform)
HASH_PROCESS(  sha1Process,  SHA1_CTX,   SHA1_BLOCK_LENGTH,   _sha1Transform)
HASH_PROCESS(sha256Process,  SHA2_CTX, SHA256_BLOCK_LENGTH, _sha256Transform)
HASH_PROCESS(sha384Process, SHA2L_CTX, SHA384_BLOCK_LENGTH, _sha512Transform)
HASH_PROCESS(sha512Process, SHA2L_CTX, SHA512_BLOCK_LENGTH, _sha512Transform)

// -----------------------
// common Output functions
// the context is backed up so that it can be used further with additional data (used in TLS protocol)
#define HASH_OUTPUT(name, typeCtx, typeUnit, blockLen, blockSuffix, transform, convertTotal, convertH, outputUnits) \
void name(typeCtx* ctx, char* digest) { \
	int i; \
	typeCtx backup; \
	typeUnit* output = (typeUnit*)digest; \
	int index = ctx->total & (blockLen - 1); \
	memcpy(&backup, ctx, sizeof(backup)); \
	ctx->in[index++] = 0x80;	\
	if (index > (blockLen - blockSuffix)) {	\
		memset(ctx->in + index, 0, blockLen - index); \
		transform(ctx); \
		index = 0; \
	} \
	memset(ctx->in + index, 0, (blockLen - 8) - index); \
	ctx->total <<= 3; ctx->total= convertTotal(ctx->total); \
	memcpy(ctx->in+blockLen - 8, &ctx->total, 8); \
	transform(ctx); \
	for (i = 0; i < outputUnits; i++) output[i] = convertH(ctx->H[i]); \
	memcpy(ctx, &backup, sizeof(backup)); \
}
HASH_OUTPUT(   md5Output,   MD5_CTX,       int,  MD5_BLOCK_LENGTH,    8,    _md5Transform, LSB64,  LSBL, 4)
HASH_OUTPUT(  sha1Output,  SHA1_CTX,       int, SHA1_BLOCK_LENGTH,    8,   _sha1Transform, MSB64,  MSBL, 5)
HASH_OUTPUT(sha256Output,  SHA2_CTX,       int, SHA256_BLOCK_LENGTH,  8, _sha256Transform, MSB64,  MSBL, 8)
HASH_OUTPUT(sha384Output, SHA2L_CTX, long long, SHA512_BLOCK_LENGTH, 16, _sha512Transform, MSB64, MSB64, 6)
HASH_OUTPUT(sha512Output, SHA2L_CTX, long long, SHA512_BLOCK_LENGTH, 16, _sha512Transform, MSB64, MSB64, 8)

