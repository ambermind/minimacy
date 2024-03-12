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
//			PRINTF(LOG_DEV,"memCountDecrement "LSX" from "LSX": "LSD" count:%lld\n", mem, mem->header.mem, mem->bytes, mem->count);
//			PRINTF(LOG_DEV,"\n");
//		}
		if (mem->count < 1)
		{
			MM.blocs_nb--;
			MM.blocs_length -= sizeof(LB) + (((HEADER_SIZE((LB*)mem) + LWLEN - 1) >> LSHIFT) << LSHIFT);
//			PRINTF(LOG_DEV,"memCountDecrement Rec "LSX" from "LSX": "LSD" count:%lld\n", mem->header.mem, mem, mem->bytes, mem->count);
			memCountDecrement(mem->header.mem);
			//if (mem == MemCheck) 
//			PRINTF(LOG_DEV,"\nmemFree "LSX" count:%lld\n", mem,  mem->count);
			VM_FREE((void*)mem);
		}
	}
}

int memForget(LB* p)
{
	Mem* mem = (Mem*)p;
//	if (mem== MemCheck) PRINTF(LOG_DEV,"memForget "LSX" count:%lld\n", mem, mem->count);
	memCountDecrement(mem);
//	if (mem == MemoryCheck) PRINTF(LOG_DEV,"release MemoryCheck ->%d\n", mem->bytes);
	return -1;	// never let the GC desalloc itself the mem structure
}
void memMark(LB* user)
{
	Mem* mem = (Mem*)user;
	MEMORY_MARK(user, (LB*)mem->name);
}
Mem* memCreate(Thread* th, Mem* parent,LINT maxBytes,LB* name)
{
	Mem* mem = (Mem*)memoryAllocExt(th, sizeof(Mem), DBG_MEMCOUNT, memForget, memMark); if (!mem) return NULL;
	mem->name = name;
	mem->maxBytes = (maxBytes>=0) ? maxBytes : (parent?parent->maxBytes:0);
	mem->bytes = 0;
	mem->count = 1;
	if (MM.mem && !MemCheck) MemCheck = mem;
//	PRINTF(LOG_DEV,"\nmemCreate ------------------- %llx\n", mem);
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
	PRINTF(LOG_DEV,"%s threads MM.USELESS="LSX"\n", title,MM.USELESS);
	while (th)
	{
		PRINTF(LOG_DEV,"-> "LSX":"LSD"  lifo="LSX"\n", th, th->uid, th->header.lifo);
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
	for(i=0;i<=th->sp;i++) if (ARRAY_IS_PNT(p,i)) MEMORY_MARK(p, ARRAY_PNT(p,i));

	MEMORY_MARK(user, (LB*)th->memDelegate);
	MEMORY_MARK(user,th->user);
	// there is no need to mark any pending worker's result pointer, as these pointers are
	// stored in the stack by workerAllocExt before they are declared as the worker's result
}

int threadForget(LB* p)
{
	Thread* th = (Thread*)p;
//	PRINTF(LOG_DEV,"threadForget "LSD"\n", th->uid);
//	PRINTF(LOG_DEV,"threadForget "LSX":"LSD"\n", th, th->uid);
	semDelete(&th->worker.sem);

	th->stack = NULL;
//	if (th->header.mem == MemoryCheck) PRINTF(LOG_DEV,"release thread of MemoryCheck ->%d\n", th->header.mem->bytes);
	return 0;
}

Thread* threadCreate(Thread* th0, Mem* mem)
{
	Thread* th;
	memoryEnterFast();
	th=(Thread*)memoryAllocExt(th0, sizeof(Thread),DBG_THREAD, threadForget,threadMark); if (!th) return NULL;
	th->uid=ThreadCounter++;
	th->count = 0;
	th->worker.state = WORKER_READY;
	semCreate(&th->worker.sem);
	th->callstack=-1;
	th->pc=0;
	th->fun=NULL;
	th->forceOpcode=THREAD_OPCODE_NONE;
	th->OM = 0;

	memoryTake(mem, (LB*)th);
	th->memDelegate = mem;

	th->user = NULL;

	th->atomic = 0;
	th->sp=-1;
	th->stack = NULL;	// actually useless due to the th->sp=-1, but meaningfull

	th->stack= memoryAllocArray(th, THREAD_STACK_LENGTH0,DBG_STACK);
	if (!th->stack) return NULL;
	th->listNext = NULL;
	threadMark((LB*)th);
//	PRINTF(LOG_DEV,"threadCreate %llx %llx\n",th,th->stack);
//	PRINTF(LOG_DEV,"threadCreate %llx\n",th->uid);
	memoryLeaveFast();
	return th;
}

int threadBigger(Thread* th)
{
	LINT i;
	LINT size= ARRAY_LENGTH(th->stack);
	LB* newstack= memoryAllocArray(th, size+ THREAD_STACK_STEP,DBG_STACK);
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
	LB* p = memoryAllocArray(th, size, dbg); if (!p) return EXEC_OM;
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
	LB* p = memoryAllocStr(th, src, size); if (!p) return EXEC_OM;
	FUN_PUSH_PNT(p);
	return 0;
}

int stackSetStr(Thread* th, LINT i, char* src,LINT size)
{
	LB* p = memoryAllocStr(th, src, size); if (!p) return EXEC_OM;
	STACK_SET_PNT(th,0, p);
	return 0;
}

int stackSetBuffer(Thread* th, LINT i, Buffer* b)
{
	LB* p = memoryAllocFromBuffer(th, b); if (!p) return EXEC_OM;
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

int fun_memCreate(Thread* th)
{
	LINT wMaxBytes = STACK_INT(th, 0);
	LB* name= STACK_PNT(th, 1);
	Mem *parent = (Mem*)STACK_PNT(th, 2);
	Mem *child = memCreate(th, parent, STACK_IS_NIL(th, 0)?-1:wMaxBytes,name); if (!child) return EXEC_OM;
	FUN_RETURN_PNT((LB*)child);
}
int fun_memName(Thread* th)
{
	Mem* m = (Mem*)STACK_PNT(th, 0);
	if (m) STACK_SET_PNT(th, 0, m->name);
	return 0;
}
int fun_memMemory(Thread* th)
{
	Mem* m = (Mem*)STACK_PNT(th, 0);
	if (m) STACK_SET_INT(th, 0, m->bytes);
	return 0;
}

int fun_memMaxMemory(Thread* th)
{
	Mem* m = (Mem*)STACK_PNT(th, 0);
	if (m) STACK_SET_INT(th, 0, m->maxBytes);
	return 0;
}
int fun_memSetMaxMemory(Thread* th)
{
	LINT maxBytes = STACK_PULL_INT(th);
	Mem* m = (Mem*)STACK_PNT(th, 0);
	if (m && (maxBytes > 0)) m->maxBytes = maxBytes;
	return 0;
}
int fun_memParent(Thread* th)
{
	Mem* m = (Mem*)STACK_PNT(th, 0);
	STACK_SET_PNT(th, 0, (LB*)(m?m->header.mem:NULL));
	return 0;
}
int fun_memNext(Thread* th)
{
	Mem* m = (Mem*)STACK_PNT(th, 0);
	STACK_SET_PNT(th, 0, (LB*)(m?m->listNext:MM.listMems));
	return 0;
}

int fun_memoryAuthorize(Thread* th)
{
	FUN_PUSH_PNT((LB*)th->header.mem);
	return 0;
}

int fun_memoryAssign(Thread* th)
{
	Mem* previous = th->memDelegate;
	Mem* mem = (Mem*)STACK_PNT(th, 0);
	if (!mem) mem = th->header.mem;
	th->memDelegate = mem;
	//	PRINTF(LOG_DEV,"*************_memoryAssign:\n");
	//	PRINTF(LOG_DEV,"_memoryAssign "LSX": " LSD" delegate to "LSX" :" LSD"\n", th, th->uid, MM.thread, MM.thread->uid);
	STACK_SET_PNT(th, 0, (LB*)previous);
	return 0;
}

int fun_memoryTake(Thread* th)
{
	LB* arg = STACK_PNT(th, 0);
	Mem* m = (Mem*)STACK_PNT(th, 1);
	if (m && STACK_IS_PNT(th,0)) memoryTake(m, arg);
	STACK_SKIP(th, 1);
	return 0;
}

int fun_threadCurrent(Thread* th)
{
	FUN_PUSH_PNT((LB*)th);
	return 0;
}

int fun_threadCreate(Thread* th)
{
	Mem* m = (Mem*)STACK_PNT(th, 0);
	Thread* t=threadCreate(th, m);
	if (th->OM) return EXEC_OM;
//interpreterTRON=1;
	if (t)
	{
		t->listNext = MM.listThreads;
		MM.listThreads = t;
	}
	STACK_SET_PNT(th,0, (LB*)t);
	return 0;
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
		MEMORY_MARK((LB*)t,t->user);
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
int fun_threadOM(Thread* th)
{
	Thread* t = (Thread*)STACK_PNT(th, 0);
	if (t) STACK_SET_BOOL(th, 0, t->OM);
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
int fun_threadMemory(Thread* th)
{
	Thread* t = (Thread*)(STACK_PNT(th, 0));
	if (t) STACK_SET_PNT(th, 0, (LB*)(t->header.mem));
	return 0;
}

int _printBuffer(Thread* th,Buffer* buffer)
{
	LB* p;
	Thread* t = (Thread*)(STACK_PNT(th, 0));
	Mem* current = th->memDelegate;
	if (t) th->memDelegate = t->memDelegate;
	p=memoryPrintBuffer(th, buffer);
	th->memDelegate = current;
	STACK_SET_PNT(th, 0, (p));
	return 0;
}

int fun_threadRun(Thread* th)
{
	LINT maxCycles=STACK_INT(th,0);
	Thread* t=(Thread*)STACK_PNT(th,1);

	if (!t) FUN_RETURN_INT(-1);
	FUN_RETURN_INT(interpreterRun(t, maxCycles));
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
		FUN_PUSH_PNT( ARRAY_PNT(fun, FUN_USER_PKG));
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
	FUN_PUSH_PNT( ARRAY_PNT(fun, FUN_USER_PKG));
	FUN_MAKE_ARRAY( 4, DBG_TUPLE);
	return 0;
}

int coreThreadInit(Thread* th, Pkg *system)
{
	Def* Mem = pkgAddType(th, system, "MemManager");
	Def* MemAuth = pkgAddType(th, system, "MemAuth");
	Type* u0=typeAllocUndef(th);
	Type* wUser = typeAllocWeak(th);
	Type* fun_u0=typeAlloc(th,TYPECODE_FUN,NULL,1,u0);
	Type* fun_Thread=typeAlloc(th,TYPECODE_FUN,NULL,1,MM.Thread);
	Type* fun_Thread_I = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Thread, MM.Int);
	Type* fun_Mem_I = typeAlloc(th,TYPECODE_FUN, NULL, 2, Mem->type, MM.Int);
	Type* fun_Mem_S = typeAlloc(th,TYPECODE_FUN, NULL, 2, Mem->type, MM.Str);
	Type* fun_Mem_Mem = typeAlloc(th,TYPECODE_FUN, NULL, 2, Mem->type, Mem->type);
	Type* fun_Thread_Mem = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Thread,Mem->type);
	Type* fun_Mem_Thread = typeAlloc(th,TYPECODE_FUN, NULL, 2, Mem->type,MM.Thread);
	Type* fun_Mem_I_Mem = typeAlloc(th,TYPECODE_FUN, NULL, 3, Mem->type, MM.Int, Mem->type);
	Type* fun_Mem_S_I_Mem = typeAlloc(th,TYPECODE_FUN, NULL, 4, Mem->type, MM.Str, MM.Int, Mem->type);
	Type* fun_Thread_B = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Thread, MM.Boolean);
	Type* fun_Thread_fun_u0_I=typeAlloc(th,TYPECODE_FUN,NULL,3,MM.Thread,fun_u0,MM.Int);
	Type* fun_Thread_I_I=typeAlloc(th,TYPECODE_FUN,NULL,3,MM.Thread,MM.Int,MM.Int);
	Type* fun_Thread_u0_I=typeAlloc(th,TYPECODE_FUN,NULL,3,MM.Thread,u0,MM.Int);
	Type* fun_S_I_S_Pkg = typeAlloc(th,TYPECODE_FUN, NULL, 1, typeAlloc(th,TYPECODE_TUPLE, NULL, 4, MM.Str, MM.Int, MM.Str, MM.Package));
	Type* fun_Thread_list_S_I_S_Pkg = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Thread, typeAlloc(th,TYPECODE_LIST, NULL, 1, typeAlloc(th,TYPECODE_TUPLE, NULL, 4, MM.Str, MM.Int, MM.Str, MM.Package)));


	pkgAddFun(th, system,"_threadCurrent",fun_threadCurrent,fun_Thread);
	pkgAddFun(th, system,"memoryAuthorize", fun_memoryAuthorize, typeAlloc(th,TYPECODE_FUN, NULL, 1, MemAuth->type));
	pkgAddFun(th, system, "_memoryAssign", fun_memoryAssign, typeAlloc(th,TYPECODE_FUN, NULL, 2, MemAuth->type, MemAuth->type));
	pkgAddFun(th, system, "memoryTake", fun_memoryTake, typeAlloc(th,TYPECODE_FUN, NULL, 3, MemAuth->type, u0, u0));

	pkgAddConstInt(th, system, "EXEC_IDLE", EXEC_IDLE, MM.Int);
	pkgAddConstInt(th, system, "EXEC_PREEMPTION", EXEC_PREEMPTION, MM.Int);
	pkgAddConstInt(th, system, "EXEC_WAIT", EXEC_WAIT, MM.Int);
	pkgAddConstInt(th, system, "EXEC_EXIT", EXEC_EXIT, MM.Int);
	pkgAddConstInt(th, system, "EXEC_OM", EXEC_OM, MM.Int);

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

	pkgAddFun(th, system, "_threadCreate",fun_threadCreate,fun_Mem_Thread);
	pkgAddFun(th, system, "_threadMemory", fun_threadMemory, fun_Thread_Mem);
	pkgAddFun(th, system, "_threadId", fun_threadId, fun_Thread_I);
	pkgAddFun(th, system, "_threadPP", fun_threadPP, fun_Thread_I);	// for debug only
	pkgAddFun(th, system, "_threadCount", fun_threadCount, fun_Thread_I);
	pkgAddFun(th, system, "_threadClear", fun_threadClear, fun_Thread_I);
	pkgAddFun(th, system, "_threadOM", fun_threadOM, fun_Thread_B);
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
