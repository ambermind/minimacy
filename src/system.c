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

LINT PkgCounter = 1;

void defMark(LB* user)
{
	Def* def=(Def*)user;
//	PRINTF(LOG_DEV,"\n---def %s %llx\n",STRSTART(def->name),def->val);
	if (def->valType==VAL_TYPE_PNT) MEMORYMARK(user,VALTOPNT(def->val));
	MEMORYMARK(user,(LB*)def->type);
	MEMORYMARK(user,def->name);
	MEMORYMARK(user,def->instances);
	MEMORYMARK(user,(LB*)def->parent);
	MEMORYMARK(user,(LB*)def->pkg);
	MEMORYMARK(user,(LB*)def->parser);
	MEMORYMARK(user,(LB*)def->next);
}

Def* defAlloc(Thread* th, LINT code,LINT index,LW val,int valType,Type* type)
{
	Def* t=(Def*)memoryAllocExt(th, sizeof(Def),DBG_DEF,NULL,defMark); if (!t) return NULL;
	t->code=code;
	t->index=index;
	t->type=type;
	t->name=NULL;
	t->proto = 0;
	t->tagged = 0;
	t->instances = NULL;
	t->parent = NULL;
	t->dI = t->dCI = 0;
	t->pkg = NULL;

	t->parser=NULL;
	t->parserIndex = 0;
	t->public = DEF_PUBLIC;
	t->next=NULL;
	defSet(t,val,valType);
	return t;
}
void defSetParser(Def* def, Compiler* c, LINT index)
{
	def->parser= c->parser;
	def->parserIndex = index;
}
void defSet(Def* def,LW val,int valType)
{
	def->val=val;
	def->valType=valType;
	if (valType==VAL_TYPE_PNT) MEMORYMARK((LB*)def,VALTOPNT(def->val));
}
char* defName(Def* def)
{
	if (!def) return "[NULL]";
	if (!def->name) return "[NO NAME]";
	return STRSTART(def->name);
}
char* defPkgName(Def* def)
{
	if (!def) return "[NULL]";
	if (!def->pkg) return "[NO PKG]";
	if (!def->pkg->name) return "[NO NAME]";
	return STRSTART(def->pkg->name);
}
void pkgMark(LB* user)
{
	Pkg* pkg=(Pkg*)user;
	MEMORYMARK(user,pkg->name);
	MEMORYMARK(user,(LB*)pkg->start);
	MEMORYMARK(user,(LB*)pkg->defs);
	MEMORYMARK(user,pkg->importList);
//	PRINTF(LOG_DEV,"\n----pkg %s %llx\n",STRSTART(pkg->name),pkg);
}

Pkg* pkgAlloc(Thread* th, LB* name, int nbits, int type)
{
	Pkg* pkg;
	memoryEnterFast();
	pkg=(Pkg*)memoryAllocExt(th, sizeof(Pkg),DBG_PKG,NULL,pkgMark);	if (!pkg) return NULL;
	pkg->name=name;
	pkg->importList = NULL;
	pkg->first=NULL;
	pkg->start=NULL;
	pkg->stage = PKG_STAGE_READY;
	if (type== PKG_FROM_IMPORT)
	{
		pkg->uid = PkgCounter++;
		pkg->forPrompt = 0;
		pkg->listNext = MM.listPkgs;
		MM.listPkgs = pkg;
	}
	else
	{
		pkg->uid = 0;
		pkg->forPrompt = 1;
		pkg->listNext = NULL;
	}
	pkg->defs = hashSlotsCreate(th, nbits ? nbits : PACKAGE_HASHMAP_NBITS,DBG_HASHMAP);
	if (!pkg->defs) return NULL;
	pkgMark((LB*)pkg);
	memoryLeaveFast();

//	PRINTF(LOG_DEV,"\nalloc pkg %s %llx\n",STRSTART(pkg->name),pkg);
	return pkg;
}
char* pkgName(Pkg* pkg)
{
	if (!pkg) return "[NULL]";
	if (!pkg->name) return "[NO NAME]";
	return STRSTART(pkg->name);
}
/*
void pkgSetStart(Pkg* pkg, LB* start)
{
	pkg->start=start;
	MEMORYMARK(pkg,pkg->start);
}
*/
int pkgAddDef(Thread* th,Pkg* pkg, LB* name, Def* def)
{
	int k;
	def->name=name;
	def->pkg = pkg;
	if ((k = hashmapDictAdd(th, pkg->defs, name, (LB*)def))) return k;
	if (name && compileFunctionIsPrivate(STRSTART(name))) def->public = DEF_HIDDEN;
	def->next = pkg->first;
	pkg->first = def;
	return 0;
}
int pkgAddImport(Compiler *c, Pkg* pkg, Pkg* pkgImport, LB* alias)
{
	Thread* th=c->th;
	FUN_PUSH_PNT((alias));
	FUN_PUSH_PNT( (LB*)(pkgImport));
	FUN_MAKE_TABLE( IMPORT_LENGTH, DBG_TUPLE);
	FUN_PUSH_PNT((pkg->importList));
	FUN_MAKE_TABLE( LIST_LENGTH, DBG_LIST);
	pkg->importList = STACKPULLPNT(th);
	return 0;
}
Pkg* pkgImportByAlias(Pkg* pkg, char* alias)
{
	LB* p= pkg->importList;
	while (p)
	{
		LB* i = (TABPNT(p, LIST_VAL));
		LB* q = (TABPNT(i, IMPORT_ALIAS));
		if (q && !strcmp(STRSTART(q), alias)) return (Pkg*)(TABPNT(i, IMPORT_PKG));
		p = (TABPNT(p, LIST_NXT));
	}
	return NULL;
}

Pkg* pkgImportByName(char* name)
{
	Pkg* p = MM.listPkgs;
	while (p)
	{
		if (p->name && !strcmp(STRSTART(p->name), name)) return p;
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
			MEMORYMARK((LB*)MM.system, (LB*)(*q));
		}
		else q = &((*q)->listNext);
	}

}

Def* pkgGet(Pkg* pkg, char* name, int followParent)
{
//	int tron = !strcmp(name, "boot");
	LINT len=strlen(name);
	LINT nbits = pkg->defs->nbits;
	LINT index=hashSlotsComputeString(pkg->defs->nbits, name, len);
	LB* list = pkg->importList;
	LB* p = hashmapDictGetOpti(pkg->defs, name, len, index);
//	if (tron) PRINTF(LOG_DEV,"searching boot\n");
	if (p) return (Def*)p;
	
//	if (tron) PRINTF(LOG_DEV,"searching boot2\n");

//	if (pkg == MM.system) return NULL;
//	if (tron) PRINTF(LOG_DEV,"searching boot3\n");
	if (!followParent) return NULL;
//	if (tron) PRINTF(LOG_DEV,"entering imports\n");
	while(1)
	{
		LB* i = list?(TABPNT(list, LIST_VAL)):NULL;
		pkg = i? (Pkg*)(TABPNT(i, IMPORT_PKG)):MM.system;
		if (pkg)
		{
			if (pkg->defs->nbits != nbits)
			{
				nbits = pkg->defs->nbits;
				index = hashSlotsComputeString(pkg->defs->nbits, name, len);
			}
			p = hashmapDictGetOpti(pkg->defs, name, len, index);
			if (p) {
				Def* def = (Def*)p;
				if (def->public!= DEF_HIDDEN) return (Def*)p;
			}
		}
		if (!list) return NULL;
		list= (TABPNT(list, LIST_NXT));
	}
}
void pkgRemoveDef(Def* def)
{
	Def* d;
	if ((!def)||(!def->pkg)) return;
	hashmapAdd(NULL, 0, def->pkg->defs, PNTTOVAL(def->name), VAL_TYPE_PNT);
	d = def->pkg->first;
	if (d == def) def->pkg->first=def->next;
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

int pkgHasWeak(Compiler* c, Thread* th, Pkg* pkg, int showError)
{
	Def* def = pkg->first;
	int flag = 0;
	while (def)
	{
		if (typeHasWeak(def->type))
		{
			compileError(c, showError?"Compiler: weak type error\n\n": "Compiler: weak type warning\n\n");
			PRINTF(LOG_SYS, "weak type %s.%s: ", pkgName(pkg),defName(def));
			typePrint(th, LOG_SYS, def->type);
			PRINTF(LOG_SYS, "\n");
			flag++;
		}
		def = def->next;
	}
	return flag;
}

LINT _pkgParamLength(Def* def)
{
	if (def->type->nb) return def->type->nb * 3 + 1;
	return 0;
}
LINT _pkgDisplayLeftSize(Def* def)
{
	LINT max;
	LINT len;

	if (!def) return 0;
	max= _pkgDisplayLeftSize(def->next);
	if (!def->name) return max;

	len = STRLEN(def->name);
	if (def->proto) len+=strlen("> proto ");
	else len+=strlen("> ");

	if (def->code >= 0)
	{
		len += strlen("fun ");
	}
	else if (def->code == DEF_CODE_VAR)
	{
		len += strlen("var ");
	}
	else if (def->code == DEF_CODE_CONST)
	{
		len += strlen("const ");
	}
	else if (def->code == DEF_CODE_STRUCT)
	{
		len += strlen("struct ")+ _pkgParamLength(def);
	}
	else if (def->code == DEF_CODE_FIELD)
	{
		len += strlen("field ");
	}
	else if (def->code == DEF_CODE_SUM)
	{
		len += strlen("sum ") + _pkgParamLength(def);
	}
	else if (def->code == DEF_CODE_CONS)
	{
		len += strlen("constr ");
	}
	else if (def->code == DEF_CODE_CONS0)
	{
		len += strlen("constr ");
	}
	else if (def->code == DEF_CODE_TYPE)
	{
		len += strlen("primary ");
	}
	else len += strlen("-->");
	if (len > max) return len;
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
int _pkgDisplay(Thread* th, int mask, LINT padding, Def* def)
{
	char* name;
	char* sep;
	LINT len;
	int k;
	if (!def) return 0;
	if ((k = _pkgDisplay(th, mask, padding,def->next))) return k;
	if (!def->name) return 0;	// will happen only for the temporary extend declaration

	name = STRSTART(def->name);
	len = STRLEN(def->name);
	if (def->proto) len += strlen("> proto ");
	else len += strlen("> ");

	sep=(def->public!= DEF_HIDDEN )?" : ":" * ";

	if (def->proto) PRINTF(mask, "> proto ");
	else PRINTF(mask, "> ");
	if (def->code>=0)
	{
		PRINTF(mask,"fun %s%s%s",name,_padding(padding-(len+strlen("fun "))),sep);
		if ((k=typePrint(th, mask,def->type))) return k;
	}
	else if (def->code==DEF_CODE_VAR)
	{
		PRINTF(mask, "var %s%s%s",name, _padding(padding - (len + strlen("var "))), sep);
		if ((k = typePrint(th, mask,def->type))) return k;
	}
	else if (def->code==DEF_CODE_CONST)
	{
		PRINTF(mask, "const %s%s%s",name, _padding(padding - (len + strlen("const "))), sep);
		if ((k=typePrint(th, mask,def->type))) return k;
	}
	else if (def->code==DEF_CODE_STRUCT)
	{
		Def* p;
		PRINTF(mask, "struct ", name);
		if ((k=typePrint(th, mask, def->type))) return k;
		PRINTF(mask, "%s%s", _padding(padding - (len + strlen("struct ") + _pkgParamLength(def))), sep);
		if (def->parent)
		{
			if ((k = typePrint(th, mask, def->parent->type))) return k;
			PRINTF(mask, " + ");
		}
		PRINTF(mask, "[ ");
		p = (Def*)VALTOPNT(def->val);
		while (p)
		{
			PRINTF(mask, "%s ", STRSTART(p->name));
			p = (Def*)VALTOPNT(p->val);
		}
		PRINTF(mask,"]");
	}
	else if (def->code==DEF_CODE_FIELD)
	{
		PRINTF(mask, "field %s%s%s",name, _padding(padding - (len + strlen("field "))), sep);
		if ((k=typePrint(th, mask,def->type))) return k;
	}
	else if (def->code == DEF_CODE_SUM)
	{
		int first = 1;
		Def* p = (Def*)VALTOPNT(def->val);
		PRINTF(mask, "sum ");
		if ((k=typePrint(th, mask, def->type))) return k;
		PRINTF(mask, "%s%s", _padding(padding - (len + strlen("sum ") + _pkgParamLength(def))), sep);
		while (p)
		{
			if (!first) PRINTF(mask, ", ");
			first = 0;
			PRINTF(mask, "%s", STRSTART(p->name));
			p = (Def*)VALTOPNT(p->val);
		}
	}
	else if (def->code==DEF_CODE_CONS)
	{
		PRINTF(mask, "constr %s%s%s",name, _padding(padding - (len + strlen("constr "))), sep);
		if ((k=typePrint(th, mask,def->type))) return k;
	}
	else if (def->code == DEF_CODE_CONS0)
	{
		PRINTF(mask, "constr %s%s%s",name, _padding(padding - (len + strlen("constr "))), sep);
		if ((k=typePrint(th, mask, def->type))) return k;
	}
	else if (def->code==DEF_CODE_TYPE)
	{
		PRINTF(mask,"primary %s%s%s",name, _padding(padding - (len + strlen("primary "))), sep);
		if ((k = typePrint(th, mask, def->type))) return k;
	}
	else PRINTF(mask, "-->%s",name);
	
	PRINTF(mask, "\n");
	return 0;
}
void pkgDisplayImports(Thread* th, int mask, LINT padding, LB* list)
{
	LB* i;
	Pkg* pkg;
	if (!list) return;
	
	pkgDisplayImports(th, mask,padding,(TABPNT(list, LIST_NXT)));

	i = (TABPNT(list, LIST_VAL));
	pkg = (Pkg*)(TABPNT(i, IMPORT_PKG));
	PRINTF(mask, "> import%s: %s\n", _padding(padding +1 - (strlen("> import"))), pkgName(pkg));
}
int pkgDisplay(Thread* th, int mask, Pkg* pkg)
{
	int k;
	LINT padding = 0;
	PRINTF(mask,">>>>>>>>> package: %s\n", pkgName(pkg));
	padding = _pkgDisplayLeftSize(pkg->first);
	if (padding < ((LINT)strlen("> import"))) padding = strlen("> import");
	pkgDisplayImports(th, mask, padding,pkg->importList);
	if ((k=_pkgDisplay(th, mask,padding,pkg->first))) return k;
	PRINTF(mask, ">\n");
	return 0;
}

Def* pkgAddType(Thread* th, Pkg *pkg,char* name)
{
	Def* def;
	Type* type;
	LB* pname=memoryAllocStr(th, name,-1); if (!pname) return NULL;
	type=typeAlloc(th, TYPECODE_PRIMARY,NULL,0); if (!type) return NULL;
	def=defAlloc(th, DEF_CODE_TYPE,0,NIL,VAL_TYPE_PNT, type); if (!def) return NULL;
	type->def = def;
	if (pkgAddDef(th, pkg, pname, def)) return NULL;
//	PRINTF(LOG_DEV,"Native Type: %s\n", name);
	return def;
}

int pkgAddFun(Thread* th, Pkg *pkg,char* name,NATIVE fun,Type* type)
{
	LB* pfun;
	Def* def;
	LB* pname;
	LB* value= memoryAllocTable(th, FUN_NATIVE_LEN,DBG_TUPLE);
	if (!value) return EXEC_OM;
	pname=memoryAllocStr(th, name,-1); if (!pname) return EXEC_OM;
	TABSETPNT(value,FUN_NATIVE_NAME,pname);
	pfun = memoryAllocBin(th, (char*)&fun, sizeof(NATIVE), DBG_BIN);
	if (!pfun) return EXEC_OM;
	TABSETPNT(value,FUN_NATIVE_POINTER,pfun);
	def = defAlloc(th, type->nb - 1, DEF_INDEX_NATIVE, PNTTOVAL(value),VAL_TYPE_PNT,  type); if (!def) return EXEC_OM;
	return pkgAddDef(th, pkg, pname, def);
}

int pkgAddOpcode(Thread* th, Pkg *pkg,char* name,LINT opcode,Type* type)
{
	Def* def;
	LB* pname;
	LB* value= memoryAllocTable(th, FUN_NATIVE_LEN,DBG_TUPLE);
	if (!value) return EXEC_OM;
	pname = memoryAllocStr(th, name, -1); if (!pname) return EXEC_OM;

	TABSETPNT(value,FUN_NATIVE_NAME,pname);
	TABSETINT(value,FUN_NATIVE_POINTER,opcode);

	def = defAlloc(th, type->nb - 1, DEF_INDEX_OPCODE, PNTTOVAL(value), VAL_TYPE_PNT, type); if (!def) return EXEC_OM;
	return pkgAddDef(th, pkg, pname, def);
}

Def* pkgAddConst(Thread* th, Pkg *pkg,char* name,LW value,int valType, Type* type)
{
	Def* def;
	LB* pname=memoryAllocStr(th, name,-1); if (!pname) return NULL;
	def = defAlloc(th, DEF_CODE_CONST, 0, value, valType, type); if (!def) return NULL;
	if (pkgAddDef(th, pkg, pname, def)) return NULL;
	return def;
}
Def* pkgAddConstInt(Thread* th, Pkg* pkg, char* name, LINT value, Type* type) { return pkgAddConst(th, pkg, name, INTTOVAL(value), VAL_TYPE_INT, type); }
Def* pkgAddConstFloat(Thread* th, Pkg* pkg, char* name, LFLOAT value, Type* type) { return pkgAddConst(th, pkg, name, FLOATTOVAL(value), VAL_TYPE_FLOAT, type); }
Def* pkgAddConstPnt(Thread* th, Pkg* pkg, char* name, LB* value, Type* type) { return pkgAddConst(th, pkg, name, PNTTOVAL(value), VAL_TYPE_PNT, type); }

Def* pkgAddSum(Thread* th, Pkg *pkg,char* name)
{
	Type* mainType;
	Def* defType;
	LB* pname=memoryAllocStr(th, name,-1); if (!pname) return NULL;
	mainType=typeAlloc(th, TYPECODE_PRIMARY, NULL,0); if (!mainType) return NULL;
	defType=defAlloc(th, DEF_CODE_SUM,0,NIL, VAL_TYPE_PNT, mainType); if (!defType) return NULL;
	mainType->def = defType;
	if (pkgAddDef(th, pkg, pname, defType)) return NULL;	// this will also set defType->name
	return defType;
}

// type is supposed to be fun x x x -> defType->type
Def* pkgAddCons(Thread* th, Pkg *pkg,char* name,Def* defType,Type* consType)
{
	Def* defCons;
	LB* pname=memoryAllocStr(th, name,-1); if (!pname) return NULL;
	defCons=defAlloc(th, DEF_CODE_CONS,defType->index++,defType->val,defType->valType, consType); if (!defCons) return NULL;

	defSet(defType,PNTTOVAL((LB*)defCons),VAL_TYPE_PNT);
	defCons->parent = defType;

	if (pkgAddDef(th, pkg, pname, defCons)) return NULL;	// this will also set defType->name
	return defCons;
}

Def* pkgAddCons0(Thread* th, Pkg* pkg, char* name, Def* defType)
{
	Def* defCons;
	LB* pname = memoryAllocStr(th, name, -1); if (!pname) return NULL;
	defCons = defAlloc(th, DEF_CODE_CONS0, defType->index++, defType->val, defType->valType, defType->type); if (!defCons) return NULL;

	defSet(defType,PNTTOVAL((LB*)defCons),VAL_TYPE_PNT);
	defCons->parent = defType;

	if (pkgAddDef(th, pkg, pname, defCons)) return NULL;	// this will also set defType->name
	return defCons;
}

void systemKeywords(Thread* th)
{
	Def* d = MM.system->first;
	while (d)
	{
		char* p = defName(d);
		if (p[0] != '_')
		{
			PRINTF(LOG_USER, "%s:%d:%s: ", p, d->code,d->parser&&d->parser->name?STRSTART(d->parser->name):"");
			typePrint(th, LOG_USER, d->type);
			PRINTF(LOG_USER, "\n");
		}
		d = d->next;
	}
}

void systemInit(Thread* th, Pkg *system)
{
	memoryEnterFast();

	coreInit(th, system);
	coreStrInit(th, system);
	coreBytesInit(th, system);
	coreBinaryInit(th, system);
	coreBignumInit(th, system);
	coreBufferInit(th, system);
	coreConvertInit(th, system);
	coreCryptoInit(th, system);
	coreThreadInit(th, system);
	core2dInit(th, system);
	coreEventInit(th, system);

	coreUiInit(th, system);
	coreAudioInit(th, system);

	sysSocketInit(th, system);
	tmpInit(th, system);
	sysWorkerInit(th, system);
	sysFileInit(th, system);

	coreLzwInit(th, system);
	coreInflateInit(th, system);

//	itemDump(LOG_SYS,PNTTOVAL(th, system),VAL_TYPE_PNT);

	memoryLeaveFast();
}

void systemTerminate(void)
{
	sysSocketClose();
}

