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

void compileInstanceRefClean(Ref* ref)
{
	LB* instances;
	if (!ref->tagged) return;
	ref->tagged = 0;
	instances = ref->instances;
	while (instances)
	{
		Ref* target = (Ref*)VALTOPNT(TABGET(instances, INSTANCE_REF));
		compileInstanceRefClean(target);
		instances = VALTOPNT(TABGET(instances, INSTANCE_NEXT));
	}
	ref->instances = NULL;
}

LINT compileInstanceRefSolver(Compiler* c,Ref* ref)
{
	LB* instances;
//	printf("compileInstanceRefSolver %llx\n", ref);
	if (ref->tagged) return 0;
	instances = ref->instances;
	if (!instances) return 0;
	ref->tagged = 1;
	while(instances)
	{
		Type* tref;
		LINT k;
		Ref* target = (Ref*)VALTOPNT(TABGET(instances, INSTANCE_REF));
		Type* t= (Type*)VALTOPNT(TABGET(instances, INSTANCE_TYPE));
		if ((k = compileInstanceRefSolver(c, target))) return k;
//		printf("#unify again instance of %s in %s\n", STRSTART(target->name), refName(ref));
		c->parser= (Parser*)VALTOPNT(TABGET(instances, INSTANCE_PARSER));
		c->pkg = ref->pkg;
		c->parser->index0=(int) VALTOINT(TABGET(instances, INSTANCE_POSITION));
		tref = typeCopy(c->th, target->type); if (!tref) return EXEC_OM;
//		{ typePrint(LOG_ERR, target->type); printf("\n"); }
//		{ typePrint(LOG_ERR, t); printf("\n"); }
//		{ typePrint(LOG_ERR, tref); printf("\n"); }
		k = typeUnify(c, tref, t);
		if (k)
		{	
			if (c->parser->name) PRINTF(c->th, LOG_USER, "Compiler: error in %s\n", c->parser->name);
			PRINTF(c->th, LOG_USER, "Compiler: type error when '%s.%s' calls '%s.%s'\n", refPkgName(ref),refName(ref), refPkgName(target), refName(target));
			return k;
		}
//		if (TypeTest) { typePrint(LOG_ERR, TypeTest); printf("\n"); }

		instances= VALTOPNT(TABGET(instances, INSTANCE_NEXT));
	}
	compileInstanceRefClean(ref);
	return 0;
}
LINT compileInstancePkgSolver(Compiler* c, Pkg* pkg)
{
	LINT k;
	Ref* ref = pkg->first;
//	printf("compileInstancePkgSolver0 %llx\n", ref);

	while (ref)
	{
		k = compileInstanceRefSolver(c, ref); if (k) return k;
		ref = ref->next;
	}
//	printf("compileInstancePkgSolver1 %s %llx\n", pkgName(pkg),pkg->start);
	k = compileInstanceRefSolver(c, pkg->start); if (k) return k;
	return 0;
}

LINT compileInstanceSolver(Compiler* c)
{
	Pkg* pkgSave = c->pkg;
	Pkg* p;
	LINT k;

	if ((c->pkg0->uid == 0)&&((k=compileInstancePkgSolver(c, c->pkg0)))) return k;

	for (p = MM.listPkgs; p->uid > c->pastUid ; p = p->listNext)
		if ((k = compileInstancePkgSolver(c, p))) return k;

	c->pkg = pkgSave;
	return 0;
}

Type* typeInstance(Compiler* c, Ref* ref)
{
	LB* list;
	Ref* from = c->fmk->refForInstances;
	Type* t = typeCopy(c->th, ref->type); if (!t) return NULL;
	if (t == ref->type) return t;
	if ((ref->proto==0) && (!ref->instances)) return t;

/*	printf("#make instance of %s.%s in %s.%s\n",
		STRSTART(ref->pkg->name),
		refName(ref),
		STRSTART(from->pkg->name),
		STRSTART(from->name));
*/
	list = memoryAllocTable(c->th,INSTANCE_LEN, DBG_TUPLE);
	if (!list) return NULL;
	TABSET(list, INSTANCE_REF, PNTTOVAL(ref));
	TABSET(list, INSTANCE_TYPE, PNTTOVAL(t));
	TABSET(list, INSTANCE_POSITION, INTTOVAL(c->parser->index0));
	TABSET(list, INSTANCE_PARSER, PNTTOVAL(c->parser));
	TABSET(list, INSTANCE_NEXT, PNTTOVAL(from->instances));
	from->instances = list;
	return t;
}
