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
#include"minimacy.h"

LINT localsNb(Locals* locals)
{
	if (!locals) return 0;
	return 1+locals->index;
}

void localsMark(LB* user)
{
	Locals* l=(Locals*)user;
	MEMORY_MARK(user,l->name);
	MEMORY_MARK(user,(LB*)l->type);
	MEMORY_MARK(user,(LB*)l->next);
}

Locals* localsCreate(Thread* th, char* name,LINT level, Type* type, Locals* next)
{
	Locals* l;
	memoryEnterFast();
	l=(Locals*)memoryAllocExt(th, sizeof(Locals),DBG_LOCALS,NULL,localsMark); if (!l) return NULL;
	l->index=localsNb(next);
	l->level=level;
	l->type=type;
	l->next=next;
	l->name = NULL;
	if (name) {
		l->name = memoryAllocStr(th, name, -1); if (!l->name) return NULL;
	}
	memoryLeaveFast();
	return l;
}


Locals* localsGet(Locals* locals,LINT level,char* name)
{
	LINT len=strlen(name);
	while(locals)
	{
		if (locals->name&&(STR_LENGTH(locals->name)==len)&&(!memcmp(STR_START(locals->name),name,len)))
		{
			locals->level=level;
			return locals;
		}
		locals=locals->next;
	}
	return NULL;
}

Locals* localsByIndex(Locals* locals, LINT index)
{
	while (locals) {
		if (locals->index == index) return locals;
		locals = locals->next;
	}
	return NULL;
}

