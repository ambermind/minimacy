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

LINT stringLengthU16(char* src,LINT len);
LINT stringLengthU8(char* src,LINT len);
LINT stringLengthLatin(char* src,LINT len);

int latinToU16(Thread* th, Buffer* tmp, char* src, LINT len);
int u16ToLatin(Thread* th, Buffer* tmp, char* src, LINT len);

int u16ToU8(Thread* th, Buffer* tmp, char* src, LINT len);
int u8ToU16(Thread* th, Buffer* tmp, char* src, LINT len);

int u8ToLatin(Thread* th, Buffer* tmp, char* src, LINT len);
int latinToU8(Thread* th, Buffer* tmp, char* src, LINT len);

int sourceToStr(Thread* th, Buffer* tmp, char* src, LINT len);
int strToSource(Thread* th, Buffer* tmp, char* src, LINT len);

int jsonToU8(Thread* th, Buffer* tmp, char* src, LINT len);
int u8ToJson(Thread* th, Buffer* tmp, char* src, LINT len);

int xmlToU8(Thread* th, Buffer* tmp, char* src, LINT len);
int xmlToLatin(Thread* th, Buffer* tmp, char* src,LINT len);
int strToXml(Thread* th, Buffer* tmp, char* src, LINT len);

int strToLF(Thread* th, Buffer* tmp, char* src, LINT len);
int strToCR(Thread* th, Buffer* tmp, char* src, LINT len);
int strToCRLF(Thread* th, Buffer* tmp, char* src, LINT len);

#endif
