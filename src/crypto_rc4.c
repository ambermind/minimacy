// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"
#include"crypto_rc4.h"


void RC4Create(Rc4Ctx* ctx, char* key, LINT key_len)
{
    int i, j;
    LINT k;
    unsigned char z;
    for (i = 0; i < 256; i++) ctx->S[i] = i;
    j = 0;
    k = 0;
    for (i = 0; i < 256; i++)
    {
        j = (j + ctx->S[i] + key[k])&255;
        z = ctx->S[i]; ctx->S[i]= ctx->S[j]; ctx->S[j]=z;
        k++;
        if (k >= key_len) k = 0;
    }
    ctx->i = ctx->j = 0;
}
void RC4Process(Rc4Ctx* ctx, char* data, LINT len, char* output)
{
    LINT n;
    unsigned char z,rnd;
    int i = ctx->i;
    int j = ctx->j;

    for (n = 0; n < len; n++)
    {
        i = (i + 1) &255;
        j = (j + ctx->S[i]) &255;
        z = ctx->S[i]; ctx->S[i] = ctx->S[j]; ctx->S[j] = z;
        rnd=ctx->S[(ctx->S[i] + ctx->S[j]) &255];
        output[n]=rnd ^ data[n];
    }
    ctx->i = i;
    ctx->j = j;
}

