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

void refMark(LB* user)
{
	Ref* ref=(Ref*)user;
//	printf("\n---ref %s %llx\n",STRSTART(ref->name),ref->val);
	if (ISVALPNT(ref->val)) MEMORYMARK(user,VALTOPNT(ref->val));
	MEMORYMARK(user,(LB*)ref->type);
	MEMORYMARK(user,ref->name);
	MEMORYMARK(user,ref->instances);
	MEMORYMARK(user,(LB*)ref->parent);
	MEMORYMARK(user,(LB*)ref->pkg);
	MEMORYMARK(user,(LB*)ref->parser);
	MEMORYMARK(user,(LB*)ref->next);
}

Ref* refAlloc(Thread* th, LINT code,LINT index,LW val,Type* type)
{
	Ref* t=(Ref*)memoryAllocExt(th, sizeof(Ref),DBG_REF,NULL,refMark); if (!t) return NULL;
	t->code=code;
	t->index=index;
	t->val=val;
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
	t->public = REF_PUBLIC_DEFAULT;
	t->next=NULL;
	return t;
}
void refSetParser(Ref* ref, Compiler* c, LINT index)
{
	ref->parser= c->parser;
	ref->parserIndex = index;
}
void refSet(Ref* ref,LW val)
{
	ref->val=val;
	if (ISVALPNT(ref->val)) MEMORYMARK((LB*)ref,VALTOPNT(ref->val));
}
char* refName(Ref* ref)
{
	if (!ref) return "[NULL]";
	if (!ref->name) return "[NO NAME]";
	return STRSTART(ref->name);
}
char* refPkgName(Ref* ref)
{
	if (!ref) return "[NULL]";
	if (!ref->pkg) return "[NO PKG]";
	if (!ref->pkg->name) return "[NO NAME]";
	return STRSTART(ref->pkg->name);
}
void pkgMark(LB* user)
{
	Pkg* pkg=(Pkg*)user;
	MEMORYMARK(user,pkg->name);
	MEMORYMARK(user,(LB*)pkg->start);
	MEMORYMARK(user,(LB*)pkg->refs);
	MEMORYMARK(user,pkg->importList);
//	printf("\n----pkg %s %llx\n",STRSTART(pkg->name),pkg);
}

Pkg* pkgAlloc(Thread* th, LB* name, int nbits, int type)
{
	Pkg* pkg;
	memoryEnterFast(th);
	pkg=(Pkg*)memoryAllocExt(th, sizeof(Pkg),DBG_PKG,NULL,pkgMark);	if (!pkg) return NULL;
	pkg->name=name;
	pkg->importList = NULL;
	pkg->first=NULL;
	pkg->start=NULL;
	if (type== PKG_FROM_IMPORT)
	{
		pkg->uid = PkgCounter++;
		pkg->stage = PKG_STAGE_EMPTY;
		pkg->listNext = MM.listPkgs;
		MM.listPkgs = pkg;
	}
	else
	{
		pkg->uid = 0;
		pkg->stage = PKG_STAGE_READY;
		pkg->listNext = NULL;
	}
	pkg->refs = hashmapCreate(th, nbits ? nbits : PACKAGE_HASHSET_NBITS);
	if (!pkg->refs) return NULL;
	pkgMark((LB*)pkg);
	memoryLeaveFast(th);

//	printf("\nalloc pkg %s %llx\n",STRSTART(pkg->name),pkg);
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
int pkgAddRef(Thread* th,Pkg* pkg, LB* name, Ref* ref)
{
	int k;
	ref->name=name;
	ref->pkg = pkg;
	k=hashmapAddAlways(th, pkg->refs, PNTTOVAL(name), PNTTOVAL((LB*)ref));
	if (k) return k;
	if (name && compileFunctionIsPrivate(STRSTART(name))) ref->public = REF_HIDDEN;
	ref->next=pkg->first;
	pkg->first=ref;
	return 0;
}
int pkgAddImport(Compiler *c, Pkg* pkg, Pkg* pkgImport, LB* alias)
{
	STACKPUSH_OM(c->th, PNTTOVAL(alias),EXEC_OM);
	STACKPUSH_OM(c->th, PNTTOVAL(pkgImport),EXEC_OM);
	if (DEFTAB(c->th, IMPORT_LENGTH, DBG_TUPLE)) return EXEC_OM;
	STACKPUSH_OM(c->th, PNTTOVAL(pkg->importList),EXEC_OM);
	if (DEFTAB(c->th, LIST_LENGTH, DBG_LIST)) return EXEC_OM;
	pkg->importList = VALTOPNT(STACKPULL(c->th));
	return 0;
}
Pkg* pkgImportByAlias(Pkg* pkg, char* alias)
{
	LB* p= pkg->importList;
	while (p)
	{
		LB* i = VALTOPNT(TABGET(p, LIST_VAL));
		LB* q = VALTOPNT(TABGET(i, IMPORT_ALIAS));
		if (q && !strcmp(STRSTART(q), alias)) return (Pkg*)VALTOPNT(TABGET(i, IMPORT_PKG));
		p = VALTOPNT(TABGET(p, LIST_NXT));
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


Ref* pkgGet(Pkg* pkg, char* name, int followParent)
{
//	int tron = !strcmp(name, "boot");
	LINT len=strlen(name);
	LINT nbits = pkg->refs->nbits;
	LINT index=hashmapComputeString(pkg->refs->nbits, name, len);
	LB* list = pkg->importList;
	LB* p = VALTOPNT(hashmapDictGetOpti(pkg->refs, name, len, index));
//	if (tron) printf("searching boot\n");
	if (p) return (Ref*)p;
	
//	if (tron) printf("searching boot2\n");

//	if (pkg == MM.system) return NULL;
//	if (tron) printf("searching boot3\n");
	if (!followParent) return NULL;
//	if (tron) printf("entering imports\n");
	while(1)
	{
		LB* i = list?VALTOPNT(TABGET(list, LIST_VAL)):NULL;
		pkg = i? (Pkg*)VALTOPNT(TABGET(i, IMPORT_PKG)):MM.system;
		if (pkg)
		{
			if (pkg->refs->nbits != nbits)
			{
				nbits = pkg->refs->nbits;
				index = hashmapComputeString(pkg->refs->nbits, name, len);
			}
			p = VALTOPNT(hashmapDictGetOpti(pkg->refs, name, len, index));
			if (p) {
				Ref* ref = (Ref*)p;
				if (ref->public!= REF_HIDDEN) return (Ref*)p;
			}
		}
		if (!list) return NULL;
		list= VALTOPNT(TABGET(list, LIST_NXT));
	}
}
void pkgRemoveRef(Ref* ref)
{
	Ref* r;
	if ((!ref)||(!ref->pkg)) return;
	hashmapAdd(NULL, ref->pkg->refs, PNTTOVAL(ref->name), NIL);
	r = ref->pkg->first;
	if (r == ref) ref->pkg->first=ref->next;
	else while(r)
	{
		if (r->next == ref)
		{
			r->next = ref->next;
			break;
		}
		r = r->next;
	}
}
// search the reference in the package, do not try with its parents
Ref* pkgFirstGet(Pkg* pkg, char* name)
{
	return (Ref*)VALTOPNT(hashmapDictGet(pkg->refs,name));
}
void pkgHideAll(Pkg* pkg)
{
	Ref* ref = pkg->first;
	while (ref)
	{
		if (ref->public==1) ref->public = REF_HIDDEN;
		ref = ref->next;
	}
}
int pkgHasWeak(Thread* th, Pkg* pkg)
{
	Ref* ref = pkg->first;
	int flag = 0;
	while (ref)
	{
		if (typeHasWeak(ref->type))
		{
			PRINTF(th, LOG_ERR, "!! weak type %s.%s: ", pkgName(pkg),refName(ref));
			typePrint(th, LOG_ERR, ref->type);
			PRINTF(th, LOG_ERR, "\n");
			flag++;
		}
		ref = ref->next;
	}
	return flag;
}

LINT _pkgParamLength(Ref* ref)
{
	if (ref->type->nb) return ref->type->nb * 3 + 1;
	return 0;
}
LINT _pkgDisplayLeftSize(Ref* ref)
{
	char* name;
	LINT max;
	LINT len;

	if (!ref) return 0;
	max= _pkgDisplayLeftSize(ref->next);
	if (!ref->name) return max;

	name = STRSTART(ref->name);
	len = STRLEN(ref->name);
	if (ref->proto) len+=strlen("> proto ");
	else len+=strlen("> ");

	if (ref->code >= 0)
	{
		len += strlen("fun ");
	}
	else if (ref->code == REFCODE_VAR)
	{
		len += strlen("var ");
	}
	else if (ref->code == REFCODE_CONST)
	{
		len += strlen("const ");
	}
	else if (ref->code == REFCODE_STRUCT)
	{
		len += strlen("struct ")+ _pkgParamLength(ref);
	}
	else if (ref->code == REFCODE_FIELD)
	{
		len += strlen("field ");
	}
	else if (ref->code == REFCODE_SUM)
	{
		len += strlen("sum ") + _pkgParamLength(ref);
	}
	else if (ref->code == REFCODE_CONS)
	{
		len += strlen("constr ");
	}
	else if (ref->code == REFCODE_CONS0)
	{
		len += strlen("constr ");
	}
	else if (ref->code == REFCODE_TYPE)
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
int _pkgDisplay(Thread* th, int mask, LINT padding, Ref* ref)
{
	char* name;
	char* sep;
	LINT len;
	int k;
	if (!ref) return 0;
	if ((k = _pkgDisplay(th, mask, padding,ref->next))) return k;
	if (!ref->name) return 0;	// will happen only for the temporary extend declaration

	name = STRSTART(ref->name);
	len = STRLEN(ref->name);
	if (ref->proto) len += strlen("> proto ");
	else len += strlen("> ");

	sep=(ref->public!= REF_HIDDEN )?" : ":" * ";

	if (ref->proto) PRINTF(th, mask, "> proto ");
	else PRINTF(th, mask, "> ");
	if (ref->code>=0)
	{
		PRINTF(th, mask,"fun %s%s%s",name,_padding(padding-(len+strlen("fun "))),sep);
		if ((k=typePrint(th, mask,ref->type))) return k;
	}
	else if (ref->code==REFCODE_VAR)
	{
		PRINTF(th, mask, "var %s%s%s",name, _padding(padding - (len + strlen("var "))), sep);
		if ((k = typePrint(th, mask,ref->type))) return k;
	}
	else if (ref->code==REFCODE_CONST)
	{
		PRINTF(th, mask, "const %s%s%s",name, _padding(padding - (len + strlen("const "))), sep);
		if ((k=typePrint(th, mask,ref->type))) return k;
	}
	else if (ref->code==REFCODE_STRUCT)
	{
		Ref* p;
		PRINTF(th, mask, "struct ", name);
		if ((k=typePrint(th, mask, ref->type))) return k;
		PRINTF(th, mask, "%s%s", _padding(padding - (len + strlen("struct ") + _pkgParamLength(ref))), sep);
		if (ref->parent)
		{
			if ((k = typePrint(th, mask, ref->parent->type))) return k;
			PRINTF(th, mask, " + ");
		}
		PRINTF(th, mask, "[ ");
		p = (Ref*)VALTOPNT(ref->val);
		while (p)
		{
			PRINTF(th, mask, "%s ", STRSTART(p->name));
			p = (Ref*)VALTOPNT(p->val);
		}
		PRINTF(th, mask,"]");
	}
	else if (ref->code==REFCODE_FIELD)
	{
		PRINTF(th, mask, "field %s%s%s",name, _padding(padding - (len + strlen("field "))), sep);
		if ((k=typePrint(th, mask,ref->type))) return k;
	}
	else if (ref->code == REFCODE_SUM)
	{
		int first = 1;
		Ref* p = (Ref*)VALTOPNT(ref->val);
		PRINTF(th, mask, "sum ");
		if ((k=typePrint(th, mask, ref->type))) return k;
		PRINTF(th, mask, "%s%s", _padding(padding - (len + strlen("sum ") + _pkgParamLength(ref))), sep);
		while (p)
		{
			if (!first) PRINTF(th, mask, ", ");
			first = 0;
			PRINTF(th, mask, "%s", STRSTART(p->name));
			p = (Ref*)VALTOPNT(p->val);
		}
	}
	else if (ref->code==REFCODE_CONS)
	{
		PRINTF(th, mask, "constr %s%s%s",name, _padding(padding - (len + strlen("constr "))), sep);
		if ((k=typePrint(th, mask,ref->type))) return k;
	}
	else if (ref->code == REFCODE_CONS0)
	{
		PRINTF(th, mask, "constr %s%s%s",name, _padding(padding - (len + strlen("constr "))), sep);
		if ((k=typePrint(th, mask, ref->type))) return k;
	}
	else if (ref->code==REFCODE_TYPE)
	{
		PRINTF(th, mask,"primary %s%s%s",name, _padding(padding - (len + strlen("primary "))), sep);
		if ((k = typePrint(th, mask, ref->type))) return k;
	}
	else PRINTF(th, mask, "-->%s",name);
	
	PRINTF(th, mask, "\n");
	return 0;
}
void pkgDisplayImports(Thread* th, int mask, LINT padding, LB* list)
{
	LB* i;
	Pkg* pkg;
	if (!list) return;
	
	pkgDisplayImports(th, mask,padding,VALTOPNT(TABGET(list, LIST_NXT)));

	i = VALTOPNT(TABGET(list, LIST_VAL));
	pkg = (Pkg*)VALTOPNT(TABGET(i, IMPORT_PKG));
	PRINTF(th, mask, "> import%s: %s\n", _padding(padding +1 - (strlen("> import"))), pkgName(pkg));
}
int pkgDisplay(Thread* th, int mask, Pkg* pkg)
{
	int k;
	LINT padding = 0;
	PRINTF(th, mask,">>>>>>>>> package: %s\n", pkgName(pkg));
	padding = _pkgDisplayLeftSize(pkg->first);
	if (padding < ((LINT)strlen("> import"))) padding = strlen("> import");
	pkgDisplayImports(th, mask, padding,pkg->importList);
	if ((k=_pkgDisplay(th, mask,padding,pkg->first))) return k;
//	hashmapprintKeys(pkg->refs);
	return 0;
}

Ref* pkgAddType(Thread* th, Pkg *pkg,char* name)
{
	Ref* ref;
	Type* type;
	LB* pname=memoryAllocStr(th, name,-1); if (!pname) return NULL;
	type=typeAlloc(th, TYPECODE_PRIMARY,NULL,0); if (!type) return NULL;
	ref=refAlloc(th, REFCODE_TYPE,0,NIL,type); if (!ref) return NULL;
	type->ref = ref;
	if (pkgAddRef(th, pkg, pname, ref)) return NULL;
//	printf("Native Type: %s\n", name);
	return ref;
}

int pkgAddFun(Thread* th, Pkg *pkg,char* name,NATIVE fun,Type* type)
{
	LB* pfun;
	Ref* ref;
	LB* pname;
	LB* value= memoryAllocTable(th, FUN_NATIVE_LEN,DBG_TUPLE);
	if (!value) return EXEC_OM;
	pname=memoryAllocStr(th, name,-1); if (!pname) return EXEC_OM;
	TABSET(value,FUN_NATIVE_NAME,PNTTOVAL(pname));
	pfun = memoryAllocBin(th, (char*)&fun, sizeof(NATIVE), DBG_BIN);
	if (!pfun) return EXEC_OM;
	TABSET(value,FUN_NATIVE_POINTER,PNTTOVAL(pfun));
	ref = refAlloc(th, type->nb - 1, REFINDEX_NATIVE, PNTTOVAL(value), type); if (!ref) return EXEC_OM;
	return pkgAddRef(th, pkg, pname, ref);
}

int pkgAddOpcode(Thread* th, Pkg *pkg,char* name,LINT opcode,Type* type)
{
	Ref* ref;
	LB* pname;
	LB* value= memoryAllocTable(th, FUN_NATIVE_LEN,DBG_TUPLE);
	if (!value) return EXEC_OM;
	pname = memoryAllocStr(th, name, -1); if (!pname) return EXEC_OM;

	TABSET(value,FUN_NATIVE_NAME,PNTTOVAL(pname));
	TABSETINT(value,FUN_NATIVE_POINTER,(opcode));

	ref = refAlloc(th, type->nb - 1, REFINDEX_OPCODE, PNTTOVAL(value), type); if (!ref) return EXEC_OM;
	return pkgAddRef(th, pkg, pname, ref);
}

Ref* pkgAddConst(Thread* th, Pkg *pkg,char* name,LW value,Type* type)
{
	Ref* ref;
	LB* pname=memoryAllocStr(th, name,-1); if (!pname) return NULL;
	ref = refAlloc(th, REFCODE_CONST, 0, value, type); if (!ref) return NULL;
	if (pkgAddRef(th, pkg, pname, ref)) return NULL;
	return ref;
}
Ref* pkgAddSum(Thread* th, Pkg *pkg,char* name)
{
	Type* mainType;
	Ref* refType;
	LB* pname=memoryAllocStr(th, name,-1); if (!pname) return NULL;
	mainType=typeAlloc(th, TYPECODE_PRIMARY, NULL,0); if (!mainType) return NULL;
	refType=refAlloc(th, REFCODE_SUM,0,NIL,mainType); if (!refType) return NULL;
	mainType->ref = refType;
	if (pkgAddRef(th, pkg, pname, refType)) return NULL;	// this will also set refType->name
	return refType;
}

// type is supposed to be fun x x x -> refType->type
Ref* pkgAddCons(Thread* th, Pkg *pkg,char* name,Ref* refType,Type* consType)
{
	Ref* refCons;
	LB* pname=memoryAllocStr(th, name,-1); if (!pname) return NULL;
	refCons=refAlloc(th, REFCODE_CONS,refType->index++,refType->val,consType); if (!refCons) return NULL;

	refType->val = PNTTOVAL((LB*)refCons);
	refCons->parent = refType;

	if (pkgAddRef(th, pkg, pname, refCons)) return NULL;	// this will also set refType->name
	return refCons;
}

Ref* pkgAddCons0(Thread* th, Pkg* pkg, char* name, Ref* refType)
{
	Ref* refCons;
	LB* pname = memoryAllocStr(th, name, -1); if (!pname) return NULL;
	refCons = refAlloc(th, REFCODE_CONS0, refType->index++, refType->val, refType->type); if (!refCons) return NULL;

	refType->val = PNTTOVAL((LB*)refCons);
	refCons->parent = refType;

	if (pkgAddRef(th, pkg, pname, refCons)) return NULL;	// this will also set refType->name
	return refCons;
}

void systemKeywords(Thread* th)
{
	Ref* r = MM.system->first;
	while (r)
	{
		char* p = refName(r);
		if (p[0] != '_')
		{
			PRINTF(th, LOG_USER, "%s:%d:%s: ", p, r->code,r->parser&&r->parser->name?STRSTART(r->parser->name):"");
			typePrint(th, LOG_USER, r->type);
			PRINTF(th, LOG_USER, "\n");
		}
		r = r->next;
	}
}

void systemInit(Thread* th, Pkg *system)
{
	memoryEnterFast(th);

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

	sysSocketInit(th, system);
	tmpInit(th, system);
	sysWorkerInit(th, system);
	sysFileInit(th, system);

	coreLzwInit(th, system);
	coreGzipInit(th, system);

//	itemDump(LOG_ERR,PNTTOVAL(th, system));

	memoryLeaveFast(th);
}

void systemTerminate()
{
	sysSocketClose();
}

