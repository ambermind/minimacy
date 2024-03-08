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
#ifndef __SHA2_H__
#define __SHA2_H__

/*** SHA-256/384/512 Various Length Definitions ***********************/
#define SHA256_BLOCK_LENGTH		64
#define SHA256_DIGEST_LENGTH		32
#define SHA384_BLOCK_LENGTH		128
#define SHA384_DIGEST_LENGTH		48
#define SHA512_BLOCK_LENGTH		128
#define SHA512_DIGEST_LENGTH		64


typedef struct {
	unsigned int	state[8];
	unsigned long long	bitcount;
	unsigned char	buffer[SHA256_BLOCK_LENGTH];
} SHA256_CTX;
typedef struct {
	unsigned long long	state[8];
	unsigned long long	bitcount[2];
	unsigned char	buffer[SHA512_BLOCK_LENGTH];
} SHA512_CTX;

typedef SHA512_CTX SHA384_CTX;


void sha256create(SHA256_CTX* context);
void sha256process(SHA256_CTX* ctx,char* data,int len);
void sha256result(SHA256_CTX* ctx,char* out);

void sha384create(SHA384_CTX* context);
void sha384process(SHA384_CTX* ctx, char* data, int len);
void sha384result(SHA384_CTX* ctx, char* out);

void sha512create(SHA512_CTX* context);
void sha512process(SHA512_CTX* ctx,char* data,int len);
void sha512result(SHA512_CTX* ctx,char* out);


#endif /* __SHA2_H__ */

