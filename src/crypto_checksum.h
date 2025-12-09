// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _CRC32_H_
#define _CRC32_H_

unsigned int crc32Str(unsigned int value, char* src, int len);
unsigned int crc7Str(unsigned int value, char* src, int len);
unsigned int adler32Str(unsigned int value, char* src, int len);
#endif //_CRC32_H_
