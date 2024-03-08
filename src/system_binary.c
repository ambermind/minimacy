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

int fun_strGet(Thread* th);
int fun_bytesSet(Thread* th);
int fun_strFromChar(Thread* th);

int fun_strU8Previous(Thread* th)
{
	LINT index = STACKINT(th, 0);
	LB* p = STACKPNT(th, 1);
	if (!p) FUN_RETURN_NIL;
	FUN_CHECK_CONTAINS(p,index, 0, STRLEN(p));

	index = u8Previous(STRSTART(p),(int)index);
	FUN_RETURN_INT(index)
}
int fun_strU8Next(Thread* th)
{
	LINT index = STACKINT(th, 0);
	LB* p = STACKPNT(th, 1);
	FUN_CHECK_CONTAINS(p, index, 1, STRLEN(p));
	index += u8Next(STRSTART(p) + index);
	if (index > STRLEN(p)) index = STRLEN(p);
	FUN_RETURN_INT(index)
}
int fun_strReadU8(Thread* th)
{
	int len;
	LINT val = 0;

	LINT index = STACKINT(th, 0);
	LB* p = STACKPNT(th, 1);
	FUN_CHECK_CONTAINS(p, index, 1, STRLEN(p));

	val = u8Value(STRSTART(p) + index, &len);
	if (val<0) FUN_RETURN_NIL;
	FUN_RETURN_INT(val)
}

int fun_strVarIntNext(Thread* th)
{
	char* str;

	LINT index = STACKINT(th, 0);
	LB* p = STACKPNT(th, 1);
	FUN_CHECK_CONTAINS(p, index, 1, STRLEN(p));
	str = STRSTART(p);
	while (str[index] & 0x80) index++;
	if (index < STRLEN(p)) index++;
	FUN_RETURN_INT(index)
}
int _strReadVarInt(Thread* th, int sign)
{
	LINT val = 0;
	char* str;

	LINT index = STACKINT(th, 0);
	LB* p = STACKPNT(th, 1);
	FUN_CHECK_CONTAINS(p, index, 1, STRLEN(p));
	str = STRSTART(p);
	while (str[index] & 0x80) val = (val << 7) + (str[index++] & 0x7f);
	if (index < STRLEN(p)) val = (val << 7) + (str[index++] & 0x7f);

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
	LINT v0 = STACKPULLINT(th);
	if (sign)
	{
		if (v0 < 0) v0 = (-v0 - 1) << 1 | 1;
		else v0 <<= 1;
	}
	val = (unsigned long long)v0;
	tmp = val;
	while (tmp >= 128) { tmp >>= 7; len++; }
	FUN_PUSH_STR( NULL, len);
	str = STRSTART((STACKPNT(th, 0)));
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
	LINT v0 = STACKPULLINT(th);
	LINT start = STACKPULLINT(th);
	LB* p = STACKPNT(th, 0);
	if (sign)
	{
		if (v0 < 0) v0 = (-v0 - 1) << 1 | 1;
		else v0 <<= 1;
	}
	val = (unsigned long long)v0;
	tmp = val;
	while (tmp >= 128) { tmp >>= 7; len++; }

	FUN_CHECK_CONTAINS(p, start, len, STRLEN(p));

	next = start + len;

	len--;
	str = STRSTART(p) + start;
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
	LFLOAT* f;
	LINT start = STACKPULLINT(th);
	LB* p = STACKPNT(th, 0);
	FUN_CHECK_CONTAINS(p, start, (int)sizeof(LFLOAT), STRLEN(p));

	str = (unsigned char*)(STRSTART(p) + start);
	f = (LFLOAT*)str;
	FUN_RETURN_FLOAT(*f);
}

#define CORE_BYTESREAD(name,type,convert) \
int name(Thread* th) {\
	char* str; LINT val; \
	LINT start = STACKPULLINT(th); \
	LB* p = STACKPNT(th, 0); \
	FUN_CHECK_CONTAINS(p, start, (int)sizeof(type), STRLEN(p));	\
	str = STRSTART(p) + start; val = (*(type*)str); val = convert(val); \
	FUN_RETURN_INT(val); \
}

#define CORE_BYTESWRITE(name,type,convert) \
int name(Thread* th) {\
	char* str; \
	LINT val = STACKPULLINT(th); \
	LINT start = STACKPULLINT(th); \
	LB* p = STACKPNT(th, 0); \
	FUN_CHECK_CONTAINS(p, start, (int)sizeof(type), STRLEN(p));	\
	str = STRSTART(p) + start; *(type*)str = (type)convert(val); \
	return 0; \
}

#define CORE_STRFROMINT(name,type,convert) \
int name(Thread* th) {\
	LINT val = STACKPULLINT(th); \
	FUN_PUSH_STR(NULL,sizeof(type));\
	*(type*)STRSTART(STACKPNT(th,0)) = (int)convert(val);\
	return 0;\
}

#define CORE_BYTESREAD16(name,convert) \
int name(Thread* th) {\
	unsigned char* str; LINT val; \
	LINT start = STACKPULLINT(th); \
	LB* p = STACKPNT(th, 0); \
	FUN_CHECK_CONTAINS(p, start, 2, STRLEN(p));	\
	str = (unsigned char*) (STRSTART(p) + start); \
	val = str[1]; val<<=8; \
	val += str[0]; \
	val = convert(val); \
	FUN_RETURN_INT(val); \
}

#define CORE_BYTESWRITE16(name,convert) \
int name(Thread* th) {\
	char* str; \
	LINT val = STACKPULLINT(th); \
	LINT start = STACKPULLINT(th); \
	LB* p = STACKPNT(th, 0); \
	FUN_CHECK_CONTAINS(p, start, 2, STRLEN(p));	\
	str = STRSTART(p) + start; \
	val = convert(val); \
	memcpy(str,&val,2);	/*see remark below on CORE_BYTESWRITE32 */	\
	return 0; \
}

#define CORE_STRFROMINT16(name,convert) \
int name(Thread* th) {\
	char* str; \
	LINT val = STACKPULLINT(th); \
	FUN_PUSH_STR(NULL,2);\
	str=STRSTART(STACKPNT(th,0)); \
	val = convert(val); \
	*(str++) = val&255; val>>=8; \
	*(str++) = val&255; val>>=8; \
	return 0;\
}

#define CORE_BYTESREAD24(name,convert) \
int name(Thread* th) {\
	unsigned char* str; LINT val; \
	LINT start = STACKPULLINT(th); \
	LB* p = STACKPNT(th, 0); \
	FUN_CHECK_CONTAINS(p, start, 3, STRLEN(p));	\
	str = (unsigned char*) (STRSTART(p) + start); \
	val = str[2]; val<<=8; \
	val += str[1]; val <<= 8; \
	val += str[0]; \
	val = convert(val); \
	FUN_RETURN_INT(val); \
}

#define CORE_BYTESWRITE24(name,convert) \
int name(Thread* th) {\
	char* str; \
	LINT val = STACKPULLINT(th); \
	LINT start = STACKPULLINT(th); \
	LB* p = STACKPNT(th, 0); \
	FUN_CHECK_CONTAINS(p, start, 3, STRLEN(p));	\
	str = STRSTART(p) + start; \
	val = convert(val); \
	memcpy(str,&val,3);	/*see remark below on CORE_BYTESWRITE32 */\
	return 0; \
}

#define CORE_STRFROMINT24(name,convert) \
int name(Thread* th) {\
	char* str; \
	LINT val = STACKPULLINT(th); \
	FUN_PUSH_STR(NULL,3);\
	str=STRSTART(STACKPNT(th,0)); \
	val = convert(val); \
	*(str++) = val&255; val>>=8; \
	*(str++) = val&255; val>>=8; \
	*(str++) = val&255; val>>=8; \
	return 0;\
}

#define CORE_BYTESREAD32(name,convert) \
int name(Thread* th) {\
	unsigned char* str; LINT val; \
	LINT start = STACKPULLINT(th); \
	LB* p = STACKPNT(th, 0); \
	FUN_CHECK_CONTAINS(p, start, 4, STRLEN(p));	\
	str = (unsigned char*) (STRSTART(p) + start); \
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
	LINT val = STACKPULLINT(th); \
	LINT start = STACKPULLINT(th); \
	LB* p = STACKPNT(th, 0); \
	FUN_CHECK_CONTAINS(p, start, 4, STRLEN(p));	\
	str = STRSTART(p) + start; \
	val = convert(val); \
	memcpy(str,&val,4);	/*see above remark */	\
	return 0; \
}

#define CORE_STRFROMINT32(name,convert) \
int name(Thread* th) {\
	char* str; \
	LINT val = STACKPULLINT(th); \
	FUN_PUSH_STR(NULL,4);\
	str=STRSTART(STACKPNT(th,0)); \
	val = convert(val); \
	*(str++) = val&255; val>>=8; \
	*(str++) = val&255; val>>=8; \
	*(str++) = val&255; val>>=8; \
	*(str++) = val&255; val>>=8; \
	return 0;\
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

CORE_STRFROMINT16(fun_strInt16Lsb, LSBW)
CORE_STRFROMINT16(fun_strInt16Msb, MSBW)
CORE_STRFROMINT32(fun_strInt32Lsb, LSBL)
CORE_STRFROMINT32(fun_strInt32Msb, MSBL)
#else
CORE_BYTESREAD(fun_bytesRead32Lsb, int, LSBL)
CORE_BYTESREAD(fun_bytesRead32Msb, int, MSBL)
CORE_BYTESREAD(fun_bytesRead16Lsb, short, LSBW)
CORE_BYTESREAD(fun_bytesRead16Msb, short, MSBW)

CORE_BYTESWRITE(fun_bytesWrite32Lsb, int, LSBL)
CORE_BYTESWRITE(fun_bytesWrite32Msb, int, MSBL)
CORE_BYTESWRITE(fun_bytesWrite16Lsb, short, LSBW)
CORE_BYTESWRITE(fun_bytesWrite16Msb, short, MSBW)

CORE_STRFROMINT(fun_strInt32Lsb, int, LSBL)
CORE_STRFROMINT(fun_strInt32Msb, int, MSBL)
CORE_STRFROMINT(fun_strInt16Lsb, short, LSBW)
CORE_STRFROMINT(fun_strInt16Msb, short, MSBW)
#endif

CORE_BYTESREAD24(fun_bytesRead24Lsb, LSB24)
CORE_BYTESREAD24(fun_bytesRead24Msb, MSB24)

CORE_BYTESWRITE24(fun_bytesWrite24Lsb, LSB24)
CORE_BYTESWRITE24(fun_bytesWrite24Msb, MSB24)

CORE_STRFROMINT24(fun_strInt24Lsb, LSB24)
CORE_STRFROMINT24(fun_strInt24Msb, MSB24)


int coreBinaryInit(Thread* th, Pkg *system)
{
	Type* fun_I_S = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.I, MM.S);
	Type* fun_S_I_I = typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.S, MM.I, MM.I);
	Type* fun_S_I_F = typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.S, MM.I, MM.F);
	Type* fun_B_I_I = typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.Bytes, MM.I, MM.I);
	Type* fun_B_I_F = typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.Bytes, MM.I, MM.F);
	Type* fun_B_I_I_I = typeAlloc(th,TYPECODE_FUN, NULL, 4, MM.Bytes, MM.I, MM.I, MM.I);
	Type* fun_B_I_I_B = typeAlloc(th,TYPECODE_FUN, NULL, 4, MM.Bytes, MM.I, MM.I, MM.Bytes);

	pkgAddFun(th, system, "strU8Next", fun_strU8Next, fun_S_I_I);
	pkgAddFun(th, system, "strU8Previous", fun_strU8Previous, fun_S_I_I);
	pkgAddFun(th, system, "strReadU8", fun_strReadU8, fun_S_I_I);

	pkgAddFun(th, system, "strVarIntNext", fun_strVarIntNext, fun_S_I_I);
	pkgAddFun(th, system, "strVarUIntNext", fun_strVarIntNext, fun_S_I_I);	// same as strVarIntNext
	pkgAddFun(th, system, "strReadVarInt", fun_strReadVarInt, fun_S_I_I);
	pkgAddFun(th, system, "strReadVarUInt", fun_strReadVarUInt, fun_S_I_I);

	pkgAddFun(th, system, "bytesVarIntNext", fun_strVarIntNext, fun_B_I_I);
	pkgAddFun(th, system, "bytesVarUIntNext", fun_strVarIntNext, fun_B_I_I);	// same as bytesVarIntNext
	pkgAddFun(th, system, "bytesReadVarInt", fun_strReadVarInt, fun_B_I_I);
	pkgAddFun(th, system, "bytesReadVarUInt", fun_strReadVarUInt, fun_B_I_I);

	pkgAddFun(th, system, "bytesWriteVarInt", fun_bytesWriteVarInt, fun_B_I_I_I);
	pkgAddFun(th, system, "bytesWriteVarUInt", fun_bytesWriteVarUInt, fun_B_I_I_I);

	pkgAddFun(th, system, "strVarInt", fun_strVarInt, fun_I_S);
	pkgAddFun(th, system, "strVarUInt", fun_strVarUInt, fun_I_S);

	pkgAddFun(th, system, "strRead32Lsb", fun_bytesRead32Lsb, fun_S_I_I);
	pkgAddFun(th, system, "strRead32Msb", fun_bytesRead32Msb, fun_S_I_I);
	pkgAddFun(th, system, "strRead24Lsb", fun_bytesRead24Lsb, fun_S_I_I);
	pkgAddFun(th, system, "strRead24Msb", fun_bytesRead24Msb, fun_S_I_I);
	pkgAddFun(th, system, "strRead16Lsb", fun_bytesRead16Lsb, fun_S_I_I);
	pkgAddFun(th, system, "strRead16Msb", fun_bytesRead16Msb, fun_S_I_I);
	pkgAddFun(th, system, "strReadFloat", fun_bytesReadFloat, fun_S_I_F);
	pkgAddFun(th, system, "strRead8", fun_strGet, fun_S_I_I);

	pkgAddFun(th, system, "bytesRead32Lsb", fun_bytesRead32Lsb, fun_B_I_I);
	pkgAddFun(th, system, "bytesRead32Msb", fun_bytesRead32Msb, fun_B_I_I);
	pkgAddFun(th, system, "bytesRead24Lsb", fun_bytesRead24Lsb, fun_B_I_I);
	pkgAddFun(th, system, "bytesRead24Msb", fun_bytesRead24Msb, fun_B_I_I);
	pkgAddFun(th, system, "bytesRead16Lsb", fun_bytesRead16Lsb, fun_B_I_I);
	pkgAddFun(th, system, "bytesRead16Msb", fun_bytesRead16Msb, fun_B_I_I);
	pkgAddFun(th, system, "bytesReadFloat", fun_bytesReadFloat, fun_B_I_F);
	pkgAddFun(th, system, "bytesRead8", fun_strGet, fun_B_I_I);

	pkgAddFun(th, system, "bytesWrite32Lsb", fun_bytesWrite32Lsb, fun_B_I_I_B);
	pkgAddFun(th, system, "bytesWrite32Msb", fun_bytesWrite32Msb, fun_B_I_I_B);
	pkgAddFun(th, system, "bytesWrite24Lsb", fun_bytesWrite24Lsb, fun_B_I_I_B);
	pkgAddFun(th, system, "bytesWrite24Msb", fun_bytesWrite24Msb, fun_B_I_I_B);
	pkgAddFun(th, system, "bytesWrite16Lsb", fun_bytesWrite16Lsb, fun_B_I_I_B);
	pkgAddFun(th, system, "bytesWrite16Msb", fun_bytesWrite16Msb, fun_B_I_I_B);
	pkgAddFun(th, system, "bytesWrite8", fun_bytesSet, fun_B_I_I_B);

	pkgAddFun(th, system, "strInt32Lsb", fun_strInt32Lsb, fun_I_S);
	pkgAddFun(th, system, "strInt32Msb", fun_strInt32Msb, fun_I_S);
	pkgAddFun(th, system, "strInt24Lsb", fun_strInt24Lsb, fun_I_S);
	pkgAddFun(th, system, "strInt24Msb", fun_strInt24Msb, fun_I_S);
	pkgAddFun(th, system, "strInt16Lsb", fun_strInt16Lsb, fun_I_S);
	pkgAddFun(th, system, "strInt16Msb", fun_strInt16Msb, fun_I_S);
	pkgAddFun(th, system, "strInt8", fun_strFromChar, fun_I_S);

	return 0;
}

