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
#include"minimacy.h"

int isletter(int c)
{
	if ((c >= 'A') && (c <= 'Z')) return 1;
	if ((c >= 'a') && (c <= 'z')) return 1;
	if (c == '_') return 1;
	return 0;
}
// is decimal digit
int isnum(int c)
{
	if ((c >= '0') && (c <= '9')) return 1;
	return 0;
}
// is hexadecimal digit
int ishex(int c)
{
	if ((c >= '0') && (c <= '9')) return 1;
	if ((c >= 'A') && (c <= 'F')) return 1;
	if ((c >= 'a') && (c <= 'f')) return 1;
	return 0;
}

// is either a letter or number
int isletnum(int c)
{
	if ((c >= 'A') && (c <= 'Z')) return 1;
	if ((c >= 'a') && (c <= 'z')) return 1;
	if ((c >= '0') && (c <= '9')) return 1;
	if (c == '_') return 1;
	return 0;
}

// check if a word is a label (one letter followed by letters or numbers)
int islabel(char* src)
{
	if (!src) return 0;
	if (!isletter(*src++)) return 0;
	while (*src)
	{
		if (!isletnum(*src++)) return 0;
	}
	return 1;
}

int isalphanum(char* src)
{
	if (!src) return 0;
	while (*src)
	{
		if (!isletnum(*src++)) return 0;
	}
	return 1;
}

int isdecimal(char* src)
{
	while (*src)
	{
		if (!isnum(*src++)) return 0;
	}
	return 1;
}

int ishexadecimal(char* src)
{
	while (*src)
	{
		if (!ishex(*src++)) return 0;
	}
	return 1;
}

int isfloat(char* src)
{
	int debut = 1;
	int point = 0;
	while (*src)
	{
		if ((*src == '.') && (!debut) && (!point)) point = 1;
		else if (!isnum(*src)) return 0;
		src++;
		debut = 0;
	}
	return point;
}

int ctoh(int c)
{
	c &= 15;
	if (c < 10) return '0' + c;
	return 'a' + c - 10;
}

int htoc(int c)
{
	if ((c >= '0') && (c <= '9')) return c - '0';
	else if ((c >= 'A') && (c <= 'F')) return c - 'A' + 10;
	else if ((c >= 'a') && (c <= 'f')) return c - 'a' + 10;
	return 0;
}

// read a decimal string (stops at the first unexpected char)
LINT ls_atoi(char* src, int acceptoctal)
{
	LINT x, c, s;
	x = s = 0;
	if ((*src) == '-') { s = 1; src++; }
	if ((acceptoctal) && ((*src) == '0'))
		while ((c = *src++))
		{
			if ((c >= '0') && (c <= '7')) x = (x * 8) + c - '0';
			else return (s ? (-x) : x);
		}
	else
		while ((c = *src++))
		{
			if ((c >= '0') && (c <= '9')) x = (x * 10) + c - '0';
			else return (s ? (-x) : x);
		}
	return (s ? (-x) : x);
}

// read an hexadecimal string (stops at the first unexpected char)
LINT ls_htoi(char* src)
{
	LINT x, c;
	x = 0;
	while ((c = *src++))
	{
		if ((c >= '0') && (c <= '9')) x = (x << 4) + c - '0';
		else if ((c >= 'A') && (c <= 'F')) x = (x << 4) + c - 'A' + 10;
		else if ((c >= 'a') && (c <= 'f')) x = (x << 4) + c - 'a' + 10;
		else return x;
	}
	return x;
}

LFLOAT ls_atofExt(char* p, char** dst)
{
	LFLOAT f;
	double n = 0;
	int sign = 1;
	int scale = 0;
	int subscale = 0;
	int signsubscale = 1;

	if ((*p) == '+') p++;
	else if (*p == '-') { sign = -1; p++; }

	while (*p == '0') p++;
	if (((*p) >= '1') && ((*p) <= '9'))
	{
		do
		{
			n = (n * 10.0) + (*p++ - '0');
		} while (((*p) >= '0') && ((*p) <= '9'));
	}
	if (((*p) == '.') && (p[1] >= '0') && (p[1] <= '9'))
	{
		p++;
		do
		{
			n = (n * 10.0) + (*(p++) - '0');
			scale--;
		} while (((*p) >= '0') && ((*p) <= '9'));
	}
	n *= sign;
	if (((*p) == 'e') || ((*p) == 'E'))
	{
		p++;
		if ((*p) == '+') p++;
		else if ((*p) == '-')
		{
			signsubscale = -1;
			p++;
		}
		while (((*p) >= '0') && ((*p) <= '9')) subscale = (subscale * 10) + (*(p++) - '0');
	}
	scale += subscale * signsubscale;
	if (scale) n *= pow(10.0, scale);
	f = (LFLOAT)n;
	if (dst) *dst = p;
	return f;
}
LFLOAT ls_atof(char* p)
{
	return ls_atofExt(p, NULL);
}

char* refCodeName(LINT code)
{
	if (code>=0) return "FUN";
	if (code==REFCODE_TYPE) return "TYPE";
	if (code==REFCODE_VAR) return "VAR";
	if (code==REFCODE_FIELD) return "FIELD";
	if (code==REFCODE_STRUCT) return "STRUCT";
	if (code==REFCODE_CONS) return "CONS";
	if (code == REFCODE_CONS0) return "CONS0";
	if (code==REFCODE_SUM) return "SUM";
	if (code==REFCODE_CONST) return "CONST";
	return "??";
}

LINT getLsbInt(char* p)
{
	LINT result=0;
	int j=sizeof(LINT)-1;
	while(j>=0) result=(result<<8)+(255&p[j--]);
	return result;
}

LINT powerInt(LINT a, LINT n)
{
	LINT res = 1;
	if (n < 0) return 0;
	if (a == 0) return 0;
	while (n)
	{
		if (n & 1) res *= a;
		a *= a;
		n >>= 1;
	}
	return res;
}


LINT lsRand32(unsigned long long* seed)
{
	unsigned long long x;
	*seed = ((*seed) * 0x5DEECE66DL + 0xBL) & 0xffffffffffff;//(one << 48) - 1);
	x = (*seed) >> (48 - 32);
	return (LINT)x;
}

void lsSrand(unsigned long long* seed, LINT val)
{
	*seed = (val ^ 0x5DEECE66DL) & 0xffffffffffff;//((1L << 48) - 1);
}

// when combining two ranges (srcIndex, len), (dstIndex, len), it is required to adjust these ranges
// so that only the valid part of the ranges (from 0 to srcLen-1, and from 0 to dstLen-1) will be processed
// it returns the length of the resulting range and the offset to apply on the start of both ranges
// if skip==NULL then consider that neither srcIndex and dstIndex should be negative
// else *skip will contain the offset to be applied to both srcIndex and dstIndex
// for example :
//
// len=rangeAdjust(dstIndex, dstLen, srcIndex, srcLen, len, &skip);
// if (len>0) memcpy(&dst[srcIndex+skip],&src[srcIndex+skip],len);
//
LINT rangeAdjust(LINT dstIndex, LINT dstLen, LINT srcIndex, LINT srcLen, LINT len, LINT* skip)
{
	LINT _skip=0;
	LINT min = (srcIndex < dstIndex) ? srcIndex : dstIndex;
	if ((srcIndex + len) >= srcLen) len = srcLen - srcIndex;
	if ((dstIndex + len) >= dstLen) len = dstLen - dstIndex;
	if (min < 0)
	{
		if (!skip) return -1;
		_skip -= min;
		len += min;
	}
	if (skip) *skip = _skip;
	return len;
}


int lwEquals(LW a, LW b)
{
	if (a == b) return 1;
	else if ((a != NIL) && (b != NIL) && ISVALPNT(a))
	{
		LB* pa = VALTOPNT(a);
		LB* pb = VALTOPNT(b);
		if (HEADER_DBG(pa) == HEADER_DBG(pb))
		{
			if (HEADER_DBG(pa) == DBG_S)
			{
				if ((STRLEN(pa) == STRLEN(pb)) && (!memcmp(STRSTART(pa), STRSTART(pb), STRLEN(pa)))) return 1;
			}
			else if (HEADER_DBG(pa) == DBG_B)
			{
				if (bignumEquals((bignum)pa, (bignum)pb)) return 1;
			}
		}
	}
	return 0;
}

void _myHexDump(char* src, int len, int offsetDisplay)
{
	int line;
	int tmp = len + offsetDisplay;
	char* format = "%04x ";
	if (tmp > 0x1000000) format = "%08x ";
	else if (tmp > 0x10000) format = "%06x ";

	for (line = 0; line < len; line += 16)
	{
		int i;
		printf(format, line);
		for (i = line; i < line + 16; i++) if (i < len) printf("%02x ", 255 & (src[i]));
		else printf("   ");
		for (i = line; i < line + 16; i++) if (i < len) printf("%c", src[i] < 32 ? '.' : src[i]);
		printf("\n");
	}
}