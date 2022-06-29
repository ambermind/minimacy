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
#ifndef __MD5_H__
#define __MD5_H__

typedef struct {
	unsigned int buf[4];
	unsigned int bits[2];
	unsigned char in[64];
}MD5_CTX;

void MD5Init(MD5_CTX* ctx);
void MD5Update(MD5_CTX* ctx, const unsigned char* buf, unsigned len);
void MD5Final(unsigned char digest[16], MD5_CTX* ctx);

#endif /* __MD5_H__ */

