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
int fun_charToStr(Thread* th);

int fun_strVarIntNext(Thread* th)
{
	char* str;
	LINT NDROP = 2 - 1;
	LW result = NIL;

	LINT index = VALTOINT(STACKGET(th, 0));
	LB* p = VALTOPNT(STACKGET(th, 1));
	if ((!p) || (index < 0) || (index >= STRLEN(p))) goto cleanup;
	str = STRSTART(p);
	while (str[index] & 0x80) index++;
	if (index < STRLEN(p)) index++;
	result = INTTOVAL(index);
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}
int _strReadVarInt(Thread* th, int sign)
{
	LINT val = 0;
	char* str;
	LINT NDROP = 2 - 1;
	LW result = NIL;

	LINT index = VALTOINT(STACKGET(th, 0));
	LB* p = VALTOPNT(STACKGET(th, 1));
	if ((!p) || (index < 0) || (index >= STRLEN(p))) goto cleanup;
	str = STRSTART(p);
	while (str[index] & 0x80) val = (val << 7) + (str[index++] & 0x7f);
	if (index < STRLEN(p)) val = (val << 7) + (str[index++] & 0x7f);

	if (sign)
	{
		if (val & 1) val = -1 - (val >> 1);
		else val >>= 1;
	}
	result = INTTOVAL(val);
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}
int fun_strReadVarInt(Thread* th) { return _strReadVarInt(th, 1); }
int fun_strReadVarUInt(Thread* th) { return _strReadVarInt(th, 0); }

int _strVarInt(Thread* th, int sign)
{
	unsigned long long val, tmp;
	int len = 1;
	int msk = 0;
	char* str;
	LINT v0 = VALTOINT(STACKPULL(th));
	if (sign)
	{
		if (v0 < 0) v0 = (-v0 - 1) << 1 | 1;
		else v0 <<= 1;
	}
	val = (unsigned long long)v0;
	tmp = val;
	while (tmp >= 128) { tmp >>= 7; len++; }
	if (stackPushStr(th, NULL, len)) return EXEC_OM;
	str = STRSTART(VALTOPNT(STACKGET(th, 0)));
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
	LINT v0 = VALTOINT(STACKPULL(th));
	LINT start = VALTOINT(STACKPULL(th));
	LB* p = VALTOPNT(STACKGET(th, 0));
	if (sign)
	{
		if (v0 < 0) v0 = (-v0 - 1) << 1 | 1;
		else v0 <<= 1;
	}
	val = (unsigned long long)v0;
	tmp = val;
	while (tmp >= 128) { tmp >>= 7; len++; }

	next = start + len;
	if ((!p) || (start < 0) || (next > STRLEN(p)))
	{
		STACKSET(th, 0, NIL);
		return 0;
	}
	len--;

	str = STRSTART(p) + start;
	while (len >= 0)
	{
		str[len--] = (char)(msk | (val & 0x7f));
		msk = 0x80;
		val >>= 7;
	}
	STACKSET(th, 0, INTTOVAL(next));
	return 0;
}

int fun_bytesWriteVarInt(Thread* th) { return _bytesWriteVarInt(th, 1); }
int fun_bytesWriteVarUInt(Thread* th) { return _bytesWriteVarInt(th, 0); }

#define CORE_BYTESREAD(name,type,convert) \
int name(Thread* th) {\
	char* str; LINT val; \
	LINT start = VALTOINT(STACKPULL(th)); \
	LB* p = VALTOPNT(STACKGET(th, 0)); \
	if ((!p) || (start < 0) || ((start + (int)sizeof(type)) > STRLEN(p))) { STACKSETNIL(th, 0); return 0; } \
	str = STRSTART(p) + start; val = (*(type*)str); val = convert(val); \
	STACKSETINT(th, 0, val); \
	return 0; \
}

#define CORE_BYTESWRITE(name,type,convert) \
int name(Thread* th) {\
	char* str; \
	LINT val = VALTOINT(STACKPULL(th)); \
	LINT start = VALTOINT(STACKPULL(th)); \
	LB* p = VALTOPNT(STACKGET(th, 0)); \
	if ((!p) || (start < 0) || ((start + (int)sizeof(type)) > STRLEN(p))) return 0; \
	str = STRSTART(p) + start; *(type*)str = (type)convert(val); \
	return 0; \
}

#define CORE_STRFROMINT(name,type,convert) \
int name(Thread* th) {\
	LINT val = VALTOINT(STACKPULL(th)); \
	if (stackPushStr(th,NULL,sizeof(type))) return EXEC_OM;\
	*(type*)STRSTART(VALTOPNT(STACKGET(th,0))) = (int)convert(val);\
	return 0;\
}

#define CORE_BYTESREAD24(name,convert) \
int name(Thread* th) {\
	unsigned char* str; LINT val; \
	LINT start = VALTOINT(STACKPULL(th)); \
	LB* p = VALTOPNT(STACKGET(th, 0)); \
	if ((!p) || (start < 0) || ((start + 3) > STRLEN(p))) { STACKSETNIL(th, 0); return 0; } \
	str = (unsigned char*) (STRSTART(p) + start); \
	val = str[2]; val<<=8; \
	val += str[1]; val <<= 8; \
	val += str[0]; \
	val = convert(val); \
	STACKSETINT(th, 0, val); \
	return 0; \
}

#define CORE_BYTESWRITE24(name,convert) \
int name(Thread* th) {\
	char* str; \
	LINT val = VALTOINT(STACKPULL(th)); \
	LINT start = VALTOINT(STACKPULL(th)); \
	LB* p = VALTOPNT(STACKGET(th, 0)); \
	if ((!p) || (start < 0) || ((start + 3) > STRLEN(p))) return 0; \
	str = STRSTART(p) + start; \
	val = convert(val); \
	*(str++) = val&255; val>>=8; \
	*(str++) = val&255; val>>=8; \
	*(str++) = val&255; val>>=8; \
	return 0; \
}

#define CORE_STRFROMINT24(name,convert) \
int name(Thread* th) {\
	char* str; \
	LINT val = VALTOINT(STACKPULL(th)); \
	if (stackPushStr(th,NULL,3)) return EXEC_OM;\
	str=STRSTART(VALTOPNT(STACKGET(th,0))); \
	val = convert(val); \
	*(str++) = val&255; val>>=8; \
	*(str++) = val&255; val>>=8; \
	*(str++) = val&255; val>>=8; \
	return 0;\
}

CORE_BYTESREAD(fun_bytesRead32Lsb, int, LSBL)
CORE_BYTESREAD(fun_bytesRead32Msb, int, MSBL)
CORE_BYTESREAD(fun_bytesRead16Lsb, short, LSBW)
CORE_BYTESREAD(fun_bytesRead16Msb, short, MSBW)

CORE_BYTESREAD24(fun_bytesRead24Lsb, LSB24)
CORE_BYTESREAD24(fun_bytesRead24Msb, MSB24)

CORE_BYTESWRITE24(fun_bytesWrite24Lsb, LSB24)
CORE_BYTESWRITE24(fun_bytesWrite24Msb, MSB24)

CORE_STRFROMINT24(fun_strInt24Lsb, LSB24)
CORE_STRFROMINT24(fun_strInt24Msb, MSB24)

CORE_BYTESWRITE(fun_bytesWrite32Lsb, int, LSBL)
CORE_BYTESWRITE(fun_bytesWrite32Msb, int, MSBL)
CORE_BYTESWRITE(fun_bytesWrite16Lsb, short, LSBW)
CORE_BYTESWRITE(fun_bytesWrite16Msb, short, MSBW)

CORE_STRFROMINT(fun_strInt32Lsb, int, LSBL)
CORE_STRFROMINT(fun_strInt32Msb, int, MSBL)
CORE_STRFROMINT(fun_strInt16Lsb, short, LSBW)
CORE_STRFROMINT(fun_strInt6Msb, short, MSBW)

int coreBinaryInit(Thread* th, Pkg *system)
{
	Type* fun_I_S = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.I, MM.S);
	Type* fun_S_I_I = typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.S, MM.I, MM.I);
	Type* fun_B_I_I = typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.Bytes, MM.I, MM.I);
	Type* fun_B_I_I_I = typeAlloc(th,TYPECODE_FUN, NULL, 4, MM.Bytes, MM.I, MM.I, MM.I);
	Type* fun_B_I_I_B = typeAlloc(th,TYPECODE_FUN, NULL, 4, MM.Bytes, MM.I, MM.I, MM.Bytes);

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
	pkgAddFun(th, system, "strRead8", fun_strGet, fun_S_I_I);

	pkgAddFun(th, system, "bytesRead32Lsb", fun_bytesRead32Lsb, fun_B_I_I);
	pkgAddFun(th, system, "bytesRead32Msb", fun_bytesRead32Msb, fun_B_I_I);
	pkgAddFun(th, system, "bytesRead24Lsb", fun_bytesRead24Lsb, fun_B_I_I);
	pkgAddFun(th, system, "bytesRead24Msb", fun_bytesRead24Msb, fun_B_I_I);
	pkgAddFun(th, system, "bytesRead16Lsb", fun_bytesRead16Lsb, fun_B_I_I);
	pkgAddFun(th, system, "bytesRead16Msb", fun_bytesRead16Msb, fun_B_I_I);
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
	pkgAddFun(th, system, "strInt16Msb", fun_strInt6Msb, fun_I_S);
	pkgAddFun(th, system, "strInt8", fun_charToStr, fun_I_S);

	return 0;
}
/* 
pour memoire dans metal v2:
 "wordextr","wordbuild","strsign","strsigni","zip",
 "unzip","strswap"
};
*/

