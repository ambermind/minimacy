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

Globals* globalsCreate(Thread* th, LB* data, Globals* next);
LINT globalsGet(Globals* globals,LB* data);
LINT globalsNb(Globals* globals);

int globalsExtract(Thread* th, Globals* globals, LB** result);

#endif