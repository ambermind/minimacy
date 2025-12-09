// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

void _oblivionMark(LB* user)
{
	Oblivion* p = (Oblivion*)user;
	MARK_OR_MOVE(p->f);
	MARK_OR_MOVE(p->popNext);
	if (MOVING_BLOCKS) MARK_OR_MOVE(p->listNext);
}

int fun_oblivionCreate(Thread* th)
{
	Oblivion* ob;
	LB* f = STACK_PNT(th, 0);
	if (!f) FUN_RETURN_NIL;
	ob = (Oblivion*)memoryAllocNative(sizeof(Oblivion), DBG_OBLIVION, NULL, _oblivionMark); if (!ob) return EXEC_OM;
	ob->f = f;
	ob->listNext = MM.listOblivions;
	MM.listOblivions = ob;
	BLOCK_MARK(ob);
	FUN_RETURN_PNT((LB*)ob);
}
int fun_oblivionPop(Thread* th)
{
	LB* f;
	Oblivion* ob = MM.popOblivions;
	if (!ob) FUN_RETURN_NIL;
	f = ob->f;
	ob->f = NULL;
	MM.popOblivions = ob->popNext;
	BLOCK_MARK(MM.popOblivions);
	FUN_RETURN_PNT(f);
}

int fun_pkgCreate(Thread* th)
{
	Pkg* pkg = pkgAlloc((STACK_PNT(th,1)),0, PKG_FROM_NOTHING); if (!pkg) return EXEC_OM;
	pkg->importList = STACK_PNT(th, 0);
	FUN_RETURN_PNT((LB*)pkg);
}

int fun_pkgNext(Thread* th)
{
	Pkg* p = (Pkg*)STACK_PNT(th, 0);
	FUN_RETURN_PNT((LB*)(p ? p->listNext : MM.listPkgs));
}

int fun_pkgImports(Thread* th)
{
	Pkg* p = (Pkg*)STACK_PNT(th, 0);
	FUN_RETURN_PNT((LB*)(p ? p->importList : NULL));
}

int fun_pkgName(Thread* th)
{
	Pkg* p = (Pkg*)STACK_PNT(th, 0);
	FUN_RETURN_PNT((LB*)(p ? p->name : NULL));
}

int fun_pkgMemory(Thread* th)
{
	Pkg* p = (Pkg*)STACK_PNT(th, 0);
	if (!p) FUN_RETURN_NIL;
	FUN_RETURN_INT(p->memory);
}

int fun_pkgDefs(Thread* th)
{
	Pkg* p = (Pkg*)STACK_PNT(th, 0);
	FUN_RETURN_PNT((LB*)(p ? p->first : NULL));
}

int fun_pkgStart(Thread* th)
{
	Pkg* p = (Pkg*)STACK_PNT(th, 0);
	FUN_RETURN_PNT((LB*)(p ? p->start : NULL));
}
int fun_pkgForget(Thread* th)
{
	Pkg* q;
	Pkg* p = (Pkg*)STACK_PNT(th, 0);
	if (p == MM.system) FUN_RETURN_FALSE;	// can't remove system package
	if (p == MM.listPkgs) {
		MM.listPkgs = p->listNext;
		FUN_RETURN_TRUE;
	}
	q = MM.listPkgs;
	while (q->listNext) {
		if (q->listNext == p) {
			q->listNext = p->listNext;
			FUN_RETURN_TRUE;
		}
		q = q->listNext;
	}
	FUN_RETURN_FALSE;
}

int fun_pkgForgetAll(Thread* th)
{
	MM.listPkgs = MM.system;
	FUN_RETURN_TRUE;
}

int fun_defName(Thread* th)
{
	Def* d = (Def*)STACK_PNT(th, 0);
	FUN_RETURN_PNT((LB*)(d ? d->name : NULL));
}

int fun_defType(Thread* th)
{
	Def* d = (Def*)STACK_PNT(th, 0);
	FUN_RETURN_PNT((LB*)(d ? d->type : NULL));
}
int fun_defPkg(Thread* th)
{
	Def* d = (Def*)STACK_PNT(th, 0);
	FUN_RETURN_PNT((LB*)(d ? d->header.pkg : NULL));
}

int fun_defIsPublic(Thread* th)
{
	Def* d = (Def*)STACK_PNT(th, 0);
	if (!d) FUN_RETURN_NIL;
	STACK_SET_BOOL(th, 0, (d->public != DEF_HIDDEN));
	return 0;
}
int fun_defCode(Thread* th)
{
	Def* d = (Def*)STACK_PNT(th, 0);
	if (!d) FUN_RETURN_NIL;
	FUN_RETURN_INT(d->code);
}
int fun_defCodeName(Thread* th)
{
	Def* d = (Def*)STACK_PNT(th, 0);
	if (!d) FUN_RETURN_NIL;
	FUN_RETURN_STR(defCodeName(d->code), -1);
}
int fun_defNext(Thread* th)
{
	Def* d = (Def*)STACK_PNT(th, 0);
	FUN_RETURN_PNT((LB*)(d ? d->next : NULL));
}

int fun_defSourceName(Thread* th)
{
	Def* d = (Def*)STACK_PNT(th, 0);
	if ((!d)||(!d->parser)) FUN_RETURN_NIL;
	FUN_RETURN_PNT(d->parser->name);
}
int fun_defSourceCode(Thread* th)
{
	Def* d = (Def*)STACK_PNT(th, 0);
	if ((!d)||(!d->parser)) FUN_RETURN_NIL;
	FUN_RETURN_PNT(d->parser->block);
}
int fun_defIndexInCode(Thread* th)
{
	Def* d = (Def*)STACK_PNT(th, 0);
	if ((!d)||(!d->parser)) FUN_RETURN_NIL;

	FUN_RETURN_INT(d->parserIndex- (d->name? STR_LENGTH(d->name):0) );
}
int fun_defSimpleFunction(Thread* th)
{
	Def* d = (Def*)STACK_PNT(th, 0);
	FUN_RETURN_PNT((LB*)((d && (d->code ==0)) ? PNT_FROM_VAL(d->val) : NULL));
}

int fun_strFromType(Thread* th)
{
	int k;
	Type* t = (Type*)STACK_PNT(th, 0);
	if (!t) FUN_RETURN_NIL;
	bufferReinit(MM.tmpBuffer);
	if ((k = typeBuffer(MM.tmpBuffer, t))) return k;
	FUN_RETURN_BUFFER(MM.tmpBuffer);
}


int fun_exit(Thread* th)
{
//	PRINTF(LOG_DEV, "> Thread Exit\n");
	STACK_SET_NIL(th,0);
	return EXEC_EXIT;
}

int fun_reboot(Thread* th)
{
	PRINTF(LOG_SYS, "> REBOOT NOW...\n");
	STACK_SET_NIL(th,0);
	MM.reboot = 1;
	return EXEC_EXIT;
}

int fun_gc(Thread* th)
{
	memoryFinalizeGC();
	FUN_RETURN_NIL
}

int fun_memoryRecount(Thread* th)
{
	memoryRecount();
	FUN_RETURN_INT(MM.blocs_length);
}
int fun_memoryTry(Thread* th)
{
	FUN_RETURN_BOOL(memoryTry(STACK_INT(th, 0)));
}
#ifdef USE_MEMORY_C
int fun_memoryLargestBlock(Thread* th)
{
	FUN_RETURN_INT(BmmMaxSize);
}
int fun_memoryReserve(Thread* th)
{
	FUN_RETURN_INT(bmmReservedMem());
}
int fun_memoryFree(Thread* th)
{
	FUN_RETURN_INT(BmmTotalFree);
}
int fun_memoryCheck(Thread* th)
{
	FUN_RETURN_BOOL(memoryCheck(STACK_BOOL(th, 0)));
}
#else
int fun_memoryLargestBlock(Thread* th) FUN_RETURN_NIL
int fun_memoryReserve(Thread* th) FUN_RETURN_NIL
int fun_memoryFree(Thread* th) FUN_RETURN_NIL
int fun_memoryCheck(Thread* th) FUN_RETURN_NIL
#endif
int fun_systemLoadAllNatives(Thread* th)
{
	int k;
	if ((k=systemMakeAllNatives())) return k;
	FUN_RETURN_INT(MM.system->defs->nb);
}

int fun_echo(Thread* th)
{
	itemEcho(LOG_USER,STACK_GET(th,0),STACK_TYPE(th,0),0);
	return 0;
}

int fun_echoLn(Thread* th)
{
	itemEcho(LOG_USER,STACK_GET(th,0),STACK_TYPE(th,0),1);
	return 0;
}
int fun_address(Thread* th)
{
	LINT v = (LINT)STACK_GET(th, 0);
	if (!STACK_IS_PNT(th, 0)) FUN_RETURN_NIL;
	FUN_RETURN_INT(v);
}

int fun_gcTrace(Thread* th)
{
	MM.gcTrace = (STACK_PNT(th, 0) == MM._true) ? 1:0;
	return 0;
}
int fun_echoEnable(Thread* th)
{
	int mask = termCheckMask(~LOG_USER);
	if (STACK_PNT(th, 0) == MM._true) mask |= LOG_USER;
	termSetMask(mask);
	return 0;
}
int fun_echoIsEnabled(Thread* th)
{
	FUN_RETURN_BOOL(termCheckMask(LOG_USER));
}

int fun_systemLogIsEnabled(Thread* th)
{
	FUN_RETURN_BOOL(termCheckMask(LOG_SYS));
}

int fun_systemLogEnable(Thread* th)
{
	int mask = termCheckMask(~LOG_SYS);
	if (STACK_PNT(th, 0) == MM._true) mask |= LOG_SYS;
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
	TimeDelta = STACK_INT(th, 0)- hwTime();
	return 0;
}


int fun_arrayCreate(Thread* th)
{
	LINT i;
	LB* p=NULL;

//	STACK_(th,0) is the initialization value
	LINT n=STACK_INT(th,1);
	if (STACK_IS_NIL(th,1)||(n<0)) FUN_RETURN_NIL;
	p= memoryAllocArray(n, DBG_ARRAY); if (!p) return EXEC_OM;
	if (!STACK_IS_NIL(th,0)) for(i=0;i<n;i++) STACK_STORE(p,i,th,0);
	FUN_RETURN_PNT(p);
}
int fun_arraySlice(Thread* th)
{
	LB* q;
	LINT i;

	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT start = STACK_INT(th,1);
	LB* p = STACK_PNT(th, 2);
	
	FUN_SUBSTR(p,start, len, lenIsNil, ARRAY_LENGTH(p ));

	q=memoryAllocArray(len, DBG_ARRAY); if (!q) return EXEC_OM;
	for (i = 0; i < len; i++) ARRAY_COPY(q, i, p, start + i);
	FUN_RETURN_PNT(q);
}


int fun_floatFromInt(Thread* th)
{
	LFLOAT f;
	if (STACK_IS_NIL(th,0)) FUN_RETURN_NIL;
	f=(LFLOAT)STACK_INT(th,0);
	FUN_RETURN_FLOAT(f);
}

int fun_intFromFloat(Thread* th)
{
	LFLOAT f;
	if (STACK_IS_NIL(th,0)) FUN_RETURN_NIL;
	f=STACK_FLOAT(th,0);
	FUN_RETURN_INT((LINT)f);
}

int fun_fifoCreate(Thread* th)
{
	FUN_PUSH_NIL;
	FUN_PUSH_NIL;
	FUN_PUSH_INT(0);
	FUN_MAKE_ARRAY( 3, DBG_FIFO);
	return 0;
}

int fun_fifoList(Thread* th)
{
	LB* fifo = STACK_PNT(th, 0);
	if (!fifo) FUN_RETURN_NIL;
	STACK_LOAD(th, 0, fifo, FIFO_START);
	ARRAY_SET_NIL(fifo, FIFO_START);
	ARRAY_SET_NIL(fifo, FIFO_END);
	ARRAY_SET_INT(fifo, FIFO_COUNT, 0);
	return 0;
}

int fun_fifoCount(Thread* th)
{
	LB* fifo = STACK_PNT(th, 0);
	if (!fifo) FUN_RETURN_NIL;
	FUN_RETURN_INT(ARRAY_INT(fifo, FIFO_COUNT));
}

int fun_fifoIn(Thread* th)
{
	LINT count;
	LB* fifo = STACK_PNT(th, 1);
	if (!fifo) FUN_RETURN_NIL;
	FUN_PUSH_NIL;
	FUN_MAKE_ARRAY( LIST_LENGTH, DBG_LIST);
	if (!ARRAY_PNT(fifo, FIFO_START))
	{
		ARRAY_SET_PNT(fifo, FIFO_START, STACK_PNT(th, 0));
		ARRAY_SET_PNT(fifo, FIFO_END, STACK_PNT(th, 0));
	}
	else
	{
		LB* p = ARRAY_PNT(fifo, FIFO_END);
		ARRAY_SET_PNT(p, LIST_NXT, STACK_PNT(th, 0));
		ARRAY_SET_PNT(fifo, FIFO_END, STACK_PNT(th, 0));
	}
	count=ARRAY_INT(fifo, FIFO_COUNT);
	ARRAY_SET_INT(fifo, FIFO_COUNT, count+1);
	STACK_DROP(th);
	return 0;
}

int fun_fifoOut(Thread* th)
{
	LINT count;
	LB* hd;
	
	LB* fifo = STACK_PNT(th, 0);
	if (!fifo) FUN_RETURN_NIL;
	hd = ARRAY_PNT(fifo, FIFO_START);
	if (!hd) FUN_RETURN_NIL;

	count= ARRAY_INT(fifo, FIFO_COUNT);
	STACK_LOAD(th, 0, hd, LIST_VAL);
	if (ARRAY_PNT(fifo, FIFO_START) == ARRAY_PNT(fifo, FIFO_END))
	{
		ARRAY_SET_NIL(fifo, FIFO_START);
		ARRAY_SET_NIL(fifo, FIFO_END);
	}
	else ARRAY_SET_PNT(fifo, FIFO_START, ARRAY_PNT(hd, LIST_NXT));
	ARRAY_SET_INT(fifo, FIFO_COUNT, count - 1);
	return 0;
}
int fun_fifoNext(Thread* th)
{
	LB* hd;
	LB* fifo = STACK_PNT(th, 0);
	if (!fifo) FUN_RETURN_NIL;
	hd = ARRAY_PNT(fifo, FIFO_START);
	if (!hd) FUN_RETURN_NIL;
	STACK_LOAD(th, 0, hd, LIST_VAL);
	return 0;
}

int fun_hashmapCreate(Thread* th)
{
	HashSlots* hash=hashSlotsCreate(STACK_INT(th,0),DBG_HASHMAP);
	if (!hash) return EXEC_OM;
	FUN_RETURN_PNT((LB*)hash);
}

int fun_hashmapGet(Thread* th)
{
	LB* hash=STACK_PNT(th,1);
	if (STACK_IS_NIL(th,0)|| !hash) FUN_RETURN_NIL;

	hashmapGet(th,1,(HashSlots*)hash, STACK_GET(th, 0), STACK_TYPE(th, 0));
	STACK_DROP(th);
	return 0;
}

int fun_hashmapBitSize(Thread* th)
{
	HashSlots* hash=(HashSlots*)STACK_PNT(th,0);
	if (!hash) FUN_RETURN_NIL;
	FUN_RETURN_INT(hashSlotsBitSize(hash));
}
int fun_hashmapCount(Thread* th)
{
	HashSlots* hash=(HashSlots*)STACK_PNT(th,0);
	if (!hash) FUN_RETURN_NIL;
	FUN_RETURN_INT(hash->nb);
}
int fun_hashmapGetSlot(Thread* th)
{
	LINT index = STACK_INT(th, 0);
	HashSlots* hash = (HashSlots*)STACK_PNT(th, 1);
	LB* first = hashSlotsGet(hash, index);
	FUN_RETURN_PNT(first);
}

int fun_hashmapSet(Thread* th)
{

	LB* hash=STACK_PNT(th,2);
	if (hash && !STACK_IS_NIL(th,1))
	{
		int k;
		if ((k=hashmapAdd(th, 0, (HashSlots*)hash, STACK_GET(th, 1), STACK_TYPE(th, 1)))) return k;
	}
	return 0;
}


int fun_hashsetCreate(Thread* th)
{
	HashSlots* hash = hashSlotsCreate(STACK_INT(th, 0), DBG_HASHSET);
	if (!hash) return EXEC_OM;
	FUN_RETURN_PNT((LB*)hash);
}

int fun_hashsetContains(Thread* th)
{
	LB* hash = STACK_PNT(th, 1);
	if (STACK_IS_NIL(th, 0) || !hash) FUN_RETURN_NIL;

	FUN_RETURN_BOOL(hashsetContains((HashSlots*)hash, STACK_GET(th, 0), STACK_TYPE(th, 0)));
}

int fun_hashsetAdd(Thread* th)
{
	int k;
	LB* hash = STACK_PNT(th, 1);
	if ((!hash) || STACK_IS_NIL(th, 0)) FUN_RETURN_NIL;
	if ((k = hashsetAdd((HashSlots*)hash, STACK_GET(th, 0), STACK_TYPE(th, 0)))) return k;
	return 0;
}
int fun_hashsetRemove(Thread* th)
{
	LB* hash = STACK_PNT(th, 1);
	if ((!hash) || STACK_IS_NIL(th, 0)) FUN_RETURN_NIL;
	FUN_RETURN_BOOL(hashsetRemove((HashSlots*)hash, STACK_GET(th, 0), STACK_TYPE(th, 0)));
}

int fun_bitTest(Thread* th)
{
	LINT b = STACK_INT(th,0);
	LINT a = STACK_INT(th,1);
	FUN_RETURN_BOOL(a & b);
}
int fun_intRand(Thread* th)
{
	unsigned int x;
	hwRandomBytes((char*)&x,4);
	x &= 0x7fffffff;
	FUN_RETURN_INT(x);
}

int fun_randomHardware(Thread* th)
{
	FUN_RETURN_BOOL(hwHasRandom());
}
int fun_randomEntropy(Thread* th)
{
	pseudoRandomEntropy(STACK_INT(th, 0));
	return 0;
}

int fun_args(Thread* th)
{
	FUN_RETURN_PNT(MM.args);
}

int fun_hostMemory(Thread* th)
{
#ifdef USE_MEMORY_C
	FUN_RETURN_INT(BmmTotalSize);
#else
	FUN_RETURN_NIL
#endif
}

int fun_setHostUser(Thread* th)
{
	int result;
	LB* p = STACK_PNT(th, 0);
	if (!p) FUN_RETURN_NIL;
	result = hwSetHostUser(STR_START(p));
	if (result < 0) FUN_RETURN_NIL;
	FUN_RETURN_BOOL(result);
}

#define intOpeI_I(name,ope)	\
int name(Thread* th)	\
{	\
	int vIsNil=STACK_IS_NIL(th,0); \
	LINT v=STACK_INT(th,0); \
	if (vIsNil) FUN_RETURN_NIL;	\
	FUN_RETURN_INT(ope(v));	\
}

intOpeI_I(fun_signExtend32,signExtend32)
intOpeI_I(fun_signExtend24,signExtend24)
intOpeI_I(fun_signExtend16,signExtend16)
intOpeI_I(fun_signExtend8,signExtend8)

int fun_signExtend(Thread* th)
{
	LINT bit=STACK_INT(th,0);
	LINT val=STACK_INT(th,1);
	val=signExtend(val,bit);
	FUN_RETURN_INT(val);
}

int systemCoreInit(Pkg *system)
{
	char device[64];
	static const LFLOAT pi = (LFLOAT)3.14159265359;
	static const LFLOAT e = (LFLOAT)2.718281828459045;
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "hostMemory", fun_hostMemory, "fun -> Int"},
		{ NATIVE_FUN, "setHostUser", fun_setHostUser, "fun Str -> Bool"},
		{ NATIVE_FUN, "_args", fun_args, "fun -> list Str"},
		{ NATIVE_FUN, "_exit", fun_exit, "fun -> Bool"},
		{ NATIVE_FUN, "gc", fun_gc, "fun -> Int"},
		{ NATIVE_FUN, "gcTrace", fun_gcTrace, "fun Bool -> Bool"},
		{ NATIVE_OPCODE, "gcCompact", (void*)OPcompact, "fun -> Int"},
		{ NATIVE_FUN, "memoryRecount", fun_memoryRecount, "fun -> Int"},
		{ NATIVE_FUN, "_memoryTry", fun_memoryTry, "fun Int -> Bool"},
		{ NATIVE_FUN, "memoryLargestBlock", fun_memoryLargestBlock, "fun -> Int"},
		{ NATIVE_FUN, "memoryReserve", fun_memoryReserve, "fun -> Int"},
		{ NATIVE_FUN, "memoryFree", fun_memoryFree, "fun -> Int"},
		{ NATIVE_FUN, "memoryCheck", fun_memoryCheck, "fun Bool -> Bool"},
		{ NATIVE_FUN, "systemLoadAllNatives", fun_systemLoadAllNatives, "fun -> Int"},
		{ NATIVE_FUN, "address", fun_address, "fun a1 -> Int"},
		{ NATIVE_FUN, "_reboot", fun_reboot, "fun -> Int"},
		{ NATIVE_FUN, "_oblivionCreate", fun_oblivionCreate, "fun (fun -> Bool) -> Oblivion"},
		{ NATIVE_FUN, "_oblivionPop", fun_oblivionPop, "fun -> (fun -> Bool)"},

		{ NATIVE_FUN, "echo", fun_echo, "fun a1 -> a1"},
		{ NATIVE_FUN, "echoLn", fun_echoLn, "fun a1 -> a1"},

		{ NATIVE_FUN, "_echoEnable", fun_echoEnable, "fun Bool -> Bool"},
		{ NATIVE_FUN, "_echoIsEnabled", fun_echoIsEnabled, "fun -> Bool"},
		{ NATIVE_FUN, "_systemLogIsEnabled", fun_systemLogIsEnabled, "fun -> Bool"},
		{ NATIVE_FUN, "_systemLogEnable",fun_systemLogEnable, "fun Bool -> Bool"},
		{ NATIVE_OPCODE, "dump", (void*)OPdump, "fun a1 -> a1"},
		{ NATIVE_OPCODE, "_dump2", (void*)OPdumpd, "fun a1 -> a1"},

		{ NATIVE_FUN, "_pkgCreate", fun_pkgCreate, "fun Str list [Str Package] -> Package"},
		{ NATIVE_FUN, "_pkgImports", fun_pkgImports, "fun Package -> list [Str Package]"},
		{ NATIVE_FUN, "_pkgNext", fun_pkgNext, "fun Package -> Package"},
		{ NATIVE_FUN, "pkgName", fun_pkgName, "fun Package -> Str"},
		{ NATIVE_FUN, "pkgMemory", fun_pkgMemory, "fun Package -> Int"},
		{ NATIVE_FUN, "pkgDefs", fun_pkgDefs, "fun Package -> Definition"},
		{ NATIVE_FUN, "pkgStart", fun_pkgStart, "fun Package -> Definition"},
		{ NATIVE_FUN, "pkgForget", fun_pkgForget, "fun Package -> Bool"},
		{ NATIVE_FUN, "pkgForgetAll", fun_pkgForgetAll, "fun -> Bool"},
		{ NATIVE_FUN, "defName", fun_defName, "fun Definition -> Str"},
		{ NATIVE_FUN, "defCode", fun_defCode, "fun Definition -> Int"},
		{ NATIVE_FUN, "defIsPublic", fun_defIsPublic, "fun Definition -> Bool"},
		{ NATIVE_FUN, "defCodeName", fun_defCodeName, "fun Definition -> Str"},
		{ NATIVE_FUN, "defType", fun_defType, "fun Definition -> Type"},
		{ NATIVE_FUN, "defPkg", fun_defPkg, "fun Definition -> Package"},
		{ NATIVE_FUN, "defNext", fun_defNext, "fun Definition -> Definition"},
		{ NATIVE_FUN, "defSourceCode", fun_defSourceCode, "fun Definition -> Str"},
		{ NATIVE_FUN, "defSourceName", fun_defSourceName, "fun Definition -> Str"},
		{ NATIVE_FUN, "defIndexInCode", fun_defIndexInCode, "fun Definition -> Int"},
		{ NATIVE_FUN, "_defSimpleFunction", fun_defSimpleFunction, "fun Definition -> (fun -> Useless)"},
		{ NATIVE_FUN, "strFromType", fun_strFromType, "fun Type -> Str"},

		{ NATIVE_FUN, "fifoList", fun_fifoList, "fun fifo a1 -> list a1" },
		{ NATIVE_FUN, "fifoCreate", fun_fifoCreate, "fun -> fifo a1" },
		{ NATIVE_OPCODE, "head", (void*)OPhd, "fun list a1 -> a1" },
		{ NATIVE_OPCODE, "tail", (void*)OPtl, "fun list a1 -> list a1" },


		{ NATIVE_FUN, "fifoCount", fun_fifoCount, "fun fifo a1 -> Int"},
		{ NATIVE_FUN, "fifoIn", fun_fifoIn, "fun fifo a1 a1 -> fifo a1"},
		{ NATIVE_FUN, "fifoNext", fun_fifoNext, "fun fifo a1 -> a1"},
		{ NATIVE_FUN, "fifoOut", fun_fifoOut, "fun fifo a1 -> a1"},

		{ NATIVE_FLOAT, "pi", (void*)&pi, "Float" },
		{ NATIVE_FLOAT, "e", (void*)&e, "Float" },

		{ NATIVE_FUN, "signExtend32", fun_signExtend32, "fun Int -> Int"},
		{ NATIVE_FUN, "signExtend24", fun_signExtend24, "fun Int -> Int"},
		{ NATIVE_FUN, "signExtend16", fun_signExtend16, "fun Int -> Int"},
		{ NATIVE_FUN, "signExtend8", fun_signExtend8, "fun Int -> Int"},
		{ NATIVE_FUN, "signExtend", fun_signExtend, "fun Int Int -> Int"},
		{ NATIVE_INT, "signBit", (void*)SIGN_BIT, "Int" },


		{ NATIVE_OPCODE, "abs", (void*)OPabs, "fun Int -> Int"},
		{ NATIVE_OPCODE, "min", (void*)OPmin, "fun Int Int -> Int"},
		{ NATIVE_OPCODE, "max", (void*)OPmax, "fun Int Int -> Int"},

		{ NATIVE_OPCODE, "isNan", (void*)OPisnan, "fun Float -> Bool"},
		{ NATIVE_OPCODE, "isInf", (void*)OPisinf, "fun Float -> Bool"},
		{ NATIVE_OPCODE, "cos", (void*)OPcos, "fun Float -> Float"},
		{ NATIVE_OPCODE, "sin", (void*)OPsin, "fun Float -> Float"},
		{ NATIVE_OPCODE, "tan", (void*)OPtan, "fun Float -> Float"},
		{ NATIVE_OPCODE, "acos", (void*)OPacos, "fun Float -> Float"},
		{ NATIVE_OPCODE, "asin", (void*)OPasin, "fun Float -> Float"},
		{ NATIVE_OPCODE, "atan", (void*)OPatan, "fun Float -> Float"},
		{ NATIVE_OPCODE, "atan2", (void*)OPatan2, "fun Float Float -> Float"},
		{ NATIVE_OPCODE, "absf", (void*)OPabsf, "fun Float -> Float"},
		{ NATIVE_OPCODE, "ln", (void*)OPln, "fun Float -> Float"},
		{ NATIVE_OPCODE, "log", (void*)OPlog, "fun Float -> Float"},
		{ NATIVE_OPCODE, "exp", (void*)OPexp, "fun Float -> Float"},
		{ NATIVE_OPCODE, "sqr", (void*)OPsqr, "fun Float -> Float"},
		{ NATIVE_OPCODE, "sqrt", (void*)OPsqrt, "fun Float -> Float"},
		{ NATIVE_OPCODE, "floor", (void*)OPfloor, "fun Float -> Float"},
		{ NATIVE_OPCODE, "ceil", (void*)OPceil, "fun Float -> Float"},
		{ NATIVE_OPCODE, "round", (void*)OPround, "fun Float -> Float"},
		{ NATIVE_OPCODE, "minf", (void*)OPminf, "fun Float Float -> Float"},
		{ NATIVE_OPCODE, "maxf", (void*)OPmaxf, "fun Float Float -> Float"},

		{ NATIVE_OPCODE, "cosh", (void*)OPcosh, "fun Float -> Float"},
		{ NATIVE_OPCODE, "sinh", (void*)OPsinh, "fun Float -> Float"},
		{ NATIVE_OPCODE, "tanh", (void*)OPtanh, "fun Float -> Float"},

		{ NATIVE_FUN, "floatFromInt", fun_floatFromInt, "fun Int -> Float" },
		{ NATIVE_FUN, "intFromFloat", fun_intFromFloat, "fun Float -> Int" },

		{ NATIVE_OPCODE, "tron", (void*)OPtron, "fun -> Int"},
		{ NATIVE_OPCODE, "troff", (void*)OPtroff, "fun -> Int"},

		{ NATIVE_OPCODE, "equals", (void*)OPeq, "fun a1 a1 -> Bool"},

		{ NATIVE_FUN, "bitTest", fun_bitTest, "fun Int Int -> Bool"},
		{ NATIVE_FUN, "intRand", fun_intRand, "fun -> Int"},
		{ NATIVE_FUN, "randomHardware", fun_randomHardware, "fun -> Bool"},
		{ NATIVE_FUN, "randomEntropy", fun_randomEntropy, "fun Int -> Int"},

		{ NATIVE_FUN, "time", fun_time, "fun -> Int"},
		{ NATIVE_FUN, "timeMs", fun_timeMs, "fun -> Int"},
		{ NATIVE_FUN, "_timeSet", fun_timeSet, "fun Int -> Int"},

		{ NATIVE_FUN, "arrayCreate",fun_arrayCreate, "fun Int a1 -> array a1"},
		{ NATIVE_FUN, "arraySlice", fun_arraySlice, "fun array a1 Int Int -> array a1"},
		{ NATIVE_OPCODE, "arrayLength", (void*)OParraylen,"fun array a1 -> Int"},

		{ NATIVE_FUN, "hashmapCreate",fun_hashmapCreate, "fun Int -> hashmap a1 -> a2"},
		{ NATIVE_FUN, "hashmapGet",fun_hashmapGet, "fun hashmap a1 -> a2 a1 -> a2"},
		{ NATIVE_FUN, "hashmapSet",fun_hashmapSet, "fun hashmap a1 -> a2 a1 a2 -> a2"},
		{ NATIVE_FUN, "hashmapCount", fun_hashmapCount, "fun hashmap a1 -> a2 -> Int"},
		{ NATIVE_FUN, "hashmapBitSize", fun_hashmapBitSize, "fun hashmap a1 -> a2 -> Int"},
		{ NATIVE_FUN, "hashmapGetSlot", fun_hashmapGetSlot, "fun hashmap a1 -> a2 Int -> [a1 a2 r0]"},

		{ NATIVE_FUN, "hashsetCreate",fun_hashsetCreate, "fun Int -> hashset a1"},
		{ NATIVE_FUN, "hashsetContains",fun_hashsetContains, "fun hashset a1 a1 -> Bool"},
		{ NATIVE_FUN, "hashsetAdd",fun_hashsetAdd, "fun hashset a1 a1 -> a1"},
		{ NATIVE_FUN, "hashsetRemove",fun_hashsetRemove, "fun hashset a1 a1 -> Bool"},
		{ NATIVE_FUN, "hashsetCount", fun_hashmapCount, "fun hashset a1 -> Int"},
		{ NATIVE_FUN, "hashsetBitSize", fun_hashmapBitSize, "fun hashset a1 -> Int"},
		{ NATIVE_FUN, "hashsetGetSlot", fun_hashmapGetSlot, "fun hashset a1 Int -> list a1"},


//		{ NATIVE_INT,"foobar",(void*)1234,"Int" },
	};

	pkgAddType(system, "Definition");
	pkgAddType(system, "Useless");
	pkgAddType(system, "Oblivion");
	Def* Device = pkgAddType(system, "Device");
	MM.Type = pkgAddType(system, "Type")->type;

	TimeDelta = 0;

	snprintf(device, sizeof(device),"%sDevice", DEVICE_MODE);

	pkgAddConstPnt(system, "version", memoryAllocStr(VERSION_MINIMACY,-1), MM.Str);
	pkgAddConstPnt(system, "device", memoryAllocStr(DEVICE_MODE,-1), MM.Str);
	pkgAddConstPnt(system, device, NULL, Device->type);

	NATIVE_DEF(nativeDefs);



	return 0;
}

