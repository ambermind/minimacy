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
//extern LB* MemoryCheck;
LINT ThreadCounter=0;
Mem* MemCheck = NULL;

void memCountDecrement(Mem* mem)
{
	if (mem)
	{
		mem->count--;
//		if (mem == MemCheck)
//		{
//			printf("memCountDecrement "LSX" from "LSX": "LSD" count:%lld\n", mem, mem->header.mem, mem->bytes, mem->count);
//			printf("\n");
//		}
		if (mem->count < 1)
		{
			MM.blocs_nb--;
			MM.blocs_length -= sizeof(LB) + (((HEADER_SIZE((LB*)mem) + LWLEN - 1) >> LSHIFT) << LSHIFT);
//			printf("memCountDecrement Rec "LSX" from "LSX": "LSD" count:%lld\n", mem->header.mem, mem, mem->bytes, mem->count);
			memCountDecrement(mem->header.mem);
			//if (mem == MemCheck) 
//			printf("\nmemFree "LSX" count:%lld\n", mem,  mem->count);
			free((void*)mem);
		}
	}
}

int memForget(LB* p)
{
	Mem* mem = (Mem*)p;
//	if (mem== MemCheck) printf("memForget "LSX" count:%lld\n", mem, mem->count);
	memCountDecrement(mem);
//	if (mem == MemoryCheck) printf("release MemoryCheck ->%d\n", mem->bytes);
	return -1;	// never let the GC desalloc itself the mem structure
}
void memMark(LB* user)
{
	Mem* mem = (Mem*)user;
	MEMORYMARK(user, (LB*)mem->name);
}
Mem* memCreate(Thread* th, Mem* parent,LINT maxBytes,LB* name)
{
	Mem* mem = (Mem*)memoryAllocExt(th, sizeof(Mem), DBG_MEMCOUNT, memForget, memMark); if (!mem) return NULL;
	mem->name = name;
	mem->maxBytes = (maxBytes>=0) ? maxBytes : (parent?parent->maxBytes:0);
	mem->bytes = 0;
	mem->count = 1;
	if (MM.mem && !MemCheck) MemCheck = mem;
//	printf("\nmemCreate ------------------- %llx\n", mem);
	memoryTake(parent, (LB*)mem);
	if (parent)
	{
		mem->listNext = NULL;
	}
	else
	{
		mem->listNext = MM.listMems;
		MM.listMems = mem;
	}
	return mem;
}
/*
void threadDumpLoop(char* title)
{
	Thread* th = MM.listThreads;
	printf("%s threads MM.USELESS="LSX"\n", title,MM.USELESS);
	while (th)
	{
		printf("-> "LSX":"LSD"  lifo="LSX"\n", th, th->uid, th->header.lifo);
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
	p->lifo=MM.USEFUL;
	for(i=0;i<=th->pp;i++)
	{
		LW val=TABGET(p,i);
		if (ISVALPNT(val)) MEMORYMARK(p, VALTOPNT(val));
	}
	MEMORYMARK(user, (LB*)th->memDelegate);
	MEMORYMARK(user,th->user);
}

int threadForget(LB* p)
{
	Thread* th = (Thread*)p;
//	printf("threadForget "LSD"\n", th->uid);
//	printf("threadForget "LSX":"LSD"\n", th, th->uid);
	semDelete(&th->worker.sem);

	th->stack = NULL;
//	if (th->header.mem == MemoryCheck) printf("release thread of MemoryCheck ->%d\n", th->header.mem->bytes);
	return 0;
}

Thread* threadCreate(Thread* th0, Mem* mem)
{
	Thread* th;
	memoryEnterFast(th0);
	th=(Thread*)memoryAllocExt(th0, sizeof(Thread),DBG_THREAD, threadForget,threadMark); if (!th) return NULL;
	th->uid=ThreadCounter++;
	th->count = 0;
	th->worker.state = WORKER_READY;
	semCreate(&th->worker.sem);
	th->seed = SEED_INIT_VALUE;
	th->callstack=-1;
	th->pc=0;
	th->fun=NULL;
	th->forceOpcode=THREAD_OPCODE_NONE;
	th->OM = 0;

	memoryTake(mem, (LB*)th);
	th->memDelegate = mem;

	th->user = NULL;

	th->atomic = 0;
	th->pp=-1;
	th->stack = NULL;	// actually useless due to the th->pp=-1, but meaningfull

	th->stack= memoryAllocTable(th, THREAD_STACK_LENGTH0,DBG_STACK);
	if (!th->stack) return NULL;
	th->listNext = NULL;
	threadMark((LB*)th);
//	printf("threadCreate %llx %llx\n",th,th->stack);
//	printf("threadCreate %llx\n",th->uid);
	memoryLeaveFast(th0);
	return th;
}

int threadBigger(Thread* th)
{
	LINT i;
	LINT size= TABLEN(th->stack);
	LB* newstack= memoryAllocTable(th, size+ THREAD_STACK_STEP,DBG_STACK);
	LB* oldstack=th->stack;
	if (!newstack) return EXEC_OM;
	for(i=1;i<=th->pp+1;i++) newstack->data[i]=oldstack->data[i];	//HACK : 1<=i<=pp+1 because data[0]=dbg and pp=0 means 1 element in the stack
	th->stack=newstack;
	threadMark((LB*)th);
	PRINTF(th,LOG_ERR, "## bigger stack " LSD " thread:" LSD "\n", TABLEN(th->stack), th->uid);
//	threadPrintCallstack(th);
	//	printf("threadBigger %llx %llx\n",th,th->stack);
	return 0;
}

void stackReset(Thread* th)
{
	th->pp = -1;
}

int stackPush(Thread* th,LW val)
{
	TABSET(th->stack,++th->pp,val);
	if (th->pp >= TABLEN(th->stack) - 1) return threadBigger(th);
	return 0;
}

int stackPushTable(Thread* th,LINT size,LW dbg)
{
	LB* p = memoryAllocTable(th, size, dbg); if (!p) return EXEC_OM;
	return STACKPUSH(th, PNTTOVAL(p));
}

int stackDeftab(Thread* th,LINT size,LW dbg)
{
	LINT i,j;
	LB* p;
	if (TABLEPUSH(th,size,dbg)) return EXEC_OM;
	p=VALTOPNT(STACKGET(th,0));
	j=1;
	for(i=size-1;i>=0;i--) TABSET(p,i,STACKGET(th,j++));
	STACKSET(th,size,PNTTOVAL(p));
	STACKDROPN(th,size);
	return 0;
}

int stackPushStr(Thread* th,char* src,LINT size)
{
	LB* p = memoryAllocStr(th, src, size); if (!p) return EXEC_OM;
	return STACKPUSH(th, PNTTOVAL(p));
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
		char* name = VALTOPNT(TABGET(fun, FUN_USER_NAME)) ? STRSTART(VALTOPNT(TABGET(fun, FUN_USER_NAME))) : "[NO NAME]";
		printf("%2d> %s: "LSD"\n", n, name, pc);
		n++;
		fun = VALTOPNT(STACKGETFROMREF(t, callstack, CALLSTACK_FUN));
		pc = VALTOINT(STACKGETFROMREF(t, callstack, CALLSTACK_PC));
		callstack = VALTOINT(STACKGETFROMREF(t, callstack, CALLSTACK_PREV));
	}
}

int fun_memCreate(Thread* th)
{
	LW wMaxBytes = STACKPULL(th);
	LB* name= VALTOPNT(STACKGET(th, 0));
	Mem *parent = (Mem*)VALTOPNT(STACKGET(th, 1));
	Mem* child = memCreate(th, parent, (wMaxBytes==NIL) ? -1:VALTOINT(wMaxBytes),name); if (!child) return EXEC_OM;
	STACKSET(th, 1, PNTTOVAL(child));
	STACKDROP(th);
	return 0;
}
int fun_memName(Thread* th)
{
	Mem* m = (Mem*)VALTOPNT(STACKGET(th, 0));
	if (m) STACKSET(th, 0, PNTTOVAL(m->name));
	return 0;
}
int fun_memMemory(Thread* th)
{
	Mem* m = (Mem*)VALTOPNT(STACKGET(th, 0));
	if (m) STACKSETINT(th, 0, (m->bytes));
	return 0;
}

int fun_memMaxMemory(Thread* th)
{
	Mem* m = (Mem*)VALTOPNT(STACKGET(th, 0));
	if (m) STACKSETINT(th, 0, (m->maxBytes));
	return 0;
}
int fun_memSetMaxMemory(Thread* th)
{
	LINT maxBytes = VALTOINT(STACKPULL(th));
	Mem* m = (Mem*)VALTOPNT(STACKGET(th, 0));
	if (m && (maxBytes > 0)) m->maxBytes = maxBytes;
	return 0;
}
int fun_memParent(Thread* th)
{
	Mem* m = (Mem*)VALTOPNT(STACKGET(th, 0));
	STACKSET(th, 0, PNTTOVAL(m?m->header.mem:NULL));
	return 0;
}
int fun_memNext(Thread* th)
{
	Mem* m = (Mem*)VALTOPNT(STACKGET(th, 0));
	STACKSET(th, 0, PNTTOVAL(m?m->listNext:MM.listMems));
	return 0;
}

int fun_memoryAuthorize(Thread* th)
{
	return STACKPUSH(th, PNTTOVAL(th->header.mem));
}

int fun_memoryAssign(Thread* th)
{
	Mem* previous = th->memDelegate;
	Mem* mem = (Mem*)VALTOPNT(STACKGET(th, 0));
	if (!mem) mem = th->header.mem;
	th->memDelegate = mem;
	//	printf("*************_memoryAssign:\n");
	//	printf("_memoryAssign "LSX": " LSD" delegate to "LSX" :" LSD"\n", th, th->uid, MM.thread, MM.thread->uid);
	STACKSET(th, 0, PNTTOVAL(previous));
	return 0;
}

int fun_memoryTake(Thread* th)
{
	LW arg = STACKGET(th, 0);
	Mem* m = (Mem*)VALTOPNT(STACKGET(th, 1));
	if (m && ISVALPNT(arg)) memoryTake(m, VALTOPNT(arg));
	STACKSKIP(th, 1);
	return 0;
}

int fun_memoryDump(Thread* th)
{
	Mem* m = (Mem*)VALTOPNT(STACKGET(th, 0));
	if (m)
	{
		LB* p = MM.listBlocks;
		printf("memoryDump "LSX"\n", (LINT)m);
		while (p)
		{
			if (p->mem && ((p->mem==m)||(p->mem->header.mem ==m))) itemDump(th, LOG_USER, PNTTOVAL(p));
			p = p->nextBlock;
		}
		p = MM.listCheck;
		while (p)
		{
			if (p->mem && ((p->mem == m) || (p->mem->header.mem == m))) itemDump(th, LOG_USER, PNTTOVAL(p));
			p = p->nextBlock;
		}
	}
	return 0;
}
int fun_memoryTest(Thread* th)
{
	LINT total = VALTOINT(STACKPULL(th));
	Mem* m = (Mem*)VALTOPNT(STACKGET(th, 0));
	if (!m) return 0;
	STACKSET(th, 0, memoryTest(m, total) ? MM.trueRef : MM.falseRef);
	return 0;
}

int fun_threadCurrent(Thread* th)
{
	return STACKPUSH(th,PNTTOVAL(th));
}

int fun_threadCreate(Thread* th)
{
	Mem* m = (Mem*)VALTOPNT(STACKGET(th, 0));
	Thread* t=threadCreate(th, m);
	if (th->OM) return EXEC_OM;
//interpreterTRON=1;
	if (t)
	{
		t->listNext = MM.listThreads;
		MM.listThreads = t;
	}
	STACKSET(th,0, PNTTOVAL(t));
	return 0;
}


int fun_threadDump(Thread* th)
{
	Thread* t = (Thread*)VALTOPNT(STACKGET(th, 0));
	if (t) threadDump(LOG_USER,t,6);
	return 0;
}
int fun_threadNext(Thread* th)
{
	Thread* t = (Thread*)VALTOPNT(STACKGET(th, 0));
	STACKSET(th, 0, PNTTOVAL(t?t->listNext:MM.listThreads));		
	return 0;
}
int fun_threadUser(Thread* th)
{
	Thread* t = (Thread*)VALTOPNT(STACKGET(th, 0));
	if (t) STACKSET(th, 0, PNTTOVAL(t->user));
	return 0;
}
int fun_threadSetUser(Thread* th)
{
	LW user = STACKPULL(th);
	Thread* t = (Thread*)VALTOPNT(STACKGET(th, 0));
	if (t && ISVALPNT(user)) {
		t->user = VALTOPNT(user);
		MEMORYMARK((LB*)t,t->user);
	}
	return 0;
}

int fun_threadId(Thread* th)
{
	Thread* t = (Thread*)VALTOPNT(STACKGET(th, 0));
	if (t) STACKSETINT(th, 0, (t->uid));
	return 0;
}
int fun_threadPP(Thread* th)
{
	Thread* t = (Thread*)VALTOPNT(STACKGET(th, 0));
	if (t) STACKSETINT(th, 0, (t->pp));
	return 0;
}
int fun_threadOM(Thread* th)
{
	Thread* t = (Thread*)VALTOPNT(STACKGET(th, 0));
	if (t) STACKSET(th, 0, t->OM?MM.trueRef:MM.falseRef);
	return 0;
}
int fun_threadCount(Thread* th)
{
	Thread* t = (Thread*)VALTOPNT(STACKGET(th, 0));
	if (t) STACKSETINT(th, 0, (t->count));
	return 0;
}
int fun_threadClear(Thread* th)
{
	Thread* t = (Thread*)VALTOPNT(STACKGET(th, 0));
	if (!t) return 0;
//	printf("_threadClear %d from %d\n", t->uid, th->uid);
	if (t == th) return 0;	// should not happen
	stackReset(t);
	t->callstack = -1;	// this will prevent function callstack from failing
	t->stack = NULL;
	t->fun = NULL;
	STACKSETINT(th, 0, 0);
	return 0;
}
int fun_threadMemory(Thread* th)
{
	Thread* t = (Thread*)VALTOPNT(STACKGET(th, 0));
	if (t) STACKSET(th, 0, PNTTOVAL(t->header.mem));
	return 0;
}

int _printBuffer(Thread* th,Buffer* buffer)
{
	LB* p;
	Thread* t = (Thread*)VALTOPNT(STACKGET(th, 0));
	Mem* current = th->memDelegate;
	if (t) th->memDelegate = t->memDelegate;
	p=memoryPrintBuffer(th, buffer);
	th->memDelegate = current;
	STACKSET(th, 0, PNTTOVAL(p));
	return 0;
}
int fun_echoBuffer(Thread* th)
{
	return _printBuffer(th, MM.userBuffer);
}
int fun_consoleBuffer(Thread* th)
{
	return _printBuffer(th, MM.errBuffer);
}

int fun_threadRun(Thread* th)
{
	LINT NDROP=2-1;
	LW result=NIL;

	LINT maxCycles=VALTOINT(STACKGET(th,0));
	Thread* t=(Thread*)VALTOPNT(STACKGET(th,1));

	if (!t) goto cleanup;
	result = INTTOVAL(interpreterRun(t, maxCycles));

cleanup:
	STACKSETSAFE(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}

int fun_threadResume(Thread* th)
{
	LINT NDROP=2-1;
	LW result=NIL;

	LW arg=STACKGET(th,0);
	Thread* t=(Thread*)VALTOPNT(STACKGET(th,1));
	if (!t) goto cleanup;
//interpreterTRON=1;
	STACKPUSH_OM(t,arg,EXEC_OM);	// move onto the other thread
	result=INTTOVAL(0);

cleanup:
	STACKSETSAFE(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}

int fun_threadExec(Thread* th)
{
	LINT NDROP = 2 - 1;
	LW result = NIL;

	LW fun = STACKGET(th, 0);
	Thread* t = (Thread*)VALTOPNT(STACKGET(th, 1));

	if ((fun == NIL) || (!t)) goto cleanup;
	//interpreterTRON=1;
	STACKPUSH_OM(t, fun, EXEC_OM);	// move onto the other thread
	interpreterExec(t, 0, 0);	// make it ready to go
	result = INTTOVAL(0);

cleanup:
	STACKSETSAFE(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}

int fun_callstack(Thread* th)
{
	LINT n = 0;
	LINT callstack, pc;
	LB* fun;

	Thread* t = (Thread*)VALTOPNT(STACKGET(th,0));
	if (!t) t = th;
	callstack = t->callstack;
	pc = t->pc;
	fun = t->fun;
	while (callstack >= 0)
	{
		STACKPUSH_OM(th, TABGET(fun, FUN_USER_NAME),EXEC_OM);
		STACKPUSH_OM(th, INTTOVAL(pc),EXEC_OM);
		STACKPUSH_OM(th, TABGET(fun, FUN_USER_BC),EXEC_OM);
		STACKPUSH_OM(th, TABGET(fun, FUN_USER_PKG), EXEC_OM);
		if (DEFTAB(th, 4, DBG_TUPLE)) return EXEC_OM;
		n++;
		fun = VALTOPNT(STACKGETFROMREF(t, callstack, CALLSTACK_FUN));
		pc = VALTOINT(STACKGETFROMREF(t, callstack, CALLSTACK_PC));
		callstack = VALTOINT(STACKGETFROMREF(t, callstack, CALLSTACK_PREV));
	}
	STACKPUSH_OM(th, NIL,EXEC_OM);
	while ((n--) > 0) if (DEFTAB(th, LIST_LENGTH, DBG_LIST)) return EXEC_OM;
	STACKSKIP(th, 1);
	return 0;
}
int fun_caller(Thread* th)
{
	LINT callstack, pc;
	LB* fun;

	Thread* t = th;
	callstack = t->callstack;

	fun = VALTOPNT(STACKGETFROMREF(t, callstack, CALLSTACK_FUN));
	if (!fun) return STACKPUSH(th, NIL);
	pc = VALTOINT(STACKGETFROMREF(t, callstack, CALLSTACK_PC));
	callstack = VALTOINT(STACKGETFROMREF(t, callstack, CALLSTACK_PREV));
	STACKPUSH_OM(th, TABGET(fun, FUN_USER_NAME),EXEC_OM);
	STACKPUSH_OM(th, INTTOVAL(pc),EXEC_OM);
	STACKPUSH_OM(th, TABGET(fun, FUN_USER_BC),EXEC_OM);
	STACKPUSH_OM(th, TABGET(fun, FUN_USER_PKG), EXEC_OM);
	if (DEFTAB(th, 4, DBG_TUPLE)) return EXEC_OM;
	return 0;
}

int coreThreadInit(Thread* th, Pkg *system)
{
	Ref* Mem = pkgAddType(th, system, "MemManager");
	Ref* MemAuth = pkgAddType(th, system, "MemAuth");
	Type* u0=typeAllocUndef(th);
	Type* wUser = typeAllocWeak(th);
	Type* fun_u0=typeAlloc(th,TYPECODE_FUN,NULL,1,u0);
	Type* fun_Thread=typeAlloc(th,TYPECODE_FUN,NULL,1,MM.Thread);
	Type* fun_Thread_I = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Thread, MM.I);
	Type* fun_Mem_I = typeAlloc(th,TYPECODE_FUN, NULL, 2, Mem->type, MM.I);
	Type* fun_Mem_S = typeAlloc(th,TYPECODE_FUN, NULL, 2, Mem->type, MM.S);
	Type* fun_Mem_Mem = typeAlloc(th,TYPECODE_FUN, NULL, 2, Mem->type, Mem->type);
	Type* fun_Thread_Mem = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Thread,Mem->type);
	Type* fun_Mem_Thread = typeAlloc(th,TYPECODE_FUN, NULL, 2, Mem->type,MM.Thread);
	Type* fun_Mem_I_Bool = typeAlloc(th,TYPECODE_FUN, NULL, 3, Mem->type, MM.I, MM.Boolean);
	Type* fun_Mem_I_Mem = typeAlloc(th,TYPECODE_FUN, NULL, 3, Mem->type, MM.I, Mem->type);
	Type* fun_Mem_S_I_Mem = typeAlloc(th,TYPECODE_FUN, NULL, 4, Mem->type, MM.S, MM.I, Mem->type);
	Type* fun_Thread_B = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Thread, MM.Boolean);
	Type* fun_Thread_S = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Thread, MM.S);
	Type* fun_Thread_fun_u0_I=typeAlloc(th,TYPECODE_FUN,NULL,3,MM.Thread,fun_u0,MM.I);
	Type* fun_Thread_I_I=typeAlloc(th,TYPECODE_FUN,NULL,3,MM.Thread,MM.I,MM.I);
	Type* fun_Thread_u0_I=typeAlloc(th,TYPECODE_FUN,NULL,3,MM.Thread,u0,MM.I);
	Type* fun_S_I_S_Pkg = typeAlloc(th,TYPECODE_FUN, NULL, 1, typeAlloc(th,TYPECODE_TUPLE, NULL, 4, MM.S, MM.I, MM.S, MM.Pkg));
	Type* fun_Thread_list_S_I_S_Pkg = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Thread, typeAlloc(th,TYPECODE_LIST, NULL, 1, typeAlloc(th,TYPECODE_TUPLE, NULL, 4, MM.S, MM.I, MM.S, MM.Pkg)));


	pkgAddFun(th, system,"_threadCurrent",fun_threadCurrent,fun_Thread);
	pkgAddFun(th, system,"memoryAuthorize", fun_memoryAuthorize, typeAlloc(th,TYPECODE_FUN, NULL, 1, MemAuth->type));
	pkgAddFun(th, system, "_memoryAssign", fun_memoryAssign, typeAlloc(th,TYPECODE_FUN, NULL, 2, MemAuth->type, MemAuth->type));
	pkgAddFun(th, system, "memoryTake", fun_memoryTake, typeAlloc(th,TYPECODE_FUN, NULL, 3, MemAuth->type, u0, u0));

	pkgAddConst(th, system, "EXEC_IDLE", INTTOVAL(EXEC_IDLE), MM.I);
	pkgAddConst(th, system, "EXEC_PREEMPTION", INTTOVAL(EXEC_PREEMPTION), MM.I);
	pkgAddConst(th, system, "EXEC_WAIT", INTTOVAL(EXEC_WAIT), MM.I);
	pkgAddConst(th, system, "EXEC_EXIT", INTTOVAL(EXEC_EXIT), MM.I);
	pkgAddConst(th, system, "EXEC_OM", INTTOVAL(EXEC_OM), MM.I);

	pkgAddFun(th, system, "_callstack", fun_callstack, fun_Thread_list_S_I_S_Pkg);
	pkgAddFun(th, system, "caller", fun_caller, fun_S_I_S_Pkg);


	// these underscored functions are available only to the scheduler Thread
	pkgAddFun(th, system, "_memCreate", fun_memCreate, fun_Mem_S_I_Mem);
	pkgAddFun(th, system, "_memMemory", fun_memMemory, fun_Mem_I);
	pkgAddFun(th, system, "_memName", fun_memName, fun_Mem_S);
	pkgAddFun(th, system, "_memNext", fun_memNext, fun_Mem_Mem);
	pkgAddFun(th, system, "_memParent", fun_memParent, fun_Mem_Mem);
	pkgAddFun(th, system, "_memMaxMemory", fun_memMaxMemory, fun_Mem_I);
	pkgAddFun(th, system, "_memSetMaxMemory", fun_memSetMaxMemory, fun_Mem_I_Mem);
	pkgAddFun(th, system, "_memMemoryTest", fun_memoryTest, fun_Mem_I_Bool);
	pkgAddFun(th, system, "_memDump", fun_memoryDump, fun_Mem_Mem);

	pkgAddFun(th, system, "_threadCreate",fun_threadCreate,fun_Mem_Thread);
	pkgAddFun(th, system, "_threadMemory", fun_threadMemory, fun_Thread_Mem);
	pkgAddFun(th, system, "_threadId", fun_threadId, fun_Thread_I);
	pkgAddFun(th, system, "_threadPP", fun_threadPP, fun_Thread_I);	// for debug only
	pkgAddFun(th, system, "_threadCount", fun_threadCount, fun_Thread_I);
	pkgAddFun(th, system, "_threadClear", fun_threadClear, fun_Thread_I);
	pkgAddFun(th, system, "_threadOM", fun_threadOM, fun_Thread_B);
	pkgAddFun(th, system, "_echoBuffer", fun_echoBuffer, fun_Thread_S);
	pkgAddFun(th, system, "_consoleBuffer", fun_consoleBuffer, fun_Thread_S);
	pkgAddFun(th, system, "_threadExec",fun_threadExec,fun_Thread_fun_u0_I);
	pkgAddFun(th, system, "_threadRun",fun_threadRun,fun_Thread_I_I);

	pkgAddOpcode(th, system,"_threadHoldOn", OPholdon, fun_u0);
	pkgAddFun(th, system,"_threadResume",fun_threadResume,fun_Thread_u0_I);

	pkgAddFun(th, system, "_threadDump", fun_threadDump, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Thread, MM.Thread));
	pkgAddFun(th, system, "_threadNext", fun_threadNext, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Thread, MM.Thread));
	pkgAddFun(th, system, "_threadUser", fun_threadUser, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Thread, wUser));
	pkgAddFun(th, system, "_threadSetUser", fun_threadSetUser, typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.Thread, wUser, MM.Thread));

	return 0;
}
