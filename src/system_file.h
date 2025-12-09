// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _SYSTEM_ASYNC_
#define _SYSTEM_ASYNC_

typedef struct {
	Def* READ_ONLY;
	Def* REWRITE;
	Def* READ_WRITE;
	Def* APPEND;
}FileModes;
extern FileModes FM;

int systemFileInit(Pkg* system);

#endif
