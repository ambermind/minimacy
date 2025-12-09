// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

LINT PkgCounter = 1;

Native* NativeDefs[NATIVE_DEF_LENGTH];
unsigned char NativeDefsArgc[NATIVE_DEF_LENGTH];
Def DefFake;
int NativeDefsCount;
void nativeDefsInit(void)
{
	LINT i;
	NativeDefsCount = 0;
	for (i = 0; i < NATIVE_DEF_LENGTH; i++) NativeDefs[i] = NULL;
}
LINT nativeDefInsert(const Native* n, int nb)
{
	while (nb--) {
		LINT index = hashSlotsComputeString(NATIVE_DEF_BITS, (char*)n->name, strlen(n->name));
		LINT i = index;
		while (1) {
			if (!NativeDefs[i]) {
				NativeDefs[i] = (Native*)n;
//				PRINTF(LOG_DEV,"nativeDefInsert "LSX" in "LSD"\n", n, i);
				n++;
				NativeDefsCount++;
				break;
//				return i;
			}
			i++; if (i >= NATIVE_DEF_LENGTH) i = 0;
			if (i == index) return -1;
		}
	}
	return 0;
}
LINT nativeDefFind(char* name)
{
	LINT index = hashSlotsComputeString(NATIVE_DEF_BITS, name, strlen(name));
	LINT i = index;
	while (1) {
		Native* n = (Native*)NativeDefs[i];
		if (!n) return -1;
//		printf("nativeDefFind test %llx in %lld", n, i);

		if (!strcmp(n->name, name)) return i;
		i++; if (i >= NATIVE_DEF_LENGTH) i = 0;
		if (i == index) return -1;
	}
}
LINT nativeOpcode(char* name, int argc)
{
	LINT i = nativeDefFind(name);
//	PRINTF(LOG_DEV,"nativeOpcode %s %d\n",name,i);
	if (i<0) return -1;
	NativeDefsArgc[i] = argc;
	return i | 0x8000;
}
Def* systemMakeNative(LINT i)
{
	Def* d=NULL;
	Native* n;
	Pkg* pkgSave;
	if (i < 0 || i>= NATIVE_DEF_LENGTH) return NULL;
	n = (Native*)NativeDefs[i]; if (!n) return NULL;

	pkgSave = MM.currentPkg;
	MM.currentPkg = MM.system;

	switch (n->code) {
	case NATIVE_FUN:
//		printf("makeNative '%s': "LSD"\n", n->name, i);
		d=pkgAddOpcodeStr(MM.system, (char*)n->name, i+0x8000, (char*)n->type);
		if (d) NativeDefsArgc[i] = (unsigned char)d->code;
		break;
	case NATIVE_OPCODE:
		d= pkgAddOpcodeStr(MM.system, (char*)n->name, (LINT)n->value, (char*)n->type);
		break;
	case NATIVE_FLOAT:
		d= pkgAddConstFloatStr(MM.system, (char*)n->name, *(LFLOAT*)n->value, (char*)n->type);
		break;
	case NATIVE_INT:
		d= pkgAddConstIntStr(MM.system, (char*)n->name, (LINT)n->value, (char*)n->type);
		break;
	}
	MM.currentPkg = pkgSave;
	return d;
}
Def* systemFakeNative(char* name)
{
	Native* n;
	LINT i = nativeDefFind(name);
	if (i < 0 || i >= NATIVE_DEF_LENGTH) return NULL;
	n = (Native*)NativeDefs[i]; if (!n) return NULL;
	DefFake.type= typeParseStatic(n->type);
	if (!DefFake.type) {
		if (!MM.OM) PRINTF(LOG_SYS,"> WRONG TYPE for native function %s\n", name);
		return NULL;
	}
	DefFake.index = DEF_INDEX_STATIC;
	DefFake.public = DEF_PUBLIC;

	DefFake.name = (LB*)name;	// big hack
	switch (n->code) {
	case NATIVE_FUN:
		DefFake.code = DefFake.type->nb - 1;
		DefFake.val = VAL_FROM_INT(i|0x8000);
		NativeDefsArgc[i] = (unsigned char)DefFake.code;
		return &DefFake;
	case NATIVE_OPCODE:
		DefFake.code = DefFake.type->nb - 1;
		DefFake.val = VAL_FROM_INT((LINT)n->value);
		return &DefFake;
	case NATIVE_FLOAT:
		DefFake.code = DEF_CODE_CONST;
		DefFake.val = VAL_FROM_FLOAT(*(LFLOAT*)n->value);
		return &DefFake;
	case NATIVE_INT:
		DefFake.code = DEF_CODE_CONST;
		DefFake.val = VAL_FROM_INT((LINT)n->value);
		return &DefFake;
	}
	return NULL;
}
Def* systemGetNative(char* name)
{
	return systemMakeNative(nativeDefFind(name));
}
int systemMakeAllNatives(void)
{
	LINT i;
	for (i = 0; i < NATIVE_DEF_LENGTH; i++) if (NativeDefs[i]) {
		Def* def = pkgGet(MM.system, (char*)NativeDefs[i]->name, 0);
		if (MM.OM && !def) return EXEC_OM;
		if (def->index == DEF_INDEX_STATIC) {
			if (!systemMakeNative(i)) return EXEC_OM;
		}
	}
	return 0;
}

void defMark(LB* user)
{
	Def* d=(Def*)user;
//	PRINTF(LOG_DEV,"\n---d %s %llx\n",STR_START(d->name),d->val);
	if (d->valType==VAL_TYPE_PNT) {
		LB* p=PNT_FROM_VAL(d->val);
		MARK_OR_MOVE(p);
		if (MOVING_BLOCKS) d->val=VAL_FROM_PNT(p);
	}
	MARK_OR_MOVE(d->type);
	MARK_OR_MOVE(d->name);
	MARK_OR_MOVE(d->instances);
	MARK_OR_MOVE(d->parent);
	MARK_OR_MOVE(d->parser);
	MARK_OR_MOVE(d->next);
}

Def* defAlloc(LINT code,LINT index,LW val,int valType,Type* type)
{
	Def* d=(Def*)memoryAllocNative(sizeof(Def),DBG_DEF,NULL,defMark); if (!d) return NULL;
	d->code=code;
	d->index=index;
	d->type=type;
	d->name=NULL;
	d->proto = 0;
	d->tagged = 0;
	d->instances = NULL;
	d->parent = NULL;
	d->dI = d->dCI = 0;

	d->parser=NULL;
	d->parserIndex = 0;
	d->public = DEF_PUBLIC;
	d->next=NULL;
	defSet(d,val,valType);
	return d;
}
void defSetParser(Def* d, Compiler* c, LINT index)
{
	d->parser= c->parser;
	BLOCK_MARK(d->parser);
	d->parserIndex = (int)index;
}
void defSet(Def* d,LW val,int valType)
{
	d->val=val;
	d->valType=valType;
	if (valType==VAL_TYPE_PNT) BLOCK_MARK(PNT_FROM_VAL(d->val));
}
char* defName(Def* d)
{
	if (!d) return "[NULL]";
	if (!d->name) return "[NO NAME]";
	return STR_START(d->name);
}
char* defPkgName(Def* d)
{
	if (!d) return "[NULL]";
	if (!d->header.pkg) return "[NO PKG]";
	if (!d->header.pkg->name) return "[NO NAME]";
	return STR_START(d->header.pkg->name);
}

void defReverse(Pkg* p)
{
	Def* newNext = NULL;
	Def* d = p->first;
	while (d) {
		Def* dNext = d->next;
		d->next = newNext;
		newNext = d;
		d = dNext;
	}
	p->first = newNext;
}

void pkgMark(LB* user)
{
	Pkg* pkg=(Pkg*)user;
	MARK_OR_MOVE(pkg->name);
	MARK_OR_MOVE(pkg->first);
	MARK_OR_MOVE(pkg->start);
	MARK_OR_MOVE(pkg->defs);
	MARK_OR_MOVE(pkg->importList);
	if (MOVING_BLOCKS) MARK_OR_MOVE(pkg->listNext);	// this special list doesn't count for marking stage
//	PRINTF(LOG_DEV,"\n----pkg %s %llx\n",STR_START(pkg->name),pkg);
}

Pkg* pkgAlloc(LB* name, int nbits, int type)
{
	Pkg* pkg;
	HashSlots* slots;

	TMP_PUSH(name, NULL);
	pkg=(Pkg*)memoryAllocNative(sizeof(Pkg),DBG_PKG,NULL,pkgMark);	if (!pkg) return NULL;
	TMP_PULL();
	pkg->name=name;
	pkg->memory = 0;
	pkg->importList = NULL;
	pkg->first=NULL;
	pkg->start=NULL;
	pkg->stage = PKG_STAGE_READY;
	pkg->forPrompt = (type == PKG_FROM_IMPORT) ? 0 : 1;
	pkg->listNext = MM.listPkgs;
	MM.listPkgs = pkg;
	TMP_PUSH((LB*)pkg, NULL);
	slots= hashSlotsCreate(nbits ? nbits : PACKAGE_HASHMAP_NBITS,DBG_HASHMAP); if (!slots) return NULL;
	TMP_PULL();
	pkg->defs = slots;
	pkgMark((LB*)pkg);

//	PRINTF(LOG_DEV,"\nalloc pkg %s %llx\n",STR_START(pkg->name),pkg);
	return pkg;
}
char* pkgName(Pkg* pkg)
{
	if (!pkg) return "[NULL]";
	if (!pkg->name) return "[NO NAME]";
	return STR_START(pkg->name);
}

int pkgAddDef(Pkg* pkg, LB* name, Def* d)
{
	int k;
//	PRINTF(LOG_DEV,"def '%s' -> %x\n",STR_START(name),(LINT)d);
	d->name=name;
	d->header.pkg = pkg;
	if ((k = hashmapDictAdd(pkg->defs, name, (LB*)d))) return k;
	if (name && compileFunctionIsPrivate(STR_START(name))) d->public = DEF_HIDDEN;
	d->next = pkg->first;
	pkg->first = d;
	return 0;
}
int pkgAddImport(Compiler *c, Pkg* pkg, LB* alias)
{
	Thread* th=MM.tmpStack;
	FUN_PUSH_PNT((alias));
	FUN_PUSH_PNT( (LB*)(pkg));
	FUN_MAKE_ARRAY( IMPORT_LENGTH, DBG_TUPLE);
	FUN_PUSH_PNT((c->pkg->importList));
	FUN_MAKE_ARRAY( LIST_LENGTH, DBG_LIST);
	c->pkg->importList = STACK_PULL_PNT(th);
	return 0;
}
Pkg* pkgImportByAlias(Pkg* pkg, char* alias)
{
	LB* p= pkg->importList;
	while (p)
	{
		LB* i = (ARRAY_PNT(p, LIST_VAL));
		LB* q = (ARRAY_PNT(i, IMPORT_ALIAS));
		if (q && !strcmp(STR_START(q), alias)) return (Pkg*)(ARRAY_PNT(i, IMPORT_PKG));
		p = (ARRAY_PNT(p, LIST_NXT));
	}
	return NULL;
}

Pkg* pkgImportByName(char* name)
{
	Pkg* p = MM.listPkgs;
	while (p)
	{
		if (p->name && !strcmp(STR_START(p->name), name)) return p;
		p = p->listNext;
	}
	return NULL;
}
void pkgForget(Pkg* pkg)
{
	Pkg** q = &MM.listPkgs;
	while (*q)
	{
		if (*q == pkg)
		{
			*q = pkg->listNext;
			return;
		}
		q = &((*q)->listNext);
	}
}
void pkgCleanCompileError(void)
{
	Pkg** q = &MM.listPkgs;
	while (*q)
	{
		if ((*q)->stage != PKG_STAGE_READY)
		{
			*q = (*q)->listNext;
			BLOCK_MARK((*q));
		}
		else q = &((*q)->listNext);
	}

}
Def* pkgGet(Pkg* pkg, char* name, int followParent)
{
	LINT len=strlen(name);
	LINT nbits = pkg->defs->nbits;
	LINT index=hashSlotsComputeString(pkg->defs->nbits, name, len);
	LB* list = pkg->importList;
	LB* p = hashmapDictGetOpti(pkg->defs, name, len, index);
	if ((!p) && (pkg == MM.system)) p = (LB*)systemFakeNative(name);
	if (p) return (Def*)p;
	
	if (!followParent) return NULL;
	while(1)
	{
		LB* i = list?(ARRAY_PNT(list, LIST_VAL)):NULL;
		pkg = i? (Pkg*)(ARRAY_PNT(i, IMPORT_PKG)):MM.system;
		if (pkg)
		{
			if (pkg->defs->nbits != nbits)
			{
				nbits = pkg->defs->nbits;
				index = hashSlotsComputeString(pkg->defs->nbits, name, len);
			}
			p = hashmapDictGetOpti(pkg->defs, name, len, index);
			if ((!p)&&(pkg==MM.system)) p=(LB*)systemFakeNative(name);
			if (p) {
				Def* d = (Def*)p;
				if (d->public!= DEF_HIDDEN) return d;
			}
		}
		if (!list) return NULL;
		list= (ARRAY_PNT(list, LIST_NXT));
	}
}
void pkgRemoveDef(Def* def)
{
	Def* d;
	if ((!def)||(!def->header.pkg)) return;
	hashmapAdd(NULL, 0, def->header.pkg->defs, VAL_FROM_PNT(def->name), VAL_TYPE_PNT);
	d = def->header.pkg->first;
	if (d == def) def->header.pkg->first=def->next;
	else while(d)
	{
		if (d->next == def)
		{
			d->next = def->next;
			break;
		}
		d = d->next;
	}
}
// search the definition in the package, do not try with its parents
Def* pkgFirstGet(Pkg* pkg, char* name)
{
	return (Def*)hashmapDictGet(pkg->defs,name);
}

int pkgHasWeak(Compiler* c, Pkg* pkg, int showError)
{
	Def* d = pkg->first;
	int flag = 0;
	while (d)
	{
		if (typeHasWeak(d->type))
		{
			compileError(c, showError?"weak type error\n": "weak type warning\n");
			PRINTF(LOG_SYS, ">   weak type %s.%s: ", pkgName(pkg),defName(d));
			typePrint(LOG_SYS, d->type);
			PRINTF(LOG_SYS, "\n");
			flag++;
		}
		d = d->next;
	}
	return flag;
}

LINT _pkgParamLength(Def* d)
{
	if (d->type->nb) return d->type->nb * 3 + 1;
	return 0;
}
LINT _pkgDisplayLeftSize(Def* d)
{
	LINT max=0;
	for(;d;d=d->next) {
		LINT len;
		if (!d->name) continue;

		len = STR_LENGTH(d->name);
		if (d->proto) len+=strlen("> proto ");
		else len+=strlen("> ");

		if (d->code >= 0)
		{
			len += strlen("fun ");
		}
		else if (d->code == DEF_CODE_VAR)
		{
			len += strlen("var ");
		}
		else if (d->code == DEF_CODE_CONST)
		{
			len += strlen("const ");
		}
		else if (d->code == DEF_CODE_STRUCT)
		{
			len += strlen("struct ")+ _pkgParamLength(d);
		}
		else if (d->code == DEF_CODE_FIELD)
		{
			len += strlen("field ");
		}
		else if (d->code == DEF_CODE_SUM)
		{
			len += strlen("sum ") + _pkgParamLength(d);
		}
		else if (d->code == DEF_CODE_CONS)
		{
			len += strlen("constr ");
		}
		else if (d->code == DEF_CODE_CONS0)
		{
			len += strlen("constr ");
		}
		else if (d->code == DEF_CODE_TYPE)
		{
			len += strlen("primary ");
		}
		else len += strlen("-->");
		if (len > max) max=len;
	}
	return max;
}

char* SPACES = "                                                                ";	//64 chars
char* _padding(LINT len)
{
	if (len < 0) len = 0;
	len = strlen(SPACES) - len;
	if (len < 0) len = 0;
	return SPACES + len;
}
int _pkgDisplay(int mask, LINT padding, Pkg* pkg)
{
	char* name;
	char* sep;
	LINT len;
	int k;
	Def* d;
	for (d = pkg->first; d; d = d->next) {
		if (!d->name) continue;	// will happen only for the temporary extend declaration

		name = STR_START(d->name);
		len = STR_LENGTH(d->name);
		if (d->proto) len += strlen("> proto ");
		else len += strlen("> ");

		sep = (d->public != DEF_HIDDEN) ? " : " : " * ";

		if (d->proto) PRINTF(mask, "> proto ");
		else PRINTF(mask, "> ");
		if (d->code >= 0)
		{
			PRINTF(mask, "fun %s%s%s", name, _padding(padding - (len + strlen("fun "))), sep);
			if ((k = typePrint(mask, d->type))) return k;
		}
		else if (d->code == DEF_CODE_VAR)
		{
			PRINTF(mask, "var %s%s%s", name, _padding(padding - (len + strlen("var "))), sep);
			if ((k = typePrint(mask, d->type))) return k;
		}
		else if (d->code == DEF_CODE_CONST)
		{
			PRINTF(mask, "const %s%s%s", name, _padding(padding - (len + strlen("const "))), sep);
			if ((k = typePrint(mask, d->type))) return k;
		}
		else if (d->code == DEF_CODE_STRUCT)
		{
			Def* p;
			PRINTF(mask, "struct ", name);
			if ((k = typePrint(mask, d->type))) return k;
			PRINTF(mask, "%s%s", _padding(padding - (len + strlen("struct ") + _pkgParamLength(d))), sep);
			if (d->parent)
			{
				if ((k = typePrint(mask, d->parent->type))) return k;
				PRINTF(mask, " + ");
			}
			PRINTF(mask, "[ ");
			p = (Def*)PNT_FROM_VAL(d->val);
			while (p)
			{
				PRINTF(mask, "%s ", STR_START(p->name));
				p = (Def*)PNT_FROM_VAL(p->val);
			}
			PRINTF(mask, "]");
		}
		else if (d->code == DEF_CODE_FIELD)
		{
			PRINTF(mask, "field %s%s%s", name, _padding(padding - (len + strlen("field "))), sep);
			if ((k = typePrint(mask, d->type))) return k;
		}
		else if (d->code == DEF_CODE_SUM)
		{
			int first = 1;
			Def* p = (Def*)PNT_FROM_VAL(d->val);
			PRINTF(mask, "sum ");
			if ((k = typePrint(mask, d->type))) return k;
			PRINTF(mask, "%s%s", _padding(padding - (len + strlen("sum ") + _pkgParamLength(d))), sep);
			while (p)
			{
				if (!first) PRINTF(mask, ", ");
				first = 0;
				PRINTF(mask, "%s", STR_START(p->name));
				p = (Def*)PNT_FROM_VAL(p->val);
			}
		}
		else if (d->code == DEF_CODE_CONS)
		{
			PRINTF(mask, "constr %s%s%s", name, _padding(padding - (len + strlen("constr "))), sep);
			if ((k = typePrint(mask, d->type))) return k;
		}
		else if (d->code == DEF_CODE_CONS0)
		{
			PRINTF(mask, "constr %s%s%s", name, _padding(padding - (len + strlen("constr "))), sep);
			if ((k = typePrint(mask, d->type))) return k;
		}
		else if (d->code == DEF_CODE_TYPE)
		{
			PRINTF(mask, "primary %s%s%s", name, _padding(padding - (len + strlen("primary "))), sep);
			if ((k = typePrint(mask, d->type))) return k;
		}
		else PRINTF(mask, "-->%s", name);

		PRINTF(mask, "\n");
	}
	return 0;
}
void pkgDisplayImports(int mask, LINT padding, LB* list)
{
	LB* i;
	Pkg* pkg;
	if (!list) return;
	
	pkgDisplayImports(mask,padding,(ARRAY_PNT(list, LIST_NXT)));

	i = (ARRAY_PNT(list, LIST_VAL));
	pkg = (Pkg*)(ARRAY_PNT(i, IMPORT_PKG));
	PRINTF(mask, "> import%s: %s\n", _padding(padding +1 - (strlen("> import"))), pkgName(pkg));
}
int pkgDisplay(int mask, Pkg* pkg)
{
	int k;
	LINT padding = 0;
	PRINTF(mask,">\n> ---- package: %s\n", pkgName(pkg));
	padding = _pkgDisplayLeftSize(pkg->first);
	if (padding < ((LINT)strlen("> import"))) padding = strlen("> import");
	pkgDisplayImports(mask, padding,pkg->importList);
	defReverse(pkg);
	k = _pkgDisplay(mask, padding, pkg);
	defReverse(pkg);
	if (k) return k;
	PRINTF(mask, ">\n");
	return 0;
}

Def* pkgAddType(Pkg *pkg,char* name)
{
	Def* d;
	Type* type;
	LB* pname;
	memoryEnterSafe();
	pname=memoryAllocStr(name,-1); if (!pname) return NULL;
	type=typeAlloc(TYPECODE_PRIMARY,NULL,0); if (!type) return NULL;
	d=defAlloc(DEF_CODE_TYPE,0,NIL,VAL_TYPE_PNT, type); if (!d) return NULL;
	type->def = d;
	if (pkgAddDef(pkg, pname, d)) return NULL;
	memoryLeaveSafe();
//	PRINTF(LOG_DEV,"Native Type: %s\n", name);
	return d;
}

Def* pkgAddOpcodeStr(Pkg* pkg, char* name, LINT opcode, char* typeStr)
{
	Type* type;
	Def* d;
	LB* pname;
	LB* value;
	memoryEnterSafe();
	value = memoryAllocArray(FUN_NATIVE_LENGTH, DBG_TUPLE);
	if (!value) return NULL;
	pname = memoryAllocStr(name, -1); if (!pname) return NULL;

	ARRAY_SET_PNT(value, FUN_NATIVE_NAME, pname);
	ARRAY_SET_INT(value, FUN_NATIVE_OPCODE, opcode);

	type = typeParseStatic(typeStr);
	if (!type) {
		if (!MM.OM) PRINTF(LOG_SYS, "> WRONG TYPE native for definition %s: '%s'\n", name, typeStr);
		return NULL;
	}
	d = defAlloc(type->nb - 1, DEF_INDEX_OPCODE, VAL_FROM_PNT(value), VAL_TYPE_PNT, type); if (!d) return NULL;
	if (pkgAddDef(pkg, pname, d)) return NULL;
	memoryLeaveSafe();
	return d;
}

Def* pkgAddConstStr(Pkg* pkg, char* name, LW value, int valType, char* typeStr)
{
	Type* type;
	Def* d;
	LB* pname;
	memoryEnterSafe();
	pname = memoryAllocStr(name, -1); if (!pname) return NULL;
	type = typeParseStatic(typeStr);
	if (!type) {
		if (!MM.OM) PRINTF(LOG_SYS, "> WRONG TYPE native for definition %s: '%s'\n", name, typeStr);
		return NULL;
	}
	d = defAlloc(DEF_CODE_CONST, 0, value, valType, type); if (!d) return NULL;
	if (pkgAddDef(pkg, pname, d)) return NULL;
	memoryLeaveSafe();
	return d;
}
Def* pkgAddConstIntStr(Pkg* pkg, char* name, LINT value, char* typeStr) { return pkgAddConstStr(pkg, name, VAL_FROM_INT(value), VAL_TYPE_INT, typeStr); }
Def* pkgAddConstFloatStr(Pkg* pkg, char* name, LFLOAT value, char* typeStr) { return pkgAddConstStr(pkg, name, VAL_FROM_FLOAT(value), VAL_TYPE_FLOAT, typeStr); }

Def* pkgAddConst(Pkg *pkg,char* name,LW value,int valType, Type* type)
{
	Def* d;
	LB* pname;
	memoryEnterSafe();
	pname=memoryAllocStr(name,-1); if (!pname) return NULL;
	d = defAlloc(DEF_CODE_CONST, 0, value, valType, type); if (!d) return NULL;
	if (pkgAddDef(pkg, pname, d)) return NULL;
	memoryLeaveSafe();
	return d;
}
Def* pkgAddConstInt(Pkg* pkg, char* name, LINT value, Type* type) { return pkgAddConst(pkg, name, VAL_FROM_INT(value), VAL_TYPE_INT, type); }
Def* pkgAddConstFloat(Pkg* pkg, char* name, LFLOAT value, Type* type) { return pkgAddConst(pkg, name, VAL_FROM_FLOAT(value), VAL_TYPE_FLOAT, type); }
Def* pkgAddConstPnt(Pkg* pkg, char* name, LB* value, Type* type) { return pkgAddConst(pkg, name, VAL_FROM_PNT(value), VAL_TYPE_PNT, type); }

Def* pkgAddSum(Pkg *pkg,char* name)
{
	Type* mainType;
	Def* defType;
	LB* pname;
	memoryEnterSafe();
	pname = memoryAllocStr(name, -1); if (!pname) return NULL;
	mainType=typeAlloc(TYPECODE_PRIMARY, NULL,0); if (!mainType) return NULL;
	defType=defAlloc(DEF_CODE_SUM,0,NIL, VAL_TYPE_PNT, mainType); if (!defType) return NULL;
	mainType->def = defType;
	if (pkgAddDef(pkg, pname, defType)) return NULL;	// this will also set defType->name
	memoryLeaveSafe();
	return defType;
}

// type is supposed to be fun x x x -> defType->type
Def* pkgAddCons(Pkg *pkg,char* name,Def* defType,Type* consType)
{
	Def* defCons;
	LB* pname;
	memoryEnterSafe();
	pname=memoryAllocStr(name,-1); if (!pname) return NULL;
	defCons=defAlloc(DEF_CODE_CONS,defType->index++,defType->val,defType->valType, consType); if (!defCons) return NULL;

	defSet(defType,VAL_FROM_PNT((LB*)defCons),VAL_TYPE_PNT);
	defCons->parent = defType;

	if (pkgAddDef(pkg, pname, defCons)) return NULL;	// this will also set defType->name
	memoryLeaveSafe();
	return defCons;
}

Def* pkgAddCons0(Pkg* pkg, char* name, Def* defType)
{
	Def* defCons;
	LB* pname;
	memoryEnterSafe();
	pname = memoryAllocStr(name, -1); if (!pname) return NULL;
	defCons = defAlloc(DEF_CODE_CONS0, defType->index++, defType->val, defType->valType, defType->type); if (!defCons) return NULL;

	defSet(defType,VAL_FROM_PNT((LB*)defCons),VAL_TYPE_PNT);
	defCons->parent = defType;

	if (pkgAddDef(pkg, pname, defCons)) return NULL;	// this will also set defType->name
	memoryLeaveSafe();
	return defCons;
}

void systemInit(Pkg *system)
{
	nativeDefsInit();

	typesInit(system);
	systemCoreInit(system);
	systemStrInit(system);
	systemBytesInit(system);
	systemBinaryInit(system);
	systemBignumInit(system);
	systemBufferInit(system);
	systemConvertInit(system);
	systemCryptoInit(system);
	systemThreadInit(system);
	system2dInit(system);
	systemEventInit(system);

	systemUiInit(system);
	systemAudioInit(system);

	sysSocketInit(system);
	sysSerialInit(system);
	systemTmpInit(system);
	systemWorkerInit(system);
	systemFileInit(system);

	systemLzwInit(system);
	systemInflateInit(system);

//	systemMakeAllNatives();

	if (MM.gcTrace) PRINTF(LOG_DEV, "> Native definitions: %d\n", system->defs->nb + NativeDefsCount);
//	itemDump(LOG_SYS,VAL_FROM_PNT(th, system),VAL_TYPE_PNT);
}

void systemTerminate(void)
{
	sysSocketClose();
}

