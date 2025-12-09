// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"
Type* TypeTest = NULL;

LB* compileName(Compiler* c, char* what, int startsWithUpper)
{
	if ((!parserNext(c)) || (!isLabel(c->parser->token))) return (LB*) compileError(c,"%s expected (found '%s')\n", what, compileToken(c));
	if ((startsWithUpper)&&(!startsWithUppercase(c->parser->token))) return (LB*) compileError(c,"%s expected, should start with uppercase (found '%s')\n", what, compileToken(c));
	if ((!startsWithUpper)&&(!startsWithLowercase(c->parser->token))) return (LB*) compileError(c,"%s expected, should start with lowercase (found '%s')\n", what, compileToken(c));
	if (pkgFirstGet(c->pkg, c->parser->token)) return (LB*) compileError(c,"'%s' already defined\n", compileToken(c));
	return memoryAllocStr(c->parser->token, -1);
}
LB* exportLabelList(Compiler* c, char* name)
{
	LB* exports = c->exports;
	while (exports) {
		LB* labels = ARRAY_PNT(exports, LIST_VAL);
		char* label = STR_START(ARRAY_PNT(labels, LIST_VAL));
		if (!strcmp(label, name)) return labels;
		exports= ARRAY_PNT(exports, LIST_NXT);
	}
	return NULL;
}
int exportLabelListIsSingle(Compiler* c, char* name)
{
	LB* labelList = exportLabelList(c, name);
	LB* labelNext = labelList ? ARRAY_PNT(labelList, LIST_NXT) : NULL;
	if (labelList && labelNext) return 0;
	return 1;
}

LB* compilePkgName(Compiler* c)
{
	bufferReinit(MM.tmpBuffer);

	while (1)
	{
		if ((!parserNext(c)) || !isAlphanum(c->parser->token))
		{
			compileError(c,"wrong package name (found '%s')\n", compileToken(c));
			return NULL;
		}
		if (bufferAddStr(MM.tmpBuffer, c->parser->token)) return NULL;
		if ((!parserNext(c)) || (strcmp(c->parser->token, "."))) return memoryAllocStr(bufferStart(MM.tmpBuffer),-1);
		if (bufferAddChar(MM.tmpBuffer, '.')) return NULL;
	}
}

Type* compileInclude(Compiler* c)
{
	// parsing of package name
	LB* name = compilePkgName(c); if (!name) return NULL;
	char* parent = pkgName(c->pkg);

	if (c->parser != c->mainParser) return compileError(c, "cannot use include in an included file\n");
	
	if (strstr(STR_START(name),parent)!= STR_START(name) || (strstr(STR_START(name) +strlen(parent), "._")!= STR_START(name) + strlen(parent)))
		return compileError(c,"cannot include package '%s'. Package to include must start with '%s._'. Use 'import' instead.\n", STR_START(name), parent);

	if (strcmp(compileToken(c), ";;"))
		return compileError(c,"';;' expected (found '%s')\n", compileToken(c));

	if (!parserFromIncludes(c, STR_START(name))) return NULL;
	return MM.Int;
}

Type* compileImports(Compiler* c)
{
	LB* p;
	
	for (p = c->pkg->importList; p; p = ARRAY_PNT(p, LIST_NXT)) {
		LINT global;
		Type* result;
		LB* src;
		Pkg* pkgSave;
		int displayed = 0;

		Pkg* pkg = (Pkg*)ARRAY_PNT(ARRAY_PNT(p, LIST_VAL), IMPORT_PKG);
		if (pkg->stage != PKG_STAGE_TO_COMPILE) continue;

		PRINTF(LOG_SYS, "> compiling '%s'\n", pkgName(pkg));
		src = fsReadPackage(pkgName(pkg), NULL, 0);
		if (!src) return compileError(c,"file not found ('%s')\n", pkgName(pkg));

		pkgSave = MM.currentPkg;
		MM.currentPkg = pkg;
		TMP_PUSH(src, NULL);
		result = compile(src, pkg, 1, &displayed);
		TMP_PULL();
		MM.currentPkg = pkgSave;
		if (!result) {
			c->displayed = displayed +1;
			return NULL;
		}

		if (funMakerAddGlobal(c->fmk, (LB*)PNT_FROM_VAL(pkg->start->val), &global)) return NULL;
		if (bc_byte_or_int(c, global, OPconstb, OPconst)) return NULL;
		if (bc_byte_or_int(c, 0, OPexecb, OPexec)) return NULL;
		if (bufferAddChar(c->bytecode, OPdrop)) return NULL;
	}
	return MM.Boolean;	// anything not null
}
Type* compileImport(Compiler* c)
{
	Pkg* pkg;
	LB* alias = NULL;
	LB* name = compilePkgName(c); if (!name) return NULL;

	pkg = pkgImportByName(STR_START(name));
	if (!pkg)
	{
		if (strstr(STR_START(name), "._"))
			return compileError(c,"cannot import package '%s'. Packages to import cannot contain the string '._' in order to distinct packages to import and packages to include\n", STR_START(name));

		pkg = pkgAlloc(name, 0, PKG_FROM_IMPORT); if (!pkg) return NULL;
		pkg->stage = PKG_STAGE_TO_COMPILE;
//		if (!compileImportNew(c, pkg)) return NULL;
	}
	else
	{
		if (pkg->stage== PKG_STAGE_COMPILING)
			return compileError(c,"'import' loop detected for package '%s'.\n", STR_START(name));
	}
	TMP_PUSH(pkg, NULL);
	if (!strcmp(compileToken(c), "as"))
	{
		if ((!parserNext(c)) || (!isLabel(c->parser->token)))
			return compileError(c,"label expected (found '%s')\n", compileToken(c));
		alias = memoryAllocStr(c->parser->token, -1); if (!alias) return NULL;
	}
	else parserRewind(c);
	TMP_PUSH(alias, NULL);
	if (pkgAddImport(c, pkg, alias)) return NULL;
	TMP_PULL();
	TMP_PULL();

	return MM.Int;
}


Type* compileFun1(Compiler *c, LB* name)
{
	LINT argc=0;
	Def* def;
	Type* t;
	Type* u;
	LINT pIndex = parserIndex(c);
	LB* labelList = exportLabelList(c,STR_START(name));
	LB* labelNext = labelList?ARRAY_PNT(labelList, LIST_NXT):NULL;
	if (!parserNext(c)) return compileError(c,"unexpected end of file\n");
	if (!strcmp(c->parser->token, ":")) {
		if (compilerSkipTypeDef(c)) return NULL;
	}
	else parserRewind(c);

	if (parserAssume(c, "(")) return NULL;
	if (labelList) {
		if (!labelNext) return compileError(c, "more arguments than in the export declaration\n");
		if (strcmp(STR_START(ARRAY_PNT(labelNext, LIST_VAL)), "("))
			return compileError(c, "argument name '%s' does not match with export declaration '%s'\n", c->parser->token, STR_START(ARRAY_PNT(labelNext, LIST_VAL)));
		labelNext = ARRAY_PNT(labelNext, LIST_NXT);
	}

	while(1)
	{
		Type* u;
		if (!parserNext(c)) return compileError(c, "locals or ')' expected (found '%s')\n", compileToken(c));
		if (labelList) {
			if (!labelNext) return compileError(c, "more arguments than in the export declaration\n");
			if (strcmp(STR_START(ARRAY_PNT(labelNext, LIST_VAL)), c->parser->token))
				return compileError(c, "argument name '%s' does not match with export declaration '%s'\n", c->parser->token, STR_START(ARRAY_PNT(labelNext, LIST_VAL)));
			labelNext = ARRAY_PNT(labelNext, LIST_NXT);
		}
		if (!strcmp(c->parser->token, ")")) break;
		
		u= typeAllocUndef();  if (!u) return NULL;
		TYPE_PUSH_NULL(u);
		parserRewind(c);
		if (!compileSkipLocal(c)) return NULL;
		argc++;
		if ((!parserNext(c)) || (strcmp(c->parser->token, ",") && strcmp(c->parser->token, ")")))
			return compileError(c, "',' or ')' expected (found '%s')\n", compileToken(c));
		if (labelList) {
			if (!labelNext) return compileError(c, "more arguments than in the export declaration\n");
			if (strcmp(STR_START(ARRAY_PNT(labelNext, LIST_VAL)), c->parser->token))
				return compileError(c, "argument name '%s' does not match with export declaration '%s'\n", c->parser->token, STR_START(ARRAY_PNT(labelNext, LIST_VAL)));
			labelNext = ARRAY_PNT(labelNext, LIST_NXT);
		}
		if (!strcmp(c->parser->token, ")")) break;
	}
	if (labelList && labelNext) return compileError(c,"less arguments than in the export declaration\n");
//	parserRewind(c);

	u = typeAllocUndef();  if (!u) return NULL;
	TYPE_PUSH_NULL(u);

	if (parserAssume(c,"=")) return NULL;

	t = typeAllocFromStack(NULL, TYPECODE_FUN, argc + 1); if (!t) return NULL;
	def = defAlloc(argc, DEF_INDEX_BC, NIL, VAL_TYPE_PNT, t); if (!def) return NULL;
	defSetParser(def, c, pIndex);
	t->def = def;

	def->proto = 1;
	if (!def) return NULL;

	if (pkgAddDef(c->pkg, name, def)) return NULL;
	if (parserUntil(c, ";;")) return NULL;

	return t;
}

Type* compileVarOrConst1(Compiler* c,LINT code, LB* name)
{
	Def* def;
	Type* t = typeAllocWeak(); if (!t) return NULL;
	if (!exportLabelListIsSingle(c, STR_START(name))) return compileError(c,"the export declaration is not compatible with a constant or a variable (should have no argument)\n");

	def = defAlloc(code, DEF_INDEX_VALUE, NIL, VAL_TYPE_PNT, t); if (!def) return NULL;
	def->proto = 1;
	defSetParser(def, c, parserIndex(c));
	if (pkgAddDef(c->pkg, name, def)) return NULL;
	if (parserUntil(c, ";;")) return NULL;
	return def->type;
}

Type* compileType1(Compiler* c, LB* name,int sum)
{
	Def* defType = NULL;
	Type* typeType;
	int n = 0;
	LINT pIndex = parserIndex(c);
	if (!exportLabelListIsSingle(c, STR_START(name))) return compileError(c,"the export declaration is not compatible with a type (should have no argument)\n");

	if (parserNext(c) && !strcmp(c->parser->token, "{"))
	{
		while (1)
		{
			Type* t;
			if ((!parserNext(c)) || !isLabel(c->parser->token)) return compileError(c,"parameter or '}' expected (found '%s')\n", compileToken(c));
			t = typeAllocUndef(); if (!t) return NULL;
			TYPE_PUSH_NULL(t);
			n++;

			if (!parserNext(c)) return compileError(c,"parameter or '}' expected (found '%s')\n", compileToken(c));

			if (!strcmp(c->parser->token, "}")) break;
			parserRewind(c);
		}
	}
	else parserRewind(c);

	typeType = typeAllocFromStack(NULL, TYPECODE_PRIMARY, n); if (!typeType) return NULL;
	if (parserAssume(c, "=")) return NULL;

	if (sum)
	{
		defType = defAlloc(DEF_CODE_SUM, 0, NIL, VAL_TYPE_PNT, typeType); if (!defType) return NULL;
		defType->proto = 1;
		defSetParser(defType, c, pIndex);
		typeType->def = defType;
		if (pkgAddDef(c->pkg, name, defType)) return NULL;	// this will also set defType->name

		return compileDefCons1(c);
	}
	defType = defAlloc(DEF_CODE_STRUCT, 0, NIL, VAL_TYPE_PNT, typeType); if (!defType) return NULL;
	defType->proto = 1;
	defSetParser(defType, c, pIndex);
	typeType->def = defType;

	if (pkgAddDef(c->pkg, name, defType)) return NULL;	// this will also set defType->name
	return compileStructure1(c, defType);
}

Type* compileExtend1(Compiler* c)
{
	Def* def;
	LINT pIndex = parserIndex(c);	// pIndex is before the name of the extended type
	if ((!parserNext(c)) || (!isLabel(c->parser->token))) return compileError(c,"sum name expected, found '%s'\n", compileToken(c));
	def = defAlloc(DEF_CODE_EXTEND, 0, NIL, VAL_TYPE_PNT, NULL); if (!def) return NULL;
	def->proto = 1;
	defSetParser(def, c, pIndex);
	def->next = c->pkg->first;
	c->pkg->first = def;

	if (parserNext(c) && !strcmp(c->parser->token, "{")) {
		if (parserUntil(c, "}")) return NULL;
	}
	else parserRewind(c);

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
		if ((!parserNext(c))||(!isLabel(c->parser->token))) return compileError(c,"label expected (found %s)\n", compileToken(c));

		pIndex = parserIndex(c);
		if (!exportLabelListIsSingle(c, c->parser->token)) return compileError(c,"the export declaration is not compatible with a type (should have no argument)\n");
		def = pkgAddConstInt(c->pkg, c->parser->token, num++, MM.Int); if (!def) return NULL;
		defSetParser(def, c, pIndex);

		if ((!parserNext(c)) || (strcmp(c->parser->token, ",") && strcmp(c->parser->token, ";;"))) return compileError(c, "',' or ';;' expected (found %s)\n", compileToken(c));
		if (!strcmp(c->parser->token, ";;")) return MM.Int;
	}
}
Type* compileExport(Compiler* c)
{
	int count = 0;
	int parentheses = 0;
	if (c->pkg->first) return compileError(c,"export declarations must be before any other declaration\n");
	while (parserNext(c)) {
		if ((!strcmp(c->parser->token, ";;"))||(!parentheses && !strcmp(c->parser->token, ","))) {
			if (count) {
				STACK_PUSH_NIL_ERR(MM.tmpStack, NULL);
				while(count) {
					STACK_PUSH_FILLED_ARRAY_ERR(MM.tmpStack, LIST_LENGTH, DBG_LIST, NULL);
					count--;
				}
				STACK_PUSH_PNT_ERR(MM.tmpStack, c->exports, NULL);
				STACK_PUSH_FILLED_ARRAY_ERR(MM.tmpStack, LIST_LENGTH, DBG_LIST, NULL);
				c->exports = STACK_PULL_PNT(MM.tmpStack);
			}
			if (!strcmp(c->parser->token, ";;")) return MM.Boolean;
		}
		else {
			if (!strcmp(c->parser->token, "(")) parentheses++;
			else if (!strcmp(c->parser->token, ")")) parentheses--;
			if ((!count)&&(compileFunctionIsPrivate(c->parser->token))) return compileError(c,"cannot export private labels starting with underscore like '%s'\n", compileToken(c));
			STACK_PUSH_STR_ERR(MM.tmpStack, c->parser->token, -1, NULL);
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
		memoryEnterSafe();
		if (!strcmp(token, "#"))
		{
			if (!parserNext(c)) return compileError(c, "missing condition after '#'\n");
			token = c->parser->token;
			if (!strcmp(token, "ifdef"))
			{
				if (!parserNext(c)) return compileError(c, "label expected (found EOF)\n");
				if (!isLabel(c->parser->token)) return compileError(c, "label expected (found '%s')\n", c->parser->token);
				depth += 2;
				if (!compileGetDef(c)) {
					if (!depthFail) depthFail = depth;
				}
			}
			else if (!strcmp(token, "ifndef"))
			{
				if (!parserNext(c)) return compileError(c, "label expected (found EOF)\n");
				if (!isLabel(c->parser->token)) return compileError(c, "label expected (found '%s')\n", c->parser->token);
				depth += 2;
				if (compileGetDef(c)) {
					if (!depthFail) depthFail = depth;
				}
			}
			else if (!strcmp(token, "else"))
			{
				if (!depth) return compileError(c, "#else without #ifdef or #ifndef\n");
				if (depthFail == depth) depthFail = 0;
				else if (!depthFail) depthFail = depth;
			}
			else if (!strcmp(token, "endif"))
			{
				if (!depth) return compileError(c, "#endif without #ifdef or #ifndef\n");
				if (depthFail >= depth - 1) depthFail = 0;
				depth -= 2;
			}
			else if (!strcmp(token, "elifdef"))
			{
				if (!depth) return compileError(c, "#elifdef without #ifdef or #ifndef\n");
				if (!parserNext(c)) return compileError(c, "label expected (found EOF)\n");
				if (!isLabel(c->parser->token)) return compileError(c, "label expected (found '%s')\n", c->parser->token);

				if (!depthFail) depthFail = depth - 1;
				else if (compileGetDef(c)) {
					if (depthFail == depth) depthFail = 0;
				}
				else {
					if (!depthFail) depthFail = depth;
				}
			}
			else if (!strcmp(token, "elifndef"))
			{
				if (!depth) return compileError(c, "#elifndef without #ifdef or #ifndef\n");
				if (!parserNext(c)) return compileError(c, "label expected (found EOF)\n");
				if (!isLabel(c->parser->token)) return compileError(c, "label expected (found '%s')\n", c->parser->token);

				if (!depthFail) depthFail = depth - 1;
				else if (!compileGetDef(c)) {
					if (depthFail == depth) depthFail = 0;
				}
				else {
					if (!depthFail) depthFail = depth;
				}
			}
			else return compileError(c, "unknown keyword after '#' '%s'\n", token);
		}
		else if (depthFail) {
			if (parserUntil(c, ";;")) return NULL;
		}
		else if (!strcmp(token, "fun"))
		{
			name = compileName(c, "name of function", 0); if (!name) return NULL;
			if (!compileFun1(c, name)) return compileErrorInFunction(c, "error compiling function '%s'\n", STR_START(name));
		}
		else if (!strcmp(token, "sum"))
		{
			name = compileName(c, "name of sum", 1); if (!name) return NULL;
			if (!compileType1(c, name, 1)) return compileError(c, "error compiling type '%s'\n", STR_START(name));
		}
		else if (!strcmp(token, "struct"))
		{
			name = compileName(c, "name of struct", 1); if (!name) return NULL;
			if (!compileType1(c, name, 0)) return compileError(c, "error compiling type '%s'\n", STR_START(name));
		}
		else if (!strcmp(token, "extend"))
		{
			if (!compileExtend1(c)) return compileError(c, "error compiling extend\n");
		}
		else if (!strcmp(token, "enum"))
		{
			if (!compileEnum(c)) return compileError(c, "error compiling enumeration\n");
		}
		else if ((!strcmp(token, "var")) || (!strcmp(token, "const")))
		{
			LINT code = strcmp(token, "var") ? DEF_CODE_CONST : DEF_CODE_VAR;
			name = compileName(c, "name of definition", 1); if (!name) return NULL;
			if (!compileVarOrConst1(c, code, name))
			{
				if (code == DEF_CODE_VAR) return compileError(c, "error compiling var '%s'\n", STR_START(name));
				return compileError(c, "error compiling const '%s'\n", STR_START(name));
			}
		}
		else if (!strcmp(token, "use"))
		{
			if (!compileImport(c)) return compileError(c, "error compiling import\n");
			if (parserAssume(c, ";;")) return NULL;
		}
		else if (!strcmp(token, "include"))
		{
			if (!compileInclude(c)) return compileError(c, "error compiling include\n");
		}
		else if (!strcmp(token, "export"))
		{
			if (!compileExport(c)) return compileError(c, "error compiling public\n");
		}
		else return compileError(c, "unknown declaration '%s'\n", token);
		c->parser->mayGetBackToParent = 1;
		memoryLeaveSafe();
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
		LB* labels = ARRAY_PNT(exports, LIST_VAL);
		char* label = STR_START(ARRAY_PNT(labels, LIST_VAL));
		Def* def = pkgGet(c->pkg, label, 0);
		if (!def) return compileError(c,"exported label '%s' not defined\n", label);
		def->public = DEF_PUBLIC;
		exports = ARRAY_PNT(exports, LIST_NXT);
	}
	return MM.Boolean;
}

Type* compileStep1(Compiler* c)
{
	Type* t = compileFile1(c, 0, 0);
	if (!t) return NULL;
	if (!checkExport(c)) return NULL;
	if (!compileImports(c)) return NULL;
	return t;
}
