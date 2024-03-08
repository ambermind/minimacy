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

#define MARK_TYPE_TRY 0
#define MARK_TYPE_BREAK 1

#define BC_PRECOMPUTE \
	globals=(TABPNT(th->fun,FUN_USER_GLOBALS)); \
	bytecode=(TABPNT(th->fun,FUN_USER_BC)); \
	locals=th->callstack-(CALLSTACK_LENGTH-1+BC_ARGS(bytecode)+BC_LOCALS(bytecode)); \
	pc=BC_START(bytecode)+th->pc; 


#define BCINT1(op) STACKSETINT(th,0,(op (STACKINT(th,0)))); break;

#define BCINT1FUN(fun) STACKSETINT(th,0,(fun( STACKINT(th,0)))); break;

#define BCINT2(op)																	\
	STACKSETINT(th,1,((STACKINT(th,1)) op (STACKINT(th,0))));	\
	STACKDROP(th); break;														

#define BCINT2BOOL(op)																	\
	STACKSETBOOL(th,1,(STACKINT(th,1)) op (STACKINT(th,0)));	\
	STACKDROP(th); break;														

#define BCINT2FUN(fun)																	\
	STACKSETINT(th,1,(fun((STACKINT(th,1)), (STACKINT(th,0)))));	\
	STACKDROP(th); break;														

#define BCFLOAT1(op)						\
	f=op (STACKFLOAT(th,0));		\
	STACKSETFLOAT(th,0,(f)); break;								

#define BCFLOAT1FUN(fun)						\
	f=(LFLOAT)fun (STACKFLOAT(th,0));		\
	STACKSETFLOAT(th,0,(f)); break;									

#define BCFLOAT2(op)													\
	f=(STACKFLOAT(th,1)) op (STACKFLOAT(th,0));			\
	STACKSETFLOAT(th,1,(f));	\
	STACKDROP(th); break;

#define BCFLOAT2FUN(fun)													\
	f=(LFLOAT)fun(STACKFLOAT(th,1), STACKFLOAT(th,0));			\
	STACKSETFLOAT(th,1,(f));	\
	STACKDROP(th); break;

#define BCFLOAT2BOOL(op)													\
	b=(STACKFLOAT(th,1)) op (STACKFLOAT(th,0));			\
	STACKSETBOOL(th,1,b);											\
	STACKDROP(th); break;			


int isHigherFun(LB * fun);
LINT interpreterExec(Thread* th,LINT argc,LINT tfc);	// stack: fun arg0 ... 0:argn-1 
LINT interpreterRun(Thread* th,LINT maxCycles);

extern LB* runtimeCheckAddress;
extern LINT runtimeCheckValue;
#endif