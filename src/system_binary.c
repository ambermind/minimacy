// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

int fun_strGet(Thread* th);
int fun_bytesSet(Thread* th);
int fun_strFromChar(Thread* th);

int fun_strU8Previous(Thread* th)
{
	LINT index = STACK_INT(th, 0);
	LB* p = STACK_PNT(th, 1);
	if (!p) FUN_RETURN_NIL;
	FUN_CHECK_CONTAINS(p,index, 0, STR_LENGTH(p));

	index = u8Previous(STR_START(p),(int)index);
	FUN_RETURN_INT(index)
}
int fun_strU8Next(Thread* th)
{
	LINT index = STACK_INT(th, 0);
	LB* p = STACK_PNT(th, 1);
	FUN_CHECK_CONTAINS(p, index, 1, STR_LENGTH(p));
	index += u8Next(STR_START(p) + index);
	if (index > STR_LENGTH(p)) index = STR_LENGTH(p);
	FUN_RETURN_INT(index)
}
int fun_strReadU8(Thread* th)
{
	int len;
	LINT val = 0;

	LINT index = STACK_INT(th, 0);
	LB* p = STACK_PNT(th, 1);
	FUN_CHECK_CONTAINS(p, index, 1, STR_LENGTH(p));

	val = u8Value(STR_START(p) + index, &len);
	if (val<0) FUN_RETURN_NIL;
	FUN_RETURN_INT(val)
}

int fun_strVarIntNext(Thread* th)
{
	char* str;

	LINT index = STACK_INT(th, 0);
	LB* p = STACK_PNT(th, 1);
	FUN_CHECK_CONTAINS(p, index, 1, STR_LENGTH(p));
	str = STR_START(p);
	while (str[index] & 0x80) index++;
	if (index < STR_LENGTH(p)) index++;
	FUN_RETURN_INT(index)
}
int _strReadVarInt(Thread* th, int sign)
{
	LINT val = 0;
	char* str;

	LINT index = STACK_INT(th, 0);
	LB* p = STACK_PNT(th, 1);
	FUN_CHECK_CONTAINS(p, index, 1, STR_LENGTH(p));
	str = STR_START(p);
	while (str[index] & 0x80) val = (val << 7) + (str[index++] & 0x7f);
	if (index < STR_LENGTH(p)) val = (val << 7) + (str[index++] & 0x7f);

	if (sign)
	{
		if (val & 1) val = -1 - (val >> 1);
		else val >>= 1;
	}
	FUN_RETURN_INT(val)
}
int fun_strReadVarInt(Thread* th) { return _strReadVarInt(th, 1); }
int fun_strReadVarUInt(Thread* th) { return _strReadVarInt(th, 0); }

int _strVarInt(Thread* th, int sign)
{
	unsigned long long val, tmp;
	int len = 1;
	int msk = 0;
	char* str;
	LINT v0 = STACK_PULL_INT(th);
	if (sign)
	{
		if (v0 < 0) v0 = (-v0 - 1) << 1 | 1;
		else v0 <<= 1;
	}
	val = (unsigned long long)v0;
	tmp = val;
	while (tmp >= 128) { tmp >>= 7; len++; }
	FUN_PUSH_STR( NULL, len);
	str = STR_START((STACK_PNT(th, 0)));
	len--;
	while (len >= 0)
	{
		str[len--] = (char)(msk | (val & 0x7f));
		msk = 0x80;
		val >>= 7;
	}
	return 0;
}
int fun_strVarInt(Thread* th) { return _strVarInt(th, 1); }
int fun_strVarUInt(Thread* th) { return _strVarInt(th, 0); }

int _bytesWriteVarInt(Thread* th, int sign)
{
	unsigned long long val, tmp;
	LINT len = 1;
	int msk = 0;
	LINT next;
	char* str;
	LINT v0 = STACK_PULL_INT(th);
	LINT start = STACK_PULL_INT(th);
	LB* p = STACK_PNT(th, 0);
	if (sign)
	{
		if (v0 < 0) v0 = (-v0 - 1) << 1 | 1;
		else v0 <<= 1;
	}
	val = (unsigned long long)v0;
	tmp = val;
	while (tmp >= 128) { tmp >>= 7; len++; }

	FUN_CHECK_CONTAINS(p, start, len, STR_LENGTH(p));

	next = start + len;

	len--;
	str = STR_START(p) + start;
	while (len >= 0)
	{
		str[len--] = (char)(msk | (val & 0x7f));
		msk = 0x80;
		val >>= 7;
	}
	FUN_RETURN_INT(next);
}

int fun_bytesWriteVarInt(Thread* th) { return _bytesWriteVarInt(th, 1); }
int fun_bytesWriteVarUInt(Thread* th) { return _bytesWriteVarInt(th, 0); }

int fun_bytesReadFloat(Thread* th)
{
	unsigned char* str;
	double* pf;
	LFLOAT f;
	LINT start = STACK_PULL_INT(th);
	LB* p = STACK_PNT(th, 0);
	FUN_CHECK_CONTAINS(p, start, (int)sizeof(double), STR_LENGTH(p));

	str = (unsigned char*)(STR_START(p) + start);
	pf = (double*)str;
	f =(LFLOAT)(*pf);
	FUN_RETURN_FLOAT(f);
}

#define CORE_BYTESREAD(name,type,convert) \
int name(Thread* th) {\
	char* str; LINT val; \
	LINT start = STACK_PULL_INT(th); \
	LB* p = STACK_PNT(th, 0); \
	FUN_CHECK_CONTAINS(p, start, (int)sizeof(type), STR_LENGTH(p));	\
	str = STR_START(p) + start; val = (*(type*)str); val = convert(val); \
	FUN_RETURN_INT(val); \
}

#define CORE_BYTESWRITE(name,type,convert) \
int name(Thread* th) {\
	char* str; \
	LINT val = STACK_PULL_INT(th); \
	LINT start = STACK_PULL_INT(th); \
	LB* p = STACK_PNT(th, 0); \
	FUN_CHECK_CONTAINS(p, start, (int)sizeof(type), STR_LENGTH(p));	\
	str = STR_START(p) + start; *(type*)str = (type)convert(val); \
	return 0; \
}

#define CORE_STRFROMINT(name,type,convert) \
int name(Thread* th) {\
	LINT val = STACK_PULL_INT(th); \
	FUN_PUSH_STR(NULL,sizeof(type));\
	*(type*)STR_START(STACK_PNT(th,0)) = (int)convert(val);\
	return 0;\
}

#define CORE_BYTESREAD16(name,convert) \
int name(Thread* th) {\
	unsigned char* str; LINT val; \
	LINT start = STACK_PULL_INT(th); \
	LB* p = STACK_PNT(th, 0); \
	FUN_CHECK_CONTAINS(p, start, 2, STR_LENGTH(p));	\
	str = (unsigned char*) (STR_START(p) + start); \
	val = str[1]; val<<=8; \
	val += str[0]; \
	val = convert(val); \
	FUN_RETURN_INT(val); \
}

#define CORE_BYTESWRITE16(name,convert) \
int name(Thread* th) {\
	char* str; \
	LINT val = STACK_PULL_INT(th); \
	LINT start = STACK_PULL_INT(th); \
	LB* p = STACK_PNT(th, 0); \
	FUN_CHECK_CONTAINS(p, start, 2, STR_LENGTH(p));	\
	str = STR_START(p) + start; \
	val = convert(val); \
	memcpy(str,&val,2);	/*see remark below on CORE_BYTESWRITE32 */	\
	return 0; \
}

#define CORE_BYTESREAD24(name,convert) \
int name(Thread* th) {\
	unsigned char* str; LINT val; \
	LINT start = STACK_PULL_INT(th); \
	LB* p = STACK_PNT(th, 0); \
	FUN_CHECK_CONTAINS(p, start, 3, STR_LENGTH(p));	\
	str = (unsigned char*) (STR_START(p) + start); \
	val = str[2]; val<<=8; \
	val += str[1]; val <<= 8; \
	val += str[0]; \
	val = convert(val); \
	FUN_RETURN_INT(val); \
}

#define CORE_BYTESWRITE24(name,convert) \
int name(Thread* th) {\
	char* str; \
	LINT val = STACK_PULL_INT(th); \
	LINT start = STACK_PULL_INT(th); \
	LB* p = STACK_PNT(th, 0); \
	FUN_CHECK_CONTAINS(p, start, 3, STR_LENGTH(p));	\
	str = STR_START(p) + start; \
	val = convert(val); \
	memcpy(str,&val,3);	/*see remark below on CORE_BYTESWRITE32 */\
	return 0; \
}

#define CORE_STRFROMINT24(name,convert) \
int name(Thread* th) {\
	char* str; \
	LINT val = STACK_PULL_INT(th); \
	FUN_PUSH_STR(NULL,3);\
	str=STR_START(STACK_PNT(th,0)); \
	val = convert(val); \
	*(str++) = val&255; val>>=8; \
	*(str++) = val&255; val>>=8; \
	*(str++) = val&255; val>>=8; \
	return 0;\
}

#define CORE_BYTESREAD32(name,convert) \
int name(Thread* th) {\
	unsigned char* str; LINT val; \
	LINT start = STACK_PULL_INT(th); \
	LB* p = STACK_PNT(th, 0); \
	FUN_CHECK_CONTAINS(p, start, 4, STR_LENGTH(p));	\
	str = (unsigned char*) (STR_START(p) + start); \
	val = str[3]; val<<=8; \
	val += str[2]; val <<= 8; \
	val += str[1]; val <<= 8; \
	val += str[0]; \
	val = convert(val); \
	FUN_RETURN_INT(val); \
}

// following fails, as if the compiler optimizes it as a single possibly unaligned 32 bits write operation
//	*(str++) = val&255; val>>=8;
//	*(str++) = val&255; val>>=8;
//	*(str++) = val&255; val>>=8;
//	*(str++) = val&255; val>>=8;

#define CORE_BYTESWRITE32(name,convert) \
int name(Thread* th) {\
	char* str; \
	LINT val = STACK_PULL_INT(th); \
	LINT start = STACK_PULL_INT(th); \
	LB* p = STACK_PNT(th, 0); \
	FUN_CHECK_CONTAINS(p, start, 4, STR_LENGTH(p));	\
	str = STR_START(p) + start; \
	val = convert(val); \
	memcpy(str,&val,4);	/*see above remark */	\
	return 0; \
}

#ifdef NEED_ALIGN
CORE_BYTESREAD16(fun_bytesRead16Lsb, LSBW)
CORE_BYTESREAD16(fun_bytesRead16Msb, MSBW)
CORE_BYTESREAD32(fun_bytesRead32Lsb, LSBL)
CORE_BYTESREAD32(fun_bytesRead32Msb, MSBL)

CORE_BYTESWRITE16(fun_bytesWrite16Lsb, LSBW)
CORE_BYTESWRITE16(fun_bytesWrite16Msb, MSBW)
CORE_BYTESWRITE32(fun_bytesWrite32Lsb, LSBL)
CORE_BYTESWRITE32(fun_bytesWrite32Msb, MSBL)
#else
CORE_BYTESREAD(fun_bytesRead32Lsb, int, LSBL)
CORE_BYTESREAD(fun_bytesRead32Msb, int, MSBL)
CORE_BYTESREAD(fun_bytesRead16Lsb, short, LSBW)
CORE_BYTESREAD(fun_bytesRead16Msb, short, MSBW)

CORE_BYTESWRITE(fun_bytesWrite32Lsb, int, LSBL)
CORE_BYTESWRITE(fun_bytesWrite32Msb, int, MSBL)
CORE_BYTESWRITE(fun_bytesWrite16Lsb, short, LSBW)
CORE_BYTESWRITE(fun_bytesWrite16Msb, short, MSBW)
#endif

CORE_STRFROMINT(fun_strInt32Lsb, int, LSBL)
CORE_STRFROMINT(fun_strInt32Msb, int, MSBL)
CORE_STRFROMINT(fun_strInt16Lsb, short, LSBW)
CORE_STRFROMINT(fun_strInt16Msb, short, MSBW)

CORE_BYTESREAD24(fun_bytesRead24Lsb, LSB24)
CORE_BYTESREAD24(fun_bytesRead24Msb, MSB24)

CORE_BYTESWRITE24(fun_bytesWrite24Lsb, LSB24)
CORE_BYTESWRITE24(fun_bytesWrite24Msb, MSB24)

CORE_STRFROMINT24(fun_strInt24Lsb, LSB24)
CORE_STRFROMINT24(fun_strInt24Msb, MSB24)


int systemBinaryInit(Pkg* system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "strU8Next", fun_strU8Next, "fun Str Int -> Int"},
		{ NATIVE_FUN, "strU8Previous", fun_strU8Previous, "fun Str Int -> Int"},
		{ NATIVE_FUN, "strReadU8", fun_strReadU8, "fun Str Int -> Int"},
		{ NATIVE_FUN, "strVarIntNext", fun_strVarIntNext, "fun Str Int -> Int"},
		{ NATIVE_FUN, "strVarUIntNext", fun_strVarIntNext, "fun Str Int -> Int"},	// same as strVarIntNext
		{ NATIVE_FUN, "strReadVarInt", fun_strReadVarInt, "fun Str Int -> Int"},
		{ NATIVE_FUN, "strReadVarUInt", fun_strReadVarUInt, "fun Str Int -> Int"},
		{ NATIVE_FUN, "bytesVarIntNext", fun_strVarIntNext, "fun Bytes Int -> Int"},
		{ NATIVE_FUN, "bytesVarUIntNext", fun_strVarIntNext, "fun Bytes Int -> Int"},	// same as bytesVarIntNext
		{ NATIVE_FUN, "bytesReadVarInt", fun_strReadVarInt, "fun Bytes Int -> Int"},
		{ NATIVE_FUN, "bytesReadVarUInt", fun_strReadVarUInt, "fun Bytes Int -> Int"},
		{ NATIVE_FUN, "bytesWriteVarInt", fun_bytesWriteVarInt, "fun Bytes Int Int -> Int"},
		{ NATIVE_FUN, "bytesWriteVarUInt", fun_bytesWriteVarUInt, "fun Bytes Int Int -> Int"},
		{ NATIVE_FUN, "strVarInt", fun_strVarInt, "fun Int -> Str"},
		{ NATIVE_FUN, "strVarUInt", fun_strVarUInt, "fun Int -> Str"},
		{ NATIVE_FUN, "strRead32Lsb", fun_bytesRead32Lsb, "fun Str Int -> Int"},
		{ NATIVE_FUN, "strRead32Msb", fun_bytesRead32Msb, "fun Str Int -> Int"},
		{ NATIVE_FUN, "strRead24Lsb", fun_bytesRead24Lsb, "fun Str Int -> Int"},
		{ NATIVE_FUN, "strRead24Msb", fun_bytesRead24Msb, "fun Str Int -> Int"},
		{ NATIVE_FUN, "strRead16Lsb", fun_bytesRead16Lsb, "fun Str Int -> Int"},
		{ NATIVE_FUN, "strRead16Msb", fun_bytesRead16Msb, "fun Str Int -> Int"},
		{ NATIVE_FUN, "strReadFloat", fun_bytesReadFloat, "fun Str Int -> Float"},
		{ NATIVE_FUN, "strRead8", fun_strGet, "fun Str Int -> Int"},
		{ NATIVE_FUN, "bytesRead32Lsb", fun_bytesRead32Lsb, "fun Bytes Int -> Int"},
		{ NATIVE_FUN, "bytesRead32Msb", fun_bytesRead32Msb, "fun Bytes Int -> Int"},
		{ NATIVE_FUN, "bytesRead24Lsb", fun_bytesRead24Lsb, "fun Bytes Int -> Int"},
		{ NATIVE_FUN, "bytesRead24Msb", fun_bytesRead24Msb, "fun Bytes Int -> Int"},
		{ NATIVE_FUN, "bytesRead16Lsb", fun_bytesRead16Lsb, "fun Bytes Int -> Int"},
		{ NATIVE_FUN, "bytesRead16Msb", fun_bytesRead16Msb, "fun Bytes Int -> Int"},
		{ NATIVE_FUN, "bytesReadFloat", fun_bytesReadFloat, "fun Bytes Int -> Float"},
		{ NATIVE_FUN, "bytesRead8", fun_strGet, "fun Bytes Int -> Int"},
		{ NATIVE_FUN, "bytesWrite32Lsb", fun_bytesWrite32Lsb, "fun Bytes Int Int -> Bytes"},
		{ NATIVE_FUN, "bytesWrite32Msb", fun_bytesWrite32Msb, "fun Bytes Int Int -> Bytes"},
		{ NATIVE_FUN, "bytesWrite24Lsb", fun_bytesWrite24Lsb, "fun Bytes Int Int -> Bytes"},
		{ NATIVE_FUN, "bytesWrite24Msb", fun_bytesWrite24Msb, "fun Bytes Int Int -> Bytes"},
		{ NATIVE_FUN, "bytesWrite16Lsb", fun_bytesWrite16Lsb, "fun Bytes Int Int -> Bytes"},
		{ NATIVE_FUN, "bytesWrite16Msb", fun_bytesWrite16Msb, "fun Bytes Int Int -> Bytes"},
		{ NATIVE_FUN, "bytesWrite8", fun_bytesSet, "fun Bytes Int Int -> Bytes"},
		{ NATIVE_FUN, "strInt32Lsb", fun_strInt32Lsb, "fun Int -> Str"},
		{ NATIVE_FUN, "strInt32Msb", fun_strInt32Msb, "fun Int -> Str"},
		{ NATIVE_FUN, "strInt24Lsb", fun_strInt24Lsb, "fun Int -> Str"},
		{ NATIVE_FUN, "strInt24Msb", fun_strInt24Msb, "fun Int -> Str"},
		{ NATIVE_FUN, "strInt16Lsb", fun_strInt16Lsb, "fun Int -> Str"},
		{ NATIVE_FUN, "strInt16Msb", fun_strInt16Msb, "fun Int -> Str"},
		{ NATIVE_FUN, "strInt8", fun_strFromChar, "fun Int -> Str"},
	};
	NATIVE_DEF(nativeDefs);
	return 0;
}

