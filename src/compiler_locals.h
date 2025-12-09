// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _LOCALS_
#define _LOCALS_

struct Locals
{
	LB header;
	FORGET forget;
	MARK mark;

	LB* name;
	LINT index;
	LINT level;
	Type* type;
	Locals* next;
};

Locals* localsCreate(char* name, LINT level, Type* type, Locals* next);
Locals* localsGet(Locals* locals,LINT level,char* name);
LINT localsNb(Locals* locals);
Locals* localsByIndex(Locals* locals, LINT index);
#endif