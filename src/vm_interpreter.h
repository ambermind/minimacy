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
#ifndef _INTERPRETER_
#define _INTERPRETER_

extern int interpreterTRON;

#define CALLSTACK_LENGTH 4 // taille de la callstack sans les variables locales
#define CALLSTACK_MARK 3	// list of marks (exception, break)
#define CALLSTACK_FUN 2	// function index
#define CALLSTACK_PC 1	// program counter
#define CALLSTACK_PREV 0	// previous callstack

#define MARK_LEN 4
#define MARK_TYPE 0
#define MARK_PC 1
#define MARK_PP 2
#define MARK_NEXT 3

#define MARK_TYPE_TRY INTTOVAL(0)
#define MARK_TYPE_BREAK INTTOVAL(1)

#define BC_PRECOMPUTE \
	globals=VALTOPNT(TABGET(th->fun,FUN_USER_GLOBALS)); \
	bytecode=VALTOPNT(TABGET(th->fun,FUN_USER_BC)); \
	locals=th->callstack-(CALLSTACK_LENGTH-1+BC_ARGS(bytecode)+BC_LOCALS(bytecode)); \
	pc=BC_START(bytecode)+th->pc; 


#define BCINT1(op) STACKSETINT(th,0,(op VALTOINT(STACKGET(th,0)))); break;

#define BCINT1FUN(fun) STACKSETINT(th,0,(fun( VALTOINT(STACKGET(th,0))))); break;

#define BCINT2(op)																	\
	STACKSETINT(th,1,(VALTOINT(STACKGET(th,1)) op VALTOINT(STACKGET(th,0))));	\
	STACKDROP(th); break;														

#define BCINT2BOOL(op)																	\
	STACKSETSAFE(th,1,(VALTOINT(STACKGET(th,1)) op VALTOINT(STACKGET(th,0)))?MM.trueRef:MM.falseRef);	\
	STACKDROP(th); break;														

#define BCINT2FUN(fun)																	\
	STACKSETINT(th,1,(fun(VALTOINT(STACKGET(th,1)), VALTOINT(STACKGET(th,0)))));	\
	STACKDROP(th); break;														

#define BCFLOAT1(op)						\
	f=op VALTOFLOAT(STACKGET(th,0));		\
	STACKSETFLOAT(th,0,(f)); break;								

#define BCFLOAT1FUN(fun)						\
	f=fun (VALTOFLOAT(STACKGET(th,0)));		\
	STACKSETFLOAT(th,0,(f)); break;									

#define BCFLOAT2(op)													\
	f=VALTOFLOAT(STACKGET(th,1)) op VALTOFLOAT(STACKGET(th,0));			\
	STACKSETFLOAT(th,1,(f));	\
	STACKDROP(th); break;

#define BCFLOAT2FUN(fun)													\
	f=fun(VALTOFLOAT(STACKGET(th,1)), VALTOFLOAT(STACKGET(th,0)));			\
	STACKSETFLOAT(th,1,(f));	\
	STACKDROP(th); break;

#define BCFLOAT2BOOL(op)													\
	b=VALTOFLOAT(STACKGET(th,1)) op VALTOFLOAT(STACKGET(th,0));			\
	STACKSETSAFE(th,1,(b)?MM.trueRef:MM.falseRef);											\
	STACKDROP(th); break;			


int isHigherFun(LB * fun);
LINT interpreterExec(Thread* th,LINT argc,LINT tfc);	// stack: fun arg0 ... 0:argn-1 
LINT interpreterRun(Thread* th,LINT maxCycles);

extern LB* runtimeCheckAddress;
extern LINT runtimeCheckValue;
#endif