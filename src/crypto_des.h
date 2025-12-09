// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _DES_H_
#define _DES_H_

#define DES_BLOCKLEN 8

typedef struct
{
    long long sub_key[16];
    long long result;
}DesCtx;
void DESCreate(DesCtx* ctx, char* key);
void DESProcess(DesCtx* ctx, char* data, int enc);
void DESOutput(DesCtx* ctx, char* output);

#endif //_AES_H_
