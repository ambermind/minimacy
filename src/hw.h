// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
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
