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
	LINT NDROP = 1 - 1;
	LW result = NIL;

	LB* a = VALTOPNT(STACKGET(th, 0));
	if (!a) goto cleanup;
	if (stackPushStr(th, NULL, STRLEN(a))) return EXEC_OM;
	NDROP++;
	result = STACKGET(th, 0);

	dst = STRSTART(VALTOPNT(result));
	src = STRSTART(a);
	offset = STRLEN(a) - 1;
	for (i = 0; i <= offset; i++) dst[offset - i] = src[i];
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}

int fun_strLowercase(Thread* th)
{
	LINT i;
	char* dst, * src;
	LINT NDROP = 1 - 1;
	LW result = NIL;

	LB* a = VALTOPNT(STACKGET(th, 0));
	if (!a) goto cleanup;
	if (stackPushStr(th, NULL, STRLEN(a))) return EXEC_OM;
	NDROP++;
	result = STACKGET(th, 0);

	dst = STRSTART(VALTOPNT(result));
	src = STRSTART(a);
	for (i = 0; i < STRLEN(a); i++)
	{
		if ((src[i] >= 'A') && (src[i] <= 'Z')) dst[i] = src[i] + 'a' - 'A';
		else dst[i] = src[i];
	}
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}

int fun_strUppercase(Thread* th)
{
	LINT i;
	char* dst, * src;
	LINT NDROP = 1 - 1;
	LW result = NIL;

	LB* a = VALTOPNT(STACKGET(th, 0));
	if (!a) goto cleanup;
	if (stackPushStr(th, NULL, STRLEN(a))) return EXEC_OM;
	NDROP++;
	result = STACKGET(th, 0);

	dst = STRSTART(VALTOPNT(result));
	src = STRSTART(a);
	for (i = 0; i < STRLEN(a); i++)
	{
		if ((src[i] >= 'a') && (src[i] <= 'z')) dst[i] = src[i] + 'A' - 'a';
		else dst[i] = src[i];
	}
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}

int fun_strToSql(Thread* th)
{
	LINT finalsize, i;
	char* dst, * src;
	LINT NDROP = 1 - 1;
	LW result = NIL;

	LB* a = VALTOPNT(STACKGET(th, 0));
	if (!a) goto cleanup;
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
	if (!finalsize)
	{
		result = STACKGET(th, 0);
		goto cleanup;
	}
	finalsize = STRLEN(a) + finalsize+2;
	if (stackPushStr(th, NULL, finalsize)) return EXEC_OM;
	NDROP++;
	result = STACKGET(th, 0);

	dst = STRSTART(VALTOPNT(result));
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
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}
int fun_strToUrl(Thread* th)
{
	LINT finalsize, spaces, i;
	char* dst, * src;
	LINT NDROP = 1 - 1;
	LW result = NIL;

	LB* a = VALTOPNT(STACKGET(th, 0));
	if (!a) goto cleanup;
	src = STRSTART(a);
	finalsize = 0;
	spaces = 0;
	for (i = 0; i < STRLEN(a); i++) if (!isletnum(src[i]))
	{
		if (src[i] == ' ') spaces++;
		else finalsize++;
	}
	if ((!spaces) && (!finalsize))
	{
		result = STACKGET(th, 0);
		goto cleanup;
	}
	finalsize = STRLEN(a) + finalsize * 2;
	if (stackPushStr(th, NULL, finalsize)) return EXEC_OM;
	NDROP++;
	result = STACKGET(th, 0);

	dst = STRSTART(VALTOPNT(result));
	for (i = 0; i < STRLEN(a); i++) if (src[i] == ' ') *(dst++) = '+';
	else if (isletnum(src[i])) *(dst++) = src[i];
	else
	{
		*dst++ = '%';
		*dst++ = ctoh(src[i] >> 4);
		*dst++ = ctoh(src[i]);
	}
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}

int fun_urlToStr(Thread* th)
{
	LINT finalsize, spaces, i;
	char* dst, * src;
	LINT NDROP = 1 - 1;
	LW result = NIL;

	LB* a = VALTOPNT(STACKGET(th, 0));
	if (!a) goto cleanup;
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
	if ((!spaces) && (!finalsize))
	{
		result = STACKGET(th, 0);
		goto cleanup;
	}
	finalsize = STRLEN(a) - finalsize * 2;
	if (stackPushStr(th, NULL, finalsize)) return EXEC_OM;
	NDROP++;
	result = STACKGET(th, 0);

	dst = STRSTART(VALTOPNT(result));
	for (i = 0; i < STRLEN(a); i++)
		if ((src[i] == '%') && (i + 2 < STRLEN(a)))
		{
			*(dst++) = (htoc(src[i + 1]) << 4) + htoc(src[i + 2]);
			i += 2;
		}
		else if (src[i] == '+') *(dst++) = ' ';
		else *(dst++) = src[i];
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}

int fun_hexToBin(Thread* th)
{
	LINT i, len, start, k;
	char* p, * q;
	LINT NDROP = 1 - 1;
	LW result = NIL;
	LB* a = VALTOPNT(STACKGET(th, 0));
	if (!a) goto cleanup;
	len = STRLEN(a);
	start = len & 1;
	if (stackPushStr(th, NULL, (len + start) >> 1)) return EXEC_OM;
	NDROP++;
	result = STACKGET(th, 0);
	p = STRSTART(a);
	q = STRSTART(VALTOPNT(result));
	k = 0;
	for (i = start; i < len + start; i++)
	{
		LINT c = htoc(*(p++));
		k = (k << 4) + c;
		if (i & 1) *(q++) = (char)k;
	}
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}
int fun_hexFilter(Thread* th)
{
	LINT i, len, lenFilter;
	char* p, * q;
	LINT NDROP = 1 - 1;
	LW result = NIL;
	LB* a = VALTOPNT(STACKGET(th, 0));
	if (!a) goto cleanup;
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
	if (lenFilter == len)
	{
		result = STACKGET(th, 0);
		goto cleanup;
	}
	
	if (stackPushStr(th, NULL, lenFilter)) return EXEC_OM;
	NDROP++;
	result = STACKGET(th, 0);
	q = STRSTART(VALTOPNT(result));
	for (i = 0; i < len; i++)
	{
		int c = p[i];
		if (
			((c >= '0') && (c <= '9')) ||
			((c >= 'A') && (c <= 'F')) ||
			((c >= 'a') && (c <= 'f'))
			) *(q++)=(char)c;
	}
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}
int fun_binToHex(Thread* th)
{
	LINT i, len;
	char* p, * q;
	LINT NDROP = 1 - 1;
	LW result = NIL;
	LB* a = VALTOPNT(STACKGET(th, 0));
	if (!a) goto cleanup;
	len = STRLEN(a);
	if (stackPushStr(th, NULL, len * 2)) return EXEC_OM;
	NDROP++;
	result = STACKGET(th, 0);
	p = STRSTART(a);
	q = STRSTART(VALTOPNT(result));
	for (i = 0; i < len; i++)
	{
		LINT c = *(p++);
		*(q++) = (char)ctoh((int)c >> 4);
		*(q++) = (char)ctoh((int)c);
	}
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}


int fun_strToFloat(Thread* th)
{
	LFLOAT f;
	LB* a = VALTOPNT(STACKGET(th, 0));
	if (!a) return 0;
	f = ls_atof(STRSTART(a));
	STACKSETFLOAT(th, 0, (f));
	return 0;
}
int fun_floatToStr(Thread* th)
{
	char buf[32];
	LW va = STACKGET(th, 0);
	if (va != NIL)
	{
		LFLOAT f = VALTOFLOAT(va);
		LINT i = (LINT)f;
		LFLOAT g = (LFLOAT)i;
		if (g == f) sprintf(buf, LSD, i);
		else sprintf(buf, "%g", VALTOFLOAT(va));
	}
	else strcpy(buf, "");
	STACKDROP(th);
	if (stackPushStr(th, buf, strlen(buf))) return EXEC_OM;
	return 0;
}

int fun_decToInt(Thread* th)
{
	LB* a = VALTOPNT(STACKGET(th, 0));
	if (a) STACKSETINT(th, 0, (ls_atoi(STRSTART(a), 0)));
	return 0;
}
int fun_intToDec(Thread* th)
{
	char buf[32];
	LW va = STACKGET(th, 0);
	if (va != NIL) sprintf(buf, LSD, VALTOINT(va));
	else strcpy(buf, "");
	STACKDROP(th);
	if (stackPushStr(th, buf, strlen(buf))) return EXEC_OM;
	return 0;
}

int fun_floatToBin(Thread* th)
{
	LB* p;
	LFLOAT f;
	double d;
	LINT NDROP = 1 - 1;
	LW result = NIL;

	LW va = STACKGET(th, 0);
	if (va == NIL) goto cleanup;
	f = VALTOFLOAT(va);
	d = (double)f;
	p = memoryAllocStr(th,(char*)&d, sizeof(double)); if (!p) return EXEC_OM;
	result = PNTTOVAL(p);
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}

int fun_binToFloat(Thread* th)
{
	LFLOAT f;
	double d;
	LB* a = VALTOPNT(STACKGET(th, 0));
	if ((!a) || (STRLEN(a) != sizeof(double)))
	{
		STACKSETNIL(th, 0);
		return 0;
	}
	d = *((double*)STRSTART(a));
	f = (float)d;
	STACKSETFLOAT(th, 0, (f));
	return 0;
}

int fun_hexToInt(Thread* th)
{
	LB* a = VALTOPNT(STACKGET(th, 0));
	if (a) STACKSETINT(th, 0, (ls_htoi(STRSTART(a))));
	return 0;
}

int fun_intToHex(Thread* th)
{
	char buf[20];
	LW va = STACKGET(th, 0);
	if (va != NIL) sprintf(buf, LSX, VALTOINT(va));
	else strcpy(buf, "");
	STACKDROP(th);
	if (stackPushStr(th, buf, strlen(buf))) return EXEC_OM;
	return 0;
}

int fun_isU8(Thread* th)
{
	LB* a = VALTOPNT(STACKGET(th, 0));
	if (!a) return 0;
	
	STACKSET(th, 0, isU8(STRSTART(a),STRLEN(a))?MM.trueRef:MM.falseRef);
	return 0;
}
int fun_strLengthU8(Thread* th)
{
	LB* a = VALTOPNT(STACKGET(th, 0));
	if (!a) STACKSETINT(th, 0, (0));
	else STACKSETINT(th, 0, (stringLengthU8(STRSTART(a), STRLEN(a))));
	return 0;
}
int fun_strLengthU16(Thread* th)
{
	LB* a = VALTOPNT(STACKGET(th, 0));
	if (!a) STACKSETINT(th, 0, (0));
	else STACKSETINT(th, 0, (stringLengthU16(STRSTART(a), STRLEN(a))));
	return 0;
}


#define STR_CONVERT(name,convert,keepSame) \
int name(Thread* th) \
{ \
	int k; \
	LB* p; \
	LINT NDROP = 1 - 1; \
	LW result = NIL; \
	LB* a = VALTOPNT(STACKGET(th, 0)); \
	if (!a) goto cleanup; \
	if (keepSame(STRSTART(a), STRLEN(a))) return 0; \
	k = convert(th, MM.tmpBuffer, STRSTART(a), STRLEN(a)); if (k == EXEC_OM) return k; \
	if (k) goto cleanup; \
	p = memoryAllocFromBuffer(th, MM.tmpBuffer); if (!p) return EXEC_OM; \
	result = PNTTOVAL(p); \
cleanup: \
	STACKSET(th, NDROP, result); \
	STACKDROPN(th, NDROP); \
	return 0; \
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

STR_CONVERT(fun_latinToU16, latinToU16, keepSameNever)
STR_CONVERT(fun_u16ToLatin, u16ToLatin, keepSameNever)
STR_CONVERT(fun_u16ToU8, u16ToU8, keepSameNever)
STR_CONVERT(fun_u8ToU16, u8ToU16, keepSameNever)
STR_CONVERT(fun_u8ToLatin, u8ToLatin, keepSameAscii)
STR_CONVERT(fun_latinToU8, latinToU8, keepSameAscii)
STR_CONVERT(fun_sourceToStr, sourceToStr, keepSameNever)
STR_CONVERT(fun_strToSource, strToSource, keepSameNever)
STR_CONVERT(fun_jsonToU8, jsonToU8, keepSameNever)
STR_CONVERT(fun_u8ToJson, u8ToJson, keepSameNever)
STR_CONVERT(fun_xmlToU8, xmlToU8, keepSameXmlDecode)
STR_CONVERT(fun_xmlToLatin, xmlToLatin, keepSameXmlDecode)
STR_CONVERT(fun_strToXml, strToXml, keepSameNever)
STR_CONVERT(fun_strToLF, strToLF, keepSameNever)
STR_CONVERT(fun_strToCR, strToCR, keepSameNever)
STR_CONVERT(fun_strToCRLF, strToCRLF, keepSameNever)


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
	pkgAddFun(th, system, "bytesSwapFromStr", fun_strSwap, fun_S_Bytes);
	pkgAddFun(th, system, "strLowercase", fun_strLowercase, fun_S_S);
	pkgAddFun(th, system, "strUppercase", fun_strUppercase, fun_S_S);
	pkgAddFun(th, system, "strToSql", fun_strToSql, fun_S_S);
	pkgAddFun(th, system, "strToUrl", fun_strToUrl, fun_S_S);
	pkgAddFun(th, system, "urlToStr", fun_urlToStr, fun_S_S);

	pkgAddFun(th, system, "hexToBin", fun_hexToBin, fun_S_S);
	pkgAddFun(th, system, "hexFilter", fun_hexFilter, fun_S_S);
	pkgAddFun(th, system, "binToHex", fun_binToHex, fun_S_S);

	pkgAddFun(th, system, "decToInt", fun_decToInt, fun_S_I);
	pkgAddFun(th, system, "intToDec", fun_intToDec, fun_I_S);

	pkgAddFun(th, system, "strToFloat", fun_strToFloat, fun_S_F);
	pkgAddFun(th, system, "floatToStr", fun_floatToStr, fun_F_S);
	pkgAddFun(th, system, "binToFloat", fun_binToFloat, fun_S_F);
	pkgAddFun(th, system, "floatToBin", fun_floatToBin, fun_F_S);

	pkgAddFun(th, system, "hexToInt", fun_hexToInt, fun_S_I);
	pkgAddFun(th, system, "intToHex", fun_intToHex, fun_I_S);

	pkgAddFun(th, system, "isU8", fun_isU8, fun_S_B);

	pkgAddFun(th, system, "strLengthU8", fun_strLengthU8, fun_S_I);
	pkgAddFun(th, system, "strLengthU16", fun_strLengthU16, fun_S_I);

	pkgAddFun(th, system, "latinToU16", fun_latinToU16, fun_S_S);
	pkgAddFun(th, system, "u16ToLatin", fun_u16ToLatin, fun_S_S);

	pkgAddFun(th, system, "u16ToU8", fun_u16ToU8, fun_S_S);
	pkgAddFun(th, system, "u8ToU16", fun_u8ToU16, fun_S_S);

	pkgAddFun(th, system, "u8ToLatin", fun_u8ToLatin, fun_S_S);
	pkgAddFun(th, system, "latinToU8", fun_latinToU8, fun_S_S);

	pkgAddFun(th, system, "sourceToStr", fun_sourceToStr, fun_S_S);
	pkgAddFun(th, system, "strToSource", fun_strToSource, fun_S_S);

	pkgAddFun(th, system,"jsonToU8",fun_jsonToU8,fun_S_S);
	pkgAddFun(th, system,"u8ToJson",fun_u8ToJson,fun_S_S);

	pkgAddFun(th, system, "xmlToLatin", fun_xmlToLatin, fun_S_S);
	pkgAddFun(th, system, "xmlToU8", fun_xmlToU8, fun_S_S);
	pkgAddFun(th, system, "strToXml", fun_strToXml, fun_S_S);

	pkgAddFun(th, system, "strToLF", fun_strToLF, fun_S_S);
	pkgAddFun(th, system, "strToCR", fun_strToCR, fun_S_S);
	pkgAddFun(th, system, "strToCRLF", fun_strToCRLF, fun_S_S);

	return 0;
}
