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
#ifndef _SYSTEM_INFLATE_
#define _SYSTEM_INFLATE_

#define MAX_BITS 32

#define MAX_LENGTH 19
#define MAX_CODE 288
#define MAX_DIST 32

extern const int LEN_ORDER[];
extern const int LENS257[];
extern const int DISTANCES[];

typedef struct HuffNode HuffNode;
struct HuffNode {
	int val;
	int count;
	HuffNode* left;
	HuffNode* right;
	HuffNode* next;
};

void _huffmanDump(HuffNode* t);
HuffNode* huffmanDump(HuffNode* t);

void getFixedCodeLengths(int* lens);
void getFixedDistLengths(int* lens);

int fun_inflate(Thread* th);
int fun_deflate(Thread* th);
#endif