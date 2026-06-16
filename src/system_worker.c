// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

#ifndef USE_WORKER_ASYNC
char WorkerScratchpad[1024 * 34];	// size of biggest worker (struct Lzw)
#endif

typedef int (*WORKER_NATIVE)(Worker*);

Thread* workerThread(Worker* w) {
	return (Thread*)fixedRootValue(w->fixedRootTh);
}

int _workerFailedOM(Worker* w)
{
	w->OM = 1;
	w->state = WORKER_RUN;
	MM.OM = 0;
#ifdef USE_WORKER_ASYNC
	semV(w->sem);
#endif
	return 0;
}
int _workerProcessCommand(Worker* w)
{
//	PRINTF(LOG_DEV,"_workerProcessCommand: %d\n", w->state);
	switch (w->state)
	{
		case WORKER_BIGGER_BUFFER:
		{
			//		PRINTF(LOG_DEV,"case WORKER_BIGGER_BUFFER\n");
			if (_bufferBiggerFinalize(fixedBufferGet(w->cmdBuffer), w->cmdAllocSize)) return _workerFailedOM(w);
			break;
		}
		default:
			return 0;
	}
	w->state = WORKER_RUN;
#ifdef USE_WORKER_ASYNC
	semV(w->sem);
#endif
	return 0;
}
void _workerClearVars(Worker* w)
{
//	PRINTF(LOG_DEV,"_workerClearVars "LSX" -> "LSD"\n", w, w->fixedRootTh);

	fixedRootRelease(w->fixedRootTh);
	w->fixedRootTh=-1;
}
void _workerFinalize(Worker* w)
{
	Thread* t = workerThread(w);
	t->sp = w->sp; // w->sp is the position  where the result should be stored
	switch (w->cmdType)
	{
	case WORKER_RESULT_INT:
		STACK_SET_INT(t, 0, w->cmdResult);
		break;
	case WORKER_RESULT_BOOL:
		STACK_SET_BOOL(t, 0, w->cmdResult);
		break;
	default:
		STACK_SET_NIL(t, 0);
	}
	// now the returned value of the native function has replaced the native function in the stack
	_workerClearVars(w);
	w->state = WORKER_READY;
}

int workerWait(Worker* w, LINT state)
{
#ifdef USE_WORKER_ASYNC
	MSEM sem;
	w->sem = &sem;
	semCreate(&sem);
	w->state = state;	// from there (ie. as soon as state is not WORKER_RUN) a gcCompact may occur
	internalPoke();
#ifdef DEPRECATED_SEM
	while(1)	// semaphor have been removed from macOs
	{
		struct timeval tm;
		if (w->state != state) break;
		tm.tv_sec =  0;
		tm.tv_usec = 1000;
		select(1, NULL, NULL, NULL, &tm);
	}
#else
	semP(&sem);
	// but now, state is WORKER_RUN again and prevents from any further gcCompact
	semDelete(&sem);
	w->sem = NULL;
#endif
#else
	w->state = state;
	_workerProcessCommand(w);
#endif

	return (w->OM)?EXEC_OM:0;
}

int workerBiggerBuffer(LINT fixedRootBuffer, LINT newSize)
{
	Buffer* b = (Buffer*)fixedRootValue(fixedRootBuffer);
	Worker* w = b->worker;
	if (w->OM) return EXEC_OM;
//	PRINTF(LOG_DEV,"workerBiggerBuffer\n");
	w->cmdBuffer = fixedRootBuffer;
	w->cmdAllocSize = newSize;
	if (workerWait(w, WORKER_BIGGER_BUFFER)) return EXEC_OM;
	return 0;
}

void workerWaitUntilAllInactive(void)
{
#ifdef USE_WORKER_ASYNC
	int i;
	for (i = 0; i < WORKERS_COUNT; i++) {
		// wait for workers to halt (end of computation or waiting for allocation)
		while(MM.workers[i].state == WORKER_RUN) hwSleepMs(1);
	}
#endif
}

WORKER_RETURN_TYPE _workerDone(Worker* w, int type, LINT result)
{
	w->cmdResult = result;
	w->cmdType = type;
	w->state = WORKER_DONE;
#ifdef USE_WORKER_ASYNC
	internalPoke();
#else
	_workerFinalize(w);
#endif
	return WORKER_RETURN;
}

WORKER_RETURN_TYPE workerDoneNil(Worker* w) { return _workerDone(w, WORKER_RESULT_NIL, 0); }
WORKER_RETURN_TYPE workerDoneBool(Worker* w, LINT result) { return _workerDone(w, WORKER_RESULT_BOOL, result); }
WORKER_RETURN_TYPE workerDoneInt(Worker* w, LINT result) { return _workerDone(w, WORKER_RESULT_INT, result); }

int fun_workerHandle(Thread* th)
{
	Worker* w;
	LINT wId = STACK_INT(th, 0);
	if (wId < 0 || wId >= WORKERS_COUNT) FUN_RETURN_NIL;
	w = &MM.workers[wId];
	if (w->OM) {
		_workerClearVars(w);
		FUN_RETURN_NIL;
	}
	if (w->state == WORKER_DONE) {
		_workerFinalize(w);
		FUN_RETURN_TRUE;
	}
	_workerProcessCommand(w);
	if (w->OM) {
		_workerClearVars(w);
		FUN_RETURN_NIL;
	}
	FUN_RETURN_FALSE;
}

int workerStart(Thread* th, int argc, void* start)
{
	Worker* w;
	LINT wId = STACK_PULL_INT(th);
	if (wId < 0 || wId >= WORKERS_COUNT) return 0;	// should never happen
	w = &MM.workers[wId];
	w->state = WORKER_RUN;
	w->OM = 0;
	w->sp = th->sp - argc + (argc ? 1 : 0);	// w->sp is the position where the result should be stored
	w->fixedRootTh = fixedRootAlloc((LB*)th);
#ifdef USE_WORKER_ASYNC
	//PRINTF(LOG_DEV,"workerStart "LSD": "LSX" -> "LSD"\n", wId, w, w->fixedRootTh);
	hwThreadCreate(start, w);
	return EXEC_WAIT;
#else
	(*(WORKER_NATIVE)start)(w);
	if (w->OM) return EXEC_OM;
	return 0;
#endif
}

int systemWorkerInit(Pkg *system)
{
	static const Native nativeDefs[] = {
#ifdef USE_WORKER_ASYNC
		{ NATIVE_INT, "_WORKERS_COUNT", (void*)WORKERS_COUNT, "Int" },
#else
		{ NATIVE_INT, "_WORKERS_COUNT", (void*)0, "Int" },
#endif
		{ NATIVE_FUN, "_workerHandle", fun_workerHandle, "fun Int -> Bool"},
	};
	NATIVE_DEF(nativeDefs);
	return 0;
}
