// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _SYSTEM_INFLATE_
#define _SYSTEM_INFLATE_

#define MAX_BITS 32

#define MAX_LENGTH 19
#define MAX_CODE 288
#define MAX_DIST 32

#define ERR_FORMAT (-1)
#define ERR_UNCOMPLETE (-2)
#define ERR_OM (-3)

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