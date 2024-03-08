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

void compileInstanceDefClean(Def* def)
{
	LB* instances;
	if (!def->tagged) return;
	def->tagged = 0;
	instances = def->instances;
	while (instances)
	{
		Def* target = (Def*)(TABPNT(instances, INSTANCE_DEF));
		compileInstanceDefClean(target);
		instances = (TABPNT(instances, INSTANCE_NEXT));
	}
	def->instances = NULL;
}

LINT compileInstanceDefSolver(Compiler* c,Def* def)
{
	LB* instances;
//	PRINTF(LOG_DEV,"compileInstanceDefSolver %llx\n", def);
	if (def->tagged) return 0;
	instances = def->instances;
	if (!instances) return 0;
	def->tagged = 1;
	while(instances)
	{
		Type* defType;
		LINT k;
		Def* target = (Def*)(TABPNT(instances, INSTANCE_DEF));
		Type* t= (Type*)(TABPNT(instances, INSTANCE_TYPE));
		if ((k = compileInstanceDefSolver(c, target))) return k;
//		PRINTF(LOG_DEV,"#unify again instance of %s in %s\n", STRSTART(target->name), defName(def));
		c->parser= (Parser*)(TABPNT(instances, INSTANCE_PARSER));
		c->parser->index0=(int) (TABINT(instances, INSTANCE_POSITION));
		defType = typeCopy(c->th, target->type); if (!defType) return EXEC_OM;
//		{ typePrint(LOG_SYS, target->type); PRINTF(LOG_DEV,"\n"); }
//		{ typePrint(LOG_SYS, t); PRINTF(LOG_DEV,"\n"); }
//		{ typePrint(LOG_SYS, defType); PRINTF(LOG_DEV,"\n"); }
		k = typeUnify(c, defType, t);
		if (k)
		{	
			if (c->parser->name) PRINTF(LOG_USER, "Compiler: error in %s\n", c->parser->name);
			PRINTF(LOG_USER, "Compiler: type error when '%s' calls '%s'\n", defName(def), defName(target));
			return k;
		}
//		if (TypeTest) { typePrint(LOG_SYS, TypeTest); PRINTF(LOG_DEV,"\n"); }

		instances= (TABPNT(instances, INSTANCE_NEXT));
	}
	compileInstanceDefClean(def);
	return 0;
}
LINT compileInstancePkgSolver(Compiler* c, Pkg* pkg)
{
	LINT k;
	Def* def = pkg->first;
//	PRINTF(LOG_DEV,"compileInstancePkgSolver0 %llx\n", def);

	while (def)
	{
		k = compileInstanceDefSolver(c, def); if (k) return k;
		def = def->next;
	}
//	PRINTF(LOG_DEV,"compileInstancePkgSolver1 %s %llx\n", pkgName(pkg),pkg->start);
	k = compileInstanceDefSolver(c, pkg->start); if (k) return k;
	return 0;
}

LINT compileInstanceSolver(Compiler* c)
{
	return compileInstancePkgSolver(c, c->pkg);
}

Type* typeInstance(Compiler* c, Def* def)
{
	LB* list;
	Def* from = c->fmk->defForInstances;
	Type* t = typeCopy(c->th, def->type); if (!t) return NULL;
	if (t == def->type) return t;
	if ((def->proto==0) && (!def->instances)) return t;

/*	PRINTF(LOG_DEV,"#make instance of %s.%s in %s.%s\n",
		STRSTART(def->pkg->name),
		defName(def),
		STRSTART(from->pkg->name),
		STRSTART(from->name));
*/
	list = memoryAllocTable(c->th,INSTANCE_LEN, DBG_TUPLE);
	if (!list) return NULL;
	TABSETPNT(list, INSTANCE_DEF, ((LB*)def));
	TABSETPNT(list, INSTANCE_TYPE, ((LB*)t));
	TABSETINT(list, INSTANCE_POSITION, (c->parser->index0));
	TABSETPNT(list, INSTANCE_PARSER, ((LB*)c->parser));
	TABSETPNT(list, INSTANCE_NEXT, (from->instances));
	from->instances = list;
	return t;
}
