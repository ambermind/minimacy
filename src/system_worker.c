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

int _workerFailedOM(Thread* t)
{
	t->worker.state = WORKER_RUN;
#ifdef USE_WORKER_ASYNC
	semV(&t->worker.sem);
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
			LB* p = memoryAllocExt(t, t->worker.allocSize, t->worker.dbg, t->worker.forget, t->worker.mark);
			if (!p) return _workerFailedOM(t);
			memset(((char*)p) + sizeof(LB) + 2 * sizeof(void*), 0, t->worker.allocSize - sizeof(LB) - 2 * sizeof(void*));
			STACKPUSHPNT_ERR(t,p,_workerFailedOM(t));
			break;
		}
		case WORKER_BIGGER_BUFFER:
		{
			//		PRINTF(LOG_DEV,"case WORKER_BIGGER_BUFFER\n");
			if (_bufferBiggerFinalize(t, t->worker.buffer, t->worker.allocSize)) return _workerFailedOM(t);
			break;
		}
		default:
			return 0;
	}
	t->worker.state = WORKER_RUN;
#ifdef USE_WORKER_ASYNC
	semV(&t->worker.sem);
#endif
	return 0;
}

void _workerFinalize(Thread* t)
{
	t->pp = t->worker.pp; // th->worker.pp is the position of the native function in the stack
	if (t->worker.type == VAL_TYPE_INT) {
		STACKSETINT(t, 0, VALTOINT(t->worker.result));
	}
	else if (t->worker.type == VAL_TYPE_PNT) {
		STACKSETPNT(t, 0, VALTOPNT(t->worker.result));
	}
	else {
		STACKSETNIL(t, 0);
	}
	// now the returned value of the native function has replaced the native function in the stack
	t->worker.state = WORKER_READY;
}

int workerStart(Thread* th, int argc, void* start)
{
	th->worker.state = WORKER_RUN;
	th->worker.pp = th->pp - argc;	// th->worker.pp is the position of the native function in the stack
#ifdef USE_WORKER_SYNC
	(*(NATIVE)start)(th);
	if (th->OM) return EXEC_OM;
	return 0;
#endif
#ifdef USE_WORKER_ASYNC
	hwThreadCreate(start, th);
	return EXEC_WAIT;
#endif
}

MTHREAD_RETURN_TYPE workerDoneNil(Thread* th)
{
	th->worker.result = NIL;
	th->worker.type = VAL_TYPE_PNT;
	th->worker.state = WORKER_DONE;
#ifdef USE_WORKER_SYNC
	_workerFinalize(th);
#endif
#ifdef USE_WORKER_ASYNC
	internalPoke();
#endif
	return MTHREAD_RETURN;
}
MTHREAD_RETURN_TYPE workerDonePnt(Thread* th, LB* result)
{
	th->worker.result = PNTTOVAL(result);
	th->worker.type = VAL_TYPE_PNT;
	th->worker.state = WORKER_DONE;
#ifdef USE_WORKER_SYNC
	_workerFinalize(th);
#endif
#ifdef USE_WORKER_ASYNC
	internalPoke();
#endif
	return MTHREAD_RETURN;
}
MTHREAD_RETURN_TYPE workerDoneInt(Thread* th, LINT result)
{
	th->worker.result = INTTOVAL(result);
	th->worker.type = VAL_TYPE_INT;
	th->worker.state = WORKER_DONE;
#ifdef USE_WORKER_SYNC
	_workerFinalize(th);
#endif
#ifdef USE_WORKER_ASYNC
	internalPoke();
#endif
	return MTHREAD_RETURN;
}

int workerWait(Thread *th, LINT state)
{
#ifdef USE_WORKER_SYNC
	_workerProcessCommand(th);
	if (th->OM) return EXEC_OM;
#endif
#ifdef USE_WORKER_ASYNC
	internalPoke();
#ifdef DEPRECATED_SEM
	while(th->worker.state == state)
	{
		struct timeval tm;
		tm.tv_sec =  0;
		tm.tv_usec = 1000;
		select(1, NULL, NULL, NULL, &tm);
	}
#else
    semP(&th->worker.sem);
#endif
#endif
	if (th->OM) return EXEC_OM;
	return 0;
}

LB* workerAllocExt(Thread* th, LINT sizeofExt, LW dbg, FORGET forget, MARK mark)
{
	th->worker.allocSize = sizeofExt;
	th->worker.dbg = dbg;
	th->worker.forget = forget;
	th->worker.mark = mark;
	th->worker.state = WORKER_ALLOC_EXT;
    if (workerWait(th,WORKER_ALLOC_EXT)) return NULL;
	return STACKPNT(th, 0);
}
int workerBiggerBuffer(Thread* th, Buffer* buffer, LINT newSize)
{
//	PRINTF(LOG_DEV,"workerBiggerBuffer\n");
	th->worker.buffer = buffer;
	th->worker.allocSize = newSize;
	th->worker.state = WORKER_BIGGER_BUFFER;
	if (workerWait(th, WORKER_BIGGER_BUFFER)) return EXEC_OM;
	return 0;
}

int fun_workerDone(Thread* th)
{
	Thread* t = (Thread*)(STACKPNT(th, 0));
	if (!t) return 0;
	if (t->worker.state == WORKER_DONE) {
		_workerFinalize(t);
		FUN_RETURN_TRUE;
	}
	_workerProcessCommand(t);
	FUN_RETURN_FALSE;
}
int sysWorkerInit(Thread* th, Pkg *system)
{
	Type* fun_T_B = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.Thread, MM.Boolean);
	pkgAddFun(th, system, "_workerDone", fun_workerDone, fun_T_B);
	return 0;
}
