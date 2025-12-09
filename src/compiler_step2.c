// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"


Def* compileParseDef(Compiler* c, char* what)
{
	Def* d;
	if ((!parserNext(c)) || (!isLabel(c->parser->token)))
	{
		compileError(c,"%s expected (found '%s')\n", what, compileToken(c));
		return NULL;
	}
	d = pkgFirstGet(c->pkg, compileToken(c));
	if (!d) compileError(c,"cannot find prototype of '%s'\n", compileToken(c));
	return d;
}

LINT compileTypeParams(Compiler* c, Locals** labels)
{
	Locals* lb = NULL;
	LINT n = 0;
	*labels = NULL;
	if (parserNext(c) && !strcmp(c->parser->token, "{"))
	{
		while (1)
		{
			if ((!parserNext(c)) || !isLabel(c->parser->token))
			{
				compileError(c,"parameter or '}' expected (found '%s')\n", compileToken(c));
				return -1;
			}
			lb = localsCreate(c->parser->token, 0, NULL, lb); if (!lb) return -1;
			n++;
			if (!parserNext(c))
			{
				compileError(c,"parameter or '}' expected (found '%s')\n", compileToken(c));
				return -1;
			}
			if (!strcmp(c->parser->token, "}")) break;
			parserRewind(c);
		}
	}
	else parserRewind(c);
	*labels = lb;
	return n;
}

Type* compileType2(Compiler* c, Def* typeDef,char* separator,int sum)
{
	Locals* labels = NULL;
	Locals* tmp;
	Type* typeType=typeDef->type;

	LINT n = compileTypeParams(c, &labels);
	if (n < 0) return NULL;
	// the following test may fail only for extend
	if (n != typeType->nb) return compileError(c,"type mismatch (%d parameters while expecting %d)\n", n, typeType->nb);

	for (tmp = labels; tmp; tmp = tmp->next) tmp->type = typeType->child[--n];

	if (parserAssume(c, separator)) return NULL;

	if (sum) return compileDefCons2(c, typeDef, labels);
	return compileStructure2(c, typeDef, labels);
}

Type* compileDefs2(Compiler* c)
{
	Def* d;
	for(d = c->pkg->first; d; d=d->next) if (d->parser && d->proto)
	{
		memoryEnterSafe();
		//		PRINTF(LOG_DEV,"Compile step2 %llx '%s.%s'\n", d, defPkgName(d), defName(d));
		if (d->code == DEF_CODE_SUM)
		{
			parserRestoreFromDef(c, d);
			if (!compileType2(c, d, "=", 1)) return compileError(c,"error compiling type '%s'\n", defName(d));
			parserReset(c);
		}
		else if (d->code == DEF_CODE_STRUCT)
		{
			parserRestoreFromDef(c, d);
			if (!compileType2(c, d, "=", 0)) return compileError(c,"error compiling type '%s'\n", defName(d));
			parserReset(c);
		}
		else if (d->code == DEF_CODE_EXTEND)
		{
			Def* dsum;
			parserRestoreFromDef(c, d);
			if (!parserNext(c)) return compileError(c,"uncomplete extend definition (found EOF)\n");
			dsum = compileGetDef(c);

			if ((!dsum) || (dsum->code != DEF_CODE_SUM)) return compileError(c,"'%s' is not a sum type\n", compileToken(c));
			if (!compileType2(c, dsum, "with", 1)) return compileError(c,"error compiling type '%s'\n", defName(dsum));
			parserReset(c);
		}
		memoryLeaveSafe();
	}
	return MM.Boolean;	// anything not null
}
// remove the temporary extend declarations
void cleanExtendDef(Compiler* c)
{
	Def* prev = NULL;
	Def* p = c->pkg->first;
	while (p)
	{
		if (p->proto && p->code == DEF_CODE_EXTEND)
		{
			if (prev) prev->next = p->next;
			else c->pkg->first = p->next;
		}
		else prev = p;
		p = p->next;
	}
}

Type* compileStep2(Compiler* c)
{
	Type* t;
	defReverse(c->pkg);
	t = compileDefs2(c);
	defReverse(c->pkg);
	if (!t) return NULL;
	cleanExtendDef(c);
	return MM.Boolean;	// anything except NIL
}
