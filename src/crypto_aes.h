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

#endif //_AES_H_
