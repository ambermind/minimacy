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
Type* TypeTest = NULL;

LB* compileName(Compiler* c, char* what)
{
	if ((!parserNext(c)) || (!islabel(c->parser->token)))
	{
		compileError(c, "Compiler: %s expected (found '%s')\n", what, compileToken(c));
		return NULL;
	}
	if (pkgFirstGet(c->pkg, c->parser->token))
	{
		compileError(c, "Compiler: '%s' already defined\n", compileToken(c));
		return NULL;
	}
	return memoryAllocStr(c->th, c->parser->token, -1);
}

char* compilePkgName(Compiler* c)
{
	bufferReinit(MM.tmpBuffer);

	while (1)
	{
		if ((!parserNext(c)) || !isalphanum(c->parser->token))
		{
			compileError(c, "Compiler: wrong package name (found '%s')\n", compileToken(c));
			return NULL;
		}
		if (bufferAddStr(c->th, MM.tmpBuffer, c->parser->token)) return NULL;
		if ((!parserNext(c)) || (strcmp(c->parser->token, "."))) return bufferStart(MM.tmpBuffer);
		if (bufferAddchar(c->th, MM.tmpBuffer, '.')) return NULL;
	}
}

Type* compileInclude(Compiler* c)
{
	// parsing of package name
	char* name = compilePkgName(c); if (!name) return NULL;
	char* parent = pkgName(c->pkg);

	if (strstr(name,parent)!=name || (name[strlen(parent)]!='.'))
		return compileError(c, "Compiler: only child files like '%s.*' may be included. Use 'import' instead.\n", parent);

	if (strcmp(compileToken(c), ";;"))
		return compileError(c, "Compiler: ';;' expected (found '%s')\n", compileToken(c));

	if (!parserFromIncludes(c, name)) return NULL;
	return MM.I;
}

Type* compileImport(Compiler* c)
{
	LB* name;
	LB* alias = NULL;
	Pkg* pkg;
	char* pkgName = compilePkgName(c); if (!pkgName) return NULL;

	pkg = pkgImportByName(pkgName);
	if (!pkg)
	{
		name = memoryAllocStr(c->th, pkgName, -1); if (!name) return NULL;
		pkg = pkgAlloc(c->th, name, 0, PKG_FROM_IMPORT); if (!pkg) return NULL;
	}

	if (!strcmp(compileToken(c), "as"))
	{
		if ((!parserNext(c)) || (!islabel(c->parser->token)))
			return compileError(c, "Compiler: label expected (found '%s')\n", compileToken(c));
		alias = memoryAllocStr(c->th, c->parser->token, -1); if (!alias) return NULL;
	}
	else parserGiveback(c);

	if (pkgAddImport(c, c->pkg, pkg, alias)) return NULL;

	return MM.I;
}


Type* compileFun1(Compiler *c, LB* name)
{
	LINT argc=0;
	Ref* ref;
	Type* t;
	Type* u;
	LINT pIndex = parserIndex(c);
	while(parserNext(c)&&strcmp(c->parser->token,"="))
	{
		Type* u = typeAllocUndef(c->th);  if (!u) return NULL;
		TYPEPUSH_NULL(c,u);
		parserGiveback(c);
		if (!compileSkipLocal(c)) return NULL;
		argc++;
	}
	parserGiveback(c);
	u = typeAllocUndef(c->th);  if (!u) return NULL;
	TYPEPUSH_NULL(c, u);
	
	if (parserAssume(c,"=")) return NULL;

	t = typeAllocFromStack(c->th, NULL, TYPECODE_FUN, argc + 1); if (!t) return NULL;
	ref = refAlloc(c->th, argc, REFINDEX_BC, NIL, t); if (!ref) return NULL;
	refSetParser(ref, c, pIndex);
	t->ref = ref;

	ref->proto = 1;
	if (!ref) return NULL;

	if (pkgAddRef(c->th, c->pkg, name, ref)) return NULL;
	parserUntil(c, ";;");

	return t;
}

Type* compileVarOrConst1(Compiler* c,LINT code, LB* name)
{
	Ref* ref;
	Type* t = typeAllocWeak(c->th); if (!t) return NULL;
	ref = refAlloc(c->th, code, REFINDEX_VALUE, NIL, t); if (!ref) return NULL;
	ref->proto = 1;
	refSetParser(ref, c, parserIndex(c));
	if (pkgAddRef(c->th, c->pkg, name, ref)) return NULL;
	parserUntil(c, ";;");
	return ref->type;
}

Type* compileType1(Compiler* c, LB* name,int sum)
{
	Ref* refType = NULL;
	Type* typeType;
	int n = 0;
	LINT pIndex = parserIndex(c);

	if (parserNext(c) && !strcmp(c->parser->token, "("))
	{
		while (1)
		{
			Type* t;
			if ((!parserNext(c)) || !islabel(c->parser->token)) return compileError(c, "Compiler: parameter or ')' expected (found '%s')\n", compileToken(c));
			t = typeAllocUndef(c->th); if (!t) return NULL;
			TYPEPUSH_NULL(c, t);
			n++;

			if (!parserNext(c)) return compileError(c, "Compiler: parameter or ')' expected (found '%s')\n", compileToken(c));

			if (!strcmp(c->parser->token, ")")) break;
			parserGiveback(c);
		}
	}
	else parserGiveback(c);

	typeType = typeAllocFromStack(c->th, NULL, TYPECODE_PRIMARY, n); if (!typeType) return NULL;
	if (parserAssume(c, "=")) return NULL;

	if (sum)
	{
		refType = refAlloc(c->th, REFCODE_SUM, 0, NIL, typeType); if (!refType) return NULL;
		refType->proto = 1;
		refSetParser(refType, c, pIndex);
		typeType->ref = refType;
		if (pkgAddRef(c->th, c->pkg, name, refType)) return NULL;	// this will also set refType->name

		return compileDefCons1(c);
	}
	refType = refAlloc(c->th, REFCODE_STRUCT, 0, NIL, typeType); if (!refType) return NULL;
	refType->proto = 1;
	refSetParser(refType, c, pIndex);
	typeType->ref = refType;

	if (pkgAddRef(c->th, c->pkg, name, refType)) return NULL;	// this will also set refType->name
	return compileStructure1(c, refType);
}

Type* compileExtend1(Compiler* c)
{
	Ref* ref;
	LINT pIndex = parserIndex(c);	// pIndex is before the name of the extended type
	if ((!parserNext(c)) || (!islabel(c->parser->token))) return compileError(c, "Compiler: sum name expected, found '%s'\n", compileToken(c));

	ref = refAlloc(c->th, REFCODE_EXTEND, 0, NIL, NULL); if (!ref) return NULL;
	refSetParser(ref, c, pIndex);
	ref->next = c->pkg->first;
	c->pkg->first = ref;

	if (parserNext(c) && !strcmp(c->parser->token, "(")) parserUntil(c, ")");
	else parserGiveback(c);

	if (parserAssume(c, "with")) return NULL;

	return compileDefCons1(c);
}

Type* compileEnum(Compiler* c)
{
	LINT num = 0;
	while (1)
	{
		if ((!parserNext(c))||((!islabel(c->parser->token))&&strcmp(c->parser->token, ";;"))) return compileError(c, "Compiler: label or ';;' expected (found %s)\n", compileToken(c));
		if (!strcmp(c->parser->token, ";;")) return MM.I;
		if (!pkgAddConst(c->th, c->pkg, c->parser->token, INTTOVAL(num++),MM.I)) return NULL;
	}
}
Type* compilePublic(Compiler* c)
{
	if ((!parserNext(c)) || (strcmp(c->parser->token, ":")&&!islabel(c->parser->token)))
	{
		return compileError(c, "Compiler: ':' or label expected (found %s)\n", compileToken(c));
	}
	if (!strcmp(c->parser->token, ":"))
	{
		pkgHideAll(c->pkg);
		return MM.Boolean;
	}
	while (strcmp(c->parser->token, ";;"))
	{
		Ref* ref = pkgGet(c->pkg, c->parser->token, 0);
		if (!ref) return compileError(c, "Compiler: label '%s' not found\n", compileToken(c));
		ref->public = REF_PUBLIC_FORCED;
		if ((!parserNext(c)) || (strcmp(c->parser->token, ";;") && !islabel(c->parser->token)))
		{
			return compileError(c, "Compiler: ';;' or label expected (found %s)\n", compileToken(c));
		}
	}
	return MM.Boolean;
}
Type* compileFile1(Compiler* c)
{
	while (parserNext(c))
	{
		LB* name;
		char* token = c->parser->token;
		if (!strcmp(token, "fun"))
		{
			name = compileName(c, "name of function"); if (!name) return NULL;
			if (!compileFun1(c, name)) return compileErrorInFunction(c, "Compiler: error compiling function '%s'\n", STRSTART(name));
		}
		else if (!strcmp(token, "sum"))
		{
			name = compileName(c, "name of sum"); if (!name) return NULL;
			if (!compileType1(c, name,1)) return compileError(c, "Compiler: error compiling type '%s'\n", STRSTART(name));
		}
		else if (!strcmp(token, "struct"))
		{
			name = compileName(c, "name of struct"); if (!name) return NULL;
			if (!compileType1(c, name,0)) return compileError(c, "Compiler: error compiling type '%s'\n", STRSTART(name));
		}
		else if (!strcmp(token, "extend"))
		{
			if (!compileExtend1(c)) return compileError(c, "Compiler: error compiling extend\n");
		}
		else if (!strcmp(token, "enum"))
		{
			if (!compileEnum(c)) return compileError(c, "Compiler: error compiling enumeration\n");
		}
		else if ((!strcmp(token, "var")) || (!strcmp(token, "const")))
		{
			LINT code = strcmp(token, "var") ? REFCODE_CONST : REFCODE_VAR;
			name = compileName(c, "name of reference"); if (!name) return NULL;
			if (!compileVarOrConst1(c, code, name))
			{
				if (code == REFCODE_VAR) return compileError(c, "Compiler: error compiling var '%s'\n", STRSTART(name));
				return compileError(c, "Compiler: error compiling const '%s'\n", STRSTART(name));
			}
		}
		else if (!strcmp(token, "import"))
		{
			if (!compileImport(c)) return compileError(c, "Compiler: error compiling import\n");
			if (parserAssume(c, ";;")) return NULL;
		}
		else if (!strcmp(token, "include"))
		{
			if (!compileInclude(c)) return compileError(c, "Compiler: error compiling include\n");
		}
		else if (!strcmp(token, "public"))
		{
			if (!compilePublic(c)) return compileError(c, "Compiler: error compiling public\n");
		}
		else return compileError(c, "Compiler: unknown declaration '%s'\n", token);
	}
	return MM.Boolean;	// anything except NIL
}

Type* compileImports1(Compiler* c)
{
	Pkg* pkg;
	LB* p;
	for (p = c->pkg->importList; p; p = VALTOPNT(TABGET(p, LIST_NXT)))
	{
		LB* i = VALTOPNT(TABGET(p, LIST_VAL));
		pkg = (Pkg*)VALTOPNT(TABGET(i, IMPORT_PKG));
		if (pkg->stage == PKG_STAGE_EMPTY)
		{
			Pkg* pkgSave = c->pkg;
			c->pkg = pkg;
			pkg->stage = PKG_STAGE_1;

			c->parser = NULL;
			PRINTF(c->th, LOG_ERR, "> compiling '%s'\n", pkgName(pkg));
			if (!parserFromIncludes(c, STRSTART(pkg->name))) return NULL;
			if (!compileStep1(c)) return NULL;
			c->pkg = pkgSave;
		}
	}
	return MM.I;
}
Type* compileStep1(Compiler* c)
{
	if (!compileFile1(c)) return NULL;
	return compileImports1(c);
}
