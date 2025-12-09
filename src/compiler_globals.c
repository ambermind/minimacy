// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

LINT globalsNb(Globals* globals)
{
	if (!globals) return 0;
	return 1+globals->index;
}

void globalsMark(LB* user)
{
	Globals* l=(Globals*)user;
	MARK_OR_MOVE(l->data);
	MARK_OR_MOVE(l->next);
}

Globals* globalsCreate(LB* data, Globals* next)
{
	Globals* l;
	memorySetTmpRoot((LB*)next);
	TMP_PUSH(data, NULL);
	l=(Globals*)memoryAllocNative(sizeof(Globals),DBG_LOCALS,NULL,globalsMark); if (!l) return NULL;
	l->data=data;
	l->index=globalsNb(next);
	l->next=next;
	TMP_PULL();
	return l;
}


LINT globalsGet(Globals* globals,LB* data)
{
	if (data)
	while(globals)
	{
		if (globals->data==data) return globals->index;
		globals=globals->next;
	}
	return -1;
}


int globalsExtract(Globals* globals, LB** result)
{
	LINT nb;
	LB* p;
	*result = NULL;
	if (!globals) return 0;
	
	nb=globalsNb(globals);
	p= memoryAllocArray(nb,DBG_TUPLE);
	if (!p) return EXEC_OM;
	nb--;
	while(nb>=0)
	{
		ARRAY_SET_PNT(p,nb--,globals->data);
		globals=globals->next;
	}
	*result = p;
	return 0;
}

