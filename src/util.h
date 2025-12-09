// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _UTIL_
#define _UTIL_

#define SEED_INIT_VALUE 0x5DEECE66DL

int isLetter(int c);
int isNumber(int c);
int isHexchar(int c);
int isBinchar(int c);
int isLetterOrNumber(int c);

int isLabel(char* src);
int isAlphanum(char* src);
int isDecimal(char* src);
int isHexstring(char* src);
int isFloat(char* src);
int isBin(char* src);

int startsWithUppercase(char* src);
int startsWithLowercase(char* src);

int hexcharFromInt(int c);
int intFromHexchar(int c);

LINT intFromAsc(char* src, int acceptoctal);
LINT intFromHex(char* src);
LINT intFromBin(char* src);
LFLOAT floatFromAsc(char* src);
LFLOAT floatFromAscExt(char* src, char** dst);

LINT signExtend(LINT val, LINT bit);
LINT signExtend8(LINT val);
LINT signExtend16(LINT val);
LINT signExtend24(LINT val);
LINT signExtend32(LINT val);

char* defCodeName(LINT code);

LINT getLsbInt(char* p);
LINT powerInt(LINT a, LINT n);

void pseudoRandomInit(void);
LINT pseudoRandom32(void);
void pseudoRandomBytes(char* dst, LINT len);
void pseudoRandomEntropy(LINT val);

int lwEquals(LW a, int ta, LW b, int tb);

int codeFromEntity(char* entity, LINT len);

void _myHexDump(char* src, int len, int offsetDisplay);
#endif
