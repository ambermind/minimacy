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
#ifndef _UTIL_CONVERT_
#define _UTIL_CONVERT_

int isU8(char* src, LINT len);
LINT u8Value(char* src, int* len);
LINT u8Next(char* src);
LINT u8Previous(char* src,int i);

LINT stringLengthU16(char* src,LINT len);
LINT stringLengthU8(char* src,LINT len);
LINT stringLengthLatin(char* src,LINT len);

int u16LeFromLatin(Thread* th, Buffer* tmp, char* src, LINT len);
int latinFromU16Le(Thread* th, Buffer* tmp, char* src, LINT len);

int u8FromU16Le(Thread* th, Buffer* tmp, char* src, LINT len);
int u16LeFromU8(Thread* th, Buffer* tmp, char* src, LINT len);

int u16BeFromLatin(Thread* th, Buffer* tmp, char* src, LINT len);
int latinFromU16Be(Thread* th, Buffer* tmp, char* src, LINT len);

int u8FromU16Be(Thread* th, Buffer* tmp, char* src, LINT len);
int u16BeFromU8(Thread* th, Buffer* tmp, char* src, LINT len);

int latinFromU8(Thread* th, Buffer* tmp, char* src, LINT len);
int u8FromLatin(Thread* th, Buffer* tmp, char* src, LINT len);

int strFromSource(Thread* th, Buffer* tmp, char* src, LINT len);
int sourceFromStr(Thread* th, Buffer* tmp, char* src, LINT len);

int u8FromJson(Thread* th, Buffer* tmp, char* src, LINT len);
int jsonFromU8(Thread* th, Buffer* tmp, char* src, LINT len);

int u8FromXml(Thread* th, Buffer* tmp, char* src, LINT len);
int latinFromXml(Thread* th, Buffer* tmp, char* src,LINT len);
int xmlFromStr(Thread* th, Buffer* tmp, char* src, LINT len);

int strWithLF(Thread* th, Buffer* tmp, char* src, LINT len);
int strWithCR(Thread* th, Buffer* tmp, char* src, LINT len);
int strWithCRLF(Thread* th, Buffer* tmp, char* src, LINT len);

int strSearchcase(Thread* th, Buffer* tmp, char* src, LINT len);
int strLowercase(Thread* th, Buffer* tmp, char* src, LINT len);
int strUppercase(Thread* th, Buffer* tmp, char* src, LINT len);
int strUnaccented(Thread* th, Buffer* tmp, char* src, LINT len);

int strSearchcaseU8(Thread* th, Buffer* tmp, char* src, LINT len);
int strLowercaseU8(Thread* th, Buffer* tmp, char* src, LINT len);
int strUppercaseU8(Thread* th, Buffer* tmp, char* src, LINT len);
int strUnaccentedU8(Thread* th, Buffer* tmp, char* src, LINT len);

int strSearchcaseU16Le(Thread* th, Buffer* tmp, char* src, LINT len);
int strLowercaseU16Le(Thread* th, Buffer* tmp, char* src, LINT len);
int strUppercaseU16Le(Thread* th, Buffer* tmp, char* src, LINT len);
int strUnaccentedU16Le(Thread* th, Buffer* tmp, char* src, LINT len);

int strSearchcaseU16Be(Thread* th, Buffer* tmp, char* src, LINT len);
int strLowercaseU16Be(Thread* th, Buffer* tmp, char* src, LINT len);
int strUppercaseU16Be(Thread* th, Buffer* tmp, char* src, LINT len);
int strUnaccentedU16Be(Thread* th, Buffer* tmp, char* src, LINT len);
#endif
