// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _HASH_H_
#define _HASH_H_

#define MD5_BLOCK_LENGTH  64
#define MD5_DIGEST_LENGTH 16

#define SHA1_BLOCK_LENGTH     64
#define SHA1_DIGEST_LENGTH    20

#define SHA256_BLOCK_LENGTH   64
#define SHA256_DIGEST_LENGTH  32

#define SHA384_BLOCK_LENGTH  128
#define SHA384_DIGEST_LENGTH  48

#define SHA512_BLOCK_LENGTH  128
#define SHA512_DIGEST_LENGTH  64

typedef struct {
	int H[4];
	long long total;
	union {
		char in[MD5_BLOCK_LENGTH];
		unsigned int W[16];
	};
}MD5_CTX;

typedef struct {
	unsigned int H[5];
	long long total;
	union {
		char in[80 * 4];
		unsigned int W[80];
	};
} SHA1_CTX;

typedef struct {
	unsigned int H[8];
	long long total;
   union {
   	unsigned char in[SHA256_BLOCK_LENGTH];
      unsigned int W[16];
   };
} SHA2_CTX;

typedef struct {
	unsigned long long H[8];
	long long total;
   union {
   	unsigned char in[SHA512_BLOCK_LENGTH];
      unsigned long long W[16];
   };
} SHA2L_CTX;

void md5Create(MD5_CTX* ctx);
void md5Process(MD5_CTX* ctx, char* data, LINT len);
void md5Output(MD5_CTX* ctx, char* digest);

void sha1Create(SHA1_CTX* ctx);
void sha1Process(SHA1_CTX* ctx, char* data, LINT len);
void sha1Output(SHA1_CTX* ctx, char* digest);

void sha256Create(SHA2_CTX* context);
void sha256Process(SHA2_CTX* ctx,char* data, LINT len);
void sha256Output(SHA2_CTX* ctx,char* out);

void sha384Create(SHA2L_CTX* context);
void sha384Process(SHA2L_CTX* ctx, char* data, LINT len);
void sha384Output(SHA2L_CTX* ctx, char* out);

void sha512Create(SHA2L_CTX* context);
void sha512Process(SHA2L_CTX* ctx,char* data, LINT len);
void sha512Output(SHA2L_CTX* ctx,char* out);


#endif /* _HASH_H_ */

