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

int fun_pkgCreate(Thread* th)
{
	Pkg* pkg = pkgAlloc(th, (STACKPNT(th,1)),0, PKG_FROM_NOTHING);
	if (!pkg) return EXEC_OM;
	pkg->importList = STACKPNT(th, 0);
	FUN_RETURN_PNT((LB*)pkg);
}

int fun_pkgNext(Thread* th)
{
	Pkg* p = (Pkg*)STACKPNT(th, 0);
	FUN_RETURN_PNT((LB*)(p ? p->listNext : MM.listPkgs));
}

int fun_pkgImports(Thread* th)
{
	Pkg* p = (Pkg*)STACKPNT(th, 0);
	FUN_RETURN_PNT((LB*)(p ? p->importList : NULL));
}

int fun_pkgName(Thread* th)
{
	Pkg* p = (Pkg*)STACKPNT(th, 0);
	FUN_RETURN_PNT((LB*)(p ? p->name : NULL));
}

int fun_pkgDefs(Thread* th)
{
	Pkg* p = (Pkg*)STACKPNT(th, 0);
	FUN_RETURN_PNT((LB*)(p ? p->first : NULL));
}

int fun_pkgStart(Thread* th)
{
	Pkg* p = (Pkg*)STACKPNT(th, 0);
	FUN_RETURN_PNT((LB*)(p ? p->start : NULL));
}

int fun_defName(Thread* th)
{
	Def* def = (Def*)STACKPNT(th, 0);
	FUN_RETURN_PNT((LB*)(def ? def->name : NULL));
}

int fun_defType(Thread* th)
{
	Def* def = (Def*)STACKPNT(th, 0);
	FUN_RETURN_PNT((LB*)(def ? def->type : NULL));
}
int fun_defPkg(Thread* th)
{
	Def* def = (Def*)STACKPNT(th, 0);
	FUN_RETURN_PNT((LB*)(def ? def->pkg : NULL));
}

int fun_defIsPublic(Thread* th)
{
	Def* def = (Def*)STACKPNT(th, 0);
	if (!def) FUN_RETURN_NIL;
	STACKSETBOOL(th, 0, (def->public != DEF_HIDDEN));
	return 0;
}
int fun_defCode(Thread* th)
{
	Def* def = (Def*)STACKPNT(th, 0);
	if (!def) FUN_RETURN_NIL;
	FUN_RETURN_INT(def->code);
}
int fun_defCodeName(Thread* th)
{
	Def* def = (Def*)STACKPNT(th, 0);
	if (!def) FUN_RETURN_NIL;
	FUN_RETURN_STR(defCodeName(def->code), -1);
}
int fun_defNext(Thread* th)
{
	Def* def = (Def*)STACKPNT(th, 0);
	FUN_RETURN_PNT((LB*)(def ? def->next : NULL));
}

int fun_defSourceName(Thread* th)
{
	Def* def = (Def*)STACKPNT(th, 0);
	if ((!def)||(!def->parser)) FUN_RETURN_NIL;
	FUN_RETURN_PNT(def->parser->name);
}
int fun_defSourceCode(Thread* th)
{
	Def* def = (Def*)STACKPNT(th, 0);
	if ((!def)||(!def->parser)) FUN_RETURN_NIL;
	FUN_RETURN_PNT(def->parser->block);
}
int fun_defIndexInCode(Thread* th)
{
	Def* def = (Def*)STACKPNT(th, 0);
	if ((!def)||(!def->parser)) FUN_RETURN_NIL;

	FUN_RETURN_INT(def->parserIndex- (def->name? STRLEN(def->name):0) );
}
int fun_defSimpleFunction(Thread* th)
{
	Def* def = (Def*)STACKPNT(th, 0);
	FUN_RETURN_PNT((LB*)((def && (def->code ==0)) ? VALTOPNT(def->val) : NULL));
}

int fun_strFromType(Thread* th)
{
	int k;
	Type* t = (Type*)STACKPNT(th, 0);
	if (!t) FUN_RETURN_NIL;
	bufferReinit(MM.tmpBuffer);
	if ((k = typeBuffer(th, MM.tmpBuffer, t))) return k;
	FUN_RETURN_BUFFER(MM.tmpBuffer);
}


int fun_exit(Thread* th)
{
	PRINTF(LOG_SYS, "-> Thread Exit\n");
	STACKSETNIL(th,0);
	return EXEC_EXIT;
}

int fun_reboot(Thread* th)
{
	PRINTF(LOG_SYS, "REBOOT NOW...\n");
	STACKSETNIL(th,0);
	MM.reboot = 1;
	return EXEC_EXIT;
}

int fun_gc(Thread* th)
{
	memoryFullGC();
	FUN_RETURN_NIL
}

int fun_echo(Thread* th)
{
	itemEcho(th, LOG_USER,STACKGET(th,0),STACKTYPE(th,0),0);
	return 0;
}

int fun_echoLn(Thread* th)
{
	itemEcho(th, LOG_USER,STACKGET(th,0),STACKTYPE(th,0),1);
	return 0;
}
int fun_address(Thread* th)
{
	LINT v = (LINT)STACKGET(th, 0);
	if (!STACKISPNT(th, 0)) FUN_RETURN_NIL;
	FUN_RETURN_INT(v);
}

int fun_gcTrace(Thread* th)
{
	MM.gcTrace = (STACKPNT(th, 0) == MM._true) ? 1:0;
	return 0;
}
int fun_echoEnable(Thread* th)
{
	int mask = termCheckMask(~LOG_USER);
	if (STACKPNT(th, 0) == MM._true) mask |= LOG_USER;
	termSetMask(mask);
	return 0;
}
int fun_echoIsEnabled(Thread* th)
{
	FUN_RETURN_BOOL(termCheckMask(LOG_USER));
}

int fun_systemLogEnable(Thread* th)
{
	int mask = termCheckMask(~LOG_SYS);
	if (STACKPNT(th, 0) == MM._true) mask |= LOG_SYS;
	termSetMask(mask);
	return 0;
}
LINT TimeDelta = 0;

int fun_time(Thread* th)
{
	FUN_RETURN_INT(hwTime()+ TimeDelta);
}

int fun_timeMs(Thread* th)
{
	FUN_RETURN_INT(hwTimeMs()+ TimeDelta*1000);
}
int fun_timeSet(Thread* th)
{
	TimeDelta = STACKINT(th, 0)- hwTime();
	return 0;
}


int fun_arrayCreate(Thread* th)
{
	LINT i;
	LB* p=NULL;

//	STACK(th,0) is the initialization value
	LINT n=STACKINT(th,1);
	if (STACKISNIL(th,1)||(n<0)) FUN_RETURN_NIL;
	p= memoryAllocTable(th, n, DBG_ARRAY); if (!p) return EXEC_OM;
	if (!STACKISNIL(th,0)) for(i=0;i<n;i++) STACKSTORE(p,i,th,0);
	FUN_RETURN_PNT(p);
}
int fun_arraySlice(Thread* th)
{
	LB* q;
	LINT i;

	int lenIsNil = STACKISNIL(th,0);
	LINT len = STACKINT(th, 0);
	LINT start = STACKINT(th,1);
	LB* p = STACKPNT(th, 2);
	FUN_SUBSTR(p,start, len, lenIsNil, TABLEN(p));

	q=memoryAllocTable(th, len, DBG_ARRAY); if (!q) return EXEC_OM;
	for (i = 0; i < len; i++) TABCOPY(q, i, p, start + i);
	FUN_RETURN_PNT(q);
}


int fun_floatFromInt(Thread* th)
{
	LFLOAT f;
	if (STACKISNIL(th,0)) FUN_RETURN_NIL;
	f=(LFLOAT)STACKINT(th,0);
	FUN_RETURN_FLOAT(f);
}

int fun_intFromFloat(Thread* th)
{
	LFLOAT f;
	if (STACKISNIL(th,0)) FUN_RETURN_NIL;
	f=STACKFLOAT(th,0);
	FUN_RETURN_INT((LINT)f);
}

int fun_fifoCreate(Thread* th)
{
	FUN_PUSH_NIL;
	FUN_PUSH_NIL;
	FUN_PUSH_INT(0);
	FUN_MAKE_TABLE( 3, DBG_FIFO);
	return 0;
}

int fun_fifoList(Thread* th)
{
	LB* fifo = STACKPNT(th, 0);
	if (!fifo) FUN_RETURN_NIL;
	STACKLOAD(th, 0, fifo, FIFO_START);
	TABSETNIL(fifo, FIFO_START);
	TABSETNIL(fifo, FIFO_END);
	TABSETINT(fifo, FIFO_COUNT, 0);
	return 0;
}

int fun_fifoCount(Thread* th)
{
	LB* fifo = STACKPNT(th, 0);
	if (!fifo) FUN_RETURN_NIL;
	FUN_RETURN_INT(TABINT(fifo, FIFO_COUNT));
}

int fun_fifoIn(Thread* th)
{
	LINT count;
	LB* fifo = STACKPNT(th, 1);
	if (!fifo) FUN_RETURN_NIL;
	FUN_PUSH_NIL;
	FUN_MAKE_TABLE( LIST_LENGTH, DBG_LIST);
	if (!TABPNT(fifo, FIFO_START))
	{
		TABSETPNT(fifo, FIFO_START, STACKPNT(th, 0));
		TABSETPNT(fifo, FIFO_END, STACKPNT(th, 0));
	}
	else
	{
		LB* p = TABPNT(fifo, FIFO_END);
		TABSETPNT(p, LIST_NXT, STACKPNT(th, 0));
		TABSETPNT(fifo, FIFO_END, STACKPNT(th, 0));
	}
	count=TABINT(fifo, FIFO_COUNT);
	TABSETINT(fifo, FIFO_COUNT, count+1);
	STACKDROP(th);
	return 0;
}

int fun_fifoOut(Thread* th)
{
	LINT count;
	LB* hd;
	
	LB* fifo = STACKPNT(th, 0);
	if (!fifo) FUN_RETURN_NIL;
	hd = TABPNT(fifo, FIFO_START);
	if (!hd) FUN_RETURN_NIL;

	count= TABINT(fifo, FIFO_COUNT);
	STACKLOAD(th, 0, hd, LIST_VAL);
	if (TABPNT(fifo, FIFO_START) == TABPNT(fifo, FIFO_END))
	{
		TABSETNIL(fifo, FIFO_START);
		TABSETNIL(fifo, FIFO_END);
	}
	else TABSETPNT(fifo, FIFO_START, TABPNT(hd, LIST_NXT));
	TABSETINT(fifo, FIFO_COUNT, count - 1);
	return 0;
}
int fun_fifoNext(Thread* th)
{
	LB* hd;
	LB* fifo = STACKPNT(th, 0);
	if (!fifo) FUN_RETURN_NIL;
	hd = TABPNT(fifo, FIFO_START);
	if (!hd) FUN_RETURN_NIL;
	STACKLOAD(th, 0, hd, LIST_VAL);
	return 0;
}

int fun_hashmapCreate(Thread* th)
{
	HashSlots* hash=hashSlotsCreate(th, STACKINT(th,0),DBG_HASHMAP);
	if (!hash) return EXEC_OM;
	FUN_RETURN_PNT((LB*)hash);
}

int fun_hashmapGet(Thread* th)
{
	LB* hash=STACKPNT(th,1);
	if (STACKISNIL(th,0)|| !hash) FUN_RETURN_NIL;

	hashmapGet(th,1,(HashSlots*)hash, STACKGET(th, 0), STACKTYPE(th, 0));
	STACKDROP(th);
	return 0;
}

int fun_hashmapBitSize(Thread* th)
{
	HashSlots* hash=(HashSlots*)STACKPNT(th,0);
	if (!hash) FUN_RETURN_NIL;
	FUN_RETURN_INT(hashSlotsBitSize(hash));
}
int fun_hashmapCount(Thread* th)
{
	HashSlots* hash=(HashSlots*)STACKPNT(th,0);
	if (!hash) FUN_RETURN_NIL;
	FUN_RETURN_INT(hash->nb);
}
int fun_hashmapGetSlot(Thread* th)
{
	LINT index = STACKINT(th, 0);
	HashSlots* hash = (HashSlots*)STACKPNT(th, 1);
	LB* first = hashSlotsGet(hash, index);
	FUN_RETURN_PNT(first);
}

int fun_hashmapSet(Thread* th)
{

	LB* hash=STACKPNT(th,2);
	if (hash && !STACKISNIL(th,1))
	{
		int k;
		if ((k=hashmapAdd(th, 0, (HashSlots*)hash, STACKGET(th, 1), STACKTYPE(th, 1)))) return k;
	}
	return 0;
}


int fun_hashsetCreate(Thread* th)
{
	HashSlots* hash = hashSlotsCreate(th, STACKINT(th, 0), DBG_HASHSET);
	if (!hash) return EXEC_OM;
	FUN_RETURN_PNT((LB*)hash);
}

int fun_hashsetContains(Thread* th)
{
	LB* hash = STACKPNT(th, 1);
	if (STACKISNIL(th, 0) || !hash) FUN_RETURN_NIL;

	FUN_RETURN_BOOL(hashsetContains((HashSlots*)hash, STACKGET(th, 0), STACKTYPE(th, 0)));
}

int fun_hashsetAdd(Thread* th)
{
	int k;
	LB* hash = STACKPNT(th, 1);
	if ((!hash) || STACKISNIL(th, 0)) FUN_RETURN_NIL;
	if ((k = hashsetAdd(th, (HashSlots*)hash, STACKGET(th, 0), STACKTYPE(th, 0)))) return k;
	return 0;
}
int fun_hashsetRemove(Thread* th)
{
	LB* hash = STACKPNT(th, 1);
	if ((!hash) || STACKISNIL(th, 0)) FUN_RETURN_NIL;
	FUN_RETURN_BOOL(hashsetRemove((HashSlots*)hash, STACKGET(th, 0), STACKTYPE(th, 0)));
}

int fun_bitTest(Thread* th)
{
	LINT b = STACKINT(th,0);
	LINT a = STACKINT(th,1);
	FUN_RETURN_BOOL(a & b);
}
int fun_intRand(Thread* th)
{
	unsigned int x;
	hwRandomBytes((char*)&x,4);
	FUN_RETURN_INT(x);
}

int fun_randomHardware(Thread* th)
{
	FUN_RETURN_BOOL(hwHasRandom());
}
int fun_randomEntropy(Thread* th)
{
	pseudoRandomEntropy(STACKINT(th, 0));
	return 0;
}

int fun_args(Thread* th)
{
	FUN_RETURN_PNT(MM.args);
}

int fun_hostMemory(Thread* th)
{
#ifdef USE_MEMORY_C
	FUN_RETURN_INT(MEMORY_C_LENGTH);
#else
	FUN_RETURN_NIL
#endif
}

int fun_setHostUser(Thread* th)
{
	int result;
	LB* p = STACKPNT(th, 0);
	if (!p) FUN_RETURN_NIL;
	result = hwSetHostUser(STRSTART(p));
	if (result > 0) FUN_RETURN_NIL;
	FUN_RETURN_BOOL(result);
}

#define intOpeI_I(name,ope)	\
int name(Thread* th)	\
{	\
	int vIsNil=STACKISNIL(th,0); \
	LINT v=STACKINT(th,0); \
	if (vIsNil) FUN_RETURN_NIL;	\
	FUN_RETURN_INT(ope(v));	\
}

intOpeI_I(fun_signExtend32,signExtend32)
intOpeI_I(fun_signExtend16,signExtend16)
intOpeI_I(fun_signExtend8,signExtend8)

int coreInit(Thread* th, Pkg *system)
{
	LFLOAT pi=3.14159265359;
	LFLOAT e=2.718281828459045;
	char device[64];

	Def* _Def = pkgAddType(th, system, "Definition");
	Def* Useless = pkgAddType(th, system, "Useless");
	Def* Device = pkgAddType(th, system, "Device");
	Type* u0=typeAllocUndef(th);
	Type* u1=typeAllocUndef(th);
	Type* list_u0=typeAlloc(th,TYPECODE_LIST,NULL,1,u0);
	Type* array_u0=typeAlloc(th,TYPECODE_ARRAY,NULL,1,u0);
	Type* hashmap_u0_u1=typeAlloc(th,TYPECODE_HASHMAP,NULL,2,u0,u1);
	Type* hashset_u0=typeAlloc(th,TYPECODE_HASHSET,NULL,1,u0);
	Type* fun_u0_u0_Boolean =typeAlloc(th,TYPECODE_FUN,NULL,3,u0,u0,MM.Boolean);
	Type* fifo_u0 = typeAlloc(th,TYPECODE_FIFO, NULL, 1, u0);
	Type* fun_I=typeAlloc(th,TYPECODE_FUN,NULL,1,MM.I);
	Type* fun_B=typeAlloc(th,TYPECODE_FUN,NULL,1,MM.Boolean);
	Type* fun_u0_I = typeAlloc(th,TYPECODE_FUN, NULL, 2, u0, MM.I);
	Type* fun_S_B = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.S, MM.Boolean);
	Type* fun_B_B = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Boolean, MM.Boolean);
	Type* fun_list_S = typeAlloc(th,TYPECODE_FUN, NULL, 1, typeAlloc(th,TYPECODE_LIST, NULL, 1, MM.S));
	Type* fun_F_B = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.F, MM.Boolean);
	Type* fun_F_F = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.F, MM.F);
	Type* fun_F_F_F=typeAlloc(th,TYPECODE_FUN,NULL,3,MM.F,MM.F,MM.F);
	Type* fun_I_I=typeAlloc(th,TYPECODE_FUN,NULL,2,MM.I,MM.I);
	Type* fun_I_I_I=typeAlloc(th,TYPECODE_FUN,NULL,3,MM.I,MM.I,MM.I);
	Type* fun_I_I_Boolean = typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.I, MM.I, MM.Boolean);
	Type* list_S_Pkg = typeAlloc(th,TYPECODE_LIST, NULL, 1, typeAlloc(th,TYPECODE_TUPLE, NULL, 2, MM.S, MM.Pkg));
	Type* hashmapSlot_u0_u1= typeAlloc(th,TYPECODE_TUPLE, NULL, 3, u0, u1, NULL);
	hashmapSlot_u0_u1->child[2] = hashmapSlot_u0_u1;
	MM.Type = pkgAddType(th, system, "Type")->type;

	snprintf(device, sizeof(device),"%sDevice", DEVICE_MODE);

	pkgAddConstPnt(th, system, "version", memoryAllocStr(th, VERSION_MINIMACY,-1), MM.S);
	pkgAddConstPnt(th, system, "device", memoryAllocStr(th, DEVICE_MODE,-1), MM.S);
	pkgAddConstPnt(th, system, device, NULL, Device->type);
	pkgAddFun(th, system, "hostMemory", fun_hostMemory, typeAlloc(th,TYPECODE_FUN, NULL, 1, MM.I));
	pkgAddFun(th, system,"setHostUser", fun_setHostUser, fun_S_B);
	pkgAddFun(th, system,"args",fun_args, fun_list_S);
	pkgAddFun(th, system,"_exit",fun_exit, fun_B);
	pkgAddFun(th, system, "gc", fun_gc, fun_I);
	pkgAddFun(th, system, "gcTrace", fun_gcTrace, fun_B_B);

	pkgAddFun(th, system, "address", fun_address, fun_u0_I);

	pkgAddFun(th, system, "_reboot", fun_reboot, fun_I);
		
	pkgAddFun(th, system, "_echoEnable", fun_echoEnable, fun_B_B);
	pkgAddFun(th, system, "_echoIsEnabled", fun_echoIsEnabled, fun_B);
	pkgAddFun(th, system, "_systemLogEnable",fun_systemLogEnable, fun_B_B);
	pkgAddOpcode(th, system,"dump", OPdump, typeAlloc(th,TYPECODE_FUN, NULL, 2, u0, u0));
	pkgAddOpcode(th, system,"_dump2", OPdumpd, typeAlloc(th,TYPECODE_FUN, NULL, 2, u0, u0));
	pkgAddFun(th, system,"echo",fun_echo,typeAlloc(th,TYPECODE_FUN,NULL,2,u0,u0));
	pkgAddFun(th, system,"echoLn",fun_echoLn,typeAlloc(th,TYPECODE_FUN,NULL,2,u0,u0));
	
	pkgAddFun(th, system, "_pkgCreate", fun_pkgCreate, typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.S, list_S_Pkg , MM.Pkg));
	pkgAddFun(th, system, "_pkgImports", fun_pkgImports, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Pkg, list_S_Pkg));
	pkgAddFun(th, system, "_pkgNext", fun_pkgNext, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Pkg, MM.Pkg));
	pkgAddFun(th, system, "pkgName", fun_pkgName, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Pkg, MM.S));
	pkgAddFun(th, system, "pkgDefs", fun_pkgDefs, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Pkg, _Def->type));
	pkgAddFun(th, system, "pkgStart", fun_pkgStart, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Pkg, _Def->type));
	pkgAddFun(th, system, "defName", fun_defName, typeAlloc(th,TYPECODE_FUN, NULL, 2, _Def->type, MM.S));
	pkgAddFun(th, system, "defCode", fun_defCode, typeAlloc(th,TYPECODE_FUN, NULL, 2, _Def->type, MM.I));
	pkgAddFun(th, system, "defIsPublic", fun_defIsPublic, typeAlloc(th,TYPECODE_FUN, NULL, 2, _Def->type, MM.Boolean));
	pkgAddFun(th, system, "defCodeName", fun_defCodeName, typeAlloc(th,TYPECODE_FUN, NULL, 2, _Def->type, MM.S));
	pkgAddFun(th, system, "defType", fun_defType, typeAlloc(th,TYPECODE_FUN, NULL, 2, _Def->type, MM.Type));
	pkgAddFun(th, system, "defPkg", fun_defPkg, typeAlloc(th,TYPECODE_FUN, NULL, 2, _Def->type, MM.Pkg));
	pkgAddFun(th, system, "defNext", fun_defNext, typeAlloc(th,TYPECODE_FUN, NULL, 2, _Def->type, _Def->type));
	pkgAddFun(th, system, "defSourceCode", fun_defSourceCode, typeAlloc(th,TYPECODE_FUN, NULL, 2, _Def->type, MM.S));
	pkgAddFun(th, system, "defSourceName", fun_defSourceName, typeAlloc(th,TYPECODE_FUN, NULL, 2, _Def->type, MM.S));
	pkgAddFun(th, system, "defIndexInCode", fun_defIndexInCode, typeAlloc(th,TYPECODE_FUN, NULL, 2, _Def->type, MM.I));
	pkgAddFun(th, system, "_defSimpleFunction", fun_defSimpleFunction, typeAlloc(th,TYPECODE_FUN, NULL, 2, _Def->type, typeAlloc(th, TYPECODE_FUN, NULL, 1, Useless->type)));
	pkgAddFun(th, system, "strFromType", fun_strFromType, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Type, MM.S));


	TimeDelta = 0;
	pkgAddFun(th, system,"time",fun_time,typeAlloc(th,TYPECODE_FUN,NULL,1,MM.I));
	pkgAddFun(th, system,"timeMs",fun_timeMs,typeAlloc(th,TYPECODE_FUN,NULL,1,MM.I));
	pkgAddFun(th, system,"timeSet",fun_timeSet,typeAlloc(th,TYPECODE_FUN,NULL,2,MM.I, MM.I));

	pkgAddFun(th, system,"arrayCreate",fun_arrayCreate,typeAlloc(th,TYPECODE_FUN,NULL,3,MM.I,u0,array_u0));
	pkgAddFun(th, system, "arraySlice", fun_arraySlice, typeAlloc(th, TYPECODE_FUN, NULL, 4, array_u0, MM.I, MM.I, array_u0));
	pkgAddOpcode(th, system,"arrayLength",OPtablen,typeAlloc(th,TYPECODE_FUN,NULL,2,array_u0,MM.I));

	pkgAddFun(th, system,"hashmapCreate",fun_hashmapCreate,typeAlloc(th,TYPECODE_FUN,NULL,2,MM.I,hashmap_u0_u1));
	pkgAddFun(th, system,"hashmapGet",fun_hashmapGet,typeAlloc(th,TYPECODE_FUN,NULL,3,hashmap_u0_u1,u0,u1));
	pkgAddFun(th, system,"hashmapSet",fun_hashmapSet,typeAlloc(th,TYPECODE_FUN,NULL,4,hashmap_u0_u1,u0,u1,u1));
	pkgAddFun(th, system,"hashmapCount", fun_hashmapCount, typeAlloc(th,TYPECODE_FUN, NULL, 2, hashmap_u0_u1, MM.I));
	pkgAddFun(th, system,"hashmapBitSize", fun_hashmapBitSize, typeAlloc(th,TYPECODE_FUN, NULL, 2, hashmap_u0_u1, MM.I));
	pkgAddFun(th, system,"hashmapGetSlot", fun_hashmapGetSlot, typeAlloc(th,TYPECODE_FUN, NULL, 3, hashmap_u0_u1, MM.I, hashmapSlot_u0_u1));

	pkgAddFun(th, system,"hashsetCreate",fun_hashsetCreate,typeAlloc(th,TYPECODE_FUN,NULL,2,MM.I,hashset_u0));
	pkgAddFun(th, system,"hashsetContains",fun_hashsetContains,typeAlloc(th,TYPECODE_FUN,NULL,3,hashset_u0,u0,MM.Boolean));
	pkgAddFun(th, system,"hashsetAdd",fun_hashsetAdd,typeAlloc(th,TYPECODE_FUN,NULL,3,hashset_u0,u0,u0));
	pkgAddFun(th, system,"hashsetRemove",fun_hashsetRemove,typeAlloc(th,TYPECODE_FUN,NULL,3,hashset_u0,u0,MM.Boolean));
	pkgAddFun(th, system,"hashsetCount", fun_hashmapCount, typeAlloc(th,TYPECODE_FUN, NULL, 2, hashset_u0, MM.I));
	pkgAddFun(th, system,"hashsetBitSize", fun_hashmapBitSize, typeAlloc(th,TYPECODE_FUN, NULL, 2, hashset_u0, MM.I));
	pkgAddFun(th, system,"hashsetGetSlot", fun_hashmapGetSlot, typeAlloc(th,TYPECODE_FUN, NULL, 3, hashset_u0, MM.I, list_u0));

	pkgAddFun(th, system, "fifoCreate", fun_fifoCreate, typeAlloc(th,TYPECODE_FUN, NULL, 1,fifo_u0));
	pkgAddFun(th, system, "fifoList", fun_fifoList, typeAlloc(th,TYPECODE_FUN, NULL, 2, fifo_u0, list_u0));
	pkgAddFun(th, system, "fifoCount", fun_fifoCount, typeAlloc(th,TYPECODE_FUN, NULL, 2, fifo_u0, MM.I));
	pkgAddFun(th, system, "fifoIn", fun_fifoIn, typeAlloc(th,TYPECODE_FUN, NULL, 3, fifo_u0, u0, fifo_u0));
	pkgAddFun(th, system, "fifoNext", fun_fifoNext, typeAlloc(th,TYPECODE_FUN, NULL, 2, fifo_u0, u0));
	pkgAddFun(th, system, "fifoOut", fun_fifoOut, typeAlloc(th,TYPECODE_FUN, NULL, 2, fifo_u0, u0));

	pkgAddFun(th, system,"floatFromInt",fun_floatFromInt,typeAlloc(th,TYPECODE_FUN,NULL,2,MM.I,MM.F));
	pkgAddFun(th, system,"intFromFloat",fun_intFromFloat,typeAlloc(th,TYPECODE_FUN,NULL,2,MM.F,MM.I));

	pkgAddOpcode(th, system,"tron", OPtron, typeAlloc(th,TYPECODE_FUN,NULL,1,MM.I));
	pkgAddOpcode(th, system,"troff", OPtroff, typeAlloc(th,TYPECODE_FUN,NULL,1,MM.I));

	pkgAddOpcode(th, system,"head", OPhd, typeAlloc(th,TYPECODE_FUN,NULL,2,list_u0,u0));
	pkgAddOpcode(th, system,"tail", OPtl, typeAlloc(th,TYPECODE_FUN,NULL,2,list_u0,list_u0));

	pkgAddOpcode(th, system, "equals", OPeq, fun_u0_u0_Boolean);

	pkgAddConstFloat(th, system,"pi",pi,MM.F);
	pkgAddConstFloat(th, system,"e",e,MM.F);

	pkgAddFun(th, system,"signExtend32",fun_signExtend32,fun_I_I);
	pkgAddFun(th, system,"signExtend16",fun_signExtend16,fun_I_I);
	pkgAddFun(th, system,"signExtend8",fun_signExtend8,fun_I_I);

	pkgAddOpcode(th, system,"abs",OPabs,fun_I_I);
	pkgAddOpcode(th, system,"min",OPmin,fun_I_I_I);
	pkgAddOpcode(th, system,"max",OPmax,fun_I_I_I);

	pkgAddOpcode(th, system,"isNan",OPisnan, fun_F_B);
	pkgAddOpcode(th, system,"isInf",OPisinf, fun_F_B);
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
	pkgAddFun(th, system, "randomHardware", fun_randomHardware, typeAlloc(th,TYPECODE_FUN, NULL, 1, MM.Boolean));
	pkgAddFun(th, system, "randomEntropy", fun_randomEntropy, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.I, MM.I));

	return 0;
}

