// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

int fun_strSwap(Thread* th)
{
	LINT i,offset;
	char* dst, * src;

	LB* a = STACK_PNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	FUN_PUSH_STR( NULL, STR_LENGTH(a));

	dst = STR_START(STACK_PNT(th, 0));
	src = STR_START(a);
	offset = STR_LENGTH(a) - 1;
	for (i = 0; i <= offset; i++) dst[offset - i] = src[i];
	return 0;
}

int fun_sqlFromStr(Thread* th)
{
	LINT finalsize, i;
	char* dst, * src;

	LB* a = STACK_PNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	src = STR_START(a);
	finalsize = 0;
	// 0 nul -> \z
	// 10 \n -> \n
	// 13 \r -> \r
	// 26 crtZ -> \Z
	// 34 " -> \"
	// 39 ' -> \'
	// 92 \ -> \\  //
	for (i = 0; i < STR_LENGTH(a); i++) {
		char c = src[i];
		if ((c == 0) || (c == 10) || (c == 13) || (c == 26) || (c == 34) || (c == 39) || (c == 92)) finalsize++;
	}
//	if (!finalsize)
//	{
//		result = STACK_PNT(th, 0);
//		FUN_RETURN_NIL;
//	}
	finalsize = STR_LENGTH(a) + finalsize+2;
	FUN_PUSH_STR( NULL, finalsize);

	dst = STR_START(STACK_PNT(th, 0));
	*(dst++) = 39;
	for (i = 0; i < STR_LENGTH(a); i++) {
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
	
	LB* a = STACK_PNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	src = STR_START(a);
	finalsize = 0;
	spaces = 0;
	for (i = 0; i < STR_LENGTH(a); i++) if (!isLetterOrNumber(src[i]))
	{
		if (src[i] == ' ') spaces++;
		else finalsize++;
	}
	if ((!spaces) && (!finalsize)) return 0;
	finalsize = STR_LENGTH(a) + finalsize * 2;
	FUN_PUSH_STR( NULL, finalsize);

	dst = STR_START(STACK_PNT(th, 0));
	for (i = 0; i < STR_LENGTH(a); i++) if (src[i] == ' ') *(dst++) = '+';
	else if (isLetterOrNumber(src[i])) *(dst++) = src[i];
	else
	{
		*dst++ = '%';
		*dst++ = hexcharFromInt(src[i] >> 4);
		*dst++ = hexcharFromInt(src[i]);
	}
	return 0;
}

int fun_strFromUrl(Thread* th)
{
	LINT finalsize, spaces, i;
	char* dst, * src;

	LB* a = STACK_PNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	src = STR_START(a);
	finalsize = 0;
	spaces = 0;
	for (i = 0; i < STR_LENGTH(a); i++)
		if (src[i] == '+') spaces++;
		else if ((src[i] == '%') && (i + 2 < STR_LENGTH(a)))
		{
			finalsize++;
			i += 2;
		}
	if ((!spaces) && (!finalsize)) return 0;
	finalsize = STR_LENGTH(a) - finalsize * 2;
	FUN_PUSH_STR( NULL, finalsize);
	dst = STR_START(STACK_PNT(th, 0));
	for (i = 0; i < STR_LENGTH(a); i++)
		if ((src[i] == '%') && (i + 2 < STR_LENGTH(a)))
		{
			*(dst++) = (intFromHexchar(src[i + 1]) << 4) + intFromHexchar(src[i + 2]);
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

	LB* a = STACK_PNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	len = STR_LENGTH(a);
	start = len & 1;
	FUN_PUSH_STR( NULL, (len + start) >> 1);
	p = STR_START(a);
	q = STR_START(STACK_PNT(th, 0));
	k = 0;
	for (i = start; i < len + start; i++)
	{
		LINT c = intFromHexchar(*(p++));
		k = (k << 4) + c;
		if (i & 1) *(q++) = (char)k;
	}
	return 0;
}
int fun_isHex(Thread* th)
{
	LINT i, len;
	char *p;

	LB* a = STACK_PNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	len = STR_LENGTH(a);
	p = STR_START(a);
	for (i = 0; i < len; i++)
	{
		if (!isHexchar(p[i])) FUN_RETURN_FALSE;
	}
	FUN_RETURN_TRUE
}
int fun_hexFilter(Thread* th)
{
	LINT i, len, lenFilter;
	char *p, * q;

	LB* a = STACK_PNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	len = STR_LENGTH(a);
	lenFilter = 0;
	p = STR_START(a);
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
	q = STR_START(STACK_PNT(th, 0));
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
int fun_hexFromStr(Thread* th)
{
	LINT i, len;
	char *p, * q;

	LB* a = STACK_PNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	len = STR_LENGTH(a);
	FUN_PUSH_STR( NULL, len * 2);
	p = STR_START(a);
	q = STR_START(STACK_PNT(th, 0));
	for (i = 0; i < len; i++)
	{
		LINT c = *(p++);
		*(q++) = (char)hexcharFromInt((int)c >> 4);
		*(q++) = (char)hexcharFromInt((int)c);
	}
	return 0;
}


int fun_floatFromStr(Thread* th)
{
	LFLOAT f;
	LB* a = STACK_PNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	f = floatFromAsc(STR_START(a));
	FUN_RETURN_FLOAT(f);
}
int fun_strFromFloat(Thread* th)
{
	LFLOAT g;
	LINT i;
	char buf[32];
	LFLOAT f = STACK_FLOAT(th, 0);
	if (STACK_IS_NIL(th,0)) FUN_RETURN_NIL;
	i = (LINT)f;
	g = (LFLOAT)i;
	if (g == f) snprintf(buf, 32, LSD, i);
	else snprintf(buf, 32, "%.10g", f);
	FUN_PUSH_STR( buf, strlen(buf));
	return 0;
}

int fun_intFromDec(Thread* th)
{
	LB* a = STACK_PNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	FUN_RETURN_INT(intFromAsc(STR_START(a), 0));
}
int fun_decFromInt(Thread* th)
{
	char buf[32];
	LINT a = STACK_INT(th, 0);
	if (STACK_IS_NIL(th,0)) FUN_RETURN_NIL;
	snprintf(buf, 32, LSD, a);
	FUN_PUSH_STR( buf, strlen(buf));
	return 0;
}

int fun_strFloat(Thread* th)
{
	double d;

	LFLOAT f = STACK_FLOAT(th, 0);
	if (STACK_IS_NIL(th, 0)) FUN_RETURN_NIL;
	d = (double)f;
	FUN_RETURN_STR((char*)&d, sizeof(double));
}

int fun_intFromHex(Thread* th)
{
	LB* a = STACK_PNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	FUN_RETURN_INT(intFromHex(STR_START(a)));
}

int fun_hexFromInt(Thread* th)
{
	char buf[20];
	LINT a = STACK_INT(th, 0);
	if (STACK_IS_NIL(th,0)) FUN_RETURN_NIL;
	snprintf(buf, 20, "%x", (int)a);
	FUN_PUSH_STR( buf, strlen(buf));
	return 0;
}

int fun_isU8(Thread* th)
{
	LB* a = STACK_PNT(th, 0);
	if (!a) FUN_RETURN_NIL;
	STACK_SET_BOOL(th, 0, isU8(STR_START(a),STR_LENGTH(a)));
	return 0;
}
int fun_strLengthU8(Thread* th)
{
	LB* a = STACK_PNT(th, 0);
	FUN_RETURN_INT(a?stringLengthU8(STR_START(a), STR_LENGTH(a)):0);
}
int fun_strLengthU16(Thread* th)
{
	LB* a = (STACK_PNT(th, 0));
	FUN_RETURN_INT(a?stringLengthU16(STR_START(a), STR_LENGTH(a)):0);
}


#define STR_CONVERT(name,convert,keepSame) \
int name(Thread* th) \
{ \
	int k; \
	LB* a = STACK_PNT(th, 0); \
	if (!a) FUN_RETURN_NIL; \
	if (keepSame(STR_START(a), STR_LENGTH(a))) return 0; \
	k = convert(MM.tmpBuffer, STR_START(a), STR_LENGTH(a)); if (k == EXEC_OM) return k; \
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


int systemConvertInit(Pkg *system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "isHex", fun_isHex, "fun Str -> Bool"},
		{ NATIVE_FUN, "isU8", fun_isU8, "fun Str -> Bool"},
		{ NATIVE_FUN, "strSwap", fun_strSwap, "fun Str -> Str"},
		{ NATIVE_FUN, "bytesSwapStr", fun_strSwap, "fun Str -> Bytes"},
		{ NATIVE_FUN, "sqlFromStr", fun_sqlFromStr, "fun Str -> Str"},
		{ NATIVE_FUN, "urlFromStr", fun_urlFromStr, "fun Str -> Str"},
		{ NATIVE_FUN, "strFromUrl", fun_strFromUrl, "fun Str -> Str"},
		{ NATIVE_FUN, "strFromHex", fun_strFromHex, "fun Str -> Str"},
		{ NATIVE_FUN, "hexFilter", fun_hexFilter, "fun Str -> Str"},
		{ NATIVE_FUN, "hexFromStr", fun_hexFromStr, "fun Str -> Str"},
		{ NATIVE_FUN, "intFromDec", fun_intFromDec, "fun Str -> Int"},
		{ NATIVE_FUN, "intFromStr", fun_intFromDec, "fun Str -> Int"},
		{ NATIVE_FUN, "decFromInt", fun_decFromInt, "fun Int -> Str"},
		{ NATIVE_FUN, "strFromInt", fun_decFromInt, "fun Int -> Str"},
		{ NATIVE_FUN, "floatFromStr", fun_floatFromStr, "fun Str -> Float"},
		{ NATIVE_FUN, "strFromFloat", fun_strFromFloat, "fun Float -> Str"},
		{ NATIVE_FUN, "strFloat", fun_strFloat, "fun Float -> Str"},
		{ NATIVE_FUN, "intFromHex", fun_intFromHex, "fun Str -> Int"},
		{ NATIVE_FUN, "hexFromInt", fun_hexFromInt, "fun Int -> Str"},
		{ NATIVE_FUN, "strLengthU8", fun_strLengthU8, "fun Str -> Int"},
		{ NATIVE_FUN, "strLengthU16", fun_strLengthU16, "fun Str -> Int"},
		{ NATIVE_FUN, "u16LeFromLatin", fun_u16LeFromLatin, "fun Str -> Str"},
		{ NATIVE_FUN, "latinFromU16Le", fun_latinFromU16Le, "fun Str -> Str"},
		{ NATIVE_FUN, "u8FromU16Le", fun_u8FromU16Le, "fun Str -> Str"},
		{ NATIVE_FUN, "u16LeFromU8", fun_u16LeFromU8, "fun Str -> Str"},
		{ NATIVE_FUN, "u16BeFromLatin", fun_u16BeFromLatin, "fun Str -> Str"},
		{ NATIVE_FUN, "latinFromU16Be", fun_latinFromU16Be, "fun Str -> Str"},
		{ NATIVE_FUN, "u8FromU16Be", fun_u8FromU16Be, "fun Str -> Str"},
		{ NATIVE_FUN, "u16BeFromU8", fun_u16BeFromU8, "fun Str -> Str"},
		{ NATIVE_FUN, "latinFromU8", fun_latinFromU8, "fun Str -> Str"},
		{ NATIVE_FUN, "u8FromLatin", fun_u8FromLatin, "fun Str -> Str"},
		{ NATIVE_FUN, "strFromSource", fun_strFromSource, "fun Str -> Str"},
		{ NATIVE_FUN, "sourceFromStr", fun_sourceFromStr, "fun Str -> Str"},
		{ NATIVE_FUN, "u8FromJson", fun_u8FromJson, "fun Str -> Str"},
		{ NATIVE_FUN, "jsonFromU8", fun_jsonFromU8, "fun Str -> Str"},
		{ NATIVE_FUN, "latinFromXml", fun_latinFromXml, "fun Str -> Str"},
		{ NATIVE_FUN, "u8FromXml", fun_u8FromXml, "fun Str -> Str"},
		{ NATIVE_FUN, "xmlFromStr", fun_xmlFromStr, "fun Str -> Str"},
		{ NATIVE_FUN, "strWithLF", fun_strWithLF, "fun Str -> Str"},
		{ NATIVE_FUN, "strWithCR", fun_strWithCR, "fun Str -> Str"},
		{ NATIVE_FUN, "strWithCRLF", fun_strWithCRLF, "fun Str -> Str"},
		{ NATIVE_FUN, "strLowercase", fun_strLowercase, "fun Str -> Str"},
		{ NATIVE_FUN, "strUppercase", fun_strUppercase, "fun Str -> Str"},
		{ NATIVE_FUN, "strSearchcase", fun_strSearchcase, "fun Str -> Str"},
		{ NATIVE_FUN, "strUnaccented", fun_strUnaccented, "fun Str -> Str"},
		{ NATIVE_FUN, "strLowercaseU8", fun_strLowercaseU8, "fun Str -> Str"},
		{ NATIVE_FUN, "strUppercaseU8", fun_strUppercaseU8, "fun Str -> Str"},
		{ NATIVE_FUN, "strSearchcaseU8", fun_strSearchcaseU8, "fun Str -> Str"},
		{ NATIVE_FUN, "strUnaccentedU8", fun_strUnaccentedU8, "fun Str -> Str"},
		{ NATIVE_FUN, "strLowercaseU16Le", fun_strLowercaseU16Le, "fun Str -> Str"},
		{ NATIVE_FUN, "strUppercaseU16Le", fun_strUppercaseU16Le, "fun Str -> Str"},
		{ NATIVE_FUN, "strSearchcaseU16Le", fun_strSearchcaseU16Le, "fun Str -> Str"},
		{ NATIVE_FUN, "strUnaccentedU16Le", fun_strUnaccentedU16Le, "fun Str -> Str"},
		{ NATIVE_FUN, "strLowercaseU16Be", fun_strLowercaseU16Be, "fun Str -> Str"},
		{ NATIVE_FUN, "strUppercaseU16Be", fun_strUppercaseU16Be, "fun Str -> Str"},
		{ NATIVE_FUN, "strSearchcaseU16Be", fun_strSearchcaseU16Be, "fun Str -> Str"},
		{ NATIVE_FUN, "strUnaccentedU16Be", fun_strUnaccentedU16Be, "fun Str -> Str"},
	};
	NATIVE_DEF(nativeDefs);
	return 0;
}
