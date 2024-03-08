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
#ifndef __SHA1_H__
#define __SHA1_H__

typedef struct {
	unsigned int H[5];
	unsigned int W[80];
	int lenW;
	unsigned int sizeHi, sizeLo;
} SHA_CTX;

void SHAInit(SHA_CTX* ctx);
void SHAUpdate(SHA_CTX* ctx, const unsigned char* dataIn, int len);
void SHAFinal(SHA_CTX* ctx, unsigned char hashout[20]);

#endif /* __SHA1_H__ */

