// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _SERIAL_
#define _SERIAL_

int fun_serialList(Thread* th);
int fun_serialOpen(Thread* th);
int fun_serialClose(Thread* th);
int fun_serialWrite(Thread* th);
int fun_serialRead(Thread* th);
int fun_serialSocket(Thread* th);

int sysSerialInit(Pkg* system);

#endif
