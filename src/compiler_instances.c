// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

void compileInstanceDefClean(Def* def)
{
	LB* instances;
	if (!def->tagged) return;
	def->tagged = 0;
	instances = def->instances;
	while (instances)
	{
		Def* target = (Def*)(ARRAY_PNT(instances, INSTANCE_DEF));
		compileInstanceDefClean(target);
		instances = (ARRAY_PNT(instances, INSTANCE_NEXT));
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
		Def* target = (Def*)(ARRAY_PNT(instances, INSTANCE_DEF));
		Type* t= (Type*)(ARRAY_PNT(instances, INSTANCE_TYPE));
		if ((k = compileInstanceDefSolver(c, target))) return k;
//		PRINTF(LOG_DEV,"#unify again instance of %s in %s\n", STR_START(target->name), defName(def));

		defType = typeCopy(target->type); if (!defType) return EXEC_OM;
		k = typeUnify(c, defType, t);
		if (k)
		{	
			c->parser = (Parser*)(ARRAY_PNT(instances, INSTANCE_PARSER));
			c->parser->index0 = (int)(ARRAY_INT(instances, INSTANCE_POSITION));
			PRINTF(LOG_USER, "> Compiler error: type error when '%s' calls '%s'\n", defName(def), defName(target));
			return k;
		}

		instances= (ARRAY_PNT(instances, INSTANCE_NEXT));
	}
	compileInstanceDefClean(def);
	return 0;
}
LINT compileInstanceSolver(Compiler* c)
{
	LINT k;
	Def* def;
	Pkg* pkg = c->pkg;
//	PRINTF(LOG_DEV,"compileInstanceSolver_0 %llx\n", def);

	for (def = pkg->first; def; def = def->next) {
		k = compileInstanceDefSolver(c, def); if (k) return k;
	}
//	PRINTF(LOG_DEV,"compileInstanceSolver_1 %s %llx\n", pkgName(pkg),pkg->start);
	k = compileInstanceDefSolver(c, pkg->start); if (k) return k;

	for (def = pkg->first; def; def = def->next) def->type = typeSimplify(def->type);
	return 0;
}


Type* typeInstance(Compiler* c, Def* def)
{
	LB* list;
	Def* from = c->fmk->defForInstances;	// from is the fun definition, either the function itself or a lambda function defined in it
	Type* t = typeCopy(def->type); if (!t) return NULL;
	if (t == def->type) return t;
	if ((def->proto==0) && (!def->instances)) return t;
	// from is referencing def which is not yet fully compiled
	// we need to remember to typecheck one more time def with 

/*	PRINTF(LOG_DEV,"#make instance of %s.%s in %s.%s\n",
		STR_START(def->pkg->name),
		defName(def),
		STR_START(from->pkg->name),
		STR_START(from->name));
*/
	list = memoryAllocArray(INSTANCE_LENGTH, DBG_TUPLE);
	if (!list) return NULL;
	ARRAY_SET_PNT(list, INSTANCE_DEF, ((LB*)def));
	ARRAY_SET_PNT(list, INSTANCE_TYPE, ((LB*)t));
	ARRAY_SET_PNT(list, INSTANCE_PARSER, ((LB*)c->parser));
	ARRAY_SET_INT(list, INSTANCE_POSITION, (c->parser->index0));
	ARRAY_SET_PNT(list, INSTANCE_NEXT, (from->instances));
	from->instances = list;
	return t;
}
