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
#ifndef _HW_
#define _HW_

void consoleWrite(int user, char* src, int len);
void consoleVPrint(int user, char* format, va_list arglist);

void hwRandomInit(void);
void hwRandomBytes(char* dst, LINT len);
int hwHasRandom(void);

void hwSleepMs(LINT ms);
LINT hwTime(void);
LINT hwTimeMs(void);
void hwTimeInit(void);

int hwSetHostUser(char* user);
int hwInit(void);

#endif
