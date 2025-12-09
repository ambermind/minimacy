// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _RC4_H_
#define _RC4_H_

#define RCA_BLOCKLEN 8

typedef struct
{
    int i;
    int j;
    unsigned char S[256];
}Rc4Ctx;
void RC4Create(Rc4Ctx* ctx, char* key, LINT key_len);
void RC4Process(Rc4Ctx* ctx, char* data, LINT len, char* output);

#endif //_RC4_H_
