// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

int interpreterTRON=0;

//LINT* runtimeCheckAddress=NULL;
//LINT runtimeCheckValue=0;

#define EXEC_COMMON th->pc=pc-BC_START(bytecode); \
	if ((result=interpreterExec(th,n,0))) \
	{ if (result>0) return INTERPRETER_END(result);\
	if (th->fun) {BC_PRECOMPUTE} } \
	else if (th->forceOpcode!=THREAD_OPCODE_NONE) \
	{ \
		op=th->forceOpcode; \
		th->forceOpcode=THREAD_OPCODE_NONE; \
		goto processOpCode; \
	}

#define EXEC_COMMON_TFC th->pc=pc-BC_START(bytecode); \
	if ((result=interpreterExec(th,n,locals))) \
	{ if (result>0) return INTERPRETER_END(result);\
	if (th->fun) {BC_PRECOMPUTE} } \
	else if (th->forceOpcode!=THREAD_OPCODE_NONE) \
	{ \
		op=th->forceOpcode; \
		th->forceOpcode=THREAD_OPCODE_NONE; \
		if (th->fun) { BC_PRECOMPUTE } \
		goto processOpCode; \
	}


LINT absint(LINT a) { return a>0?a:-a;}
LFLOAT sqr(LFLOAT a) { return a*a; }
LFLOAT roundls(LFLOAT a) { 
	double p=((a-floor(a))>0.5)?ceil(a):floor(a);
	return (LFLOAT)p;
}
LFLOAT minf(LFLOAT a,LFLOAT b) { return a>b?b:a; }
LFLOAT maxf(LFLOAT a,LFLOAT b) { return a<b?b:a; }

LINT minint(LINT a,LINT b) { return a>b?b:a; }
LINT maxint(LINT a,LINT b) { return a<b?b:a; }

int isHigherFun(LB* fun)
{
	LB* p;
	if (!fun) return 0;
	p=ARRAY_PNT(fun,FUN_USER_NAME);
	if ((p)&&(HEADER_TYPE(p)==TYPE_ARRAY)) return 1;
	return 0;
}
LB* getFunStruct(LB* fun)
{
	if (isHigherFun(fun)) return ARRAY_PNT(fun,FUN_USER_NAME);
	return fun;
}

char* interpreterCurrentFun(Thread* th)
{
	LB* p=ARRAY_PNT(getFunStruct(th->fun),FUN_USER_NAME);
	if (p) return STR_START(p);
	return "lambda function";
}

// prepare the execution of a function
// for a native function, this performs the call
// for a bytecode function, this prepares the callstack
// return :
// 0 : native function without tfc
// -1 : bytecode function or native with tfc : it is required to recompute
// >0 : the error code of a native function
// tfc is either 0 (no tfc) or the index of locals (always >0).
LINT interpreterExec(Thread* th,LINT argc,LINT tfc)	// fun arg0 ... 0:argn-1 
{
	LINT i,nlocals;
	LB* bytecode;
	LB* fun=STACK_PNT(th,argc);
	th->forceOpcode=THREAD_OPCODE_NONE;
	if (!fun)
	{
		STACK_DROPN(th,argc);
		return 0;
	}
	if (tfc) {
		for (i = 0; i< th->sp-th->callstack; i++) {
			if (STACK_IS_PNT(th, i) && (STACK_PNT(th, i) == MM.abortMark)) {	// we can't use tfc if inside a try ... else
				tfc = 0;
				break;
			}
		}
	}
	if (tfc)
	{
//		threadDump(LOG_SYS,th,10); PRINTF(LOG_DEV,">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>before tfc\n"); getchar();

		th->fun=STACK_REF_PNT(th,th->callstack,CALLSTACK_FUN);
		th->pc=STACK_REF_INT(th,th->callstack,CALLSTACK_PC);
		th->callstack=STACK_REF_INT(th,th->callstack,CALLSTACK_PREV);
		for(i=-1;i<argc;i++) STACK_COPY_TO_REF(th,tfc,-i,argc-1-i);

		STACK_DROPN(th,th->sp-(tfc+argc)+1);

//		threadDump(LOG_SYS,th,10); PRINTF(LOG_DEV,">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>after tfc\n"); getchar();
	}

	if (isHigherFun(fun))
	{
		LINT i;
		LINT binds=ARRAY_LENGTH(fun)-1;

//		threadDump(LOG_SYS,th,10); PRINTF(LOG_DEV,">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>start higherfun\n"); getchar();

		for(i=0;i<binds;i++) FUN_PUSH_NIL;
		for(i=0;i<argc;i++) STACK_INTERNAL_COPY(th,i,i+binds);
		for(i=0;i<binds;i++) STACK_LOAD(th,argc+i,fun,binds-i);
		STACK_LOAD(th,argc+binds,fun,FUN_USER_NAME);
		fun=ARRAY_PNT(fun,FUN_USER_NAME);
//		threadDump(LOG_SYS,th,10); PRINTF(LOG_DEV,">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>done higherfun\n"); getchar();
//		interpreterTRON=1;

	}
	else if (ARRAY_LENGTH(fun)==FUN_NATIVE_LENGTH)
	{
		LINT i;
		for(i=argc-1;i>=0;i--) STACK_INTERNAL_COPY(th,i+1,i);
		STACK_DROP(th);
		th->forceOpcode=ARRAY_INT(fun,FUN_NATIVE_OPCODE);
//			printf("th->forceOpcode=%x", th->forceOpcode & 0xffff);
//			return tfc?-1:0;
		return 0;
	}

	bytecode=(ARRAY_PNT(fun,FUN_USER_BC));

	nlocals=BC_LOCALS(bytecode);
	for(i=0;i<nlocals;i++) FUN_PUSH_NIL;
	// implicit use of CALLSTACK_LENGTH, CALLSTACK_FUN, CALLSTACK_PC, CALLSTACK_PREV
	FUN_PUSH_PNT(th->fun);
	FUN_PUSH_INT(th->pc);
	FUN_PUSH_INT(th->callstack);
	th->callstack=STACK_REF(th);
	th->pc=0;
	th->fun=fun;
	return -1;
}

int interpreterContinue(Thread* th, LW data, int dataType)
{
	while (th->sp > th->callstack + 1)
	{
		if (STACK_IS_PNT(th, 1) && (STACK_PNT(th, 1) == MM.loopMark)) {
			STACK_SET_TYPE(th, 0, data, dataType);
			return 0;
		}
		STACK_DROP(th);
	}
	PRINTF(LOG_SYS, "> Uncaught continue!\n");	// should never happen
	return 0;
}

int interpreterBreak(Thread* th, LW data, int dataType)
{
	while (th->sp > th->callstack)
	{
		if (STACK_IS_PNT(th, 0) && (STACK_PNT(th, 0) == MM.loopMark)) {
			STACK_SET_TYPE(th, 0, data, dataType);
			return 0;
		}
		STACK_DROP(th);
	}
	PRINTF(LOG_SYS, "> Uncaught break!\n");	// should never happen
	return 0;
}

int interpreterAbort(Thread* th)
{
	LINT callstack = th->callstack;
	LINT i = th->sp;
	LB* fun = th->fun;
	LB* bytecode;

	while(callstack>=0)
	{
//		if (fun->pkg == th->fun->pkg) {	// we force the try to be in the same package as abort
			while (i > callstack + 1)
			{
				if ((ARRAY_IS_PNT(STACK_BLOCK(th), i - 1)) && (ARRAY_PNT(STACK_BLOCK(th), i - 1) == MM.abortMark)) {
					th->sp = i;
					th->pc = STACK_PULL_INT(th);
					th->fun = fun;
					th->callstack = callstack;
					STACK_SET_NIL(th, 0);
					return 0;
				}
				i--;
			}
//		}
		i = callstack - CALLSTACK_LENGTH;
		fun=STACK_REF_PNT(th,callstack,CALLSTACK_FUN);
		callstack=STACK_REF_INT(th,callstack,CALLSTACK_PREV);
	}
	PRINTF(LOG_SYS,"> Uncaught abort in '%s.%s'!\n", pkgName(MM.currentPkg),interpreterCurrentFun(th));

	th->sp = th->callstack + 1;	// OPabort ensures there is at least one element there
	STACK_SET_NIL(th, 0);

	bytecode = (ARRAY_PNT(th->fun, FUN_USER_BC));
	STACK_SKIP(th, BC_ARGS(bytecode) + BC_LOCALS(bytecode) + CALLSTACK_LENGTH + 1);
	th->fun = STACK_REF_PNT(th, th->callstack, CALLSTACK_FUN);
	th->pc = STACK_REF_INT(th, th->callstack, CALLSTACK_PC);
	th->callstack = STACK_REF_INT(th, th->callstack, CALLSTACK_PREV);

	return 0;
}
LINT interpreterEnd(Thread* th,LINT count, LINT result)
{
	th->count += count;
//	stackReset(th);
	if (result == EXEC_OM || MM.OM) return EXEC_OM;
	if (result == EXEC_IDLE) th->sp--;
	return result;
}

#define INTERPRETER_END(val) interpreterEnd(th, nloop - count, val)
#define INTERPRETER_OM INTERPRETER_END(EXEC_OM)

LINT interpreterRun(Thread* th,LINT maxCycles)
{
	LB* globals;
	LB* bytecode;
	LINT locals,result;
	char* pc;
	LINT op;
	LINT cyclesToGo = maxCycles;
	LINT preemptiveCount = th->count + maxCycles;
	stackReset(MM.tmpStack);
	if (th->callstack<0) return EXEC_IDLE;	// nothing to execute

	BC_PRECOMPUTE

	systemBignumReset();
	while(1)
	{
		LINT count,i,j,b,n;
		int isNil;
		LFLOAT f;
		LB* p;
		LW wi;
		Def* def;

		LINT nloop = MM.gc_period;
		if (maxCycles && (cyclesToGo < nloop)) nloop = cyclesToGo;
		if ((th->atomic) && (nloop < 1)) nloop = 1;

//		PRINTF(LOG_DEV,"go %lld: %lld (%lld, %lld)",th->uid,nloop, maxCycles, cyclesToGo);
		count = nloop;
		while ((count--) > 0)
		{
			op = (*(pc++))&255;
			if (op & 0x80) op = (op<< 8) + ((*(pc++)) & 255);
		processOpCode:

//			if (interpreterTRON)
//			{
//				LINT pci=(LINT)(pc-1-BC_START(bytecode));
//				threadDump(LOG_USER,th,20);
//				PRINTF(LOG_USER,"#" LSD " (sp=" LSD " locals=" LSD " callstack=" LSD ") %s -> ",th->uid,th->sp, STACK_REF(th)-locals,STACK_REF(th)-th->callstack,interpreterCurrentFun(th));
//				PRINTF(LOG_USER,"count=" LSD "/" LSD " pc=" LSD " op=%2d:",th->count+nloop-count, maxCycles,pci,op);
//				opcodePrint(LOG_USER,op,pc,pci);
////				if (1) PRINTF(LOG_DEV,"");
////				getchar();
//			}

			switch (op) {
			case OPabort:
				if (interpreterAbort(th)) return INTERPRETER_OM;
				if (th->callstack < 0) return INTERPRETER_END(EXEC_IDLE);  // Exception uncaught
				BC_PRECOMPUTE
				break;
			case OPabs: BCINT1FUN(absint)
			case OPabsf: BCFLOAT1FUN(fabs)
			case OPacos: BCFLOAT1FUN(acos)
			case OPadd: BCINT2(+)
			case OPaddf: BCFLOAT2(+)
			case OPand: BCINT2(&)
			case OPasin: BCFLOAT1FUN(asin)
			case OPatan: BCFLOAT1FUN(atan)
			case OPatan2: BCFLOAT2FUN(atan2)
			case OPatomic:
				th->atomic = (*(pc++)) & 255;
				break;
			case OPbreak:
				if (interpreterBreak(th, STACK_GET(th, 0), STACK_TYPE(th, 0))) return INTERPRETER_OM;
//				BC_PRECOMPUTE
				break;
			case OPcast:
				if (STACK_GET(th, 0) != STACK_GET(th, 1)) STACK_SET_NIL(th, 2);
				STACK_DROPN(th, 2);
				break;
			case OPcastb:
				i = (*(pc++)) & 255;
				if (STACK_GET(th, 0) != VAL_FROM_INT(i)) STACK_SET_NIL(th, 1);
				STACK_DROP(th);
				break;
			case OPceil: BCFLOAT1FUN(ceil)
			case OPcompact:
#ifdef USE_MEMORY_C
				if (MM.gcTrace) PRINTF(LOG_SYS, "> GC: Compact from opcode (larger block "LSD"/"LSD")\n",BmmMaxSize,MEMORY_SAFE_SIZE);
				th->pc = pc - BC_START(bytecode);
				memorySetTmpRoot((LB*)th);
				i= bmmCompact();
				th = (Thread*)MM.tmpRoot;
				STACK_PUSH_INT_ERR(th, i, INTERPRETER_OM);
				BC_PRECOMPUTE
#else
				memoryFinalizeGC();
				memoryFinalizeGC();
				STACK_PUSH_NIL_ERR(th, INTERPRETER_OM);
#endif
				break;
			case OPconst:
				STACK_LOAD(th, 0, globals, STACK_INT(th, 0));
				break;
			case OPconstb:
				i = (*(pc++)) & 255;
				STACK_PUSH_NIL_ERR(th, INTERPRETER_OM);
				STACK_LOAD(th, 0, globals, i);
				break;
			case OPcontinue:
				if (interpreterContinue(th, STACK_GET(th, 0), STACK_TYPE(th, 0))) return INTERPRETER_OM;
				break;
			case OPcos: BCFLOAT1FUN(cos)
			case OPcosh: BCFLOAT1FUN(cosh)
			case OPdftup:
				n = STACK_PULL_INT(th);
				STACK_PUSH_EMPTY_ARRAY_ERR(th, n, DBG_TUPLE, INTERPRETER_OM);
				p = STACK_PNT(th, 0);
				j = 1;
				for (i = n - 1; i >= 0; i--) STACK_STORE(p, i, th, j++);
				STACK_SKIP(th, n);
				break;
			case OPdftupb:
				n = (*(pc++)) & 255;
				STACK_PUSH_EMPTY_ARRAY_ERR(th, n, DBG_TUPLE, INTERPRETER_OM);
				p = STACK_PNT(th, 0);
				j = 1;
				for (i = n - 1; i >= 0; i--) STACK_STORE(p, i, th, j++);
				STACK_SKIP(th, n);
				break;
			case OPdfarray:
				n = STACK_PULL_INT(th);
				STACK_PUSH_EMPTY_ARRAY_ERR(th, n, DBG_ARRAY, INTERPRETER_OM);
				p = STACK_PNT(th, 0);
				j = 1;
				for (i = n - 1; i >= 0; i--) STACK_STORE(p, i, th, j++);
				STACK_SKIP(th, n);
				break;
			case OPdfarrayb:
				n = (*(pc++)) & 255;
				STACK_PUSH_EMPTY_ARRAY_ERR(th, n, DBG_ARRAY, INTERPRETER_OM);
				p = STACK_PNT(th, 0);
				j = 1;
				for (i = n - 1; i >= 0; i--) STACK_STORE(p, i, th, j++);
				STACK_SKIP(th, n);
				break;
			case OPdiv:
				b = STACK_INT(th, 0);
				if (!b)
				{
					PRINTF(LOG_SYS, "> Error: BCdiv division by zero in function %s\n", interpreterCurrentFun(th));
					STACK_SET_INT(th, 1, (0));
				}
				else STACK_SET_INT(th, 1, STACK_INT(th, 1) / b);
				STACK_DROP(th);
				break;
			case OPdivf: BCFLOAT2(/ )
			case OPdrop:
				STACK_DROP(th);
				break;
			case OPdump:
				PRINTF(LOG_USER, "->");
				itemDump(LOG_USER, STACK_GET(th, 0), STACK_TYPE(th, 0));
				break;
			case OPdumpd:
				//PRINTF(LOG_USER, "->");
				itemDumpDirect(LOG_USER, STACK_GET(th, 0), STACK_TYPE(th, 0));
				break;
			case OPdup:
				 STACK_PUSH_NIL_ERR(th, INTERPRETER_OM);
				STACK_INTERNAL_COPY(th, 0, 1);
				break;
			case OPelse:
				if (STACK_PULL_PNT(th) != MM._true) pc += bytecodeGetJump(pc);
				else pc += BC_JUMP_SIZE;
				break;
			case OPeor: BCINT2(^)
			case OPeq:
				STACK_SET_BOOL(th, 1, lwEquals(STACK_GET(th, 0), STACK_TYPE(th, 0), STACK_GET(th, 1), STACK_TYPE(th, 1)));
				STACK_DROP(th);
				break;
			case OPexec:// [fonction, argn, ..., arg1]
				n = STACK_PULL_INT(th);
				EXEC_COMMON
				break;
			case OPexecb:
				n = (*(pc++)) & 255;
				EXEC_COMMON
				break;
			case OPexp: BCFLOAT1FUN(exp)
			case OPfalse:
				STACK_PUSH_PNT_ERR(th, MM._false, INTERPRETER_OM);
				break;
			case OPfetch:
				isNil = STACK_IS_NIL(th, 0);
				i = STACK_PULL_INT(th);
				p = STACK_PNT(th, 0);
				if ((!p) || isNil) {
					STACK_SET_NIL(th, 0);
				}
				else
				{
					if ((i < 0) || (i >= ARRAY_LENGTH(p))) {
						STACK_SET_NIL(th, 0);
					}
					else STACK_LOAD(th, 0, p, i);
				}
				break;
			case OPfetchb:
				i = (*(pc++)) & 255;
				p = STACK_PNT(th, 0);
				if (p) STACK_LOAD(th, 0, p, i);
				break;
			case OPfinal:
				STACK_COPY_TO_REF(th, th->callstack, -1, 0);
				th->sp = th->callstack + 1;
				break;
			case OPfirst:
				STACK_PUSH_NIL_ERR(th, INTERPRETER_OM);
				p = STACK_PNT(th, 1);
				if (p&&(HEADER_TYPE(p) == TYPE_ARRAY)) STACK_LOAD(th, 0, p, 0);
				break;
			case OPfloat:
				i = getLsbInt(pc);
				pc += LWLEN;
				STACK_PUSH_NIL_ERR(th, INTERPRETER_OM);
				STACK_SET_FLOAT(th, 0, *(LFLOAT*)(&i));
				break;
			case OPfloor: BCFLOAT1FUN(floor)
			case OPformat:
				n = STACK_PULL_INT(th);
				if (bufferFormat(MM.tmpBuffer, th, n)) return INTERPRETER_OM;
				if (n > 0) STACK_SKIP(th, n);
				break;
			case OPformatb:
				n = (*(pc++)) & 255;
				if (bufferFormat(MM.tmpBuffer, th, n)) return INTERPRETER_OM;
				if (n > 0) STACK_SKIP(th, n);
				break;
			case OPge: BCINT2BOOL(>= )
			case OPgef: BCFLOAT2BOOL(>= )
			case OPgoto:
				pc += bytecodeGetJump(pc);
				break;
			case OPgt: BCINT2BOOL(> )
			case OPgtf: BCFLOAT2BOOL(> )
			case OPhd:
				if (STACK_PNT(th, 0)) STACK_LOAD(th, 0, STACK_PNT(th, 0), 0);
				break;
			case OPhide:
				def = (Def*)ARRAY_PNT(globals, STACK_INT(th, 0));
				def->public = DEF_HIDDEN;
				STACK_SET_BOOL(th, 0, 1);
				break;
			case OPholdon:
				th->pc = pc - BC_START(bytecode);
				th->atomic = 0;
				return EXEC_WAIT;
			case OPint:
				i = getLsbInt(pc);
				pc += LWLEN;
				STACK_PUSH_INT_ERR(th,i, INTERPRETER_OM);
				break;
			case OPintb:
				i = pc[0] & 255;
				pc++;
				STACK_PUSH_INT_ERR(th,i, INTERPRETER_OM);
				break;
			case OPisnan:
				f = STACK_FLOAT(th, 0);
				STACK_SET_BOOL(th, 0, isnan(f));
				break;
			case OPisinf:
				f = STACK_FLOAT(th, 0);
				STACK_SET_BOOL(th, 0, isinf(f));
				break;
			case OPlambda:
				n = STACK_PULL_INT(th);
				STACK_PUSH_EMPTY_ARRAY_ERR(th, n, DBG_LAMBDA, INTERPRETER_OM);
				p = STACK_PNT(th, 0);
				j = 1;
				for (i = n - 1; i >= 0; i--) STACK_STORE(p, i, th, j++);
				STACK_SKIP(th, n);
				break;
			case OPlambdab:
				n = (*(pc++)) & 255;
				STACK_PUSH_EMPTY_ARRAY_ERR(th, n, DBG_LAMBDA, INTERPRETER_OM);
				p = STACK_PNT(th, 0);
				j = 1;
				for (i = n - 1; i >= 0; i--) STACK_STORE(p, i, th, j++);
				STACK_SKIP(th, n);
				break;
			case OPle:	BCINT2BOOL(<= )
			case OPlef: BCFLOAT2BOOL(<= )
			case OPln: BCFLOAT1FUN(log)
			case OPlog: BCFLOAT1FUN(log10)
			case OPloop:
				STACK_PUSH_PNT_ERR(th, MM.loopMark, INTERPRETER_OM)	// push loopMark
				STACK_PUSH_NIL_ERR(th, INTERPRETER_OM);
				break;
			case OPlt:	BCINT2BOOL(< )
			case OPltf: BCFLOAT2BOOL(< )
			case OPmax: BCINT2FUN(maxint)
			case OPmaxf: BCFLOAT2FUN(maxf)
			case OPmin: BCINT2FUN(minint)
			case OPminf: BCFLOAT2FUN(minf)
			case OPmklist:
				STACK_PUSH_FILLED_ARRAY_ERR(th, LIST_LENGTH, DBG_LIST,INTERPRETER_OM);
				break;
			case OPmod:
				b = STACK_INT(th, 0);
				if (!b)
				{
					PRINTF(LOG_SYS, "> Error: BCmod division by zero in function %s\n", interpreterCurrentFun(th));
					STACK_SET_INT(th, 1, (0));
				}
				else STACK_SET_INT(th, 1, STACK_INT(th, 1) % b);
				STACK_DROP(th);
				break;
			case OPmodf:BCFLOAT2FUN(fmod)
			case OPmul: BCINT2(*)
			case OPmulf: BCFLOAT2(*)
			case OPne:
				STACK_SET_BOOL(th, 1, !lwEquals(STACK_GET(th, 0), STACK_TYPE(th, 0), STACK_GET(th, 1), STACK_TYPE(th, 1)));
				STACK_DROP(th);
				break;
			case OPneg: BCINT1(-)
			case OPnegf: BCFLOAT1(-)
			case OPnil:
				STACK_PUSH_NIL_ERR(th, INTERPRETER_OM);
				break;
			case OPnon:
				STACK_SET_BOOL(th, 0, STACK_PNT(th, 0) != MM._true);
				break;
			case OPnop:
				break;
			case OPnot: BCINT1(~)
			case OPor: BCINT2(| )
			case OPpick:
				wi = STACK_GET(th,0);
				p = STACK_PNT(th, 1);
				if ((!p) || (wi == NIL)) {
					STACK_SET_NIL(th, 0);
				}
				else
				{
					i = INT_FROM_VAL(wi);
					if ((i < 0) || (i >= ARRAY_LENGTH(p))) {
						STACK_SET_NIL(th, 0);
					}
					else STACK_LOAD(th, 0, p, i);
				}
				break;
			case OPpickb:
				i = (*(pc++)) & 255;
				p = STACK_PNT(th, 0);
				 STACK_PUSH_NIL_ERR(th, INTERPRETER_OM);
				if (p && (i >= 0) && (i < ARRAY_LENGTH(p))) STACK_LOAD(th,0,p,i);
				break;
			case OPpow: BCFLOAT2FUN(pow)
			case OPpowint: BCINT2FUN(powerInt)
			case OPprompt:
				if (promptOnThread(th)) return INTERPRETER_OM;
//				PRINTF(LOG_SYS, "> ----return from OPprompt\n");
//				interpreterTRON = 1;
//				threadDump(LOG_SYS, th, 20);
				break;
			case OPret:
				if (STACK_REF(th) - th->callstack != 1)
				{
					PRINTF(LOG_SYS, "> Error: RET sp=" LSD ", should be " LSD " in %s\n", STACK_REF(th), th->callstack + 1, interpreterCurrentFun(th));
					threadDump(LOG_SYS, th, 6);
					return EXEC_EXIT;  // wrong implementation
				}
				th->fun = STACK_REF_PNT(th, th->callstack, CALLSTACK_FUN);
				th->pc = STACK_REF_INT(th, th->callstack, CALLSTACK_PC);
				th->callstack = STACK_REF_INT(th, th->callstack, CALLSTACK_PREV);
				STACK_SKIP(th, BC_ARGS(bytecode) + BC_LOCALS(bytecode) + CALLSTACK_LENGTH + 1);
				if (th->callstack < 0) return INTERPRETER_END(EXEC_IDLE);  // successful end of bytecode execution
				BC_PRECOMPUTE
				break;
			case OPrglob:
				def = (Def*)ARRAY_PNT(globals, STACK_INT(th, 0));
				STACK_SET_TYPE(th, 0, def->val, def->valType);
				break;
			case OPrglobb:
				n = (*(pc++)) & 255;
				def = (Def*)ARRAY_PNT(globals, n);
				STACK_PUSH_NIL_ERR(th, INTERPRETER_OM);
				STACK_SET_TYPE(th, 0, def->val, def->valType);
				break;
			case OPrloc:
				STACK_COPY_FROM_REF(th, 0, locals, -STACK_INT(th, 0));
				break;
			case OPrlocb:
				n = (*(pc++)) & 255;
				STACK_PUSH_NIL_ERR(th, INTERPRETER_OM);
				STACK_COPY_FROM_REF(th, 0, locals, -n);
				break;
			case OPround: BCFLOAT1FUN(roundls)
			case OPsglobi:
				def = (Def*)ARRAY_PNT(globals, STACK_INT(th, 1));
				defSet(def, STACK_GET(th, 0),STACK_TYPE(th, 0));
				STACK_SKIP(th, 1);
				break;
			case OPshl: BCINT2(<< )
			case OPshr: BCINT2(>> )
			case OPsin: BCFLOAT1FUN(sin)
			case OPsinh: BCFLOAT1FUN(sinh)
			case OPskip:
				i = STACK_INT(th, 0);
				STACK_SKIP(th, i + 1);
				break;
			case OPskipb:
				i = (*(pc++)) & 255;
				STACK_SKIP(th, i);
				break;
			case OPsloc: // [index val] -> 0
				STACK_COPY_TO_REF(th, locals, -STACK_INT(th, 0), 1);
				STACK_DROPN(th, 2);
				break;
			case OPslocb: // [val] -> 0
				n = (*(pc++)) & 255;
				STACK_COPY_TO_REF(th, locals, -n, 0);
				STACK_DROP(th);
				break;
			case OPsloci: // [val index] -> [val]
				STACK_COPY_TO_REF(th, locals, -STACK_INT(th, 1), 0);
				STACK_SKIP(th, 1);
				break;
			case OPsqr: BCFLOAT1FUN(sqr)
			case OPsqrt: BCFLOAT1FUN(sqrt)
			case OPstore:	 // [val index table] -> [val]
				isNil = STACK_IS_NIL(th, 1);
				p = STACK_PNT(th, 2);
				if (p && !isNil)
				{
					i = STACK_INT(th,1);
					if ((i >= 0) && (i < ARRAY_LENGTH(p))) STACK_STORE(p, i, th, 0);
				}
				STACK_SKIP(th, 2);
				break;
			case OPstruct:
				def = (Def*)ARRAY_PNT(globals, STACK_INT(th, 0));
				STACK_DROP(th);
				STACK_PUSH_EMPTY_ARRAY_ERR(th, def->index, VAL_FROM_PNT((LB*)def), INTERPRETER_OM);
				break;
			case OPsub:BCINT2(-)
			case OPsubf: BCFLOAT2(-)
			case OPsum:
				def = (Def*)ARRAY_PNT(globals, STACK_INT(th, 0));
				STACK_DROP(th);
				STACK_PUSH_FILLED_ARRAY_ERR(th, def->type->nb, VAL_FROM_PNT((LB*)def), INTERPRETER_OM);
				break;
			case OPswap:
				STACK_PUSH_NIL_ERR(th, INTERPRETER_OM);
				STACK_INTERNAL_COPY(th, 0, 1);
				STACK_INTERNAL_COPY(th, 1, 2);
				STACK_INTERNAL_COPY(th, 2, 0);
				STACK_DROP(th);
				break;
			case OParraylen:
				p = STACK_PNT(th, 0);
				if (p) STACK_SET_INT(th, 0, ARRAY_LENGTH(p));
				break;
			case OPresign:
				STACK_PUSH_NIL_ERR(th, INTERPRETER_OM);
				th->count += nloop;
				th->pc = pc - BC_START(bytecode);
				return EXEC_PREEMPTION;
			case OPtan: BCFLOAT1FUN(tan)
			case OPtanh: BCFLOAT1FUN(tanh)
			case OPtfc:// [fonction, argn, ..., arg1]
				n = STACK_PULL_INT(th);
				EXEC_COMMON_TFC
				if (th->callstack < 0) return INTERPRETER_END(EXEC_IDLE);  // successful end of bytecode execution
				break;
			case OPtfcb:
				n = (*(pc++)) & 255;
				EXEC_COMMON_TFC
				if (th->callstack < 0) return INTERPRETER_END(EXEC_IDLE);  // successful end of bytecode execution
				break;
			case OPtl:
				if (STACK_PNT(th, 0)) STACK_LOAD(th, 0, STACK_PNT(th, 0), 1);
				break;
			case OPtron:
				interpreterTRON = 1;
				STACK_PUSH_NIL_ERR(th, INTERPRETER_OM);
				break;
			case OPtroff:
				interpreterTRON = 0;
				 STACK_PUSH_NIL_ERR(th, INTERPRETER_OM);
				break;
			case OPtrue:
				STACK_PUSH_PNT_ERR(th, MM._true, INTERPRETER_OM);
				break;
			case OPtry:
				th->error = NULL;
				STACK_PUSH_PNT_ERR(th, MM.abortMark, INTERPRETER_OM)
				STACK_PUSH_INT_ERR(th, (LINT)(pc + bytecodeGetJump(pc) - BC_START(bytecode)), INTERPRETER_OM);
				pc += BC_JUMP_SIZE;
				break;
			case OPupdt: // [index val table] -> [table]
				i = STACK_PULL_INT(th);
				if (STACK_PNT(th,1)) STACK_STORE(STACK_PNT(th, 1), i, th, 0);
				STACK_DROP(th);
				break;
			case OPupdtb: // [val table] -> [table]
				i = (*(pc++)) & 255;
				p = STACK_PNT(th, 1);
				if (p) STACK_STORE(p, i, th, 0);
				STACK_DROP(th);
				break;
			default:
//				printf("default %d\n", op);
				if (op & 0x8000) {
					Native* n;
					NATIVE native;
					int k,argc;
					LINT skip, def;
					LINT safeAlloc = MM.safeAlloc;
					int scheduler = (th == MM.scheduler);
					int opext = op&0x7fff;
					if (opext >= NATIVE_DEF_LENGTH) {
						bytecodePrint(LOG_SYS, bytecode);
						_hexDump(LOG_SYS, BC_START(bytecode), STR_LENGTH(bytecode)-BC_OFFSET, 0);
						PRINTF(LOG_SYS, "> Error: NATIVE pc=" LSD" ("LSX") opcode = %d (%x) is out of range in '%s'\n", pc - BC_START(bytecode), pc - BC_START(bytecode), opext, opext|0x8000, interpreterCurrentFun(th));
						threadPrintCallstack(th);
						return EXEC_EXIT;
					}
					n = NativeDefs[opext];
					if (!n) {
						PRINTF(LOG_SYS, "> Error: NATIVE pc=" LSD" ("LSX") opcode = %d (%x) is NULL in '%s'\n", pc - BC_START(bytecode), pc - BC_START(bytecode), opext, opext | 0x8000, interpreterCurrentFun(th));
						return EXEC_EXIT;
					}
//					PRINTF(LOG_SYS,"native %s\n",n->name);
					native = (NATIVE)n->value;
					argc = NativeDefsArgc[opext];
					if (!argc) {	// we add a fake nil argument for function without arg
						STACK_PUSH_NIL_ERR(th, INTERPRETER_OM);
						argc = 1;
					}
					def = STACK_REF(th) - argc +1;	// def is the position of the first arg
					th->pc = pc - BC_START(bytecode);
					MM.tmpRoot = NULL;
					k = (*native)(th);
					if (scheduler) {
						th = MM.scheduler;
						BC_PRECOMPUTE
					}
//					PRINTF(LOG_SYS,"native %s done\n",n->name);
					//		if (MM.gcTrace) PRINTF(LOG_DEV, "/");
					MM.tmpRoot = NULL;
					while (MM.safeAlloc > safeAlloc) memoryLeaveSafe();	// this simplifies a lot how to write native functions which need "safeAlloc"
					if (k>0) return INTERPRETER_END(k);
					skip = STACK_REF(th) - def;
					if (skip < 0)
					{
						PRINTF(LOG_SYS, "> Error: NATIVE sp=" LSD ", should be at least " LSD " in %s\n", STACK_REF(th), def, n->name);
						threadPrintCallstack(th);
						//			threadDump(LOG_SYS,th,5);
						return EXEC_EXIT;  // wrong implementation
					}
					else if (skip > 0) STACK_SKIP(th, skip);

				}
				else {
					PRINTF(LOG_SYS, "\n> Error: Illegal Opcode %d in %s\n", 255 & op, interpreterCurrentFun(th));
					//					memoryAbort(m,1);
					return EXEC_EXIT;
				}
			}
		}
#ifdef USE_MEMORY_C
		if (BmmMaxSize < MEMORY_SAFE_SIZE) {
			if (MM.gcTrace) PRINTF(LOG_SYS, "> GC: Compact in interpreter loop (larger block "LSD"/"LSD")\n",BmmMaxSize,MEMORY_SAFE_SIZE);
			th->pc = pc - BC_START(bytecode);
			memorySetTmpRoot((LB*)th);
			bmmCompact();
			th = (Thread*)MM.tmpRoot;
			if (th!=MM.scheduler && !th->atomic) {
				th->count += nloop;
				return EXEC_PREEMPTION;
			}
			if (MM.gcTrace) PRINTF(LOG_SYS, "> GC: Compact and continue\n");
			BC_PRECOMPUTE
		}
#endif
//		PRINTF(LOG_DEV,"N");
		th->count+=nloop;
		if (((preemptiveCount-th->count) < 0) && (maxCycles) && !th->atomic)
		{
			th->pc = pc - BC_START(bytecode);
//			PRINTF(LOG_DEV,"P");
			return EXEC_PREEMPTION;
		}

		memoryGC(MM.gc_period);
	}
}

