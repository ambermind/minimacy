// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#ifndef _UTIL_ANSI_
#define _UTIL_ANSI_
#include"minimacy.h"

#ifdef USE_STDARG_ANSI
#include<stdarg.h>
#endif
#ifdef USE_STDARG_GCC
#define va_list __builtin_va_list
#define va_arg __builtin_va_arg
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_copy __builtin_va_copy
#endif

#ifdef USE_MATH_ANSI
#include<math.h>
#endif
#ifdef USE_MATH_C
#define floor cFloor
#define ceil cCeil
#define fabs cFabs
#define acos cAcos
#define asin cAsin
#define atan cAtan
#define atan2 cAtan2
#define cos cCos
#define cosh cCosh
#define exp cExp
#define log cLog
#define log10 cLog10
#define fmod cFmod
#define pow cPow
#define sin cSin
#define sinh cSinh
#define sqrt cSqrt
#define tan cTan
#define tanh cTanh
#define isnan cIsnan
#define isinf cIsinf
#define abs cAbs
double cFloor(double p);
double cCeil(double p);
double cFabs(double p);
double cAcos(double p);
double cAsin(double p);
double cAtan(double p);
double cAtan2(double p, double q);
double cCos(double p);
double cCosh(double p);
double cExp(double p);
double cLog(double p);
double cLog10(double p);
double cFmod(double p, double q);
double cPow(double p, double q);
double cSin(double p);
double cSinh(double p);
double cSqrt(double p);
double cTan(double p);
double cTanh(double p);
int cIsnan(double p);
int cIsinf(double p);
int cAbs(int p);
#endif

#ifdef USE_TIME_ANSI
#ifdef WIN32
#include<time.h>
#else
#include<sys/time.h>
#endif
#endif

#ifdef USE_STR_ANSI
#include<string.h>
#endif

#ifdef USE_STR_C
#define strlen cStrlen
#define strcmp cStrcmp
#define strcpy cStrcpy
#define strncpy cStrncpy
#define memcmp cMemcmp
#define memcpy cMemcpy
#define memset cMemset
#define strstr cStrstr
#ifndef vsnprintf
#define vsnprintf cVsnprintf
#endif
#define snprintf cSnprintf
LINT cStrlen(const char* p);
int cStrcmp(const char* p, const char* q);
void cStrcpy(char* p, const char* q);
void cStrncpy(char* p, const char* q, LINT n);
int cMemcmp(const void* p, const void* q, LINT n);
void cMemcpy(void* p, const void* q, LINT n);
void cMemset(void* p, char val, LINT n);
char* cStrstr(const char* p, const char* q);
LINT cVsnprintf(char* dst, LINT size, const char* format, va_list arglist);
LINT cSnprintf(char* dst, LINT size, const char* format, ...);
#endif

#ifdef USE_TYPES_ANSI
#include<sys/types.h>
#endif
#ifdef USE_TYPES_C
#ifndef NULL
#define NULL ((void*) 0)
#endif
#define size_t unsigned long long
#define uint8_t unsigned char
#define uint64_t unsigned long long
#define uint32_t unsigned long
#endif

#endif
