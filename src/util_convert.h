// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _UTIL_CONVERT_
#define _UTIL_CONVERT_

int isU8(char* src, LINT len);
LINT u8Value(char* src, int* len);
LINT u8Next(char* src);
LINT u8Previous(char* src,int i);

LINT stringLengthU16(char* src,LINT len);
LINT stringLengthU8(char* src,LINT len);
LINT stringLengthLatin(char* src,LINT len);

int u16LeFromLatin(Buffer* tmp, char* src, LINT len);
int latinFromU16Le(Buffer* tmp, char* src, LINT len);

int u8FromU16Le(Buffer* tmp, char* src, LINT len);
int u16LeFromU8(Buffer* tmp, char* src, LINT len);

int u16BeFromLatin(Buffer* tmp, char* src, LINT len);
int latinFromU16Be(Buffer* tmp, char* src, LINT len);

int u8FromU16Be(Buffer* tmp, char* src, LINT len);
int u16BeFromU8(Buffer* tmp, char* src, LINT len);

int latinFromU8(Buffer* tmp, char* src, LINT len);
int u8FromLatin(Buffer* tmp, char* src, LINT len);

int strFromSource(Buffer* tmp, char* src, LINT len);
int sourceFromStr(Buffer* tmp, char* src, LINT len);

int u8FromJson(Buffer* tmp, char* src, LINT len);
int jsonFromU8(Buffer* tmp, char* src, LINT len);

int u8FromXml(Buffer* tmp, char* src, LINT len);
int latinFromXml(Buffer* tmp, char* src,LINT len);
int xmlFromStr(Buffer* tmp, char* src, LINT len);

int strWithLF(Buffer* tmp, char* src, LINT len);
int strWithCR(Buffer* tmp, char* src, LINT len);
int strWithCRLF(Buffer* tmp, char* src, LINT len);

int strSearchcase(Buffer* tmp, char* src, LINT len);
int strLowercase(Buffer* tmp, char* src, LINT len);
int strUppercase(Buffer* tmp, char* src, LINT len);
int strUnaccented(Buffer* tmp, char* src, LINT len);

int strSearchcaseU8(Buffer* tmp, char* src, LINT len);
int strLowercaseU8(Buffer* tmp, char* src, LINT len);
int strUppercaseU8(Buffer* tmp, char* src, LINT len);
int strUnaccentedU8(Buffer* tmp, char* src, LINT len);

int strSearchcaseU16Le(Buffer* tmp, char* src, LINT len);
int strLowercaseU16Le(Buffer* tmp, char* src, LINT len);
int strUppercaseU16Le(Buffer* tmp, char* src, LINT len);
int strUnaccentedU16Le(Buffer* tmp, char* src, LINT len);

int strSearchcaseU16Be(Buffer* tmp, char* src, LINT len);
int strLowercaseU16Be(Buffer* tmp, char* src, LINT len);
int strUppercaseU16Be(Buffer* tmp, char* src, LINT len);
int strUnaccentedU16Be(Buffer* tmp, char* src, LINT len);
#endif
