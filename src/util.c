// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

int isLetter(int c)
{
	if ((c >= 'A') && (c <= 'Z')) return 1;
	if ((c >= 'a') && (c <= 'z')) return 1;
	if (c == '_') return 1;
	return 0;
}
// is decimal digit
int isNumber(int c)
{
	if ((c >= '0') && (c <= '9')) return 1;
	return 0;
}
// is hexadecimal digit
int isHexchar(int c)
{
	if ((c >= '0') && (c <= '9')) return 1;
	if ((c >= 'A') && (c <= 'F')) return 1;
	if ((c >= 'a') && (c <= 'f')) return 1;
	return 0;
}
// is binary digit
int isBinchar(int c)
{
	if ((c >= '0') && (c <= '1')) return 1;
	return 0;
}

// is either a letter or number
int isLetterOrNumber(int c)
{
	if ((c >= 'A') && (c <= 'Z')) return 1;
	if ((c >= 'a') && (c <= 'z')) return 1;
	if ((c >= '0') && (c <= '9')) return 1;
	if (c == '_') return 1;
	return 0;
}

// check if a word is a label (one letter followed by letters or numbers)
int isLabel(char* src)
{
	if (!src) return 0;
	if (!isLetter(*src++)) return 0;
	while (*src)
	{
		if (!isLetterOrNumber(*src++)) return 0;
	}
	return 1;
}

int isAlphanum(char* src)
{
	if (!src) return 0;
	while (*src)
	{
		if (!isLetterOrNumber(*src++)) return 0;
	}
	return 1;
}

int isDecimal(char* src)
{
	if (!*src) return 0;
	if (!isNumber(*src++)) return 0;
	while (*src)
	{
		if (((*src)!='_')&&(!isNumber(*src))) return 0;
		src++;
	}
	return 1;
}

int isHexstring(char* src)
{
	if (!*src) return 0;
	if (!isHexchar(*src++)) return 0;
	while (*src)
	{
		if (((*src) != '_') && (!isHexchar(*src))) return 0;
		src++;
	}
	return 1;
}

int isBin(char* src)
{
	if (!*src) return 0;
	if (!isBinchar(*src++)) return 0;
	while (*src)
	{
		if (((*src) != '_') && (!isBinchar(*src))) return 0;
		src++;
	}
	return 1;
}

int isFloat(char* src)
{
	int debut = 1;
	int point = 0;
	int acceptE = 0;
	int acceptSign = 0;
	while (*src)
	{
		int c = *src;
		if ((c == '.') && (!debut) && (!point)) {
			acceptE = 1;
			point = 1;
			goto next;
		}
		if (isNumber(*src)) goto next;
		if (acceptE && (c == 'E' || c == 'e')) {
			acceptSign = 1;
			acceptE = 0;
			goto next;
		}
		if ((c == '+' || c == '-') && acceptSign) {
			acceptSign = 0;
			goto next;
		}
		return 0;
	next:
		src++;
		debut = 0;
	}
	return point;
}
int startsWithUppercase(char* src)
{
	while (*src)
	{
		char c = *(src++);
		if (c == '_') continue;
		if ((c < 'A') || (c > 'Z')) return 0;
		return 1;
	};
	return 1;
}
int startsWithLowercase(char* src)
{
	while (*src)
	{
		char c = *(src++);
		if (c == '_') continue;
		if ((c < 'a') || (c > 'z')) return 0;
		return 1;
	};
	return 1;
}
int hexcharFromInt(int c)
{
	c &= 15;
	if (c < 10) return '0' + c;
	return 'a' + c - 10;
}

int intFromHexchar(int c)
{
	if ((c >= '0') && (c <= '9')) return c - '0';
	else if ((c >= 'A') && (c <= 'F')) return c - 'A' + 10;
	else if ((c >= 'a') && (c <= 'f')) return c - 'a' + 10;
	return 0;
}

// read a decimal string (stops at the first unexpected char)
LINT intFromAsc(char* src, int acceptoctal)
{
	LINT x, c, s;
	x = s = 0;
	if ((*src) == '-') { s = 1; src++; }
	if ((acceptoctal) && ((*src) == '0'))
		while ((c = *src++))
		{
			if (c=='_') continue;
			if ((c >= '0') && (c <= '7')) x = (x * 8) + c - '0';
			else return (s ? (-x) : x);
		}
	else
		while ((c = *src++))
		{
			if (c=='_') continue;
			if ((c >= '0') && (c <= '9')) x = (x * 10) + c - '0';
			else return (s ? (-x) : x);
		}
	return (s ? (-x) : x);
}

// read an hexadecimal string (stops at the first unexpected char)
LINT intFromHex(char* src)
{
	LINT x, c;
	x = 0;
	while ((c = *src++))
	{
		if (c == '_') continue;
		if ((c >= '0') && (c <= '9')) x = (x << 4) + c - '0';
		else if ((c >= 'A') && (c <= 'F')) x = (x << 4) + c - 'A' + 10;
		else if ((c >= 'a') && (c <= 'f')) x = (x << 4) + c - 'a' + 10;
		else return x;
	}
	return x;
}

// read an hexadecimal string (stops at the first unexpected char)
LINT intFromBin(char* src)
{
	LINT x, c;
	x = 0;
	while ((c = *src++))
	{
		if (c == '_') continue;
		if ((c >= '0') && (c <= '1')) x = (x << 1) + c - '0';
		else return x;
	}
	return x;
}

LFLOAT floatFromAscExt(char* p, char** dst)
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
	if (scale>0) while ((scale--) > 0) n *= 10;	// may be slow
	if (scale<0) while ((scale++) < 0) n /= 10;	// may be slow
//	if (scale) n *= pow(10.0, scale);
	f = (LFLOAT)n;
	if (dst) *dst = p;
	return f;
}
LFLOAT floatFromAsc(char* p)
{
	return floatFromAscExt(p, NULL);
}
LINT signExtend(LINT val, LINT bit)
{
	LINT signBit = ((LINT)1) << (bit-1);
	LINT mask = signBit -1; // bit=7 -> 0x7f
	if (val& signBit) return val|~mask;
	return val&mask;
}
LINT signExtend8(LINT val)
{
	val &= 0xff;
	if (val & 0x80) val |= ~0xff;
	return val;
}
LINT signExtend16(LINT val)
{
	val &= 0xffff;
	if (val & 0x8000) val |= ~0xffff;
	return val;
}
LINT signExtend24(LINT val)
{
	val &= 0xffffff;
	if (val & 0x800000) val |= ~0xffffff;
	return val;
}
LINT signExtend32(LINT val)
{
	val &= 0xffffffff;
	if (val & 0x80000000) val |= ~((LINT)0xffffffff);
	return val;
}
char* defCodeName(LINT code)
{
	if (code>=0) return "FUN";
	if (code==DEF_CODE_TYPE) return "TYPE";
	if (code==DEF_CODE_VAR) return "VAR";
	if (code==DEF_CODE_FIELD) return "FIELD";
	if (code==DEF_CODE_STRUCT) return "STRUCT";
	if (code==DEF_CODE_CONS) return "CONS";
	if (code == DEF_CODE_CONS0) return "CONS0";
	if (code==DEF_CODE_SUM) return "SUM";
	if (code==DEF_CODE_CONST) return "CONST";
	if (code== DEF_CODE_TYPECHECK) return "TYPECHECK";
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

unsigned long long RandomSeed=SEED_INIT_VALUE;

void pseudoRandomInit(void)
{
	pseudoRandomEntropy(hwTimeMs());	// clearly not enough...
}
LINT pseudoRandom32(void)
{
	unsigned long long x;
	RandomSeed= (RandomSeed * 0x5DEECE66DL + 0xBL) & 0xffffffffffff;//(one << 48) - 1);
	x = (RandomSeed) >> (48 - 32);
	return (LINT)x;
}
void pseudoRandomBytes(char* dst, LINT len)
{
    LINT x=0;
    LINT i;
	for (i = 0; i < len; i++)
	{
		if (!(i & 3)) x = pseudoRandom32();
		*(dst++) = (char)x; x >>= 8;
	}
}
void pseudoRandomEntropy(LINT val)
{
	RandomSeed = ((RandomSeed * 0x5DEECE66DL) ^ val) & 0xffffffffffff;//((1L << 48) - 1);
}

int lwEquals(LW a, int ta, LW b, int tb)
{
	if (ta!=tb) return 0;
	if (a == b) return 1;
	else if ((a != NIL) && (b != NIL) && (ta==VAL_TYPE_PNT))
	{
		LB* pa = PNT_FROM_VAL(a);
		LB* pb = PNT_FROM_VAL(b);
		if (HEADER_DBG(pa) == HEADER_DBG(pb))
		{
			if (HEADER_DBG(pa) == DBG_S)
			{
				if ((STR_LENGTH(pa) == STR_LENGTH(pb)) && (!memcmp(STR_START(pa), STR_START(pb), STR_LENGTH(pa)))) return 1;
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
	for (line = 0; line < len; line += 16)
	{
		int i;
		PRINTF(LOG_DEV,"%08x", line+offsetDisplay);
		for (i = line; i < line + 16; i++) if (i < len) PRINTF(LOG_DEV," %02x", 255 & (src[i]));
		else PRINTF(LOG_DEV,"   ");
		PRINTF(LOG_DEV,"  ");
		for (i = line; i < line + 16; i++) if (i < len) PRINTF(LOG_DEV,"%c", src[i] < 32 ? '.' : src[i]);
		PRINTF(LOG_DEV,"\n");
	}
}
