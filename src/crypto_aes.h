// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _AES_H_
#define _AES_H_

#define AES_BLOCKLEN 16 //Block length in bytes AES is 128b block only

typedef struct
{
	int Nr;
	unsigned int RK[60];
	unsigned int W[4];
}AesCtx;

void AESCreate(AesCtx* ctx, char* key, int key_len);
void AESEncrypt(AesCtx* ctx, char* data);
void AESDecrypt(AesCtx* ctx, char* data);
void AESOutput(AesCtx* ctx, char* output);

void aesGcmPrecompute(unsigned char* H, unsigned char* dst);
void aesGcmMul(unsigned char* V, unsigned char* tAll, unsigned char* dst);
void bytesMsbInc(unsigned char* V, int len);
void aesGcmGhash(char* src, int len, unsigned char* tAll, unsigned char* Y);

#endif //_AES_H_
