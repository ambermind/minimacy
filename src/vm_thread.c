// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"
//extern LB* MemoryCheck;
LINT ThreadCounter=0;
/*
void threadDumpLoop(char* title)
{
	Thread* th = MM.listThreads;
	PRINTF(LOG_DEV,"%s threads MM.USELESS="LSX"\n", title,MM.USELESS);
	while (th)
	{
		PRINTF(LOG_DEV,"-> "LSX":"LSD"  listMark="LSX"\n", th, th->uid, th->header.listMark);
		th = th->listNext;
	}
}
*/
void threadMark(LB* user)
{
	Thread* th=(Thread*)user;
	LINT i;
	LB* p = th->stack;

	if (!p) return;

	MARK_OR_MOVE(th->stack);
	MARK_OR_MOVE(th->user);
	MARK_OR_MOVE(th->error);
	// there is no need to mark any pending worker's result pointer, as these pointers are
	// stored in the stack by workerAllocExt before they are declared as the worker's result
	if (MOVING_BLOCKS) {
		MARK_OR_MOVE(th->listNext);	// this special list doesn't count for marking stage
		MARK_OR_MOVE(th->fun);
		MARK_OR_MOVE(th->worker.buffer);	// possible because all workers are inactive at this stage, see bmmCompact() function
		if (th->worker.type == VAL_TYPE_PNT) {
			LB* pnt=PNT_FROM_VAL(th->worker.result);
			MARK_OR_MOVE(pnt);
			th->worker.result=VAL_FROM_PNT(pnt);
		}
		for (i = 0; i <= th->sp; i++) if (ARRAY_IS_PNT(p, i)) {
			LB* pnt = ARRAY_PNT(p, i);
			MARK_OR_MOVE(pnt);
			ARRAY_GET(p, i) = VAL_FROM_PNT(pnt);
		}
		if (th->link) *th->link = (Thread*)th->header.listMark;
	}
	else {
		for(i=0;i<=th->sp;i++) if (ARRAY_IS_PNT(p,i)) BLOCK_MARK(ARRAY_PNT(p,i));
	}
}

int threadForget(LB* p)
{
	Thread* th = (Thread*)p;
//	PRINTF(LOG_DEV,"threadForget "LSD"\n", th->uid);
//	PRINTF(LOG_DEV,"threadForget "LSX":"LSD"\n", th, th->uid);
	th->stack = NULL;
//	if (th->header.mem == MemoryCheck) PRINTF(LOG_DEV,"release thread of MemoryCheck ->%d\n", th->header.mem->bytes);
	return 0;
}

Thread* threadCreate(LINT stackLen)
{
	Thread* th;
	memoryEnterSafe();
	th=(Thread*)memoryAllocNative(sizeof(Thread),DBG_THREAD, threadForget,threadMark); if (!th) return NULL;
	th->uid=ThreadCounter++;
	th->count = 0;
	th->worker.result = NIL;
	th->worker.type = VAL_TYPE_PNT;
	th->worker.buffer = NULL;
	th->worker.state = WORKER_READY;
	th->worker.sem = NULL;
	th->worker.OM = 0;
	th->link = NULL;
	th->callstack=-1;
	th->pc=0;
	th->fun=NULL;
	th->forceOpcode=THREAD_OPCODE_NONE;

	th->user = NULL;
	th->error = NULL;

	th->atomic = 0;
	th->sp=-1;
	th->stack = NULL;	// actually useless due to the th->sp=-1, but meaningfull
	th->stack= memoryAllocArray(stackLen,DBG_STACK); if (!th->stack) return NULL;
	th->listNext = NULL;
	memoryLeaveSafe();
	threadMark((LB*)th);
//	PRINTF(LOG_DEV,"threadCreate %llx %llx\n",th,th->stack);
//	PRINTF(LOG_DEV,"threadCreate %llx\n",th->uid);
	return th;
}

int threadBigger(Thread* th)
{
	LINT i;
	LINT size= ARRAY_LENGTH(th->stack);
	LB* newstack= memoryAllocArray(size+ THREAD_STACK_STEP,DBG_STACK);
	LB* oldstack=th->stack;
	if (!newstack) return EXEC_OM;
//	for(i=1;i<=th->sp+1;i++) newstack->data[i]=oldstack->data[i];	//HACK : 1<=i<=sp+1 because data[0]=dbg and sp=0 means 1 element in the stack
	for(i=0;i<=th->sp;i++) ARRAY_COPY(newstack,i,oldstack,i);	// sp=0 means 1 element in the stack
	th->stack=newstack;
	threadMark((LB*)th);
	if (MM.gcTrace) PRINTF(LOG_SYS, "> bigger stack " LSD " thread:" LSD "\n", ARRAY_LENGTH(th->stack), th->uid);
//	threadPrintCallstack(th);
	//	PRINTF(LOG_DEV,"threadBigger %llx %llx\n",th,th->stack);
	return 0;
}

void stackReset(Thread* th)
{
	th->sp = -1;
}

int stackPushEmptyArray(Thread* th,LINT size,LW dbg)
{
	LB* p = memoryAllocArray(size, dbg); if (!p) return EXEC_OM;
	FUN_PUSH_PNT(p);
	return 0;
}

int stackPushFilledArray(Thread* th,LINT size,LW dbg)
{
	LINT i,j;
	LB* p;
	STACK_PUSH_EMPTY_ARRAY_ERR(th,size,dbg,EXEC_OM);
	p=STACK_PNT(th,0);
	j=1;
	for(i=size-1;i>=0;i--) STACK_STORE(p,i,th,j++);
	STACK_SET_PNT(th,size,p);
	STACK_DROPN(th,size);
	return 0;
}

int stackPushStr(Thread* th,char* src,LINT size)
{
	LB* p = memoryAllocStr(src, size); if (!p) return EXEC_OM;
	FUN_PUSH_PNT(p);
	return 0;
}

int stackSetStr(Thread* th, LINT i, char* src,LINT size)
{
	LB* p = memoryAllocStr(src, size); if (!p) return EXEC_OM;
	STACK_SET_PNT(th,i, p);
	return 0;
}

int stackSetBuffer(Thread* th, LINT i, Buffer* b)
{
	LB* p = memoryAllocFromBuffer(b); if (!p) return EXEC_OM;
	STACK_SET_PNT(th, i, p);
	return 0;
}

void threadPrintCallstack(Thread* t)
{
	int n = 0;
	LINT callstack, pc;
	LB* fun;

	if (!t) return;

	callstack = t->callstack;
	pc = t->pc;
	fun = t->fun;
	if (!fun) return;
	while (callstack >= 0)
	{
		char* name = ARRAY_PNT(fun, FUN_USER_NAME) ? STR_START((ARRAY_PNT(fun, FUN_USER_NAME))) : "[NO NAME]";
		PRINTF(LOG_DEV,"%2d> %s: "LSD"\n", n, name, pc);
		n++;
		fun = STACK_REF_PNT(t, callstack, CALLSTACK_FUN);
		pc = STACK_REF_INT(t, callstack, CALLSTACK_PC);
		callstack = STACK_REF_INT(t, callstack, CALLSTACK_PREV);
	}
}

int fun_threadCurrent(Thread* th)
{
	FUN_PUSH_PNT((LB*)th);
	return 0;
}

int fun_threadCreate(Thread* th)
{
	Thread* t=threadCreate(THREAD_STACK_LENGTH0);
	if (MM.OM) return EXEC_OM;
//interpreterTRON=1;
	if (t)
	{
		t->listNext = MM.listThreads;
		MM.listThreads = t;
	}
	FUN_RETURN_PNT((LB*)t);
}

int fun_threadDump(Thread* th)
{
	Thread* t = (Thread*)STACK_PNT(th, 0);
	if (t) threadDump(LOG_USER,t,6);
	return 0;
}
int fun_threadNext(Thread* th)
{
	Thread* t = (Thread*)STACK_PNT(th, 0);
	STACK_SET_PNT(th, 0, (LB*)(t?t->listNext:MM.listThreads));
	return 0;
}
int fun_threadUser(Thread* th)
{
	Thread* t = (Thread*)STACK_PNT(th, 0);
	if (t) STACK_SET_PNT(th, 0, t->user);
	return 0;
}
int fun_threadSetUser(Thread* th)
{
	LB* user = STACK_PNT(th,0);
	Thread* t = (Thread*)STACK_PNT(th, 1);
	if (t && STACK_IS_PNT(th, 0)) {
		t->user = user;
		BLOCK_MARK(t->user);
	}
	STACK_DROP(th);
	return 0;
}

int fun_threadId(Thread* th)
{
	Thread* t = (Thread*)STACK_PNT(th, 0);
	if (t) STACK_SET_INT(th, 0, t->uid);
	return 0;
}
int fun_threadPP(Thread* th)
{
	Thread* t = (Thread*)STACK_PNT(th, 0);
	if (t) STACK_SET_INT(th, 0, t->sp);
	return 0;
}
int fun_threadCount(Thread* th)
{
	Thread* t = (Thread*)STACK_PNT(th, 0);
	if (t) STACK_SET_INT(th, 0, t->count);
	return 0;
}
int fun_threadClear(Thread* th)
{
	Thread* t = (Thread*)STACK_PNT(th, 0);
	if (!t) return 0;
//	PRINTF(LOG_DEV,"_threadClear %d from %d\n", t->uid, th->uid);
	if (t == th) return 0;	// should not happen
	stackReset(t);
	t->callstack = -1;	// this will prevent function callstack from failing
	t->stack = NULL;
	t->fun = NULL;
	STACK_SET_INT(th, 0, 0);
	return 0;
}

int fun_threadResume(Thread* th)
{
//	stack(th,0) contains the result of the function to resume
	Thread* t=(Thread*)STACK_PNT(th,1);
	if (!t) FUN_RETURN_INT(-1);
//interpreterTRON=1;
	STACK_PUSH_NIL_ERR(t,EXEC_OM);	// make space on the other thread
	STACK_COPY(t,0,th,0);	// move onto the other thread
	FUN_RETURN_INT(0);
}

int fun_threadRun(Thread* th)
{
	LINT result;
	LINT maxCycles = STACK_INT(th, 0);
	Thread* t = (Thread*)STACK_PNT(th, 1);

	if (!t) FUN_RETURN_INT(-1);
	result = interpreterRun(t, maxCycles);
	MM.OM = 0;
	FUN_RETURN_INT(result);
}

int fun_threadExec(Thread* th)
{
//	stack(th,0) contains the function to store on the thread t stack
	Thread* t = (Thread*)STACK_PNT(th, 1);

	if (!STACK_PNT(th,0) || (!t)) FUN_RETURN_INT(-1);
	//interpreterTRON=1;
	STACK_PUSH_NIL_ERR(t,EXEC_OM);	// make space on the other thread
	STACK_COPY(t,0,th,0);	// move onto the other thread
	interpreterExec(t, 0, 0);	// make it ready to go
	FUN_RETURN_INT(0);
}

int fun_callstack(Thread* th)
{
	LINT n = 0;
	LINT callstack, pc;
	LB* fun;

	Thread* t = (Thread*)STACK_PNT(th,0);
	if (!t) t = th;
	callstack = t->callstack;
	pc = t->pc;
	fun = t->fun;
	while (callstack >= 0)
	{
		FUN_PUSH_PNT( ARRAY_PNT(fun, FUN_USER_NAME));
		FUN_PUSH_INT(pc);
		FUN_PUSH_PNT( ARRAY_PNT(fun, FUN_USER_BC));
		FUN_PUSH_PNT((LB*)fun->pkg);
		FUN_MAKE_ARRAY( 4, DBG_TUPLE);
		n++;
		fun = STACK_REF_PNT(t, callstack, CALLSTACK_FUN);
		pc = STACK_REF_INT(t, callstack, CALLSTACK_PC);
		callstack = STACK_REF_INT(t, callstack, CALLSTACK_PREV);
	}
	FUN_PUSH_NIL;
	while ((n--) > 0) FUN_MAKE_ARRAY( LIST_LENGTH, DBG_LIST);
	STACK_SKIP(th, 1);
	return 0;
}
int fun_caller(Thread* th)
{
	LINT callstack, pc;
	LB* fun;

	Thread* t = th;
	callstack = t->callstack;

	fun = STACK_REF_PNT(t, callstack, CALLSTACK_FUN);
	if (!fun) FUN_RETURN_NIL;
	pc = STACK_REF_INT(t, callstack, CALLSTACK_PC);
	callstack = STACK_REF_INT(t, callstack, CALLSTACK_PREV);
	FUN_PUSH_PNT( ARRAY_PNT(fun, FUN_USER_NAME));
	FUN_PUSH_INT( pc);
	FUN_PUSH_PNT( ARRAY_PNT(fun, FUN_USER_BC));
	FUN_PUSH_PNT((LB*)fun->pkg);
	FUN_MAKE_ARRAY( 4, DBG_TUPLE);
	return 0;
}
int fun_lastError(Thread* th)
{
	FUN_PUSH_PNT(th->error);
	th->error=NULL;
	return 0;
}
int fun_setError(Thread* th)
{
	th->error=STACK_PNT(th,0);
	BLOCK_MARK(th->error);
	return 0;
}
int systemThreadInit(Pkg *system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "_threadCurrent", fun_threadCurrent, "fun -> _Thread" },
		{ NATIVE_INT, "EXEC_IDLE", (void*)EXEC_IDLE, "Int" },
		{ NATIVE_INT, "EXEC_PREEMPTION", (void*)EXEC_PREEMPTION, "Int" },
		{ NATIVE_INT, "EXEC_WAIT", (void*)EXEC_WAIT, "Int" },
		{ NATIVE_INT, "EXEC_EXIT", (void*)EXEC_EXIT, "Int" },
		{ NATIVE_INT, "EXEC_OM", (void*)EXEC_OM, "Int" },
		{ NATIVE_FUN, "_callstack", fun_callstack, "fun _Thread -> list [Str Int Str Package]" },
		{ NATIVE_FUN, "caller", fun_caller, "fun -> [Str Int Str Package]" },
		{ NATIVE_FUN, "_threadCreate", fun_threadCreate, "fun -> _Thread" },
		{ NATIVE_FUN, "_threadId", fun_threadId, "fun _Thread -> Int" },
		{ NATIVE_FUN, "_threadPP", fun_threadPP, "fun _Thread -> Int" },
		{ NATIVE_FUN, "_threadCount", fun_threadCount, "fun _Thread -> Int" },
		{ NATIVE_FUN, "_threadClear", fun_threadClear, "fun _Thread -> Int" },
		{ NATIVE_FUN, "_threadExec", fun_threadExec, "fun _Thread (fun -> a1) -> Int" },
		{ NATIVE_FUN, "_threadRun", fun_threadRun, "fun _Thread Int -> Int" },
		{ NATIVE_OPCODE, "_threadHoldOn", (void*)OPholdon, "fun -> a1" },
		{ NATIVE_OPCODE, "_threadResign", (void*)OPresign, "fun -> a1" },
		{ NATIVE_FUN, "_threadResume", fun_threadResume, "fun _Thread a1 -> Int" },
		{ NATIVE_FUN, "_threadDump", fun_threadDump, "fun _Thread -> _Thread" },
		{ NATIVE_FUN, "_threadNext", fun_threadNext, "fun _Thread -> _Thread" },
		{ NATIVE_FUN, "_threadUser", fun_threadUser, "fun _Thread -> a1" },
		{ NATIVE_FUN, "_threadSetUser", fun_threadSetUser, "fun _Thread  a1 -> _Thread" },
		{ NATIVE_FUN, "lastError", fun_lastError, "fun -> Error" },
		{ NATIVE_FUN, "setError", fun_setError, "fun Error -> Error" },
	};
	NATIVE_DEF(nativeDefs);

	return 0;
}
