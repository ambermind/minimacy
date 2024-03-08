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

int interpreterTRON=0;

LB* runtimeCheckAddress=NULL;
LINT runtimeCheckValue=0;

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
	p=TABPNT(fun,FUN_USER_NAME);
	if ((p)&&(HEADER_TYPE(p)==TYPE_TAB)) return 1;
	return 0;
}
LB* getFunStruct(LB* fun)
{
	if (isHigherFun(fun)) return TABPNT(fun,FUN_USER_NAME);
	return fun;
}

char* interpreterCurrentFun(Thread* th)
{
	LB* p=TABPNT(getFunStruct(th->fun),FUN_USER_NAME);
	if (p) return STRSTART(p);
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
	int k;
	LINT i,nlocals;
	LB* bytecode;
	LB* fun=STACKPNT(th,argc);
	th->forceOpcode=THREAD_OPCODE_NONE;
	if (!fun)
	{
		STACKDROPN(th,argc);
		return 0;
	}
	if (tfc)
	{
//		threadDump(LOG_SYS,th,10); PRINTF(LOG_DEV,">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>before tfc\n"); getchar();

		th->fun=STACKREFPNT(th,th->callstack,CALLSTACK_FUN);
		th->pc=STACKREFINT(th,th->callstack,CALLSTACK_PC);
		th->callstack=STACKREFINT(th,th->callstack,CALLSTACK_PREV);
		for(i=-1;i<argc;i++) STACKCOPYTOREF(th,tfc,-i,argc-1-i);

		STACKDROPN(th,th->pp-(tfc+argc)+1);
//		threadDump(LOG_SYS,th,10); PRINTF(LOG_DEV,">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>after tfc\n"); getchar();
	}

	if (isHigherFun(fun))
	{
		LINT i;
		LINT binds=TABLEN(fun)-1;

//		threadDump(LOG_SYS,th,10); PRINTF(LOG_DEV,">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>start higherfun\n"); getchar();

		for(i=0;i<binds;i++) FUN_PUSH_NIL;
		for(i=0;i<argc;i++) STACKINTERNALCOPY(th,i,i+binds);
		for(i=0;i<binds;i++) STACKLOAD(th,argc+i,fun,binds-i);
		STACKLOAD(th,argc+binds,fun,FUN_USER_NAME);
		fun=TABPNT(fun,FUN_USER_NAME);
//		threadDump(LOG_SYS,th,10); PRINTF(LOG_DEV,">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>done higherfun\n"); getchar();
//		interpreterTRON=1;

	}
	else if (TABLEN(fun)==FUN_NATIVE_LEN)
	{
		NATIVE native;
		LINT fastAlloc= memoryGetFast();
		LINT def=STACKREF(th)-argc;	// def is the position of the native function in the stack
		LINT skip;

		if (!TABISPNT(fun,FUN_NATIVE_POINTER))
		{
			LINT i;
			for(i=argc-1;i>=0;i--) STACKINTERNALCOPY(th,i+1,i);
			STACKDROP(th);
			th->forceOpcode=(char)TABINT(fun,FUN_NATIVE_POINTER);
//			return tfc?-1:0;
			return 0;
		}
//		if (MM.gcTrace) PRINTF(LOG_DEV,"%s",STRSTART((TABPNT(fun,FUN_NATIVE_NAME))));
		native=(NATIVE)TABGET(TABPNT(fun,FUN_NATIVE_POINTER),0);
		k=(*native)(th);
//		if (MM.gcTrace) PRINTF(LOG_DEV, "/");
		while(memoryGetFast()>fastAlloc) memoryLeaveFast();	// this simplifies a lot how to write native functions which need "fastAlloc"
//		if (k) {
//			PRINTF(LOG_DEV,"native %s returns %d\n", STRSTART((TABPNT(fun, FUN_NATIVE_NAME))), k);
//			threadDump(LOG_SYS, th, 3);
//		}
		if (k) return k;
		skip = STACKREF(th) - def;
		if (skip<0)
		{
			PRINTF(LOG_SYS,"NATIVE pp=" LSD ", should be at least " LSD " in %s\n",STACKREF(th),def,STRSTART((TABPNT(fun,FUN_NATIVE_NAME))));
//			threadDump(LOG_SYS,th,5);
			return -1;  // wrong implementation
		}
		else if (skip >0) STACKSKIP(th, skip);
		// now the returned value of the native function has replaced the native function in the stack
		return tfc ? -1: 0;
	}

	bytecode=(TABPNT(fun,FUN_USER_BC));

	nlocals=BC_LOCALS(bytecode);
	for(i=0;i<nlocals;i++) FUN_PUSH_NIL;
	FUN_PUSH_NIL;
	FUN_PUSH_PNT(th->fun);
	FUN_PUSH_INT(th->pc);
	FUN_PUSH_INT(th->callstack);
	th->callstack=STACKREF(th);
	th->pc=0;
	th->fun=fun;
	return -1;
}

int interpreterBreakThrow(Thread* th,LINT type, LW data, int dataType)
{
	while(th->callstack>=0)
	{
		LB* last=NULL;
		LB* mark=STACKREFPNT(th,th->callstack,CALLSTACK_MARK);

		while(mark)
		{
			if (TABINT(mark,MARK_TYPE)==type)
			{
				if (last) TABSETPNT(last,MARK_NEXT,TABPNT(mark,MARK_NEXT))
				else STACKREFSETPNT(th,th->callstack,CALLSTACK_MARK,TABPNT(mark,MARK_NEXT));	// pop the mark
				th->pc=TABINT(mark,MARK_PC);
				th->pp=TABINT(mark,MARK_PP);
				FUN_PUSH_NIL;
				STACKSETTYPE(th,0,data,dataType);
				return 0;
			}
			last=mark;
			mark=TABPNT(mark,MARK_NEXT);
		}

		th->fun=STACKREFPNT(th,th->callstack,CALLSTACK_FUN);
		th->pc=STACKREFINT(th,th->callstack,CALLSTACK_PC);
		th->callstack=STACKREFINT(th,th->callstack,CALLSTACK_PREV);
	}
	PRINTF(LOG_USER,"Uncaught exception!\n");
	itemDump(th, LOG_USER,data, dataType);
	return 0;
}
LINT interpreterEnd(Thread* th,LINT count, LINT result)
{
	th->count += count;
//	stackReset(th);
	if (result == EXEC_IDLE) th->pp--;
//	if (result == EXEC_OM) PRINTF(LOG_DEV,"EXEC_OM "LSX":" LSD" / "LSX" :" LSD"\n", th,th->uid, MM.thread, MM.thread->uid);
	if (result == EXEC_OM)
	{
		if (th->header.mem == th->memDelegate) th->OM = 1;
		else
		{
			result = interpreterBreakThrow(th, MARK_TYPE_TRY, MM.MemoryException, VAL_TYPE_PNT ); if (result) return result;
			if (th->callstack < 0) return EXEC_EXIT;  // Exception uncaught, should not happen if _memoryAssign is called only from Bios memoryUse function
			result = EXEC_PREEMPTION;	// the easiest : return to the scheduler
		}
	}
	return result;
}
LINT interpreterCount0(Thread* th, LINT maxCycles, LINT cyclesToGo)
{
	LINT nloop = MM.gc_period;
	if (maxCycles && (cyclesToGo < nloop)) nloop = cyclesToGo;
	if ((th->atomic) && (nloop < 1)) nloop = 1;
	return nloop;
}

#define INTERPRETER_END(val) interpreterEnd(th, nloop - count, val)
#define INTERPRETER_OM INTERPRETER_END(EXEC_OM)

LINT interpreterRun(Thread* th,LINT maxCycles)
{
	LB* globals;
	LB* bytecode;
	LINT locals,result;
	char* pc;
	char op;
	LINT cyclesToGo = maxCycles;
	LINT preemptiveCount = th->count + maxCycles;

	if (th->callstack<0) return EXEC_IDLE;	// nothing to execute

	BC_PRECOMPUTE

	coreBignumReset();
	while(1)
	{
		LINT count,i,j,b,n;
		int isNil;
		LFLOAT f;
		LB* p;
		LW wi;
		Def* def;
		LINT nloop = interpreterCount0(th, maxCycles, cyclesToGo);
//		PRINTF(LOG_DEV,"go %lld: %lld (%lld, %lld)",th->uid,nloop, maxCycles, cyclesToGo);
		count = nloop;
		while ((count--) > 0)
		{
			op = *(pc++);
		processOpCode:
			/*			if (th->count==0x6ba-3)
						{
			//				threadDump(LOG_SYS,th,100);
			//				getchar();
							interpreterTRON=1;
						}
			*/
			//			if (op==OPtry) interpreterTRON=1;
			/*			if (runtimeCheckAddress && (runtimeCheckValue!=HEADER_SIZE(runtimeCheckAddress)))
						{
							PRINTF(LOG_DEV,"--------VALUE HAS CHANGED\n");
							PRINTF(LOG_DEV, LSX ": " LSX "-> " LSX "\n",runtimeCheckAddress, runtimeCheckValue, HEADER_SIZE(runtimeCheckAddress));
							PRINTF(LOG_DEV,"stack=" LSX "\n" , th->stack);
							PRINTF(LOG_DEV,"count=" LSX "\n" , th->count);

							interpreterTRON=1;
							getchar();
						}
			*/
			//			if (op==OPtfcb)
/*			if (interpreterTRON)
			{
				LINT pci=(LINT)(pc-1-BC_START(bytecode));
				threadDump(LOG_USER,th,10);
				PRINTF(LOG_USER,"#" LSD " (pp=" LSD " locals=" LSD " callstack=" LSD ") %s -> ",th->uid,th->pp, STACKREF(th)-locals,STACKREF(th)-th->callstack,interpreterCurrentFun(th));
				PRINTF(LOG_USER,"count=" LSD "/" LSD " pc=" LSD " op=%2d:",th->count+nloop-count, maxCycles,pci,op);
				opcodePrint(th, LOG_USER,op,pc,pci);
//				if (1) PRINTF(LOG_DEV,"");
				getchar();
			}
*/
//			PRINTF(LOG_DEV,"$");
//			{LINT pci = (LINT)(pc - 1 - BC_START(bytecode));
//			opcodePrint(th, LOG_USER, op, pc, pci); }
			switch (op) {
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
				if (interpreterBreakThrow(th, MARK_TYPE_BREAK, STACKGET(th, 0), STACKTYPE(th, 0))) return INTERPRETER_OM;
				BC_PRECOMPUTE
				break;
			case OPcast:
				if (STACKGET(th, 0) != STACKGET(th, 1)) STACKSETNIL(th, 2);
				STACKDROPN(th, 2);
				break;
			case OPcastb:
				i = (*(pc++)) & 255;
				if (STACKGET(th, 0) != INTTOVAL(i)) STACKSETNIL(th, 1);
				STACKDROP(th);
				break;
			case OPceil: BCFLOAT1FUN(ceil)
			case OPconst:
				STACKLOAD(th, 0, globals, STACKINT(th, 0));
				break;
			case OPconstb:
				i = (*(pc++)) & 255;
				 STACKPUSHNIL_ERR(th, INTERPRETER_OM);
				STACKLOAD(th, 0, globals, i);
				break;
			case OPcos: BCFLOAT1FUN(cos)
			case OPcosh: BCFLOAT1FUN(cosh)
			case OPdftab:
				n = STACKPULLINT(th);
				STACKPUSHTABLE_ERR(th, n, DBG_TUPLE, INTERPRETER_OM);
				p = STACKPNT(th, 0);
				j = 1;
				for (i = n - 1; i >= 0; i--) STACKSTORE(p, i, th, j++);
				STACKSKIP(th, n);
				break;
			case OPdftabb:
				n = (*(pc++)) & 255;
				STACKPUSHTABLE_ERR(th, n, DBG_TUPLE, INTERPRETER_OM);
				p = STACKPNT(th, 0);
				j = 1;
				for (i = n - 1; i >= 0; i--) STACKSTORE(p, i, th, j++);
				STACKSKIP(th, n);
				break;
			case OPdfarray:
				n = STACKPULLINT(th);
				STACKPUSHTABLE_ERR(th, n, DBG_ARRAY, INTERPRETER_OM);
				p = STACKPNT(th, 0);
				j = 1;
				for (i = n - 1; i >= 0; i--) STACKSTORE(p, i, th, j++);
				STACKSKIP(th, n);
				break;
			case OPdfarrayb:
				n = (*(pc++)) & 255;
				STACKPUSHTABLE_ERR(th, n, DBG_ARRAY, INTERPRETER_OM);
				p = STACKPNT(th, 0);
				j = 1;
				for (i = n - 1; i >= 0; i--) STACKSTORE(p, i, th, j++);
				STACKSKIP(th, n);
				break;
			case OPdiv:
				b = STACKINT(th, 0);
				if (!b)
				{
					PRINTF(LOG_SYS, "BCdiv: division by zero in function %s\n", interpreterCurrentFun(th));
					STACKSETINT(th, 1, (0));
				}
				else STACKSETINT(th, 1, STACKINT(th, 1) / b);
				STACKDROP(th);
				break;
			case OPdivf: BCFLOAT2(/ )
			case OPdrop:
				STACKDROP(th);
				break;
			case OPdump:
				PRINTF(LOG_USER, "->");
				itemDump(th, LOG_USER, STACKGET(th, 0), STACKTYPE(th, 0));
				break;
			case OPdumpd:
				//PRINTF(LOG_USER, "->");
				itemDumpDirect(th, LOG_USER, STACKGET(th, 0), STACKTYPE(th, 0));
				break;
			case OPdup:
				 STACKPUSHNIL_ERR(th, INTERPRETER_OM);
				STACKINTERNALCOPY(th, 0, 1);
				break;
			case OPelse:
				if (STACKPULLPNT(th) != MM._true) pc += bytecodeGetJump(pc);
				else pc += BC_JUMP_SIZE;
				break;
			case OPeor: BCINT2(^)
			case OPeq:
				STACKSETBOOL(th, 1, lwEquals(STACKGET(th, 0), STACKTYPE(th, 0), STACKGET(th, 1), STACKTYPE(th, 1)));
				STACKDROP(th);
				break;
			case OPexec:// [fonction, argn, ..., arg1]
				n = STACKPULLINT(th);
				EXEC_COMMON
					break;
			case OPexecb:
				n = (*(pc++)) & 255;
				EXEC_COMMON
					break;
			case OPexp: BCFLOAT1FUN(exp)
			case OPfalse:
				STACKPUSHPNT_ERR(th, MM._false, INTERPRETER_OM);
				break;
			case OPfetch:
				isNil = STACKISNIL(th, 0);
				i = STACKPULLINT(th);
				p = STACKPNT(th, 0);
				if ((!p) || isNil) {
					STACKSETNIL(th, 0);
				}
				else
				{
					if ((i < 0) || (i >= TABLEN(p))) {
						STACKSETNIL(th, 0);
					}
					else STACKLOAD(th, 0, p, i);
				}
				break;
			case OPfetchb:
				i = (*(pc++)) & 255;
				p = STACKPNT(th, 0);
				if (p) STACKLOAD(th, 0, p, i);
				break;
			case OPfinal:
				STACKCOPYTOREF(th, th->callstack, -1, 0);
				th->pp = th->callstack + 1;
				break;
			case OPfirst:
				STACKPUSHNIL_ERR(th, INTERPRETER_OM);
				p = STACKPNT(th, 1);
				if (p&&(HEADER_TYPE(p) == TYPE_TAB)) STACKLOAD(th, 0, p, 0);
				break;
			case OPfloat:
				i = getLsbInt(pc);
				pc += LWLEN;
				STACKPUSHNIL_ERR(th, INTERPRETER_OM);
				STACKSETFLOAT(th, 0, *(LFLOAT*)(&i));
				break;
			case OPfloor: BCFLOAT1FUN(floor)
			case OPformat:
				n = STACKPULLINT(th);
				if (bufferFormat(MM.tmpBuffer, th, n)) return INTERPRETER_OM;
				if (n > 0) STACKSKIP(th, n);
				break;
			case OPformatb:
				n = (*(pc++)) & 255;
				if (bufferFormat(MM.tmpBuffer, th, n)) return INTERPRETER_OM;
				if (n > 0) STACKSKIP(th, n);
				break;
			case OPge: BCINT2BOOL(>= )
			case OPgef: BCFLOAT2BOOL(>= )
			case OPgoto:
				pc += bytecodeGetJump(pc);
				break;
			case OPgt: BCINT2BOOL(> )
			case OPgtf: BCFLOAT2BOOL(> )
			case OPhd:
				if (STACKPNT(th, 0)) STACKLOAD(th, 0, STACKPNT(th, 0), 0);
				break;
			case OPholdon:
				th->pc = pc - BC_START(bytecode);
				th->atomic = 0;
				return EXEC_WAIT;
			case OPint:
				i = getLsbInt(pc);
				pc += LWLEN;
				STACKPUSHINT_ERR(th,(i), INTERPRETER_OM);
				break;
			case OPintb:
				i = pc[0] & 255;
				pc++;
				STACKPUSHINT_ERR(th,(i), INTERPRETER_OM);
				break;
			case OPisnan:
				f = STACKFLOAT(th, 0);
				STACKSETBOOL(th, 0, isnan(f));
				break;
			case OPisinf:
				f = STACKFLOAT(th, 0);
				STACKSETBOOL(th, 0, isinf(f));
				break;
			case OPlambda:
				n = STACKPULLINT(th);
				STACKPUSHTABLE_ERR(th, n, DBG_LAMBDA, INTERPRETER_OM);
				p = STACKPNT(th, 0);
				j = 1;
				for (i = n - 1; i >= 0; i--) STACKSTORE(p, i, th, j++);
				STACKSKIP(th, n);
				break;
			case OPlambdab:
				n = (*(pc++)) & 255;
				STACKPUSHTABLE_ERR(th, n, DBG_LAMBDA, INTERPRETER_OM);
				p = STACKPNT(th, 0);
				j = 1;
				for (i = n - 1; i >= 0; i--) STACKSTORE(p, i, th, j++);
				STACKSKIP(th, n);
				break;
			case OPle:	BCINT2BOOL(<= )
			case OPlef: BCFLOAT2BOOL(<= )
			case OPln: BCFLOAT1FUN(log)
			case OPlog: BCFLOAT1FUN(log10)
			case OPlt:	BCINT2BOOL(< )
			case OPltf: BCFLOAT2BOOL(< )
			case OPmark:
				p = memoryAllocTable(th, MARK_LEN, DBG_TUPLE);
				if (!p) return INTERPRETER_OM;
				TABSETINT(p, MARK_TYPE, MARK_TYPE_BREAK);
				TABSETINT(p, MARK_PC, (pc + bytecodeGetJump(pc) - BC_START(bytecode)));
				TABSETINT(p, MARK_PP, (th->pp));
				TABSETPNT(p, MARK_NEXT, STACKREFPNT(th, th->callstack, CALLSTACK_MARK));
				STACKREFSETPNT(th, th->callstack, CALLSTACK_MARK, p);
				pc += BC_JUMP_SIZE;
				break;
			case OPmax: BCINT2FUN(maxint)
			case OPmaxf: BCFLOAT2FUN(maxf)
			case OPmin: BCINT2FUN(minint)
			case OPminf: BCFLOAT2FUN(minf)
			case OPmklist:
				STACKMAKETABLE_ERR(th, LIST_LENGTH, DBG_LIST,INTERPRETER_OM);
				break;
			case OPmktab:
				n = STACKPULLINT(th);
				STACKPUSHTABLE_ERR(th, n, DBG_TUPLE, INTERPRETER_OM);
				break;
			case OPmktabb:
				n = (*(pc++)) & 255;
				STACKPUSHTABLE_ERR(th, n, DBG_TUPLE, INTERPRETER_OM);
				break;
			case OPmod:
				b = STACKINT(th, 0);
				if (!b)
				{
					PRINTF(LOG_SYS, "BCmod: division by zero in function %s\n", interpreterCurrentFun(th));
					STACKSETINT(th, 1, (0));
				}
				else STACKSETINT(th, 1, STACKINT(th, 1) % b);
				STACKDROP(th);
				break;
			case OPmodf:BCFLOAT2FUN(fmod)
			case OPmul: BCINT2(*)
			case OPmulf: BCFLOAT2(*)
			case OPne:
				STACKSETBOOL(th, 1, !lwEquals(STACKGET(th, 0), STACKTYPE(th, 0), STACKGET(th, 1), STACKTYPE(th, 1)));
				STACKDROP(th);
				break;
			case OPneg: BCINT1(-)
			case OPnegf: BCFLOAT1(-)
			case OPnil:
				 STACKPUSHNIL_ERR(th, INTERPRETER_OM);
				break;
			case OPnon:
				STACKSETBOOL(th, 0, STACKPNT(th, 0) != MM._true);
				break;
			case OPnop:
				break;
			case OPnot: BCINT1(~)
			case OPor: BCINT2(| )
			case OPpick:
				wi = STACKGET(th,0);
				p = STACKPNT(th, 1);
				if ((!p) || (wi == NIL)) {
					STACKSETNIL(th, 0);
				}
				else
				{
					i = VALTOINT(wi);
					if ((i < 0) || (i >= TABLEN(p))) {
						STACKSETNIL(th, 0);
					}
					else STACKLOAD(th, 0, p, i);
				}
				break;
			case OPpickb:
				i = (*(pc++)) & 255;
				p = STACKPNT(th, 0);
				 STACKPUSHNIL_ERR(th, INTERPRETER_OM);
				if (p && (i >= 0) && (i < TABLEN(p))) STACKLOAD(th,0,p,i);
				break;
			case OPpow: BCFLOAT2FUN(pow)
			case OPpowint: BCINT2FUN(powerInt)
			case OPprompt:
				if (promptOnThread(th)) return INTERPRETER_OM;
				break;
			case OPret:
				if (STACKREF(th) - th->callstack != 1)
				{
					PRINTF(LOG_SYS, "RET pp=" LSD ", should be " LSD " in %s\n", STACKREF(th), th->callstack + 1, interpreterCurrentFun(th));
					threadDump(LOG_SYS, th, 6);
					return EXEC_EXIT;  // wrong implementation
				}
				th->fun = STACKREFPNT(th, th->callstack, CALLSTACK_FUN);
				th->pc = STACKREFINT(th, th->callstack, CALLSTACK_PC);
				th->callstack = STACKREFINT(th, th->callstack, CALLSTACK_PREV);
				STACKSKIP(th, BC_ARGS(bytecode) + BC_LOCALS(bytecode) + CALLSTACK_LENGTH + 1);
				if (th->callstack < 0) return INTERPRETER_END(EXEC_IDLE);  // successful end of bytecode execution

				BC_PRECOMPUTE
					break;
			case OPrglob:
				def = (Def*)TABPNT(globals, STACKINT(th, 0));
				STACKSETTYPE(th, 0, def->val, def->valType);
				break;
			case OPrglobb:
				n = (*(pc++)) & 255;
				def = (Def*)TABPNT(globals, n);
				 STACKPUSHNIL_ERR(th, INTERPRETER_OM);
				STACKSETTYPE(th, 0, def->val, def->valType);
				break;
			case OPrloc:
				STACKCOPYFROMREF(th, 0, locals, -STACKINT(th, 0));
				break;
			case OPrlocb:
				n = (*(pc++)) & 255;
				 STACKPUSHNIL_ERR(th, INTERPRETER_OM);
				STACKCOPYFROMREF(th, 0, locals, -n);
				break;
			case OPround: BCFLOAT1FUN(roundls)
			case OPsglobi:
				def = (Def*)TABPNT(globals, STACKINT(th, 1));
				defSet(def, STACKGET(th, 0),STACKTYPE(th, 0));
				STACKSKIP(th, 1);
				break;
			case OPshl: BCINT2(<< )
			case OPshr: BCINT2(>> )
			case OPsin: BCFLOAT1FUN(sin)
			case OPsinh: BCFLOAT1FUN(sinh)
			case OPskip:
				i = STACKINT(th, 0);
				STACKSKIP(th, i + 1);
				break;
			case OPskipb:
				i = (*(pc++)) & 255;
				STACKSKIP(th, i);
				break;
			case OPsloc: // [index val] -> 0
				STACKCOPYTOREF(th, locals, -STACKINT(th, 0), 1);
				STACKDROPN(th, 2);
				break;
			case OPslocb: // [val] -> 0
				n = (*(pc++)) & 255;
				STACKCOPYTOREF(th, locals, -n, 0);
				STACKDROP(th);
				break;
			case OPsloci: // [val index] -> [val]
				STACKCOPYTOREF(th, locals, -STACKINT(th, 1), 0);
				STACKSKIP(th, 1);
				break;
			case OPsqr: BCFLOAT1FUN(sqr)
			case OPsqrt: BCFLOAT1FUN(sqrt)
			case OPstore:	 // [val index table] -> [val]
				isNil = STACKISNIL(th, 1);
				p = STACKPNT(th, 2);
				if (p && !isNil)
				{
					i = STACKINT(th,1);
					if ((i >= 0) && (i < TABLEN(p))) STACKSTORE(p, i, th, 0);
				}
				STACKSKIP(th, 2);
				break;
			case OPstruct:
				def = (Def*)TABPNT(globals, STACKINT(th, 0));
				STACKDROP(th);
				STACKPUSHTABLE_ERR(th, def->index, PNTTOVAL((LB*)def), INTERPRETER_OM);
				break;
			case OPsub:BCINT2(-)
			case OPsubf: BCFLOAT2(-)
			case OPsum:
				def = (Def*)TABPNT(globals, STACKINT(th, 0));
				STACKDROP(th);
				STACKMAKETABLE_ERR(th, def->type->nb, PNTTOVAL((LB*)def), INTERPRETER_OM);
				break;
			case OPswap:
				 STACKPUSHNIL_ERR(th, INTERPRETER_OM);
				STACKINTERNALCOPY(th, 0, 1);
				STACKINTERNALCOPY(th, 1, 2);
				STACKINTERNALCOPY(th, 2, 0);
				STACKDROP(th);
				break;
			case OPtablen:
				p = STACKPNT(th, 0);
				if (p) STACKSETINT(th, 0, TABLEN(p));
				break;
			case OPtan: BCFLOAT1FUN(tan)
			case OPtanh: BCFLOAT1FUN(tanh)
			case OPtfc:// [fonction, argn, ..., arg1]
				n = STACKPULLINT(th);
				EXEC_COMMON_TFC
					if (th->callstack < 0) return INTERPRETER_END(EXEC_IDLE);  // successful end of bytecode execution
				break;
			case OPtfcb:
				n = (*(pc++)) & 255;
				EXEC_COMMON_TFC
					if (th->callstack < 0) return INTERPRETER_END(EXEC_IDLE);  // successful end of bytecode execution
				break;
			case OPthrow:
				if (interpreterBreakThrow(th, MARK_TYPE_TRY, STACKGET(th, 0), STACKTYPE(th, 0))) return INTERPRETER_OM;
				if (th->callstack < 0) return INTERPRETER_END(EXEC_EXIT);  // Exception uncaught
				BC_PRECOMPUTE
				break;
			case OPtl:
				if (STACKPNT(th, 0)) STACKLOAD(th, 0, STACKPNT(th, 0), 1);
				break;
			case OPtron:
				interpreterTRON = 1;
				 STACKPUSHNIL_ERR(th, INTERPRETER_OM);
				break;
			case OPtroff:
				interpreterTRON = 0;
				 STACKPUSHNIL_ERR(th, INTERPRETER_OM);
				break;
			case OPtrue:
				STACKPUSHPNT_ERR(th, MM._true, INTERPRETER_OM);
				break;
			case OPtry:
				p = memoryAllocTable(th, MARK_LEN, DBG_TUPLE);
				if (!p) return INTERPRETER_OM;
				TABSETINT(p, MARK_TYPE, MARK_TYPE_TRY);
				TABSETINT(p, MARK_PC, (pc + bytecodeGetJump(pc) - BC_START(bytecode)));
				TABSETINT(p, MARK_PP, (th->pp));
				TABSETPNT(p, MARK_NEXT, STACKREFPNT(th, th->callstack, CALLSTACK_MARK));
				STACKREFSETPNT(th, th->callstack, CALLSTACK_MARK, p);
				pc += BC_JUMP_SIZE;
				break;
			case OPunmark:
				p = STACKREFPNT(th, th->callstack, CALLSTACK_MARK);
				STACKREFSETPNT(th, th->callstack, CALLSTACK_MARK, TABPNT(p, MARK_NEXT));
				break;
			case OPupdt: // [index val table] -> [table]
				i = STACKPULLINT(th);
				if (STACKPNT(th,1)) STACKSTORE(STACKPNT(th, 1), i, th, 0);
				STACKDROP(th);
				break;
			case OPupdtb: // [val table] -> [table]
				i = (*(pc++)) & 255;
				p = STACKPNT(th, 1);
				if (p) STACKSTORE(p, i, th, 0);
				STACKDROP(th);
				break;
			default:
				PRINTF(LOG_SYS, "\nInterpreter: RUNTIME ERROR = Illegal Opcode %d in %s\n", 255 & op, interpreterCurrentFun(th));
				//					memoryAbort(m,1);
				return EXEC_EXIT;
			}
		}
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

