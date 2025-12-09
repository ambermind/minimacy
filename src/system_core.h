// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _CORE_
#define _CORE_

#define FIFO_START 0
#define FIFO_END 1
#define FIFO_COUNT 2

#define SUFFIX_CODE ".mcy"
#define SUFFIX_NONE ""

extern LINT TimeDelta;
int systemCryptoInit(Pkg *system);
int systemStrInit(Pkg *system);
int systemBytesInit(Pkg *system);
int systemBinaryInit(Pkg *system);
int systemConvertInit(Pkg *system);
int systemBufferInit(Pkg *system);
int systemLzwInit(Pkg* system);
int systemInflateInit(Pkg* system);
int systemCoreInit(Pkg *system);

#endif