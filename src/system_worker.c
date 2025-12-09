// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

int _workerFailedOM(Thread* t)
{
	t->worker.OM = 1;
	t->worker.state = WORKER_RUN;
	MM.OM = 0;
#ifdef USE_WORKER_ASYNC
	semV(t->worker.sem);
#endif
	return 0;
}
int _workerProcessCommand(Thread* t)
{
//	PRINTF(LOG_DEV,"_workerProcessCommand: %d\n", t->worker.state);
	switch (t->worker.state)
	{
		case WORKER_ALLOC_EXT:
		{
			LB* p = memoryAllocNative(t->worker.allocSize, t->worker.dbg, t->worker.forget, t->worker.mark);
			if (!p) return _workerFailedOM(t);
			STACK_PUSH_PNT_ERR(t,p,_workerFailedOM(t));
			break;
		}
		case WORKER_BIGGER_BUFFER:
		{
			//		PRINTF(LOG_DEV,"case WORKER_BIGGER_BUFFER\n");
			if (_bufferBiggerFinalize(t->worker.buffer, t->worker.allocSize)) return _workerFailedOM(t);
			t->worker.buffer = NULL;
			break;
		}
		default:
			return 0;
	}
	t->worker.state = WORKER_RUN;
#ifdef USE_WORKER_ASYNC
	semV(t->worker.sem);
#endif
	return 0;
}
void _workerClearVars(Thread* t)
{
	t->worker.result = NIL;
	t->worker.type = VAL_TYPE_PNT;
	t->worker.buffer = NULL;	
}
void _workerFinalize(Thread* t)
{
	t->sp = t->worker.sp; // th->worker.sp is the position  where the result should be stored
	if (t->worker.type == VAL_TYPE_INT) {
		STACK_SET_INT(t, 0, INT_FROM_VAL(t->worker.result));
	}
	else if (t->worker.type == VAL_TYPE_PNT) {
		STACK_SET_PNT(t, 0, PNT_FROM_VAL(t->worker.result));
	}
	else {
		STACK_SET_NIL(t, 0);
	}
	// now the returned value of the native function has replaced the native function in the stack
	_workerClearVars(t);
	t->worker.state = WORKER_READY;
}

WORKER_RETURN_TYPE workerDoneNil(volatile Thread* th)
{
	th->worker.result = NIL;
	th->worker.type = VAL_TYPE_PNT;
	th->worker.state = WORKER_DONE;
#ifdef USE_WORKER_ASYNC
	internalPoke();
#else
	_workerFinalize((Thread*)th);
#endif
	return WORKER_RETURN;
}

WORKER_RETURN_TYPE workerDonePnt(volatile Thread* th, LB* result)
{
	th->worker.result = VAL_FROM_PNT(result);
	th->worker.type = VAL_TYPE_PNT;
	th->worker.state = WORKER_DONE;
#ifdef USE_WORKER_ASYNC
	internalPoke();
#else
	_workerFinalize((Thread *)th);
#endif
	return WORKER_RETURN;
}
WORKER_RETURN_TYPE workerDoneInt(volatile Thread* th, LINT result)
{
	th->worker.result = VAL_FROM_INT(result);
	th->worker.type = VAL_TYPE_INT;
	th->worker.state = WORKER_DONE;
#ifdef USE_WORKER_ASYNC
	internalPoke();
#else
	_workerFinalize((Thread*)th);
#endif
	return WORKER_RETURN;
}


int workerWait(volatile Thread **pth, LINT state)
{
	Thread* th = (Thread*)*pth;
#ifdef USE_WORKER_ASYNC
	MSEM sem;
	th->worker.sem = &sem;
	semCreate(&sem);
	th->worker.state = state;	// from there (ie. as soon as state is not WORKER_RUN) a gcCompact may occur
	internalPoke();
#ifdef DEPRECATED_SEM
	while(1)	// semaphor have been removed from macOs
	{
		struct timeval tm;
		LINT currentState;
		lockEnter(&MM.lock);	// a gcCompact may occur, we need a stable version of pth
		currentState = (*pth)->worker.state;
		lockLeave(&MM.lock);
		if (currentState != state) break;
		tm.tv_sec =  0;
		tm.tv_usec = 1000;
		select(1, NULL, NULL, NULL, &tm);
	}
#else
    semP(&sem);
	// here th could have moved if a gcCompact occured before semP returns
	th = (Thread*)*pth;
	// however from now, state is WORKER_RUN and prevents from any further gcCompact
	semDelete(&sem);
	th->worker.sem = NULL;
#endif
#else
	th->worker.state = state;
	_workerProcessCommand(th);
#endif

	return (th->worker.OM)?EXEC_OM:0;
}

int workerAllocExt(volatile Thread** pth, LINT sizeofExt, LW dbg, FORGET forget, MARK mark)
{
	int k;
	Thread* th = (Thread*)*pth;
	th->worker.allocSize = sizeofExt;
	th->worker.dbg = dbg;
	th->worker.forget = forget;
	th->worker.mark = mark;
	(*pth)->link = pth;
	k = workerWait(pth, WORKER_ALLOC_EXT);
	(*pth)->link = NULL;
	return k;
}
int workerBiggerBuffer(volatile Thread** pth, Buffer* buffer, LINT newSize)
{
	Thread* th=(Thread*)*pth;
//	PRINTF(LOG_DEV,"workerBiggerBuffer\n");
	th->worker.buffer = buffer;
	th->worker.allocSize = newSize;
	if (workerWait(pth, WORKER_BIGGER_BUFFER)) return EXEC_OM;
	return 0;
}

void workerWaitUntilAllInactive(void)
{
#ifdef USE_WORKER_ASYNC
	Thread* th;
	for (th = MM.listThreads; th; th = th->listNext) {
		// wait for workers to halt (end of computation or waiting for allocation)
		// TODO: this is not enough because during memory allocation by workers, the worker's thread may move as well
		while (th->worker.state == WORKER_RUN) hwSleepMs(1);
	}
#endif
}
int fun_workerDone(Thread* th)
{
	Thread* t = (Thread*)STACK_PNT(th, 0);
	if (!t) FUN_RETURN_NIL;
	if (t->worker.OM) {
		_workerClearVars(t);
		FUN_RETURN_NIL;
	}
	if (t->worker.state == WORKER_DONE) {
		_workerFinalize(t);
		FUN_RETURN_TRUE;
	}
	_workerProcessCommand(t);
	if (t->worker.OM) {
		_workerClearVars(t);
		FUN_RETURN_FALSE;
	}
	FUN_RETURN_FALSE;
}

int workerStart(Thread* th, int argc, void* start)
{
	if (th->worker.OM) return EXEC_OM;
	_workerClearVars(th);
	th->worker.state = WORKER_RUN;
	th->worker.sp = th->sp - argc + (argc ? 1 : 0);	// th->worker.sp is the position where the result should be stored
#ifdef USE_WORKER_ASYNC
	hwThreadCreate(start, th);
	return EXEC_WAIT;
#else
	(*(NATIVE)start)(th);
	if (th->worker.OM) return EXEC_OM;
	return 0;
#endif
}

int systemWorkerInit(Pkg *system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "_workerDone", fun_workerDone, "fun _Thread -> Bool"},
	};
	NATIVE_DEF(nativeDefs);
	return 0;
}
