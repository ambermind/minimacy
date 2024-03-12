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


Def* compileParseDef(Compiler* c, char* what)
{
	Def* def;
	if ((!parserNext(c)) || (!islabel(c->parser->token)))
	{
		compileError(c,"%s expected (found '%s')\n", what, compileToken(c));
		return NULL;
	}
	def = pkgFirstGet(c->pkg, compileToken(c));
	if (!def) compileError(c,"cannot find prototype of '%s'\n", compileToken(c));
	return def;
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
			if ((!parserNext(c)) || !islabel(c->parser->token))
			{
				compileError(c,"parameter or '}' expected (found '%s')\n", compileToken(c));
				return -1;
			}
			lb = localsCreate(c->th, c->parser->token, 0, NULL, lb); if (!lb) return -1;
			n++;
			if (!parserNext(c))
			{
				compileError(c,"parameter or '}' expected (found '%s')\n", compileToken(c));
				return -1;
			}
			if (!strcmp(c->parser->token, "}")) break;
			parserGiveback(c);
		}
	}
	else parserGiveback(c);
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

Type* compileRecDefs2(Compiler* c, Def* def)
{
	if (!def) return MM.Boolean;	// anything not null
	if (!compileRecDefs2(c, def->next)) return NULL;

	if (def->parser && def->proto)
	{
		//		PRINTF(LOG_DEV,"Compile step2 '%s.%s'\n", defPkgName(def), defName(def));
		if (def->code == DEF_CODE_SUM)
		{
			parserRestoreFromDef(c, def);
			if (!compileType2(c, def, "=", 1)) return compileError(c,"error compiling type '%s'\n", defName(def));
			parserReset(c);
		}
		else if (def->code == DEF_CODE_STRUCT)
		{
			parserRestoreFromDef(c, def);
			if (!compileType2(c, def, "=", 0)) return compileError(c,"error compiling type '%s'\n", defName(def));
			parserReset(c);
		}
		else if (def->code == DEF_CODE_EXTEND)
		{
			parserRestoreFromDef(c, def);
			if (!parserNext(c)) return compileError(c,"uncomplete extend definition (found EOF)\n");
			def = compileGetDef(c);

			if ((!def) || (def->code != DEF_CODE_SUM)) return compileError(c,"'%s' is not a sum type\n", compileToken(c));
			if (!compileType2(c, def, "with", 1)) return compileError(c,"error compiling type '%s'\n", defName(def));
			parserReset(c);
		}
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
	if (!parserFromIncludes(c, pkgName(c->pkg))) return NULL;
	if (!compileRecDefs2(c, c->pkg->first)) return NULL;
	cleanExtendDef(c);
	return MM.Boolean;	// anything except NIL
}
