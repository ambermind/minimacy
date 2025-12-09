// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _GLOBALS_
#define _GLOBALS_

typedef struct Globals Globals;
struct Globals
{
	LB header;
	FORGET forget;
	MARK mark;

	LB* data;
	LINT index;
	struct Globals* next;
};

Globals* globalsCreate(LB* data, Globals* next);
LINT globalsGet(Globals* globals,LB* data);
LINT globalsNb(Globals* globals);

int globalsExtract(Globals* globals, LB** result);

#endif