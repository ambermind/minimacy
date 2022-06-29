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
	{ if (result>0) return interpreterEnd(th, nloop - count, result);\
	if (th->fun) {BC_PRECOMPUTE} } \
	else if (th->forceOpcode!=THREAD_OPCODE_NONE) \
	{ \
		op=th->forceOpcode; \
		th->forceOpcode=THREAD_OPCODE_NONE; \
		goto processOpCode; \
	}

#define EXEC_COMMON_TFC th->pc=pc-BC_START(bytecode); \
	if ((result=interpreterExec(th,n,locals))) \
	{ if (result>0) return interpreterEnd(th, nloop - count, result);\
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
LFLOAT roundls(LFLOAT a) { return ((a-floor(a))>0.5)?ceil(a):floor(a); }
LFLOAT minf(LFLOAT a,LFLOAT b) { return a>b?b:a; }
LFLOAT maxf(LFLOAT a,LFLOAT b) { return a<b?b:a; }

LINT minint(LINT a,LINT b) { return a>b?b:a; }
LINT maxint(LINT a,LINT b) { return a<b?b:a; }

int isHigherFun(LB* fun)
{
	LB* p;
	if (!fun) return 0;
	p=VALTOPNT(TABGET(fun,FUN_USER_NAME));
	if ((p)&&(HEADER_TYPE(p)==TYPE_TAB)) return 1;
	return 0;
}
LB* getFunStruct(LB* fun)
{
	if (isHigherFun(fun)) return VALTOPNT(TABGET(fun,FUN_USER_NAME));
	return fun;
}

char* interpreterCurrentFun(Thread* th)
{
	LB* p=VALTOPNT(TABGET(getFunStruct(th->fun),FUN_USER_NAME));
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
	LB* fun=VALTOPNT(STACKGET(th,argc));
	th->forceOpcode=THREAD_OPCODE_NONE;
	if (!fun)
	{
		STACKDROPN(th,argc);
		return 0;
	}
//	if (tfc && !((HEADER_SIZE(fun) == FUN_NATIVE_LEN)&& ISVALINT(TABGET(fun, FUN_NATIVE_POINTER))))	// no TFC for OPcode call
	if (tfc)
	{
		LINT i;
//		threadDump(LOG_ERR,th,10); printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>before tfc\n"); getchar();

		th->fun=VALTOPNT(STACKGETFROMREF(th,th->callstack,CALLSTACK_FUN));
		th->pc=VALTOINT(STACKGETFROMREF(th,th->callstack,CALLSTACK_PC));
		th->callstack=VALTOINT(STACKGETFROMREF(th,th->callstack,CALLSTACK_PREV));
		for(i=-1;i<argc;i++) STACKSETFROMREFSAFE(th,tfc,-i,STACKGET(th,argc-1-i));
		STACKDROPN(th,th->pp-(tfc+argc)+1);
//		threadDump(LOG_ERR,th,10); printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>after tfc\n"); getchar();
	}

	if (isHigherFun(fun))
	{
		LINT i;
		LINT binds=TABLEN(fun)-1;

//		threadDump(LOG_ERR,th,10); printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>start higherfun\n"); getchar();

		for(i=0;i<binds;i++) STACKPUSH_OM(th,NIL,EXEC_OM);
		for(i=0;i<argc;i++) STACKSET(th,i,STACKGET(th,i+binds));
		for(i=0;i<binds;i++) STACKSET(th,argc+i,TABGET(fun,binds-i));
		STACKSET(th,argc+binds,TABGET(fun,FUN_USER_NAME));
		fun=VALTOPNT(TABGET(fun,FUN_USER_NAME));
//		threadDump(LOG_ERR,th,10); printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>done higherfun\n"); getchar();
//		interpreterTRON=1;

	}
	else if (TABLEN(fun)==FUN_NATIVE_LEN)
	{
		NATIVE native;
		LINT fastAlloc= memoryGetFast(th);
		LINT ref=STACKREF(th);
		LW val=TABGET(fun,FUN_NATIVE_POINTER);

		if (ISVALINT(val))
		{
			LINT i;
			for(i=argc-1;i>=0;i--) STACKSET(th,i+1,STACKGET(th,i));
			STACKDROP(th);
			th->forceOpcode=(char)VALTOINT(val);
//			return tfc?-1:0;
			return 0;
		}
//		printf("call %s\n",STRSTART(VALTOPNT(TABGET(fun,FUN_NATIVE_NAME))));
		native=(NATIVE)TABGET(VALTOPNT(val),0);
		k=(*native)(th);
		while(memoryGetFast(th)>fastAlloc) memoryLeaveFast(th);	// this simplifies a lot how to write native functions which need "fastAlloc"
//		if (k) {
//			printf("native %s returns %d\n", STRSTART(VALTOPNT(TABGET(fun, FUN_NATIVE_NAME))), k);
//			threadDump(LOG_ERR, th, 3);
//		}
		if (k) return k;
		if (STACKREF(th)!=ref-argc+1)
		{
			PRINTF(th,LOG_ERR,"NATIVE pp=" LSD ", should be " LSD " in %s\n",STACKREF(th),ref-argc+1,STRSTART(VALTOPNT(TABGET(fun,FUN_NATIVE_NAME))));
//			threadDump(LOG_ERR,th,5);
			return -1;  // wrong implementation
		}
		STACKSKIP(th,1);	// remove fun from stack
		return tfc ? -1: 0;
	}

	bytecode=VALTOPNT(TABGET(fun,FUN_USER_BC));

	nlocals=BC_LOCALS(bytecode);
	for(i=0;i<nlocals;i++) STACKPUSH_OM(th,NIL,EXEC_OM);
	STACKPUSH_OM(th,NIL,EXEC_OM);
	STACKPUSH_OM(th,PNTTOVAL(th->fun),EXEC_OM);
	STACKPUSH_OM(th,INTTOVAL(th->pc),EXEC_OM);
	STACKPUSH_OM(th,INTTOVAL(th->callstack),EXEC_OM);
	th->callstack=STACKREF(th);
	th->pc=0;
	th->fun=fun;
	return -1;
}

int interpreterBreakThrow(Thread* th,LW type, LW data)
{
	while(th->callstack>=0)
	{
		LB* last=NULL;
		LB* mark=VALTOPNT(STACKGETFROMREF(th,th->callstack,CALLSTACK_MARK));

		while(mark)
		{
			if (TABGET(mark,MARK_TYPE)==type)
			{
				if (last) TABSET(last,MARK_NEXT,TABGET(mark,MARK_NEXT))
				else STACKSETFROMREF(th,th->callstack,CALLSTACK_MARK,TABGET(mark,MARK_NEXT));	// pop the mark
				th->pc=VALTOINT(TABGET(mark,MARK_PC));
				th->pp=VALTOINT(TABGET(mark,MARK_PP));
				STACKPUSH_OM(th,data,EXEC_OM);
				return 0;
			}
			last=mark;
			mark=VALTOPNT(TABGET(mark,MARK_NEXT));
		}

		th->fun=VALTOPNT(STACKGETFROMREF(th,th->callstack,CALLSTACK_FUN));
		th->pc=VALTOINT(STACKGETFROMREF(th,th->callstack,CALLSTACK_PC));
		th->callstack=VALTOINT(STACKGETFROMREF(th,th->callstack,CALLSTACK_PREV));
	}
	PRINTF(th,LOG_USER,"Uncaught exception!\n");
	itemDump(th, LOG_USER,data);
	return 0;
}
LINT interpreterEnd(Thread* th,LINT count, LINT result)
{
	th->count += count;
//	stackReset(th);
	if (result == EXEC_IDLE) th->pp--;
//	if (result == EXEC_OM) printf("EXEC_OM "LSX":" LSD" / "LSX" :" LSD"\n", th,th->uid, MM.thread, MM.thread->uid);
	if (result == EXEC_OM)
	{
		if (th->header.mem == th->memDelegate) th->OM = 1;
		else
		{
			result = interpreterBreakThrow(th, MARK_TYPE_TRY, MM.MemoryException); if (result) return result;
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
LINT interpreterRun(Thread* th,LINT maxCycles)
{
	LB* globals;
	LB* bytecode;
	LINT locals,result;
	char* pc;
	char op;
	LINT cyclesToGo = maxCycles;

	if (th->callstack<0) return EXEC_IDLE;	// nothing to execute

	BC_PRECOMPUTE

	coreBignumReset();
	while(1)
	{
		LINT count,i,j,b,n;
		LFLOAT f,r;
		LB* p;
		LW w,wi;
		Ref* ref;
		const LINT nloop = interpreterCount0(th, maxCycles, cyclesToGo);

		count = nloop; while ((count--) > 0)
			//		count = 0; while((count++)<nloop)
		{
			op = *(pc++);
		processOpCode:
			/*			if (th->count==0x6ba-3)
						{
			//				threadDump(LOG_ERR,th,100);
			//				getchar();
							interpreterTRON=1;
						}
			*/
			//			if (op==OPtry) interpreterTRON=1;
			/*			if (runtimeCheckAddress && (runtimeCheckValue!=HEADER_SIZE(runtimeCheckAddress)))
						{
							printf("--------VALUE HAS CHANGED\n");
							printf( LSX ": " LSX "-> " LSX "\n",runtimeCheckAddress, runtimeCheckValue, HEADER_SIZE(runtimeCheckAddress));
							printf("stack=" LSX "\n" , th->stack);
							printf("count=" LSX "\n" , th->count);

							interpreterTRON=1;
							getchar();
						}
			*/
			//			if (op==OPtfcb)
/*					if (interpreterTRON)
						{
							LINT pci=(LINT)(pc-1-BC_START(bytecode));
							threadDump(LOG_USER,th,10);
							PRINTF(th,LOG_USER,"#" LSD " (pp=" LSD " locals=" LSD " callstack=" LSD ") %s -> ",th->uid,th->pp, STACKREF(th)-locals,STACKREF(th)-th->callstack,interpreterCurrentFun(th));
							PRINTF(th,LOG_USER,"count=" LSD "/" LSD " pc=" LSD " op=%2d:",th->count, maxCycles,pci,op);
							opcodePrint(th, LOG_USER,op,pc,pci);
			//				if (1) printf("");
							getchar();
						}
*/			//			printf("$");
//			{LINT pci = (LINT)(pc - 1 - BC_START(bytecode));
//			opcodePrint(th, LOG_USER, op, pc, pci); }
			switch (op) {
			case OPadd: BCINT2(+)
			case OPatomic:
				th->atomic = (*(pc++)) & 255;
				break;
			case OPbreak:
				if (interpreterBreakThrow(th, MARK_TYPE_BREAK, STACKGET(th, 0))) return interpreterEnd(th, nloop - count, EXEC_OM);
				BC_PRECOMPUTE
				break;
			case OPconst:
				STACKSET(th, 0, TABGET(globals, VALTOINT(STACKGET(th, 0))));
				break;
			case OPconstb:
				i = (*(pc++)) & 255;
				STACKPUSH_OM(th, TABGET(globals, i), interpreterEnd(th, nloop - count, EXEC_OM));
				break;
			case OPdrop:
				STACKDROP(th);
				break;
			case OPdup:
				STACKPUSH_OM(th, STACKGET(th, 0), interpreterEnd(th, nloop - count, EXEC_OM));
				break;
			case OPelse:
				if (STACKPULL(th) != MM.trueRef) pc += bytecodeGetJump(pc);
				else pc += BC_JUMP_SIZE;
				break;
			case OPeq:
				STACKSET(th, 1, lwEquals(STACKGET(th, 0), STACKGET(th, 1))?MM.trueRef:MM.falseRef);
				STACKDROP(th);
				break;
			case OPfalse:
				STACKPUSH_OM(th, MM.falseRef, interpreterEnd(th, nloop - count, EXEC_OM));
				break;
			case OPpick:
				wi = STACKGET(th,0);
				p = VALTOPNT(STACKGET(th, 1));
				if ((!p) || (wi == NIL)) STACKSETNIL(th, 0);
				else
				{
					i = VALTOINT(wi);
					if ((i < 0) || (i >= TABLEN(p))) STACKSETNIL(th, 0);
					else STACKSET(th, 0, TABGET(p, i));
				}
				break;
			case OPpickb:
				i = (*(pc++)) & 255;
				p = VALTOPNT(STACKGET(th, 0));
				if ((!p)|| (i < 0) || (i >= TABLEN(p))) {
					STACKPUSH_OM(th, NIL, interpreterEnd(th, nloop - count, EXEC_OM));
				}
				else
				{
					STACKPUSH_OM(th, TABGET(p, i), interpreterEnd(th, nloop - count, EXEC_OM));
				}
				break;
			case OPfetch:
				wi = STACKPULL(th);
				p = VALTOPNT(STACKGET(th, 0));
				if ((!p) || (wi == NIL)) STACKSETNIL(th, 0);
				else
				{
					i = VALTOINT(wi);
					if ((i < 0) || (i >= TABLEN(p))) STACKSETNIL(th, 0);
					else STACKSET(th, 0, TABGET(p, i));
				}
				break;
			case OPfetchb:
				i = (*(pc++)) & 255;
				if (STACKGET(th, 0) != NIL) STACKSET(th, 0, TABGET(VALTOPNT(STACKGET(th, 0)), i));
				break;
			case OPfinal:
				STACKSETFROMREFSAFE(th, th->callstack, -1, STACKGET(th, 0));
				th->pp = th->callstack + 1;
				break;
			case OPfirst:
				w = STACKGET(th, 0);
				STACKPUSH_OM(th, (w == NIL) ? NIL : TABGET(VALTOPNT(w), 0), interpreterEnd(th, nloop - count, EXEC_OM));
				break;
			case OPgoto:
				pc += bytecodeGetJump(pc);
				break;
			case OPge:	BCINT2BOOL(>= )
			case OPgt: BCINT2BOOL(> )
			case OPhd:
				w = STACKGET(th, 0);
				STACKSET(th, 0, (w == NIL) ? NIL : TABGET(VALTOPNT(w), 0));
				break;
			case OPint:
				i = getLsbInt(pc);
				pc += LWLEN;
				STACKPUSH_OM(th, INTTOVAL(i), interpreterEnd(th, nloop - count, EXEC_OM));
				break;
			case OPintb:
				i = pc[0] & 255;
				pc++;
				STACKPUSH_OM(th, INTTOVAL(i), interpreterEnd(th, nloop - count, EXEC_OM));
				break;
			case OPle:	BCINT2BOOL(<= )
			case OPlt:	BCINT2BOOL(< )
			case OPmark:
				p = memoryAllocTable(th, MARK_LEN, DBG_TUPLE);
				if (!p) return interpreterEnd(th, nloop - count, EXEC_OM);
				TABSET(p, MARK_TYPE, MARK_TYPE_BREAK);
				TABSETINT(p, MARK_PC, (pc + bytecodeGetJump(pc) - BC_START(bytecode)));
				TABSETINT(p, MARK_PP, (th->pp));
				TABSET(p, MARK_NEXT, STACKGETFROMREF(th, th->callstack, CALLSTACK_MARK));
				STACKSETFROMREF(th, th->callstack, CALLSTACK_MARK, PNTTOVAL(p));
				pc += BC_JUMP_SIZE;
				break;
			case OPmklist:
				if (DEFTAB(th, LIST_LENGTH, DBG_LIST)) return interpreterEnd(th, nloop - count, EXEC_OM);
				break;
			case OPmul: BCINT2(*)
			case OPne:
				STACKSET(th, 1, lwEquals(STACKGET(th, 0), STACKGET(th, 1)) ? MM.falseRef:MM.trueRef);
				STACKDROP(th);
				break;
			case OPneg: BCINT1(-)
			case OPnil:
				STACKPUSH_OM(th, NIL, interpreterEnd(th, nloop - count, EXEC_OM));
				break;
			case OPnon:
				STACKSET(th, 0, ((STACKGET(th, 0) == MM.trueRef) ? MM.falseRef : MM.trueRef));
				break;
			case OPnop:
				break;
			case OPnot: BCINT1(~)
			case OPor: BCINT2(| )
			case OPrglob:
				ref = VALTOREF(TABGET(globals, VALTOINT(STACKGET(th, 0))));
				STACKSET(th, 0, ref->val);
				break;
			case OPrglobb:
				n = (*(pc++)) & 255;
				ref = VALTOREF(TABGET(globals, n));
				STACKPUSH_OM(th, ref->val, interpreterEnd(th, nloop - count, EXEC_OM));
				break;
			case OPrloc:
				STACKSET(th, 0, STACKGETFROMREF(th, locals, -VALTOINT(STACKGET(th, 0))));
				break;
			case OPrlocb:
				n = (*(pc++)) & 255;
				STACKPUSH_OM(th, STACKGETFROMREF(th, locals, -n), interpreterEnd(th, nloop - count, EXEC_OM));
				break;
			case OPsglobi:
				ref = VALTOREF(TABGET(globals, VALTOINT(STACKGET(th, 1))));
				refSet(ref, STACKGET(th, 0));
				STACKSKIP(th, 1);
				break;
			case OPshl: BCINT2(<< )
			case OPshr: BCINT2(>> )
			case OPskip:
				i = VALTOINT(STACKGET(th, 0));
				STACKSKIP(th, i + 1);
				break;
			case OPskipb:
				i = (*(pc++)) & 255;
				STACKSKIP(th, i);
				break;
			case OPsloc: // [index val] -> 0
				STACKSETFROMREFSAFE(th, locals, -VALTOINT(STACKGET(th, 0)), STACKGET(th, 1));
				STACKDROPN(th, 2);
				break;
			case OPslocb: // [val] -> 0
				n = (*(pc++)) & 255;
				STACKSETFROMREFSAFE(th, locals, -n, STACKGET(th, 0));
				STACKDROP(th);
				break;
			case OPsloci: // [val index] -> [val]
				STACKSETFROMREFSAFE(th, locals, -VALTOINT(STACKGET(th, 1)), STACKGET(th, 0));
				STACKSKIP(th, 1);
				break;
			case OPstore:	 // [val index table] -> [val]
				wi = STACKGET(th, 1);
				p = VALTOPNT(STACKGET(th, 2));
				if ((p) && (wi != NIL))
				{
					i = VALTOINT(wi);
					if ((i >= 0) && (i < TABLEN(p))) TABSET(p, i, STACKGET(th, 0));
				}
				STACKSKIP(th, 2);
				break;
			case OPstruct:
				ref = VALTOREF(TABGET(globals, VALTOINT(STACKGET(th, 0))));
				STACKDROP(th);
				if (TABLEPUSH(th, ref->index, PNTTOVAL(ref))) return interpreterEnd(th, nloop - count, EXEC_OM);
				break;
			case OPsum:
				ref = VALTOREF(TABGET(globals, VALTOINT(STACKGET(th, 0))));
				STACKDROP(th);
				if (DEFTAB(th, ref->type->nb, PNTTOVAL(ref))) return interpreterEnd(th, nloop - count, EXEC_OM);
				break;
			case OPsub:BCINT2(-)
			case OPswap:
				w = STACKGET(th, 0);
				STACKSETSAFE(th, 0, STACKGET(th, 1));
				STACKSETSAFE(th, 1, w);
				break;
			case OPtfc:// [fonction, argn, ..., arg1]
				n = VALTOINT(STACKPULL(th));
				EXEC_COMMON_TFC
					if (th->callstack < 0) return interpreterEnd(th, nloop - count, EXEC_IDLE);  // successful end of bytecode execution
				break;
			case OPtfcb:
				n = (*(pc++)) & 255;
				EXEC_COMMON_TFC
					if (th->callstack < 0) return interpreterEnd(th, nloop - count, EXEC_IDLE);  // successful end of bytecode execution
				break;
			case OPthrow:
				if (interpreterBreakThrow(th, MARK_TYPE_TRY, STACKGET(th, 0))) return interpreterEnd(th, nloop - count, EXEC_OM);
				if (th->callstack < 0) return interpreterEnd(th, nloop - count, EXEC_EXIT);  // Exception uncaught
				BC_PRECOMPUTE
				break;
			case OPtl:
				w = STACKGET(th, 0);
				STACKSET(th, 0, (w == NIL) ? NIL : TABGET(VALTOPNT(w), 1));
				break;
			case OPtrue:
				STACKPUSH_OM(th, MM.trueRef, interpreterEnd(th, nloop - count, EXEC_OM));
				break;
			case OPtry:
				p = memoryAllocTable(th, MARK_LEN, DBG_TUPLE);
				if (!p) return interpreterEnd(th, nloop - count, EXEC_OM);
				TABSET(p, MARK_TYPE, MARK_TYPE_TRY);
				TABSETINT(p, MARK_PC, (pc + bytecodeGetJump(pc) - BC_START(bytecode)));
				TABSETINT(p, MARK_PP, (th->pp));
				TABSET(p, MARK_NEXT, STACKGETFROMREF(th, th->callstack, CALLSTACK_MARK));
				STACKSETFROMREF(th, th->callstack, CALLSTACK_MARK, PNTTOVAL(p));
				pc += BC_JUMP_SIZE;
				break;
			case OPunmark:
				p = VALTOPNT(STACKGETFROMREF(th, th->callstack, CALLSTACK_MARK));
				STACKSETFROMREF(th, th->callstack, CALLSTACK_MARK, TABGET(p, MARK_NEXT));
				break;
			case OPupdt: // [index val table] -> [table]
				i = VALTOINT(STACKPULL(th));
				w = STACKGET(th, 1);
				if (w != NIL) TABSET(VALTOPNT(w), i, STACKGET(th, 0));
				STACKDROP(th);
				break;
			case OPupdtb: // [val table] -> [table]
				i = (*(pc++)) & 255;
				w = STACKGET(th, 1);
				if (w != NIL) TABSET(VALTOPNT(w), i, STACKGET(th, 0));
				STACKDROP(th);
				break;
			case OPabs: BCINT1FUN(absint)
			case OPabsf: BCFLOAT1FUN(fabs)
			case OPacos: BCFLOAT1FUN(acos)
			case OPaddf: BCFLOAT2(+)
			case OPasin: BCFLOAT1FUN(asin)
			case OPand: BCINT2(&)
			case OPatan: BCFLOAT1FUN(atan)
			case OPatan2: BCFLOAT2FUN(atan2)
			case OPcast:
				if (STACKGET(th,0)!=STACKGET(th,1)) STACKSETNIL(th, 2);
				STACKDROPN(th,2);
				break;
			case OPcastb:
				i = (*(pc++)) & 255;
				if (STACKGET(th,0)!=INTTOVAL(i)) STACKSETNIL(th, 1);
				STACKDROP(th);
				break;
			case OPceil: BCFLOAT1FUN(ceil)
			case OPcos: BCFLOAT1FUN(cos)
			case OPcosh: BCFLOAT1FUN(cosh)
			case OPdftab:
				n = VALTOINT(STACKPULL(th));
				if (TABLEPUSH(th, n, DBG_TUPLE)) return interpreterEnd(th, nloop - count, EXEC_OM);
				p = VALTOPNT(STACKGET(th, 0));
				j = 1;
				for (i = n - 1; i >= 0; i--) TABSET(p, i, STACKGET(th, j++));
				STACKSKIP(th, n);
				break;
			case OPdftabb:
				n = (*(pc++)) & 255;
				if (TABLEPUSH(th, n, DBG_TUPLE)) return interpreterEnd(th, nloop - count, EXEC_OM);
				p = VALTOPNT(STACKGET(th, 0));
				j = 1;
				for (i = n - 1; i >= 0; i--) TABSET(p, i, STACKGET(th, j++));
				STACKSKIP(th, n);
				break;
			case OPdfarray:
				n = VALTOINT(STACKPULL(th));
				if (TABLEPUSH(th, n, DBG_ARRAY)) return interpreterEnd(th, nloop - count, EXEC_OM);
				p = VALTOPNT(STACKGET(th, 0));
				j = 1;
				for (i = n - 1; i >= 0; i--) TABSET(p, i, STACKGET(th, j++));
				STACKSKIP(th, n);
				break;
			case OPdfarrayb:
				n = (*(pc++)) & 255;
				if (TABLEPUSH(th, n, DBG_ARRAY)) return interpreterEnd(th, nloop - count, EXEC_OM);
				p = VALTOPNT(STACKGET(th, 0));
				j = 1;
				for (i = n - 1; i >= 0; i--) TABSET(p, i, STACKGET(th, j++));
				STACKSKIP(th, n);
				break;
			case OPlambda:
				n = VALTOINT(STACKPULL(th));
				if (TABLEPUSH(th, n, DBG_LAMBDA)) return interpreterEnd(th, nloop - count, EXEC_OM);
				p = VALTOPNT(STACKGET(th, 0));
				j = 1;
				for (i = n - 1; i >= 0; i--) TABSET(p, i, STACKGET(th, j++));
				STACKSKIP(th, n);
				break;
			case OPlambdab:
				n = (*(pc++)) & 255;
				if (TABLEPUSH(th, n, DBG_LAMBDA)) return interpreterEnd(th, nloop - count, EXEC_OM);
				p = VALTOPNT(STACKGET(th, 0));
				j = 1;
				for (i = n - 1; i >= 0; i--) TABSET(p, i, STACKGET(th, j++));
				STACKSKIP(th, n);
				break;
			case OPdiv:
				b = VALTOINT(STACKGET(th, 0));
				if (!b)
				{
					PRINTF(th,LOG_ERR, "BCdiv: division by zero in function %s\n", interpreterCurrentFun(th));
					STACKSETINT(th, 1, (0));
				}
				else STACKSETINT(th, 1, (VALTOINT(STACKGET(th, 1)) / b));
				STACKDROP(th);
				break;
			case OPdivf:
				f = VALTOFLOAT(STACKGET(th, 0));
				if (!f)
				{
					PRINTF(th,LOG_ERR, "BCdivf: division by zero in function %s\n", interpreterCurrentFun(th));
					r = 0;
				}
				else r = VALTOFLOAT(STACKGET(th, 1)) / f;
				STACKSETFLOAT(th, 1, (r));
				STACKDROP(th);
				break;
			case OPdump:
				PRINTF(th,LOG_USER, "->");
				itemDump(th, LOG_USER, STACKGET(th, 0));
				break;
			case OPdumpd:
				//PRINTF(th,LOG_USER, "->");
				itemDumpDirect(th, LOG_USER, STACKGET(th, 0));
				break;
			case OPeor: BCINT2(^)
			case OPexec:// [fonction, argn, ..., arg1]
				n = VALTOINT(STACKPULL(th));
				EXEC_COMMON
				break;
			case OPexecb:
				n = (*(pc++)) & 255;
				EXEC_COMMON
				break;

			case OPexp: BCFLOAT1FUN(exp)
			case OPfloat:
				i = getLsbInt(pc);
				pc += LWLEN;
				STACKPUSH_OM(th, FLOATTOVAL(*(LFLOAT*)(&i)), interpreterEnd(th, nloop - count, EXEC_OM));
				break;
			case OPfloor: BCFLOAT1FUN(floor)
			case OPgef: BCFLOAT2BOOL(>= )
			case OPgtf: BCFLOAT2BOOL(> )
			case OPlef: BCFLOAT2BOOL(<= )
			case OPltf: BCFLOAT2BOOL(< )
			case OPholdon:
				th->pc = pc - BC_START(bytecode);
				th->atomic = 0;
				return EXEC_WAIT;
			case OPln: BCFLOAT1FUN(log)
			case OPlog: BCFLOAT1FUN(log10)
			case OPmax: BCINT2FUN(maxint)
			case OPmaxf: BCFLOAT2FUN(maxf)
			case OPmin: BCINT2FUN(minint)
			case OPminf: BCFLOAT2FUN(minf)
			case OPmktab:
				n = VALTOINT(STACKPULL(th));
				if (TABLEPUSH(th, n, DBG_TUPLE)) return interpreterEnd(th, nloop - count, EXEC_OM);
				break;
			case OPmktabb:
				n = (*(pc++)) & 255;
				if (TABLEPUSH(th, n, DBG_TUPLE)) return interpreterEnd(th, nloop - count, EXEC_OM);
				break;
			case OPmod:
				b = VALTOINT(STACKGET(th, 0));
				if (!b)
				{
					PRINTF(th,LOG_ERR, "BCmod: division by zero in function %s\n", interpreterCurrentFun(th));
					STACKSETINT(th, 1, (0));
				}
				else STACKSETINT(th, 1, (VALTOINT(STACKGET(th, 1)) % b));
				STACKDROP(th);
				break;
			case OPmodf:
				f = VALTOFLOAT(STACKGET(th, 0));
				if (!f)
				{
					PRINTF(th,LOG_ERR, "BCmodf: modulus zero in function %s\n", interpreterCurrentFun(th));
					r = 0;
				}
				else r = fmod(VALTOFLOAT(STACKGET(th, 1)), f);
				STACKSETFLOAT(th, 1, (r));
				STACKDROP(th);
				break;
			case OPmulf: BCFLOAT2(*)
			case OPnegf: BCFLOAT1(-); break;
			case OPpowint: BCINT2FUN(powerInt)
			case OPpow: BCFLOAT2FUN(pow)
			case OPprompt:
				if (promptOnThread(th)) return interpreterEnd(th, nloop - count, EXEC_OM);
				break;
			case OPret:
				if (STACKREF(th) - th->callstack != 1)
				{
					PRINTF(th,LOG_ERR, "RET pp=" LSD ", should be " LSD " in %s\n", STACKREF(th), th->callstack + 1, interpreterCurrentFun(th));
					threadDump(LOG_ERR, th, 6);
					return EXEC_EXIT;  // wrong implementation
				}
				th->fun = VALTOPNT(STACKGETFROMREF(th, th->callstack, CALLSTACK_FUN));
				th->pc = VALTOINT(STACKGETFROMREF(th, th->callstack, CALLSTACK_PC));
				th->callstack = VALTOINT(STACKGETFROMREF(th, th->callstack, CALLSTACK_PREV));
				STACKSKIP(th, BC_ARGS(bytecode) + BC_LOCALS(bytecode) + CALLSTACK_LENGTH + 1);
				if (th->callstack < 0) return interpreterEnd(th, nloop - count, EXEC_IDLE);  // successful end of bytecode execution

				BC_PRECOMPUTE
				break;
			case OPround: BCFLOAT1FUN(roundls)
			case OPsin: BCFLOAT1FUN(sin)
			case OPsinh: BCFLOAT1FUN(sinh)
			case OPsqr: BCFLOAT1FUN(sqr)
			case OPsqrt: BCFLOAT1FUN(sqrt)
			case OPsubf: BCFLOAT2(-)
			case OPtan: BCFLOAT1FUN(tan)
			case OPtanh: BCFLOAT1FUN(tanh)
			case OPtron:
				interpreterTRON = 1;
				STACKPUSH_OM(th, NIL, interpreterEnd(th, nloop - count, EXEC_OM));
				break;
			case OPtroff:
				interpreterTRON = 0;
				STACKPUSH_OM(th, NIL, interpreterEnd(th, nloop - count, EXEC_OM));
				break;
			case OPformat:
				n = VALTOINT(STACKPULL(th));
				if (bufferFormat(MM.tmpBuffer,th,n)) return interpreterEnd(th, nloop - count, EXEC_OM);
				break;
			case OPformatb:
				n = (*(pc++)) & 255;
				if (bufferFormat(MM.tmpBuffer, th, n)) return interpreterEnd(th, nloop - count, EXEC_OM);
				break;
			case OPtablen:
				p = VALTOPNT(STACKGET(th, 0));
				if (p) STACKSETINT(th, 0, TABLEN(p));
				break;
			default:
				PRINTF(th,LOG_ERR, "\nInterpreter: RUNTIME ERROR = Illegal Opcode %d in %s\n", 255 & op, interpreterCurrentFun(th));
				//					memoryAbort(m,1);
				return EXEC_EXIT;
			}
		}

		th->count+=nloop;
		if ((nloop >= cyclesToGo) && (maxCycles) && !th->atomic)
		{
			th->pc = pc - BC_START(bytecode);
			return EXEC_PREEMPTION;
		}

		memoryGC(MM.gc_period);
	}
}

