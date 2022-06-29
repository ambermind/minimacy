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

int fun_empty(Thread* th, int argc)
{
	STACKDROPN(th,argc);
	return STACKPUSH(th,NIL);
}

int fun_pkgCreate(Thread* th)
{
	Pkg* pkg = pkgAlloc(th, VALTOPNT(STACKGET(th,1)),0, PKG_EMPTY);
	if (!pkg) return EXEC_OM;
	pkg->importList = VALTOPNT(STACKGET(th, 0));
	STACKSET(th, 1, PNTTOVAL(pkg));
	STACKDROP(th);
	return 0;
}

int fun_pkgNext(Thread* th)
{
	Pkg* p = (Pkg*)VALTOPNT(STACKGET(th, 0));
	STACKSET(th, 0, PNTTOVAL(p ? p->listNext : MM.listPkgs));
	return 0;
}


int fun_pkgImports(Thread* th)
{
	Pkg* pkg = (Pkg*)VALTOPNT(STACKGET(th, 0));
	if (!pkg) return 0;
	STACKSET(th, 0, PNTTOVAL(pkg->importList));
	return 0;
}

int fun_pkgName(Thread* th)
{
	Pkg* pkg = (Pkg*)VALTOPNT(STACKGET(th, 0));
	if (!pkg) return 0;
	STACKSET(th, 0, PNTTOVAL(pkg->name));
	return 0;
}

int fun_pkgRefs(Thread* th)
{
	Pkg* pkg = (Pkg*)VALTOPNT(STACKGET(th, 0));
	if (!pkg) return 0;
	STACKSET(th, 0, PNTTOVAL(pkg->first));
	return 0;
}

int fun_pkgStart(Thread* th)
{
	Pkg* pkg = (Pkg*)VALTOPNT(STACKGET(th, 0));
	if (!pkg) return 0;
	STACKSET(th, 0, PNTTOVAL(pkg->start));
	return 0;
}

int fun_refName(Thread* th)
{
	Ref* ref = (Ref*)VALTOPNT(STACKGET(th, 0));
	if (!ref) return 0;
	STACKSET(th, 0, PNTTOVAL(ref->name));
	return 0;
}

int fun_refType(Thread* th)
{
	Ref* ref = (Ref*)VALTOPNT(STACKGET(th, 0));
	if (!ref) return 0;
	STACKSET(th, 0, PNTTOVAL(ref->type));
	return 0;
}
int fun_refPkg(Thread* th)
{
	Ref* ref = (Ref*)VALTOPNT(STACKGET(th, 0));
	if (!ref) return 0;
	STACKSET(th, 0, PNTTOVAL(ref->pkg));
	return 0;
}

int fun_refIsPublic(Thread* th)
{
	Ref* ref = (Ref*)VALTOPNT(STACKGET(th, 0));
	if (!ref) return 0;
	STACKSET(th, 0, (ref->public != REF_HIDDEN) ?MM.trueRef:MM.falseRef);
	return 0;
}
int fun_refCode(Thread* th)
{
	Ref* ref = (Ref*)VALTOPNT(STACKGET(th, 0));
	if (!ref) return 0;
	STACKSET(th, 0, INTTOVAL(ref->code));
	return 0;
}
int fun_refCodeName(Thread* th)
{
	LB* name;
	Ref* ref = (Ref*)VALTOPNT(STACKGET(th, 0));
	if (!ref) return 0;
	name = memoryAllocStr(th, refCodeName(ref->code), -1); if (!name) return EXEC_OM;
	STACKSET(th, 0, PNTTOVAL(name));
	return 0;
}
int fun_refNext(Thread* th)
{
	Ref* ref = (Ref*)VALTOPNT(STACKGET(th, 0));
	if (!ref) return 0;
	STACKSET(th, 0, PNTTOVAL(ref->next));
	return 0;
}

int fun_refSourceName(Thread* th)
{
	Ref* ref = (Ref*)VALTOPNT(STACKGET(th, 0));
	if (!ref) return 0;
	if (ref->parser) STACKSET(th, 0, PNTTOVAL(ref->parser->name))
	else STACKSET(th, 0, NIL);

	return 0;
}
int fun_refSourceCode(Thread* th)
{
	Ref* ref = (Ref*)VALTOPNT(STACKGET(th, 0));
	if (!ref) return 0;
	if (ref->parser) STACKSET(th, 0, PNTTOVAL(ref->parser->block))
	else STACKSET(th, 0, NIL);

	return 0;
}
int fun_refIndexInCode(Thread* th)
{
	Ref* ref = (Ref*)VALTOPNT(STACKGET(th, 0));
	if (!ref) return 0;
	STACKSET(th, 0, INTTOVAL(ref->parserIndex- (ref->name? STRLEN(ref->name):0) ));
	return 0;
}
int fun_refSimpleFunction(Thread* th)
{
	LW result = NIL;
	Ref* ref = (Ref*)VALTOPNT(STACKGET(th, 0));
	if (!ref) return 0;
	if (ref->code == 0) result = ref->val;
	STACKSET(th, 0, result);
	return 0;
}

int fun_typeToStr(Thread* th)
{
	int k;
	LB* result;
	Type* t = (Type*)VALTOPNT(STACKGET(th, 0));
	if (!t) return 0;
	bufferReinit(MM.tmpBuffer);
	if ((k = typeBuffer(th, MM.tmpBuffer, t))) return k;
	result = memoryAllocFromBuffer(th, MM.tmpBuffer); if (!result) return EXEC_OM;
	STACKSET(th, 0, PNTTOVAL(result));
	return 0;
}


int fun_exit(Thread* th)
{
	PRINTF(th, LOG_ERR, "-> Thread Exit\n");
	STACKPUSH_OM(th, NIL,EXEC_OM);
	return EXEC_EXIT;
}

int fun_reboot(Thread* th)
{
	PRINTF(th, LOG_ERR, "REBOOT NOW...\n");
	STACKPUSH_OM(th, NIL,EXEC_OM);
	MM.reboot = 1;
	return EXEC_EXIT;
}

int fun_gc(Thread* th)
{
	memoryFullGC();
	STACKPUSH_OM(th, NIL,EXEC_OM);
	return 0;
}

#ifdef DBG_MEM_CHECK

int fun_check(Thread* th)
{
	LW w = STACKGET(th, 0);
	if (ISVALPNT(w)) memoryCheck(VALTOPNT(w));
	else memoryCheck((LB*)VALTOINT(w));
	STACKSET(th, 0, NIL);
	memoryFullGC();
	memoryFullGC();
	return 0;
}

int fun_mem(Thread* th)
{
	LW w = STACKGET(th, 0);
	if (ISVALPNT(w)) STACKSETINT(th,0,(LINT)VALTOPNT(w));
	else STACKSET(th, 0, NIL);
	return 0;
}
#endif

int fun_echo(Thread* th)
{
	itemEcho(th, LOG_USER,STACKGET(th,0),0);
	return 0;
}

int fun_echoLn(Thread* th)
{
	itemEcho(th, LOG_USER,STACKGET(th,0),1);
	return 0;
}
int fun_address(Thread* th)
{
	LW w=STACKGET(th, 0);
	if (ISVALPNT(w)&&(w!=MM.trueRef)&&(w!=MM.falseRef))
	{
		LB* p = VALTOPNT(STACKGET(th, 0));
		LINT v = (LINT)p;
		v >>= 2;
		STACKSETINT(th, 0, v);
	}
	else STACKSETNIL(th, 0);
	return 0;
}
int fun_echoBufferize(Thread* th)
{
	memoryUserBufferize(STACKGET(th, 0) == MM.trueRef ? 1 : 0);
	return 0;
}
int fun_echoEnable(Thread* th)
{
	int mask = termCheckMask(~LOG_USER);
	if (STACKGET(th, 0) == MM.trueRef) mask |= LOG_USER;
	termSetMask(mask);
	return 0;
}
int fun_echoIsEnabled(Thread* th)
{
	return STACKPUSH(th,termCheckMask(LOG_USER)?MM.trueRef:MM.falseRef);
}

int fun_console(Thread* th)
{
	int userBufferize= MainTerm.userBufferize;
	MainTerm.userBufferize = 0;
	itemEcho(th, LOG_ERR, STACKGET(th, 0),0);
	MainTerm.userBufferize = userBufferize;
	return 0;
}

int fun_consoleLn(Thread* th)
{
	int userBufferize = MainTerm.userBufferize;
	MainTerm.userBufferize = 0;
	itemEcho(th, LOG_ERR, STACKGET(th, 0),1);
	MainTerm.userBufferize = userBufferize;
	return 0;
}

int fun_consoleBufferize(Thread* th)
{
	memoryErrBufferize(STACKGET(th, 0) == MM.trueRef ? 1 : 0);
	return 0;
}
int fun_consoleEnable(Thread* th)
{
	int mask = termCheckMask(~LOG_ERR);
	if (STACKGET(th, 0) == MM.trueRef) mask |= LOG_ERR;
	termSetMask(mask);
	return 0;
}
int fun_consoleIsEnabled(Thread* th)
{
	return STACKPUSH(th, termCheckMask(LOG_ERR) ? MM.trueRef : MM.falseRef);
}


int fun_time(Thread* th)
{
	STACKPUSH_OM(th,INTTOVAL(hwTime()),EXEC_OM);
	return 0;
}

int fun_timeMs(Thread* th)
{
	STACKPUSH_OM(th, INTTOVAL(hwTimeMs()),EXEC_OM);
	return 0;
}

int fun_arrayCreate(Thread* th)
{
	LINT i;
	LINT NDROP=2-1;
	LW result=NIL;

	LW v=STACKGET(th,0);
	LINT n=VALTOINT(STACKGET(th,1));
	if ((STACKGET(th,1)==NIL)||(n<0)) goto cleanup;
	if (TABLEPUSH(th,n,DBG_ARRAY)) return EXEC_OM;
	NDROP++;
	result = STACKGET(th, 0);
	if (v!=NIL)	for(i=0;i<n;i++) TABSET(VALTOPNT(result),i,v);
cleanup:
	STACKSET(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}
int fun_arrayLength(Thread* th)
{
	LB* p=VALTOPNT(STACKGET(th,0));
	if (!p) return 0;
	STACKSETINT(th,0,(TABLEN(p)));
	return 0;
}

int fun_intToFloat(Thread* th)
{
	LFLOAT f;
	if (STACKGET(th,0)==NIL) return 0;
	f=(LFLOAT)VALTOINT(STACKGET(th,0));
	STACKSETFLOAT(th,0,(f));
	return 0;
}

int fun_floatToInt(Thread* th)
{
	LFLOAT f;
	if (STACKGET(th,0)==NIL) return 0;
	f=VALTOFLOAT(STACKGET(th,0));
	STACKSETINT(th,0,((LINT)f));
	return 0;
}

int fun_fifoCreate(Thread* th)
{
	STACKPUSH_OM(th, NIL,EXEC_OM);
	STACKPUSH_OM(th, NIL,EXEC_OM);
	STACKPUSH_OM(th, INTTOVAL(0),EXEC_OM);
	if (DEFTAB(th, 3, DBG_FIFO)) return EXEC_OM;
	return 0;
}

int fun_fifoList(Thread* th)
{
	LB* fifo = VALTOPNT(STACKGET(th, 0));
	if (!fifo) return 0;
	STACKSET(th, 0, TABGET(fifo, FIFO_START));
	return 0;
}

int fun_fifoCount(Thread* th)
{
	LB* fifo = VALTOPNT(STACKGET(th, 0));
	if (!fifo) return 0;
	STACKSET(th, 0, TABGET(fifo, FIFO_COUNT));
	return 0;
}


int fun_fifoIn(Thread* th)
{
	LINT count;
	LB* fifo = VALTOPNT(STACKGET(th, 1));
	if (!fifo)
	{
		STACKDROP(th);
		return 0;
	}
	STACKPUSH_OM(th, NIL,EXEC_OM);
	if (DEFTAB(th, LIST_LENGTH, DBG_LIST)) return EXEC_OM;
	if (TABGET(fifo, FIFO_START) == NIL)
	{
		TABSET(fifo, FIFO_START, STACKGET(th, 0));
		TABSET(fifo, FIFO_END, STACKGET(th, 0));
	}
	else
	{
		LB* p = VALTOPNT(TABGET(fifo, FIFO_END));
		TABSET(p, LIST_NXT, STACKGET(th, 0));
		TABSET(fifo, FIFO_END, STACKGET(th, 0));
	}
	count=VALTOINT(TABGET(fifo, FIFO_COUNT));
	TABSETINT(fifo, FIFO_COUNT, (count+1));
	STACKDROP(th);
	return 0;
}

int fun_fifoOut(Thread* th)
{
	LB* hd;
	LB* fifo = VALTOPNT(STACKGET(th, 0));
	if (!fifo) return 0;
	hd = VALTOPNT(TABGET(fifo, FIFO_START));
	if (!hd) STACKSETNIL(th, 0);
	else
	{
		LINT count= VALTOINT(TABGET(fifo, FIFO_COUNT));
		STACKSET(th, 0, TABGET(hd, LIST_VAL));
		if (TABGET(fifo, FIFO_START) == TABGET(fifo, FIFO_END))
		{
			TABSETNIL(fifo, FIFO_START);
			TABSETNIL(fifo, FIFO_END);
		}
		else TABSET(fifo, FIFO_START, TABGET(hd, LIST_NXT));
		TABSETINT(fifo, FIFO_COUNT, (count - 1));
	}
	return 0;
}
int fun_fifoNext(Thread* th)
{
	LB* hd;
	LB* fifo = VALTOPNT(STACKGET(th, 0));
	if (!fifo) return 0;
	hd = VALTOPNT(TABGET(fifo, FIFO_START));
	if (!hd) STACKSETNIL(th, 0);
	else STACKSET(th, 0, TABGET(hd, LIST_VAL));
	return 0;
}

/*
int fun_hashIndex(Thread* th)
{
	LINT nbits = VALTOINT(STACKPULL(th));
	LW key = STACKGET(th, 0);
	if (key==NIL) return 0;
	if (nbits < 0) STACKSETNIL(th, 0);
	else STACKSETINT(th, 0, hashmapComputeIndex(nbits,key));
	return 0;
}
*/
int fun_hashmapCreate(Thread* th)
{
	Hashmap* hash=hashmapCreate(th, VALTOINT(STACKGET(th,0)));
	if (!hash) return EXEC_OM;
	STACKSET(th,0,PNTTOVAL(hash));
	return 0;
}

int fun_hashmapGet(Thread* th)
{
	LW key=STACKGET(th,0);
	LB* hash=VALTOPNT(STACKGET(th,1));
	LW result=hashmapGet((Hashmap*)hash,key);
	STACKSET(th,1,result);
	STACKDROP(th);
	return 0;
}

int fun_hashmapBitSize(Thread* th)
{
	Hashmap* hash=(Hashmap*)VALTOPNT(STACKGET(th,0));
	if (hash) STACKSETINT(th, 0, hashmapBitSize(hash));
	return 0;
}
int fun_hashmapCount(Thread* th)
{
	Hashmap* hash=(Hashmap*)VALTOPNT(STACKGET(th,0));
	if (hash) STACKSETINT(th, 0, hash->nb);
	return 0;
}
int fun_hashmapGetSlot(Thread* th)
{
	LINT index = VALTOINT(STACKGET(th, 0));
	Hashmap* hash = (Hashmap*)VALTOPNT(STACKGET(th, 1));
	LW first = hashmapSlotGet(hash, index);
	STACKSET(th, 1, first);
	STACKDROP(th);
	return 0;
}

int fun_hashmapSet(Thread* th)
{
	LW val=STACKGET(th,0);
	LW key=STACKGET(th,1);
	LB* hash=VALTOPNT(STACKGET(th,2));
	int k=hashmapAdd(th, (Hashmap*)hash,key,val);
	if (k) return k;
	STACKSET(th,2,val);
	STACKDROPN(th,2);
	return 0;
}

int fun_bitTest(Thread* th)
{
	LINT b = VALTOINT(STACKPULL(th));
	LINT a = VALTOINT(STACKGET(th,0));
	STACKSET(th, 0, a & b ? MM.trueRef : MM.falseRef);
	return 0;
}
int fun_intRand(Thread* th)
{
	STACKPUSH_OM(th, INTTOVAL(lsRand32(&th->seed)),EXEC_OM);
	return 0;
}
int fun_intRandSetSeed(Thread* th)
{
	lsSrand(&th->seed,VALTOINT(STACKGET(th, 0)));
	return 0;
}

int fun_sleepMs(Thread* th)	// this function is hidden and used only by the scheduler to enter idle state
{
	LINT ms = (int)VALTOINT(STACKGET(th, 0));
	hwSleepMs(ms);
	return 0;
}
int fun_args(Thread* th)
{
	return STACKPUSH(th, PNTTOVAL(MM.args));
}

int fun_setSystemDir(Thread* th)
{
	LB* p = VALTOPNT(STACKGET(th, 0));
	if (!p) return 0;
	if (hwSetSystemDir(STRSTART(p)))
	{
		STACKSETNIL(th, 0);
		return 0;
	}
	return 0;
}

int coreInit(Thread* th, Pkg *system)
{
	LFLOAT pi=3.14159265359;
	LFLOAT e=2.718281828459045;

	Ref* _Ref = pkgAddType(th, system, "Reference");
	Ref* Useless = pkgAddType(th, system, "Useless");
	Type* u0=typeAllocUndef(th);
	Type* u1=typeAllocUndef(th);
	Type* list_u0=typeAlloc(th,TYPECODE_LIST,NULL,1,u0);
	Type* array_u0=typeAlloc(th,TYPECODE_ARRAY,NULL,1,u0);
	Type* map_u0_u1=typeAlloc(th,TYPECODE_MAP,NULL,2,u0,u1);
	Type* fun_u0_u0_Boolean =typeAlloc(th,TYPECODE_FUN,NULL,3,u0,u0,MM.Boolean);
	Type* fifo_u0 = typeAlloc(th,TYPECODE_FIFO, NULL, 1, u0);
	Type* fun_I=typeAlloc(th,TYPECODE_FUN,NULL,1,MM.I);
	Type* fun_B=typeAlloc(th,TYPECODE_FUN,NULL,1,MM.Boolean);
	Type* fun_u0_I = typeAlloc(th,TYPECODE_FUN, NULL, 2, u0, MM.I);
	Type* fun_S_S = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.S, MM.S);
	Type* fun_B_B = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Boolean, MM.Boolean);
	Type* fun_list_S = typeAlloc(th,TYPECODE_FUN, NULL, 1, typeAlloc(th,TYPECODE_LIST, NULL, 1, MM.S));
	Type* fun_F_F = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.F, MM.F);
	Type* fun_F_F_F=typeAlloc(th,TYPECODE_FUN,NULL,3,MM.F,MM.F,MM.F);
	Type* fun_I_I=typeAlloc(th,TYPECODE_FUN,NULL,2,MM.I,MM.I);
	Type* fun_I_I_I=typeAlloc(th,TYPECODE_FUN,NULL,3,MM.I,MM.I,MM.I);
	Type* fun_I_I_Boolean = typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.I, MM.I, MM.Boolean);
	Type* list_S_Pkg = typeAlloc(th,TYPECODE_LIST, NULL, 1, typeAlloc(th,TYPECODE_TUPLE, NULL, 2, MM.S, MM.Pkg));
	Type* mapSlot= typeAlloc(th,TYPECODE_TUPLE, NULL, 3, u0, u1, NULL);
	mapSlot->child[2] = mapSlot;
	MM.Type = pkgAddType(th, system, "Type")->type;


	pkgAddConst(th, system, "version", PNTTOVAL(memoryAllocStr(th, VERSION_MINIMACY,-1)), MM.S);
	pkgAddConst(th, system, "device", PNTTOVAL(memoryAllocStr(th, DEVICE_MODE,-1)), MM.S);
	pkgAddConst(th, system, "_currentDir", PNTTOVAL(memoryAllocStr(th, hwCurrentDir(), -1)), MM.S);
	pkgAddConst(th, system, "_romDir", PNTTOVAL(memoryAllocStr(th, hwRomDir(), -1)), MM.S);
	pkgAddConst(th, system, "userDir", PNTTOVAL(memoryAllocStr(th, hwUserDir(), -1)), MM.S);
	pkgAddFun(th, system,"_setSystemDir",fun_setSystemDir, fun_S_S);
	pkgAddFun(th, system,"args",fun_args, fun_list_S);
	pkgAddFun(th, system,"_exit",fun_exit, fun_B);
	pkgAddFun(th, system, "gc", fun_gc, fun_I);
	pkgAddFun(th, system, "address", fun_address, fun_u0_I);
#ifdef DBG_MEM_CHECK

	pkgAddFun(th, system, "check", fun_check, typeAlloc(th,TYPECODE_FUN, NULL, 2, u0, u0));
	pkgAddFun(th, system, "mem", fun_mem, typeAlloc(th,TYPECODE_FUN, NULL, 2, u0, MM.I));
#endif
	pkgAddFun(th, system, "_reboot", fun_reboot, fun_I);


	pkgAddFun(th, system, "_echoEnable",fun_echoEnable, fun_B_B);
	pkgAddFun(th, system, "_echoIsEnabled",fun_echoIsEnabled, fun_B);
	pkgAddFun(th, system, "_echoBufferize", fun_echoBufferize, fun_B_B);
	
	pkgAddFun(th, system, "_consoleEnable",fun_consoleEnable, fun_B_B);
	pkgAddFun(th, system, "_consoleIsEnabled", fun_consoleIsEnabled, fun_B);
	pkgAddFun(th, system, "_consoleBufferize", fun_consoleBufferize, fun_B_B);
	
	pkgAddOpcode(th, system,"dump", OPdump, typeAlloc(th,TYPECODE_FUN, NULL, 2, u0, u0));
	pkgAddOpcode(th, system,"_dump2", OPdumpd, typeAlloc(th,TYPECODE_FUN, NULL, 2, u0, u0));
	pkgAddFun(th, system,"echo",fun_echo,typeAlloc(th,TYPECODE_FUN,NULL,2,u0,u0));
	pkgAddFun(th, system,"echoLn",fun_echoLn,typeAlloc(th,TYPECODE_FUN,NULL,2,u0,u0));
	pkgAddFun(th, system, "console", fun_console, typeAlloc(th,TYPECODE_FUN, NULL, 2, u0, u0));
	pkgAddFun(th, system, "consoleLn", fun_consoleLn, typeAlloc(th,TYPECODE_FUN, NULL, 2, u0, u0));

	pkgAddFun(th, system, "_pkgCreate", fun_pkgCreate, typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.S, list_S_Pkg , MM.Pkg));
	pkgAddFun(th, system, "_pkgImports", fun_pkgImports, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Pkg, list_S_Pkg));
	pkgAddFun(th, system, "_pkgNext", fun_pkgNext, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Pkg, MM.Pkg));
	pkgAddFun(th, system, "pkgName", fun_pkgName, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Pkg, MM.S));
	pkgAddFun(th, system, "pkgRefs", fun_pkgRefs, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Pkg, _Ref->type));
	pkgAddFun(th, system, "pkgStart", fun_pkgStart, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Pkg, _Ref->type));
	pkgAddFun(th, system, "refName", fun_refName, typeAlloc(th,TYPECODE_FUN, NULL, 2, _Ref->type, MM.S));
	pkgAddFun(th, system, "refCode", fun_refCode, typeAlloc(th,TYPECODE_FUN, NULL, 2, _Ref->type, MM.I));
	pkgAddFun(th, system, "refIsPublic", fun_refIsPublic, typeAlloc(th,TYPECODE_FUN, NULL, 2, _Ref->type, MM.Boolean));
	pkgAddFun(th, system, "refCodeName", fun_refCodeName, typeAlloc(th,TYPECODE_FUN, NULL, 2, _Ref->type, MM.S));
	pkgAddFun(th, system, "refType", fun_refType, typeAlloc(th,TYPECODE_FUN, NULL, 2, _Ref->type, MM.Type));
	pkgAddFun(th, system, "refPkg", fun_refPkg, typeAlloc(th,TYPECODE_FUN, NULL, 2, _Ref->type, MM.Pkg));
	pkgAddFun(th, system, "refNext", fun_refNext, typeAlloc(th,TYPECODE_FUN, NULL, 2, _Ref->type, _Ref->type));
	pkgAddFun(th, system, "refSourceCode", fun_refSourceCode, typeAlloc(th,TYPECODE_FUN, NULL, 2, _Ref->type, MM.S));
	pkgAddFun(th, system, "refSourceName", fun_refSourceName, typeAlloc(th,TYPECODE_FUN, NULL, 2, _Ref->type, MM.S));
	pkgAddFun(th, system, "refIndexInCode", fun_refIndexInCode, typeAlloc(th,TYPECODE_FUN, NULL, 2, _Ref->type, MM.I));
	pkgAddFun(th, system, "_refSimpleFunction", fun_refSimpleFunction, typeAlloc(th,TYPECODE_FUN, NULL, 2, _Ref->type, typeAlloc(th, TYPECODE_FUN, NULL, 1, Useless->type)));
	pkgAddFun(th, system, "typeToStr", fun_typeToStr, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Type, MM.S));


	pkgAddFun(th, system,"time",fun_time,typeAlloc(th,TYPECODE_FUN,NULL,1,MM.I));
	pkgAddFun(th, system,"timeMs",fun_timeMs,typeAlloc(th,TYPECODE_FUN,NULL,1,MM.I));

	pkgAddFun(th, system,"arrayCreate",fun_arrayCreate,typeAlloc(th,TYPECODE_FUN,NULL,3,MM.I,u0,array_u0));
//	pkgAddFun(th, system,"arrayLength",fun_arrayLength,typeAlloc(th,TYPECODE_FUN,NULL,2,array_u0,MM.I));
	pkgAddOpcode(th, system,"arrayLength",OPtablen,typeAlloc(th,TYPECODE_FUN,NULL,2,array_u0,MM.I));

	pkgAddFun(th, system,"hashmapCreate",fun_hashmapCreate,typeAlloc(th,TYPECODE_FUN,NULL,2,MM.I,map_u0_u1));
	pkgAddFun(th, system,"hashmapGet",fun_hashmapGet,typeAlloc(th,TYPECODE_FUN,NULL,3,map_u0_u1,u0,u1));
	pkgAddFun(th, system,"hashmapSet",fun_hashmapSet,typeAlloc(th,TYPECODE_FUN,NULL,4,map_u0_u1,u0,u1,u1));
	pkgAddFun(th, system,"hashmapCount", fun_hashmapCount, typeAlloc(th,TYPECODE_FUN, NULL, 2, map_u0_u1, MM.I));
	pkgAddFun(th, system,"hashmapBitSize", fun_hashmapBitSize, typeAlloc(th,TYPECODE_FUN, NULL, 2, map_u0_u1, MM.I));
	pkgAddFun(th, system,"hashmapGetSlot", fun_hashmapGetSlot, typeAlloc(th,TYPECODE_FUN, NULL, 3, map_u0_u1, MM.I, mapSlot));

//	pkgAddFun(th, system, "hashIndex", fun_hashIndex, typeAlloc(th,TYPECODE_FUN, NULL, 3, u0, MM.I, MM.I));

	pkgAddFun(th, system, "fifoCreate", fun_fifoCreate, typeAlloc(th,TYPECODE_FUN, NULL, 1,fifo_u0));
	pkgAddFun(th, system, "fifoList", fun_fifoList, typeAlloc(th,TYPECODE_FUN, NULL, 2, fifo_u0, list_u0));
	pkgAddFun(th, system, "fifoCount", fun_fifoCount, typeAlloc(th,TYPECODE_FUN, NULL, 2, fifo_u0, MM.I));
	pkgAddFun(th, system, "fifoIn", fun_fifoIn, typeAlloc(th,TYPECODE_FUN, NULL, 3, fifo_u0, u0, fifo_u0));
	pkgAddFun(th, system, "fifoNext", fun_fifoNext, typeAlloc(th,TYPECODE_FUN, NULL, 2, fifo_u0, u0));
	pkgAddFun(th, system, "fifoOut", fun_fifoOut, typeAlloc(th,TYPECODE_FUN, NULL, 2, fifo_u0, u0));

	pkgAddFun(th, system,"intToFloat",fun_intToFloat,typeAlloc(th,TYPECODE_FUN,NULL,2,MM.I,MM.F));
	pkgAddFun(th, system,"floatToInt",fun_floatToInt,typeAlloc(th,TYPECODE_FUN,NULL,2,MM.F,MM.I));

	pkgAddOpcode(th, system,"tron", OPtron, typeAlloc(th,TYPECODE_FUN,NULL,1,MM.I));
	pkgAddOpcode(th, system,"troff", OPtroff, typeAlloc(th,TYPECODE_FUN,NULL,1,MM.I));

	pkgAddOpcode(th, system,"hd", OPhd, typeAlloc(th,TYPECODE_FUN,NULL,2,list_u0,u0));
	pkgAddOpcode(th, system,"tl", OPtl, typeAlloc(th,TYPECODE_FUN,NULL,2,list_u0,list_u0));

	pkgAddOpcode(th, system, "equals", OPeq, fun_u0_u0_Boolean);

	pkgAddConst(th, system,"pi",FLOATTOVAL(pi),MM.F);
	pkgAddConst(th, system,"e",FLOATTOVAL(e),MM.F);

	pkgAddOpcode(th, system,"abs",OPabs,fun_I_I);
	pkgAddOpcode(th, system,"min",OPmin,fun_I_I_I);
	pkgAddOpcode(th, system,"max",OPmax,fun_I_I_I);

	pkgAddOpcode(th, system,"cos",OPcos,fun_F_F);
	pkgAddOpcode(th, system,"sin",OPsin,fun_F_F);
	pkgAddOpcode(th, system,"tan",OPtan,fun_F_F);
	pkgAddOpcode(th, system,"acos",OPacos,fun_F_F);
	pkgAddOpcode(th, system,"asin",OPasin,fun_F_F);
	pkgAddOpcode(th, system,"atan",OPatan,fun_F_F);
	pkgAddOpcode(th, system,"atan2",OPatan2,fun_F_F_F);
	pkgAddOpcode(th, system,"absf",OPabsf,fun_F_F);
	pkgAddOpcode(th, system,"ln",OPln,fun_F_F);
	pkgAddOpcode(th, system,"log",OPlog,fun_F_F);
	pkgAddOpcode(th, system,"exp",OPexp,fun_F_F);
	pkgAddOpcode(th, system,"sqr",OPsqr,fun_F_F);
	pkgAddOpcode(th, system,"sqrt",OPsqrt,fun_F_F);
	pkgAddOpcode(th, system,"floor",OPfloor,fun_F_F);
	pkgAddOpcode(th, system,"ceil",OPceil,fun_F_F);
	pkgAddOpcode(th, system,"round",OPround,fun_F_F);
	pkgAddOpcode(th, system,"minf",OPminf,fun_F_F_F);
	pkgAddOpcode(th, system,"maxf",OPmaxf,fun_F_F_F);

	pkgAddOpcode(th, system,"cosh",OPcosh,fun_F_F);
	pkgAddOpcode(th, system,"sinh",OPsinh,fun_F_F);
	pkgAddOpcode(th, system,"tanh",OPtanh,fun_F_F);

	pkgAddFun(th, system, "bitTest", fun_bitTest, fun_I_I_Boolean);
	pkgAddFun(th, system, "intRand", fun_intRand, typeAlloc(th,TYPECODE_FUN, NULL, 1, MM.I));
	pkgAddFun(th, system, "intRandSetSeed", fun_intRandSetSeed, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.I, MM.I));
	pkgAddFun(th, system, "_sleepMs", fun_sleepMs, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.I, MM.I));
	return 0;
}

