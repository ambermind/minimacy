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
