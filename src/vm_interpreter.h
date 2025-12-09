// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _INTERPRETER_
#define _INTERPRETER_

extern int interpreterTRON;

#define CALLSTACK_LENGTH 3 // taille de la callstack sans les variables locales
#define CALLSTACK_FUN 2	// function index
#define CALLSTACK_PC 1	// program counter
#define CALLSTACK_PREV 0	// previous callstack


#define BC_PRECOMPUTE \
	globals=ARRAY_PNT(th->fun,FUN_USER_GLOBALS); \
	bytecode=ARRAY_PNT(th->fun,FUN_USER_BC); \
	locals=th->callstack-(CALLSTACK_LENGTH-1+BC_ARGS(bytecode)+BC_LOCALS(bytecode)); \
	MM.currentPkg=th->fun->pkg; \
	pc=BC_START(bytecode)+th->pc; 


#define BCINT1(op) STACK_SET_INT(th,0,(op (STACK_INT(th,0)))); break;

#define BCINT1FUN(fun) STACK_SET_INT(th,0,(fun( STACK_INT(th,0)))); break;

#define BCINT2(op)																	\
	STACK_SET_INT(th,1,((STACK_INT(th,1)) op (STACK_INT(th,0))));	\
	STACK_DROP(th); break;														

#define BCINT2BOOL(op)																	\
	STACK_SET_BOOL(th,1,(STACK_INT(th,1)) op (STACK_INT(th,0)));	\
	STACK_DROP(th); break;														

#define BCINT2FUN(fun)																	\
	STACK_SET_INT(th,1,(fun((STACK_INT(th,1)), (STACK_INT(th,0)))));	\
	STACK_DROP(th); break;														

#define BCFLOAT1(op)						\
	f=op (STACK_FLOAT(th,0));		\
	STACK_SET_FLOAT(th,0,(f)); break;								

#define BCFLOAT1FUN(fun)						\
	f=(LFLOAT)fun (STACK_FLOAT(th,0));		\
	STACK_SET_FLOAT(th,0,(f)); break;									

#define BCFLOAT2(op)													\
	f=(STACK_FLOAT(th,1)) op (STACK_FLOAT(th,0));			\
	STACK_SET_FLOAT(th,1,(f));	\
	STACK_DROP(th); break;

#define BCFLOAT2FUN(fun)													\
	f=(LFLOAT)fun(STACK_FLOAT(th,1), STACK_FLOAT(th,0));			\
	STACK_SET_FLOAT(th,1,(f));	\
	STACK_DROP(th); break;

#define BCFLOAT2BOOL(op)													\
	b=(STACK_FLOAT(th,1)) op (STACK_FLOAT(th,0));			\
	STACK_SET_BOOL(th,1,b);											\
	STACK_DROP(th); break;			


int isHigherFun(LB * fun);
LINT interpreterExec(Thread* th,LINT argc,LINT tfc);	// stack: fun arg0 ... 0:argn-1 
LINT interpreterRun(Thread* th,LINT maxCycles);

extern LINT* runtimeCheckAddress;
extern LINT runtimeCheckValue;
#endif