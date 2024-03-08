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

LB* compileName(Compiler* c, char* what, int startsWithUpper)
{
	if ((!parserNext(c)) || (!islabel(c->parser->token))) return (LB*) compileError(c, "Compiler: %s expected (found '%s')\n", what, compileToken(c));
	if ((startsWithUpper)&&(!startsWithUppercase(c->parser->token))) return (LB*) compileError(c, "Compiler: %s expected, should start with uppercase (found '%s')\n", what, compileToken(c));
	if ((!startsWithUpper)&&(!startsWithLowercase(c->parser->token))) return (LB*) compileError(c, "Compiler: %s expected, should start with lowercase (found '%s')\n", what, compileToken(c));
	if (pkgFirstGet(c->pkg, c->parser->token)) return (LB*) compileError(c, "Compiler: '%s' already defined\n", compileToken(c));
//	printf("Compiler: '%s' step 1\n", compileToken(c));
	return memoryAllocStr(c->th, c->parser->token, -1);
}
LB* exportLabelList(Compiler* c, char* name)
{
	LB* exports = c->exports;
	while (exports) {
		LB* labels = TABPNT(exports, LIST_VAL);
		char* label = STRSTART(TABPNT(labels, LIST_VAL));
		if (!strcmp(label, name)) return labels;
		exports= TABPNT(exports, LIST_NXT);
	}
	return NULL;
}
int exportLabelListIsSingle(Compiler* c, char* name)
{
	LB* labelList = exportLabelList(c, name);
	LB* labelNext = labelList ? TABPNT(labelList, LIST_NXT) : NULL;
	if (labelList && labelNext) return 0;
	return 1;
}

LB* compilePkgName(Compiler* c)
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
		if ((!parserNext(c)) || (strcmp(c->parser->token, "."))) return memoryAllocStr(c->th, bufferStart(MM.tmpBuffer),-1);
		if (bufferAddChar(c->th, MM.tmpBuffer, '.')) return NULL;
	}
}

Type* compileInclude(Compiler* c)
{
	// parsing of package name
	LB* name = compilePkgName(c); if (!name) return NULL;
	char* parent = pkgName(c->pkg);
	
	if (strstr(STRSTART(name),parent)!= STRSTART(name) || (strstr(STRSTART(name) +strlen(parent), "._")!= STRSTART(name) + strlen(parent)))
		return compileError(c, "Compiler: cannot include package '%s'. Package to include must start with '%s._'. Use 'import' instead.\n", STRSTART(name), parent);

	if (strcmp(compileToken(c), ";;"))
		return compileError(c, "Compiler: ';;' expected (found '%s')\n", compileToken(c));

	if (!parserFromIncludes(c, STRSTART(name))) return NULL;
	return MM.I;
}

Type* compileImportNew(Compiler* c, Pkg* pkg)
{
	Compiler cc;
	LINT global;
	Type* result;
	LB* src;
	
	PRINTF(LOG_USER, "> compiling '%s'\n", pkgName(pkg));
	src = fsReadPackage(c->th, pkgName(pkg), NULL, 0);
	if (!src) return compileError(c, "Compiler: file not found ('%s')\n", pkgName(pkg));

	if (!(result = compile(&cc, src, pkg, NULL))) {
		c->displayed = cc.displayed+1;
		return NULL;
	}

	if (funMakerAddGlobal(c->fmk, (LB*)VALTOPNT(pkg->start->val), &global)) return NULL;
	if (bc_byte_or_int(c, global, OPconstb, OPconst)) return NULL;
	if (bc_byte_or_int(c, 0, OPexecb, OPexec)) return NULL;
	if (bufferAddChar(c->th, c->bytecode, OPdrop)) return NULL;
	pkg->stage = PKG_STAGE_READY;
	return result;
}
Type* compileImport(Compiler* c)
{
	Pkg* pkg;
	LB* alias = NULL;
	LB* name = compilePkgName(c); if (!name) return NULL;

	pkg = pkgImportByName(STRSTART(name));
	if (!pkg)
	{
		if (strstr(STRSTART(name), "._"))
			return compileError(c, "Compiler: cannot import package '%s'. Packages to import cannot contain the string '._' in order to distinct packages to import and packages to include\n", STRSTART(name));

		pkg = pkgAlloc(c->th, name, 0, PKG_FROM_IMPORT); if (!pkg) return NULL;
		if (!compileImportNew(c, pkg)) return NULL;
	}
	else
	{
		if (pkg->stage!= PKG_STAGE_READY)
			return compileError(c, "Compiler: 'import' loop detected for package '%s'.\n", STRSTART(name));
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
	Def* def;
	Type* t;
	Type* u;
	LINT pIndex = parserIndex(c);
	LB* labelList = exportLabelList(c,STRSTART(name));
	LB* labelNext = labelList?TABPNT(labelList, LIST_NXT):NULL;
	if (!parserNext(c)) return compileError(c, "Compiler: unexpected end of file\n");
	if (!strcmp(c->parser->token, "@")) {
		if (compilerSkipTypeDef(c)) return NULL;
	}
	else parserGiveback(c);

	while(parserNext(c)&&strcmp(c->parser->token,"="))
	{
		Type* u = typeAllocUndef(c->th);  if (!u) return NULL;
		TYPEPUSH_NULL(c,u);
		if (labelList) {
			if (!labelNext) return compileError(c, "Compiler: more arguments than in the export declaration\n");
			if (strcmp(STRSTART(TABPNT(labelNext, LIST_VAL)),c->parser->token))
				return compileError(c, "Compiler: argument name '%s' does not match with export declaration '%s'\n", c->parser->token, STRSTART(TABPNT(labelNext, LIST_VAL)));
			labelNext = TABPNT(labelNext, LIST_NXT);
		}
		parserGiveback(c);
		if (!compileSkipLocal(c)) return NULL;
		argc++;
	}
	if (labelList && labelNext) return compileError(c, "Compiler: less arguments than in the export declaration\n");
	parserGiveback(c);
	u = typeAllocUndef(c->th);  if (!u) return NULL;
	TYPEPUSH_NULL(c, u);
	
	if (parserAssume(c,"=")) return NULL;

	t = typeAllocFromStack(c->th, NULL, TYPECODE_FUN, argc + 1); if (!t) return NULL;
	def = defAlloc(c->th, argc, DEF_INDEX_BC, NIL, VAL_TYPE_PNT, t); if (!def) return NULL;
	defSetParser(def, c, pIndex);
	t->def = def;

	def->proto = 1;
	if (!def) return NULL;

	if (pkgAddDef(c->th, c->pkg, name, def)) return NULL;
	if (parserUntil(c, ";;")) return NULL;

	return t;
}

Type* compileVarOrConst1(Compiler* c,LINT code, LB* name)
{
	Def* def;
	Type* t = typeAllocWeak(c->th); if (!t) return NULL;
	if (!exportLabelListIsSingle(c, STRSTART(name))) return compileError(c, "Compiler: the export declaration is not compatible with a constant or a variable (should have no argument)\n");

	def = defAlloc(c->th, code, DEF_INDEX_VALUE, NIL, VAL_TYPE_PNT, t); if (!def) return NULL;
	def->proto = 1;
	defSetParser(def, c, parserIndex(c));
	if (pkgAddDef(c->th, c->pkg, name, def)) return NULL;
	if (parserUntil(c, ";;")) return NULL;
	return def->type;
}

Type* compileType1(Compiler* c, LB* name,int sum)
{
	Def* defType = NULL;
	Type* typeType;
	int n = 0;
	LINT pIndex = parserIndex(c);
	if (!exportLabelListIsSingle(c, STRSTART(name))) return compileError(c, "Compiler: the export declaration is not compatible with a type (should have no argument)\n");

	if (parserNext(c) && !strcmp(c->parser->token, "{"))
	{
		while (1)
		{
			Type* t;
			if ((!parserNext(c)) || !islabel(c->parser->token)) return compileError(c, "Compiler: parameter or '}' expected (found '%s')\n", compileToken(c));
			t = typeAllocUndef(c->th); if (!t) return NULL;
			TYPEPUSH_NULL(c, t);
			n++;

			if (!parserNext(c)) return compileError(c, "Compiler: parameter or '}' expected (found '%s')\n", compileToken(c));

			if (!strcmp(c->parser->token, "}")) break;
			parserGiveback(c);
		}
	}
	else parserGiveback(c);

	typeType = typeAllocFromStack(c->th, NULL, TYPECODE_PRIMARY, n); if (!typeType) return NULL;
	if (parserAssume(c, "=")) return NULL;

	if (sum)
	{
		defType = defAlloc(c->th, DEF_CODE_SUM, 0, NIL, VAL_TYPE_PNT, typeType); if (!defType) return NULL;
		defType->proto = 1;
		defSetParser(defType, c, pIndex);
		typeType->def = defType;
		if (pkgAddDef(c->th, c->pkg, name, defType)) return NULL;	// this will also set defType->name

		return compileDefCons1(c);
	}
	defType = defAlloc(c->th, DEF_CODE_STRUCT, 0, NIL, VAL_TYPE_PNT, typeType); if (!defType) return NULL;
	defType->proto = 1;
	defSetParser(defType, c, pIndex);
	typeType->def = defType;

	if (pkgAddDef(c->th, c->pkg, name, defType)) return NULL;	// this will also set defType->name
	return compileStructure1(c, defType);
}

Type* compileExtend1(Compiler* c)
{
	Def* def;
	LINT pIndex = parserIndex(c);	// pIndex is before the name of the extended type
	if ((!parserNext(c)) || (!islabel(c->parser->token))) return compileError(c, "Compiler: sum name expected, found '%s'\n", compileToken(c));
	def = defAlloc(c->th, DEF_CODE_EXTEND, 0, NIL, VAL_TYPE_PNT, NULL); if (!def) return NULL;
	def->proto = 1;
	defSetParser(def, c, pIndex);
	def->next = c->pkg->first;
	c->pkg->first = def;

	if (parserNext(c) && !strcmp(c->parser->token, "{")) {
		if (parserUntil(c, "}")) return NULL;
	}
	else parserGiveback(c);

	if (parserAssume(c, "with")) return NULL;

	return compileDefCons1(c);
}

Type* compileEnum(Compiler* c)
{
	LINT num = 0;
	while (1)
	{
		Def* def;
		LINT pIndex;
		if ((!parserNext(c))||((!islabel(c->parser->token))&&strcmp(c->parser->token, ";;"))) return compileError(c, "Compiler: label or ';;' expected (found %s)\n", compileToken(c));
		if (!strcmp(c->parser->token, ";;")) return MM.I;

		pIndex = parserIndex(c);
		if (!exportLabelListIsSingle(c, c->parser->token)) return compileError(c, "Compiler: the export declaration is not compatible with a type (should have no argument)\n");
		def = pkgAddConstInt(c->th, c->pkg, c->parser->token, num++, MM.I); if (!def) return NULL;
		defSetParser(def, c, pIndex);
	}
}
Type* compileExport(Compiler* c)
{
	int count = 0;
	if (c->pkg->first) return compileError(c, "Compiler: export declarations must be before any other declaration\n");
	while (parserNext(c)) {
		if (!islabel(c->parser->token)) {
			if (count) {
				STACKPUSHNIL_ERR(c->th, NULL);
				while(count) {
					STACKMAKETABLE_ERR(c->th, LIST_LENGTH, DBG_LIST, NULL);
					count--;
				}
				STACKPUSHPNT_ERR(c->th, c->exports, NULL);
				STACKMAKETABLE_ERR(c->th, LIST_LENGTH, DBG_LIST, NULL);
				c->exports = STACKPULLPNT(c->th);
			}
			if (!strcmp(c->parser->token, ";;")) return MM.Boolean;
			if (strcmp(c->parser->token, ",")) return compileError(c, "Compiler: ';;' or ';' or label expected (found %s)\n", compileToken(c));
		}
		else {
			if ((!count)&&(compileFunctionIsPrivate(c->parser->token))) return compileError(c, "Compiler: cannot export private labels starting with underscore like '%s'\n", compileToken(c));
			STACKPUSHSTR_ERR(c->th, c->parser->token, -1, NULL);
			count++;
		}
	}
	return NULL;
}

Type* compileFile1(Compiler* c, int depth, int depthFail)
{
	c->parser->mayGetBackToParent = 1;
	while (parserNext(c))
	{
		LB* name;
		char* token = c->parser->token;
		c->parser->mayGetBackToParent = 0;
		if (!strcmp(token, "#"))
		{
			if (!parserNext(c)) return compileError(c, "Compiler: missing condition after '#'\n");
			token = c->parser->token;
			if (!strcmp(token, "ifdef"))
			{
				if (!parserNext(c)) return compileError(c, "Compiler : label expected (found EOF)\n");
				if (!islabel(c->parser->token)) return compileError(c, "Compiler : label expected (found '%s')\n", c->parser->token);
				depth+=2;
				if (!compileGetDef(c)) {
					if (!depthFail) depthFail = depth;
				}
			}
			else if (!strcmp(token, "ifndef"))
			{
				if (!parserNext(c)) return compileError(c, "Compiler : label expected (found EOF)\n");
				if (!islabel(c->parser->token)) return compileError(c, "Compiler : label expected (found '%s')\n", c->parser->token);
				depth+=2;
				if (compileGetDef(c)) {
					if (!depthFail) depthFail = depth;
				}
			}
			else if (!strcmp(token, "else"))
			{
				if (!depth) return compileError(c, "Compiler : #else without #ifdef or #ifndef\n");
				if (depthFail == depth) depthFail = 0;
				else if (!depthFail) depthFail = depth;
			}
			else if (!strcmp(token, "endif"))
			{
				if (!depth) return compileError(c, "Compiler : #endif without #ifdef or #ifndef\n");
				if (depthFail >= depth-1) depthFail = 0;
				depth-=2;
			}
			else if (!strcmp(token, "elifdef"))
			{
				if (!depth) return compileError(c, "Compiler : #elifdef without #ifdef or #ifndef\n");
				if (!parserNext(c)) return compileError(c, "Compiler : label expected (found EOF)\n");
				if (!islabel(c->parser->token)) return compileError(c, "Compiler : label expected (found '%s')\n", c->parser->token);

				if (!depthFail) depthFail = depth-1;
				else if (compileGetDef(c)) {
					if (depthFail == depth) depthFail = 0;
				}
				else {
					if (!depthFail) depthFail = depth;
				}
			}
			else if (!strcmp(token, "elifndef"))
			{
				if (!depth) return compileError(c, "Compiler : #elifndef without #ifdef or #ifndef\n");
				if (!parserNext(c)) return compileError(c, "Compiler : label expected (found EOF)\n");
				if (!islabel(c->parser->token)) return compileError(c, "Compiler : label expected (found '%s')\n", c->parser->token);

				if (!depthFail) depthFail = depth - 1;
				else if (!compileGetDef(c)) {
					if (depthFail == depth) depthFail = 0;
				}
				else {
					if (!depthFail) depthFail = depth;
				}
			}
			else return compileError(c, "Compiler: unknown keyword after '#' '%s'\n", token);
		}
		else if (depthFail) {
			if (parserUntil(c, ";;")) return NULL;
		}
		else if (!strcmp(token, "fun"))
		{
			name = compileName(c, "name of function", 0); if (!name) return NULL;
			if (!compileFun1(c, name)) return compileErrorInFunction(c, "Compiler: error compiling function '%s'\n", STRSTART(name));
		}
		else if (!strcmp(token, "sum"))
		{
			name = compileName(c, "name of sum", 1); if (!name) return NULL;
			if (!compileType1(c, name,1)) return compileError(c, "Compiler: error compiling type '%s'\n", STRSTART(name));
		}
		else if (!strcmp(token, "struct"))
		{
			name = compileName(c, "name of struct", 1); if (!name) return NULL;
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
			LINT code = strcmp(token, "var") ? DEF_CODE_CONST : DEF_CODE_VAR;
			name = compileName(c, "name of definition", 1); if (!name) return NULL;
			if (!compileVarOrConst1(c, code, name))
			{
				if (code == DEF_CODE_VAR) return compileError(c, "Compiler: error compiling var '%s'\n", STRSTART(name));
				return compileError(c, "Compiler: error compiling const '%s'\n", STRSTART(name));
			}
		}
		else if (!strcmp(token, "use"))
		{
			if (!compileImport(c)) return compileError(c, "Compiler: error compiling import\n");
			if (parserAssume(c, ";;")) return NULL;
		}
		else if (!strcmp(token, "include"))
		{
			if (!compileInclude(c)) return compileError(c, "Compiler: error compiling include\n");
		}
		else if (!strcmp(token, "export"))
		{
			if (!compileExport(c)) return compileError(c, "Compiler: error compiling public\n");
		}
		else return compileError(c, "Compiler: unknown declaration '%s'\n", token);
		c->parser->mayGetBackToParent = 1;
	}
	return MM.Boolean;	// anything except NIL
}
Type* checkExport(Compiler* c)
{
	LB* exports = c->exports;
	if (exports) {
		Def* p = c->pkg->first;
		while (p) {
			p->public = DEF_HIDDEN;
			p = p->next;
		}
	}
	while (exports) {
		LB* labels = TABPNT(exports, LIST_VAL);
		char* label = STRSTART(TABPNT(labels, LIST_VAL));
		Def* def = pkgGet(c->pkg, label, 0);
		if (!def) return compileError(c, "Compiler: exported label '%s' not defined\n", label);
		def->public = DEF_PUBLIC;
		exports = TABPNT(exports, LIST_NXT);
	}
	return MM.Boolean;
}

Type* compileStep1(Compiler* c)
{
	Type* t = compileFile1(c, 0, 0);
	if (!t) return NULL;
	if (!checkExport(c)) return NULL;
	return t;
}
