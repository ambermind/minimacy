// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _CORE_BIGNUM_
#define _CORE_BIGNUM_

#include"util_bignum.h"

int bigDecToBuffer(bignum b, Buffer* buffer);

LB* bigAlloc(bignum b0);

int systemBignumInit(Pkg *system);


#endif
