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
