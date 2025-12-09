// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"
#ifdef USE_CONSOLE_OUT_ANSI
void consoleWrite(int user, char* src, int len)
{
	FILE* out = user ? stdout : stderr;
	fwrite(src, 1, len, out);
	fflush(out);
}
void consoleVPrint(int user, char* format, va_list arglist)
{
	FILE* out = user ? stdout : stderr;
	vfprintf(out, format, arglist);
	fflush(out);
}
#endif
#ifdef USE_CONSOLE_OUT_STUB
void consoleWrite(int user, char* src, int len)
{
}
void consoleVPrint(int user, char* format, va_list arglist)
{
}
#endif
