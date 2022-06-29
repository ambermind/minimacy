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


Ref* compileParseRef(Compiler* c, char* what)
{
	Ref* ref;
	if ((!parserNext(c)) || (!islabel(c->parser->token)))
	{
		compileError(c, "Compiler: %s expected (found '%s')\n", what, compileToken(c));
		return NULL;
	}
	ref = pkgFirstGet(c->pkg, compileToken(c));
	if (!ref) compileError(c, "Compiler: cannot find prototype of '%s'\n", compileToken(c));
	return ref;
}

LINT compileTypeParams(Compiler* c, Locals** labels)
{
	Locals* lb = NULL;
	LINT n = 0;
	*labels = NULL;
	if (parserNext(c) && !strcmp(c->parser->token, "("))
	{
		while (1)
		{
			if ((!parserNext(c)) || !islabel(c->parser->token))
			{
				compileError(c, "Compiler: parameter or ')' expected (found '%s')\n", compileToken(c));
				return -1;
			}
			lb = localsCreate(c->th, c->parser->token, 0, NULL, lb); if (!lb) return -1;
			n++;
			if (!parserNext(c))
			{
				compileError(c, "Compiler: parameter or ')' expected (found '%s')\n", compileToken(c));
				return -1;
			}
			if (!strcmp(c->parser->token, ")")) break;
			parserGiveback(c);
		}
	}
	else parserGiveback(c);
	*labels = lb;
	return n;
}

Type* compileType2(Compiler* c, Ref* typeRef,char* separator,int sum)
{
	Locals* labels = NULL;
	Locals* tmp;
	Type* typeType=typeRef->type;

	LINT n = compileTypeParams(c, &labels);
	if (n < 0) return NULL;
	// the following test may fail only for extend
	if (n != typeType->nb) return compileError(c, "Compiler: type mismatch (%d parameters while expecting %d)\n", n, typeType->nb);

	for (tmp = labels; tmp; tmp = tmp->next) tmp->type = typeType->child[--n];

	if (parserAssume(c, separator)) return NULL;

	if (sum) return compileDefCons2(c, typeRef, labels);
	return compileStructure2(c, typeRef, labels);
}

Type* compileRecRefs2(Compiler* c, Ref* ref)
{
	Type* result = NULL;
	if (!ref) return MM.Boolean;	// anything not null
	if (!compileRecRefs2(c, ref->next)) return NULL;

	if (ref->parser)
	{
		//		printf("Compile step2 '%s.%s'\n", refPkgName(ref), refName(ref));
		if (ref->code == REFCODE_SUM)
		{
			parserRestoreFromRef(c, ref);
			if (!compileType2(c, ref, "=", 1)) return compileError(c, "Compiler: error compiling type '%s'\n", refName(ref));
		}
		else if (ref->code == REFCODE_STRUCT)
		{
			parserRestoreFromRef(c, ref);
			if (!compileType2(c, ref, "=", 0)) return compileError(c, "Compiler: error compiling type '%s'\n", refName(ref));
		}
		else if (ref->code == REFCODE_EXTEND)
		{
			parserRestoreFromRef(c, ref);
			if (!parserNext(c)) return compileError(c, "Compiler: uncomplete extend definition (found EOF)\n");
			ref = compileGetRef(c);

			if ((!ref) || (ref->code != REFCODE_SUM)) return compileError(c, "Compiler: '%s' is not a sum type\n", compileToken(c));
			if (!compileType2(c, ref, "with", 1)) return compileError(c, "Compiler: error compiling type '%s'\n", refName(ref));
		}
	}
	return MM.Boolean;	// anything not null
}
// remove the temporary extend declarations
void cleanExtendRef(Compiler* c)
{
	Ref* prev = NULL;
	Ref* p = c->pkg->first;
	while (p)
	{
		if (p->code == REFCODE_EXTEND)
		{
			if (prev) prev->next = p->next;
			else c->pkg->first = p->next;
		}
		else prev = p;
		p = p->next;
	}
}
Type* compileFile2(Compiler *c)
{	
	if (!compileRecRefs2(c, c->pkg->first)) return NULL;
	cleanExtendRef(c);
	return MM.Boolean;	// anything except NIL
}

Type* compileStep2(Compiler* c)
{
	Pkg* pkg;
	for (pkg = MM.listPkgs; pkg->stage == PKG_STAGE_1; pkg = pkg->listNext)
	{
		Pkg* pkgSave = c->pkg;
		c->pkg = pkg;
		pkg->stage = PKG_STAGE_2;

		c->parser = NULL;
		if (!parserFromIncludes(c, STRSTART(pkg->name))) return NULL;
		if (!compileFile2(c)) return NULL;

		c->pkg = pkgSave;
	}

	c->parser = NULL;
	if (!parserFromIncludes(c, c->pkg == MM.system ? BOOT_FILE : "")) return NULL;
	return compileFile2(c);
}
