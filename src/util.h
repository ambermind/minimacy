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
#ifndef _UTIL_
#define _UTIL_

#define SEED_INIT_VALUE 0x5DEECE66DL

int isletter(int c);
int isnum(int c);
int ishex(int c);
int isletnum(int c);

int islabel(char* src);
int isalphanum(char* src);
int isdecimal(char* src);
int ishexadecimal(char* src);
int isfloat(char* src);

int startsWithUppercase(char* src);
int startsWithLowercase(char* src);

int ctoh(int c);
int htoc(int c);

LINT ls_atoi(char* src, int acceptoctal);
LINT ls_htoi(char* src);
LFLOAT ls_atof(char* src);
LFLOAT ls_atofExt(char* src, char** dst);

LINT signExtend(LINT val, LINT bit);
LINT signExtend8(LINT val);
LINT signExtend16(LINT val);
LINT signExtend32(LINT val);

char* defCodeName(LINT code);

LINT getLsbInt(char* p);
LINT powerInt(LINT a, LINT n);

void pseudoRandomInit();
LINT pseudoRandom32();
void pseudoRandomBytes(char* dst, LINT len);
void pseudoRandomEntropy(LINT val);

int lwEquals(LW a, int ta, LW b, int tb);

int codeFromEntity(char* entity, LINT len);

void _myHexDump(char* src, int len, int offsetDisplay);
#endif
