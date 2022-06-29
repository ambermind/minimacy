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

int workerStart(Thread* th, int argc, void* start)
{
	th->worker.state = WORKER_RUN;
	th->worker.pp = th->pp - argc;	// we remove argc-1 args + 1 native function structure
	hwThreadCreate(start, th);
	return EXEC_WAIT;
}

MTHREAD_RETURN_TYPE workerDone(Thread* th, LW result)
{
	th->worker.result = result;
	th->worker.state = WORKER_DONE;
	internalPoke();
	return MTHREAD_RETURN;
}
void workerWait(Thread *th, LINT state)
{
#ifdef ON_UNIX
    // Semaphores are broken on MacOS. The following is a workaround hack
    // We detect MacOS based on the size of MSEM, and then we wait until the worker state changes
    // in <semaphore.h> : typedef int sem_t;
    if (macDetect())
	{
		while(th->worker.state == state)
		{
			struct timeval tm;
			tm.tv_sec =  0;
			tm.tv_usec = 1000;
			select(1, NULL, NULL, NULL, &tm);
		}
		return;
	}
#endif
    semP(&th->worker.sem);
}
char* workerAllocStr(Thread* th, LINT size)
{
	LB* p;
	th->worker.allocSize = size;
	th->worker.state = WORKER_ALLOC_STR;
	internalPoke();
    workerWait(th,WORKER_ALLOC_STR);
	p=VALTOPNT(STACKGET(th, 0));
	if (p) return STRSTART(p);
	return NULL;
}
LB* workerAllocContent(Thread* th, char* content, LINT size)
{
	LB* p;
	th->worker.allocSize = size;
	th->worker.state = WORKER_ALLOC_STR;
	internalPoke();
	workerWait(th, WORKER_ALLOC_STR);
	p = VALTOPNT(STACKGET(th, 0));
	if (p && content) memcpy(STRSTART(p), content, size);
	return p;
}
int workerMakeList(Thread* th, LINT size)
{
	th->worker.allocSize = size;
	th->worker.state = WORKER_MAKE_LIST;
	internalPoke();
	workerWait(th, WORKER_MAKE_LIST);
	return 0;
}

LB* workerAllocExt(Thread* th, LINT sizeofExt, LW dbg, FORGET forget, MARK mark)
{
	th->worker.allocSize = sizeofExt;
	th->worker.dbg = dbg;
	th->worker.forget = forget;
	th->worker.mark = mark;
	th->worker.state = WORKER_ALLOC_EXT;
	internalPoke();
    workerWait(th,WORKER_ALLOC_EXT);
	return VALTOPNT(STACKGET(th, 0));
}

LB* workerAllocBitmap(Thread* th, LINT w, LINT h)
{
	th->worker.w = w;
	th->worker.h = h;
	th->worker.state = WORKER_MAKE_BITMAP;
	internalPoke();
    workerWait(th, WORKER_MAKE_BITMAP);
	return VALTOPNT(STACKGET(th, 0));
}
void workerBiggerBuffer(Thread* th, Buffer* buffer, LINT newSize)
{
	printf("workerBiggerBuffer\n");
	th->worker.buffer = buffer;
	th->worker.allocSize = newSize;
	th->worker.state = WORKER_BIGGER_BUFFER;
	internalPoke();
	workerWait(th, WORKER_BIGGER_BUFFER);
}
void workerWriteBuffer(Thread* th, char* src, LINT len)
{
	bufferAddBin(th, th->worker.buffer, src, len);
}


int fun_workerDone(Thread* th)
{
	Thread* t = (Thread*)VALTOPNT(STACKGET(th, 0));
	if (!t) return 0;
	switch (t->worker.state)
	{
	case WORKER_DONE:
	{
		t->pp = t->worker.pp;
		STACKSET(t, 0, t->worker.result);
		t->worker.state = WORKER_READY;
		STACKSET(th, 0, MM.trueRef);
		return 0;
	}
	case WORKER_ALLOC_STR:
	{
		LB* p = memoryAllocStr(t, NULL, t->worker.allocSize);
		if ((!p) || (STACKPUSH(t, PNTTOVAL(p))))  t->OM = 1;
		t->worker.state = WORKER_RUN;
		semV(&t->worker.sem);
		break;
	}
	case WORKER_MAKE_LIST:
	{
		LINT n = t->worker.allocSize;
		if (STACKPUSH(t, NIL)) t->OM = 1;
		while ((n > 0) && !t->OM)
		{
			if (DEFTAB(t, 2, DBG_LIST)) t->OM = 1;
			n--;
		}
		t->worker.state = WORKER_RUN;
		semV(&t->worker.sem);
		break;
	}
	case WORKER_ALLOC_EXT:
	{
		LB* p = memoryAllocExt(t, t->worker.allocSize, t->worker.dbg, t->worker.forget, t->worker.mark);
		if (p) memset(((char*)p) + sizeof(LB) + 2 * sizeof(LW), 0, t->worker.allocSize - sizeof(LB) - 2 * sizeof(LW));
		if ((!p) || (STACKPUSH(t, PNTTOVAL(p))))  t->OM = 1;

		t->worker.state = WORKER_RUN;
		semV(&t->worker.sem);
		break;
	}
	case WORKER_MAKE_BITMAP:
	{
		LBitmap* p = _bitmapCreate(t, t->worker.w, t->worker.h);
		if ((!p) || (STACKPUSH(t, PNTTOVAL(p))))  t->OM = 1;
		t->worker.state = WORKER_RUN;
		semV(&t->worker.sem);
		break;
	}
	case WORKER_BIGGER_BUFFER:
	{
		printf("case WORKER_BIGGER_BUFFER\n");
		_bufferBiggerFinalize(t, t->worker.buffer,t->worker.allocSize);
		t->worker.state = WORKER_RUN;
		semV(&t->worker.sem);
		break;
	}
	}
	STACKSET(th, 0, MM.falseRef);
	return 0;
}
int sysWorkerInit(Thread* th, Pkg *system)
{
	Type* fun_T_B = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.Thread, MM.Boolean);
	pkgAddFun(th, system, "_workerDone", fun_workerDone, fun_T_B);
	return 0;
}
