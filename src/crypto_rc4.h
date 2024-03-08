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
