// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"
#ifdef USE_STR_C

LINT cStrlen(const char* p)
{
	LINT i = 0;
	while (*(p++)) i++;
	return i;
}
int cStrcmp(const char* p, const char* q) {
	while (1) {
		char a = *(p++);
		char b = *(q++);
		if (a < b) return -1;
		if (a > b) return 1;
		if (!b) return 0;
	}
}
void cStrcpy(char* p, const char* q)
{
	while (*q) *(p++) = *(q++);
	*p = 0;
}
void cStrncpy(char* p, const char* q, LINT n)
{
	while ((*q) && ((n--) > 0)) *(p++) = *(q++);
	while ((n--) > 0)*(p++) = 0;
}
int cMemcmp(const void* vp, const void* vq, LINT n) {
	char* p = (char*)vp;
	char* q = (char*)vq;
	while ((n--) > 0) {
		char a = *(p++);
		char b = *(q++);
		if (a < b) return -1;
		if (a > b) return 1;
	}
	return 0;
}
void cMemcpy(void* p, const void* q, LINT n)
{
	if ((((LINT)p) & 3) || (((LINT)q) & 3) || (n & 3)) {
		char* pc = (char*)p;
		char* qc = (char*)q;
		while ((n--) > 0) {
			*(pc++) = *(qc++);
		}
	}
	else {
		int* pi = (int*)p;
		int* qi = (int*)q;
		n >>= 2;
		while ((n--) > 0) {
			*(pi++) = *(qi++);
		}
	}
}
void cMemset(void* p, char val, LINT n)
{
	if ((((LINT)p) & 3) || (n & 3)) {
		char* pc = (char*)p;
		while ((n--) > 0) *(pc++) = val;
	}
	else {
		int* pi = (int*)p;
		int vi = val & 255;
		if (vi) vi |= (vi << 24) | (vi << 16) | (vi << 8);
		n >>= 2;
		while ((n--) > 0) *(pi++) = vi;
	}
}
char* cStrstr(const char* p, const char* q)
{
	LINT len = cStrlen(q);
	while (*p) {
		if (!cMemcmp(p, q, len)) return (char*)p;
		p++;
	}
	return NULL;
}
LINT myItoa(char* dst, LINT x, int zerofirst, int number)
{
	int i = 0;
	int j;
	char buffer[32];
	LUINT v = (LUINT)(x < 0 ? -x : x);

	if (number == 0) zerofirst = number = 1;
	while (v > 0) {
		LUINT c = v / 10;
		buffer[i++] = '0' + (char)(v - c * 10);
		v = c;
	}
	if (zerofirst) while (i < number) buffer[i++] = '0';
	if (x < 0) buffer[i++] = '-';
	if (!zerofirst) while (i < number) buffer[i++] = 32;
	for (j = 0; j < i; j++) dst[j] = buffer[i - 1 - j];
	dst[i] = 0;
	//	printf("myItoa %lld -> %s\n", x, dst);
	return i;
}
LINT myFtoa(char* dst, LFLOAT x, int dstLength)
{
	int n = 6;
	int i = 0;
	int j = 0;
	LINT c = 0;
	int exp = 0;
	int comma;
//	printf("\n   %g\n", x);
	dstLength--;	// save place for final zero
	if (isnan(x)) {
		strcpy(dst, "nan");
		return 3;
	}
	if (isinf(x)) {
		strcpy(dst, "inf");
		return 3;
	}

	if (x < 0) {
		x = -x;
		dst[i++] = '-';
	}
	if (x >= 1.0) {
		LFLOAT k = 1;
		while (x >= 10 * k) {
			k *= 10;
			exp++;
		}
		//now x is [10e(exp) 10e(exp+1)[
//		x += 0.000005 * k;
		comma = (exp < 6) ? exp + 1 : 1;
		for (j = 0; j < n; j++) {
			if (j == comma) dst[i++] = '.';
			c = 0;
			while (k * (c + 1) <= x) c++;
			dst[i++] = 48 + (char)c;
			x -= k * c;
			k /= 10;
		}
		while (dst[i - 1] == '0') i--;
		if (dst[i - 1] == '.') i--;

		if (exp >= 6) {
			dst[i++] = 'e';
			dst[i++] = '+';
			i += (int)myItoa(dst + i, exp, 1, 2);
			return i;
		}
	}
	else if (x == 0) {
		dst[i++] = '0';
	}
	else {
		while (x * 10 < 1) {
			exp++;
			x *= 10;
		}
		// now x is in ]0,1[
//		x += 0.0000005;
		if (exp < 4) {
			dst[i++] = '0';
			dst[i++] = '.';
			comma = -1;
			for (j = 0; j < exp; j++) dst[i++] = '0';
		}
		else comma = 1;

		for (j = 0; j < n; j++)
		{
			if (j == comma) dst[i++] = '.';
			x *= 10;
			c = 0;
			while ((c + 1) <= x) c++;
			dst[i++] = 48 + (char)c;
			x -= c;
		}
		while (dst[i - 1] == '0') i--;
		if (exp >= 4) {
			dst[i++] = 'e';
			dst[i++] = '-';
			i += (int)myItoa(dst + i, exp + 1, 1, 2);
			return i;
		}
	}
	dst[i] = 0;
	return i;
}
LINT myItox(char* dst, LUINT v, int zerofirst, int number)
{
	int i = 0;
	if (number == 0) zerofirst = number = 1;
	while (v > 0) {
		LUINT c = v & 15;
		dst[i++] = hexcharFromInt((int)c);
		v >>= 4;
	}
	if (zerofirst) while (i < number) dst[i++] = '0';
	if (!zerofirst) while (i < number) dst[i++] = 32;
	dst[i] = 0;
	return i;
}

// when dst is NULL, it sends to the uart.
LINT cVsnprintf(char* dst, LINT size, const char* format, va_list arglist)
{
	char buffer[64];
	LINT len;
	int i = 0;
	LINT out = 0;
	int loop;
	while (format[i]) {
		int zerofirst = 0;
		int number = 0;
		int countL =0;
		if (format[i] != '%') {
#ifdef USE_CONSOLE_OUT_UART
			if (!dst) uartPutChar(format[i]);
			else
#endif
			if (out < size) dst[out] = format[i];
			out++; i++;
			continue;
		}
		i++;
		loop = 1;
		while (loop) {
			char* argS;
			char argC;
			LINT argI;
			LFLOAT argF;
			int j;
			int c = format[i++];
			if (c == 0) {
				loop = 0;
			}
			else if (c == 's') {
				argS = va_arg(arglist, char*);
				len = cStrlen(argS);
#ifdef USE_CONSOLE_OUT_UART
				if (!dst) uartPut(argS, len);
				else
#endif
				if (out + len <= size) cMemcpy(&dst[out], argS, len);
				out += len;
				loop = 0;
			}
			else if (c == 'c') {
				argC = va_arg(arglist, int);
#ifdef USE_CONSOLE_OUT_UART
				if (!dst) uartPutChar(argC);
				else
#endif
				if (out < size) dst[out] = argC;
				out++;
				loop = 0;
			}
			else if (c == 'd') {
				argI = va_arg(arglist, LINT);
				if (countL<2) {
					argI&=0xffffffff;
					if (argI&0x80000000) argI|=~0xffffffff;
				}
				len = myItoa(buffer, argI, zerofirst, number);
#ifdef USE_CONSOLE_OUT_UART
				if (!dst) uartPut(buffer,len);
				else
#endif
				if (out + len < size) for (j = 0; j < len; j++) dst[out + j] = buffer[j];
				out += len;
				loop = 0;
			}
			else if ((c == 'x') || (c == 'X')) {
				argI = va_arg(arglist, LINT);
				if (countL<2) {
					argI&=0xffffffff;
					if (argI&0x80000000) argI|=~0xffffffff;
				}
				len = myItox(buffer, argI, zerofirst, number);
#ifdef USE_CONSOLE_OUT_UART
				if (!dst) for (j = 0; j < len; j++) uartPutChar(buffer[len - 1 - j]);
				else
#endif
				if (out + len < size) for (j = 0; j < len; j++) dst[out + j] = buffer[len - 1 - j];
				out += len;
				loop = 0;
			}
			else if (c == 'g') {
				argF = va_arg(arglist, double);
				len = myFtoa(buffer, argF, 64);
#ifdef USE_CONSOLE_OUT_UART
				if (!dst) uartPut(buffer,len);
				else
#endif
				if (out + len < size) for (j = 0; j < len; j++) dst[out + j] = buffer[j];
				out += len;
				loop = 0;
			}
			else if (c == 'l') {
				countL++;
			}
			else {
				if (c == '0' && !number) zerofirst = 1;
				if (c >= '0' && c <= '9') {
					number = number * 10 + c - '0';
					if (number > 30) number = 30;
				}
			}
		}
	}
#ifdef USE_CONSOLE_OUT_UART
	if (!dst) {}
	else
#endif
	if (out < size) dst[out] = 0;
	else dst[size - 1] = 0;
	return out;

}
LINT cSnprintf(char* dst, LINT size, const char* format, ...)
{
	LINT n;
	va_list arglist;
	va_start(arglist, format);
	n = cVsnprintf(dst, size, format, arglist);
	va_end(arglist);
	return n;
}
#endif
