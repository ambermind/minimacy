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

Locals* localsCreate(Thread* th, char* name, LINT level, Type* type, Locals* next);
Locals* localsGet(Locals* locals,LINT level,char* name);
LINT localsNb(Locals* locals);
Locals* localsByIndex(Locals* locals, LINT index);
#endif