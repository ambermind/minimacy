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

int fun_strSwap(Thread* th)
{
	LINT i,offset;
	char* dst, * src;

	LB* a = STACKPNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	FUN_PUSH_STR( NULL, STRLEN(a));

	dst = STRSTART(STACKPNT(th, 0));
	src = STRSTART(a);
	offset = STRLEN(a) - 1;
	for (i = 0; i <= offset; i++) dst[offset - i] = src[i];
	return 0;
}

int fun_sqlFromStr(Thread* th)
{
	LINT finalsize, i;
	char* dst, * src;

	LB* a = STACKPNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	src = STRSTART(a);
	finalsize = 0;
	// 0 nul -> \z
	// 10 \n -> \n
	// 13 \r -> \r
	// 26 crtZ -> \Z
	// 34 " -> \"
	// 39 ' -> \'
	// 92 \ -> \\  //
	for (i = 0; i < STRLEN(a); i++) {
		char c = src[i];
		if ((c == 0) || (c == 10) || (c == 13) || (c == 26) || (c == 34) || (c == 39) || (c == 92)) finalsize++;
	}
//	if (!finalsize)
//	{
//		result = STACKPNT(th, 0);
//		FUN_RETURN_NIL;
//	}
	finalsize = STRLEN(a) + finalsize+2;
	FUN_PUSH_STR( NULL, finalsize);

	dst = STRSTART(STACKPNT(th, 0));
	*(dst++) = 39;
	for (i = 0; i < STRLEN(a); i++) {
		char c = src[i];
		if ((c == 0) || (c == 10) || (c == 13) || (c == 26))
		{
			*(dst++) = 92;
			if (c == 0) *(dst++) = 'z';
			else if (c == 10) *(dst++) = 'n';
			else if (c == 13) *(dst++) = 'r';
			else *(dst++) = 'Z';
		}
		else if ((c == 34) || (c == 39) || (c == 92))
		{
			*(dst++) = 92;
			*(dst++) = c;
		}
		else *(dst++) = c;
	}
	*(dst++) = 39;
	return 0;
}
int fun_urlFromStr(Thread* th)
{
	LINT finalsize, spaces, i;
	char* dst, * src;
	
	LB* a = STACKPNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	src = STRSTART(a);
	finalsize = 0;
	spaces = 0;
	for (i = 0; i < STRLEN(a); i++) if (!isletnum(src[i]))
	{
		if (src[i] == ' ') spaces++;
		else finalsize++;
	}
	if ((!spaces) && (!finalsize)) return 0;
	finalsize = STRLEN(a) + finalsize * 2;
	FUN_PUSH_STR( NULL, finalsize);

	dst = STRSTART(STACKPNT(th, 0));
	for (i = 0; i < STRLEN(a); i++) if (src[i] == ' ') *(dst++) = '+';
	else if (isletnum(src[i])) *(dst++) = src[i];
	else
	{
		*dst++ = '%';
		*dst++ = ctoh(src[i] >> 4);
		*dst++ = ctoh(src[i]);
	}
	return 0;
}

int fun_strFromUrl(Thread* th)
{
	LINT finalsize, spaces, i;
	char* dst, * src;

	LB* a = STACKPNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	src = STRSTART(a);
	finalsize = 0;
	spaces = 0;
	for (i = 0; i < STRLEN(a); i++)
		if (src[i] == '+') spaces++;
		else if ((src[i] == '%') && (i + 2 < STRLEN(a)))
		{
			finalsize++;
			i += 2;
		}
	if ((!spaces) && (!finalsize)) return 0;
	finalsize = STRLEN(a) - finalsize * 2;
	FUN_PUSH_STR( NULL, finalsize);
	dst = STRSTART(STACKPNT(th, 0));
	for (i = 0; i < STRLEN(a); i++)
		if ((src[i] == '%') && (i + 2 < STRLEN(a)))
		{
			*(dst++) = (htoc(src[i + 1]) << 4) + htoc(src[i + 2]);
			i += 2;
		}
		else if (src[i] == '+') *(dst++) = ' ';
		else *(dst++) = src[i];
	return 0;
}

int fun_strFromHex(Thread* th)
{
	LINT i, len, start, k;
	char *p, * q;

	LB* a = STACKPNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	len = STRLEN(a);
	start = len & 1;
	FUN_PUSH_STR( NULL, (len + start) >> 1);
	p = STRSTART(a);
	q = STRSTART(STACKPNT(th, 0));
	k = 0;
	for (i = start; i < len + start; i++)
	{
		LINT c = htoc(*(p++));
		k = (k << 4) + c;
		if (i & 1) *(q++) = (char)k;
	}
	return 0;
}
int fun_hexFilter(Thread* th)
{
	LINT i, len, lenFilter;
	char *p, * q;

	LB* a = STACKPNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	len = STRLEN(a);
	lenFilter = 0;
	p = STRSTART(a);
	for (i = 0; i < len; i++)
	{
		int c = p[i];
		if (
			((c >= '0') && (c <= '9')) ||
			((c >= 'A') && (c <= 'F')) ||
			((c >= 'a') && (c <= 'f'))
			) lenFilter++;
	}
	if (lenFilter == len) return 0;
	
	FUN_PUSH_STR( NULL, lenFilter);
	q = STRSTART(STACKPNT(th, 0));
	for (i = 0; i < len; i++)
	{
		int c = p[i];
		if (
			((c >= '0') && (c <= '9')) ||
			((c >= 'A') && (c <= 'F')) ||
			((c >= 'a') && (c <= 'f'))
			) *(q++)=(char)c;
	}
	return 0;
}
int fun_hexFromBin(Thread* th)
{
	LINT i, len;
	char *p, * q;

	LB* a = STACKPNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	len = STRLEN(a);
	FUN_PUSH_STR( NULL, len * 2);
	p = STRSTART(a);
	q = STRSTART(STACKPNT(th, 0));
	for (i = 0; i < len; i++)
	{
		LINT c = *(p++);
		*(q++) = (char)ctoh((int)c >> 4);
		*(q++) = (char)ctoh((int)c);
	}
	return 0;
}


int fun_floatFromStr(Thread* th)
{
	LFLOAT f;
	LB* a = STACKPNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	f = ls_atof(STRSTART(a));
	FUN_RETURN_FLOAT(f);
}
int fun_strFromFloat(Thread* th)
{
	LFLOAT g;
	LINT i;
	char buf[32];
	LFLOAT f = STACKFLOAT(th, 0);
	if (STACKISNIL(th,0)) FUN_RETURN_NIL;
	i = (LINT)f;
	g = (LFLOAT)i;
	if (g == f) snprintf(buf, 32, LSD, i);
	else snprintf(buf, 32, "%.10g", f);
	FUN_PUSH_STR( buf, strlen(buf));
	return 0;
}

int fun_intFromDec(Thread* th)
{
	LB* a = STACKPNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	FUN_RETURN_INT(ls_atoi(STRSTART(a), 0));
}
int fun_decFromInt(Thread* th)
{
	char buf[32];
	LINT a = STACKINT(th, 0);
	if (STACKISNIL(th,0)) FUN_RETURN_NIL;
	snprintf(buf, 32, LSD, a);
	FUN_PUSH_STR( buf, strlen(buf));
	return 0;
}

int fun_strFloat(Thread* th)
{
	double d;

	LFLOAT f = STACKFLOAT(th, 0);
	if (STACKISNIL(th, 0)) FUN_RETURN_NIL;
	d = (double)f;
	FUN_RETURN_STR((char*)&d, sizeof(double));
}

int fun_intFromHex(Thread* th)
{
	LB* a = STACKPNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	FUN_RETURN_INT(ls_htoi(STRSTART(a)));
}

int fun_hexFromInt(Thread* th)
{
	char buf[20];
	LINT a = STACKINT(th, 0);
	if (STACKISNIL(th,0)) FUN_RETURN_NIL;
	snprintf(buf, 20, LSX, a);
	FUN_PUSH_STR( buf, strlen(buf));
	return 0;
}

int fun_isU8(Thread* th)
{
	LB* a = STACKPNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	STACKSETBOOL(th, 0, isU8(STRSTART(a),STRLEN(a)));
	return 0;
}
int fun_strLengthU8(Thread* th)
{
	LB* a = STACKPNT(th, 0);
	FUN_RETURN_INT(a?stringLengthU8(STRSTART(a), STRLEN(a)):0);
}
int fun_strLengthU16(Thread* th)
{
	LB* a = (STACKPNT(th, 0));
	FUN_RETURN_INT(a?stringLengthU16(STRSTART(a), STRLEN(a)):0);
}


#define STR_CONVERT(name,convert,keepSame) \
int name(Thread* th) \
{ \
	int k; \
	LB* a = STACKPNT(th, 0); \
	if (!a) FUN_RETURN_NIL; \
	if (keepSame(STRSTART(a), STRLEN(a))) return 0; \
	k = convert(th, MM.tmpBuffer, STRSTART(a), STRLEN(a)); if (k == EXEC_OM) return k; \
	if (k) FUN_RETURN_NIL; \
	FUN_RETURN_BUFFER(MM.tmpBuffer);	\
}

int keepSameNever(char* src, LINT len)
{
	return 0;
}
int keepSameXmlDecode(char* src, LINT len)
{
	if (!strstr(src, "&")) return 1;
	return 0;
}
int keepSameAscii(char* src, LINT len)
{
	LINT i;
	for (i = 0; i < len; i++)
	{
		int c = src[i] & 255;
		if (c >= 128) return 0;
	}
	return 1;
}

STR_CONVERT(fun_u16LeFromLatin, u16LeFromLatin, keepSameNever)
STR_CONVERT(fun_latinFromU16Le, latinFromU16Le, keepSameNever)
STR_CONVERT(fun_u8FromU16Le, u8FromU16Le, keepSameNever)
STR_CONVERT(fun_u16LeFromU8, u16LeFromU8, keepSameNever)
STR_CONVERT(fun_u16BeFromLatin, u16BeFromLatin, keepSameNever)
STR_CONVERT(fun_latinFromU16Be, latinFromU16Be, keepSameNever)
STR_CONVERT(fun_u8FromU16Be, u8FromU16Be, keepSameNever)
STR_CONVERT(fun_u16BeFromU8, u16BeFromU8, keepSameNever)
STR_CONVERT(fun_latinFromU8, latinFromU8, keepSameAscii)
STR_CONVERT(fun_u8FromLatin, u8FromLatin, keepSameAscii)
STR_CONVERT(fun_strFromSource, strFromSource, keepSameNever)
STR_CONVERT(fun_sourceFromStr, sourceFromStr, keepSameNever)
STR_CONVERT(fun_u8FromJson, u8FromJson, keepSameNever)
STR_CONVERT(fun_jsonFromU8, jsonFromU8, keepSameNever)
STR_CONVERT(fun_u8FromXml, u8FromXml, keepSameXmlDecode)
STR_CONVERT(fun_latinFromXml, latinFromXml, keepSameXmlDecode)
STR_CONVERT(fun_xmlFromStr, xmlFromStr, keepSameNever)
STR_CONVERT(fun_strWithLF, strWithLF, keepSameNever)
STR_CONVERT(fun_strWithCR, strWithCR, keepSameNever)
STR_CONVERT(fun_strWithCRLF, strWithCRLF, keepSameNever)

STR_CONVERT(fun_strLowercase, strLowercase, keepSameNever)
STR_CONVERT(fun_strUppercase, strUppercase, keepSameNever)
STR_CONVERT(fun_strSearchcase, strSearchcase, keepSameNever)
STR_CONVERT(fun_strUnaccented, strUnaccented, keepSameNever)

STR_CONVERT(fun_strLowercaseU8, strLowercaseU8, keepSameNever)
STR_CONVERT(fun_strUppercaseU8, strUppercaseU8, keepSameNever)
STR_CONVERT(fun_strSearchcaseU8, strSearchcaseU8, keepSameNever)
STR_CONVERT(fun_strUnaccentedU8, strUnaccentedU8, keepSameNever)

STR_CONVERT(fun_strLowercaseU16Le, strLowercaseU16Le, keepSameNever)
STR_CONVERT(fun_strUppercaseU16Le, strUppercaseU16Le, keepSameNever)
STR_CONVERT(fun_strSearchcaseU16Le, strSearchcaseU16Le, keepSameNever)
STR_CONVERT(fun_strUnaccentedU16Le, strUnaccentedU16Le, keepSameNever)

STR_CONVERT(fun_strLowercaseU16Be, strLowercaseU16Be, keepSameNever)
STR_CONVERT(fun_strUppercaseU16Be, strUppercaseU16Be, keepSameNever)
STR_CONVERT(fun_strSearchcaseU16Be, strSearchcaseU16Be, keepSameNever)
STR_CONVERT(fun_strUnaccentedU16Be, strUnaccentedU16Be, keepSameNever)


int coreConvertInit(Thread* th, Pkg *system)
{
	Type* fun_S_I = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.S, MM.I);
	Type* fun_S_F = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.S, MM.F);
	Type* fun_F_S = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.F, MM.S);
	Type* fun_I_S = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.I, MM.S);
	Type* fun_S_S = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.S, MM.S);
	Type* fun_S_Bytes = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.S, MM.Bytes);
	Type* fun_S_B = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.S, MM.Boolean);

	pkgAddFun(th, system, "strSwap", fun_strSwap, fun_S_S);
	pkgAddFun(th, system, "bytesSwapStr", fun_strSwap, fun_S_Bytes);
	pkgAddFun(th, system, "sqlFromStr", fun_sqlFromStr, fun_S_S);
	pkgAddFun(th, system, "urlFromStr", fun_urlFromStr, fun_S_S);
	pkgAddFun(th, system, "strFromUrl", fun_strFromUrl, fun_S_S);

	pkgAddFun(th, system, "strFromHex", fun_strFromHex, fun_S_S);
	pkgAddFun(th, system, "hexFilter", fun_hexFilter, fun_S_S);
	pkgAddFun(th, system, "hexFromStr", fun_hexFromBin, fun_S_S);

	pkgAddFun(th, system, "intFromDec", fun_intFromDec, fun_S_I);
	pkgAddFun(th, system, "decFromInt", fun_decFromInt, fun_I_S);

	pkgAddFun(th, system, "floatFromStr", fun_floatFromStr, fun_S_F);
	pkgAddFun(th, system, "strFromFloat", fun_strFromFloat, fun_F_S);
	pkgAddFun(th, system, "strFloat", fun_strFloat, fun_F_S);

	pkgAddFun(th, system, "intFromHex", fun_intFromHex, fun_S_I);
	pkgAddFun(th, system, "hexFromInt", fun_hexFromInt, fun_I_S);

	pkgAddFun(th, system, "isU8", fun_isU8, fun_S_B);

	pkgAddFun(th, system, "strLengthU8", fun_strLengthU8, fun_S_I);
	pkgAddFun(th, system, "strLengthU16", fun_strLengthU16, fun_S_I);

	pkgAddFun(th, system, "u16LeFromLatin", fun_u16LeFromLatin, fun_S_S);
	pkgAddFun(th, system, "latinFromU16Le", fun_latinFromU16Le, fun_S_S);

	pkgAddFun(th, system, "u8FromU16Le", fun_u8FromU16Le, fun_S_S);
	pkgAddFun(th, system, "u16LeFromU8", fun_u16LeFromU8, fun_S_S);

	pkgAddFun(th, system, "u16BeFromLatin", fun_u16BeFromLatin, fun_S_S);
	pkgAddFun(th, system, "latinFromU16Be", fun_latinFromU16Be, fun_S_S);

	pkgAddFun(th, system, "u8FromU16Be", fun_u8FromU16Be, fun_S_S);
	pkgAddFun(th, system, "u16BeFromU8", fun_u16BeFromU8, fun_S_S);

	pkgAddFun(th, system, "latinFromU8", fun_latinFromU8, fun_S_S);
	pkgAddFun(th, system, "u8FromLatin", fun_u8FromLatin, fun_S_S);

	pkgAddFun(th, system, "strFromSource", fun_strFromSource, fun_S_S);
	pkgAddFun(th, system, "sourceFromStr", fun_sourceFromStr, fun_S_S);

	pkgAddFun(th, system,"u8FromJson",fun_u8FromJson,fun_S_S);
	pkgAddFun(th, system,"jsonFromU8",fun_jsonFromU8,fun_S_S);

	pkgAddFun(th, system, "latinFromXml", fun_latinFromXml, fun_S_S);
	pkgAddFun(th, system, "u8FromXml", fun_u8FromXml, fun_S_S);
	pkgAddFun(th, system, "xmlFromStr", fun_xmlFromStr, fun_S_S);

	pkgAddFun(th, system, "strWithLF", fun_strWithLF, fun_S_S);
	pkgAddFun(th, system, "strWithCR", fun_strWithCR, fun_S_S);
	pkgAddFun(th, system, "strWithCRLF", fun_strWithCRLF, fun_S_S);

	pkgAddFun(th, system, "strLowercase", fun_strLowercase, fun_S_S);
	pkgAddFun(th, system, "strUppercase", fun_strUppercase, fun_S_S);
	pkgAddFun(th, system, "strSearchcase", fun_strSearchcase, fun_S_S);
	pkgAddFun(th, system, "strUnaccented", fun_strUnaccented, fun_S_S);

	pkgAddFun(th, system, "strLowercaseU8", fun_strLowercaseU8, fun_S_S);
	pkgAddFun(th, system, "strUppercaseU8", fun_strUppercaseU8, fun_S_S);
	pkgAddFun(th, system, "strSearchcaseU8", fun_strSearchcaseU8, fun_S_S);
	pkgAddFun(th, system, "strUnaccentedU8", fun_strUnaccentedU8, fun_S_S);

	pkgAddFun(th, system, "strLowercaseU16Le", fun_strLowercaseU16Le, fun_S_S);
	pkgAddFun(th, system, "strUppercaseU16Le", fun_strUppercaseU16Le, fun_S_S);
	pkgAddFun(th, system, "strSearchcaseU16Le", fun_strSearchcaseU16Le, fun_S_S);
	pkgAddFun(th, system, "strUnaccentedU16Le", fun_strUnaccentedU16Le, fun_S_S);

	pkgAddFun(th, system, "strLowercaseU16Be", fun_strLowercaseU16Be, fun_S_S);
	pkgAddFun(th, system, "strUppercaseU16Be", fun_strUppercaseU16Be, fun_S_S);
	pkgAddFun(th, system, "strSearchcaseU16Be", fun_strSearchcaseU16Be, fun_S_S);
	pkgAddFun(th, system, "strUnaccentedU16Be", fun_strUnaccentedU16Be, fun_S_S);

	return 0;
}
