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

#define MOD_ADLER 65521

unsigned int adler32Str(unsigned int value, char* src, int len)
{
	// value is expected to be 1 the first time
	int i;
	int A = value & 0xffff;
	int B = (value>>16) & 0xffff;

	for (i = 0; i < len; i++) {
		int c = src[i] & 255;
		A += c; if (A >= MOD_ADLER) A -= MOD_ADLER;
		B += A; if (B >= MOD_ADLER) B -= MOD_ADLER;
	}
	value = (B << 16) | A;
	return value;
}