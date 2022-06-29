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
#ifndef _THREAD_
#define _THREAD_

extern LINT ThreadCounter;

#define WORKER_READY 0
#define WORKER_RUN 1
#define WORKER_DONE 2
#define WORKER_ALLOC_STR 3
#define WORKER_ALLOC_EXT 4
#define WORKER_MAKE_LIST 5
#define WORKER_MAKE_BITMAP 6
#define WORKER_BIGGER_BUFFER 7

typedef struct {
	volatile LINT state;
	LINT pp;
	LINT allocSize;
	LINT w;
	LINT h;
	MSEM sem;
	Buffer* buffer;
	LW result;
	LW dbg;
	FORGET forget;
	MARK mark;
} Worker;

struct Thread
{
	LB header;
	FORGET forget;
	MARK mark;

	LINT uid;
	unsigned long long seed;

	LINT count;
	LINT callstack;
	LINT pc;
	LB* fun;
	char forceOpcode;
	char OM;	// 0 until Out of Memory error
	char atomic;

	Mem* memDelegate;

	Thread* listNext;

	Worker worker;

	LB* user;

	LINT pp;
	LB* stack;
};


#define THREAD_STACK_LENGTH0 1024
#define THREAD_STACK_STEP (1024*16)

#define THREAD_OPCODE_NONE ((char)-1)

#define EXEC_IDLE 0
#define EXEC_PREEMPTION 1
#define EXEC_WAIT 2
#define EXEC_EXIT 3
#define EXEC_OM 4

#define EXEC_FORMAT 5



#define STACKGET(th,i) TABGET((th)->stack,(th)->pp-(i))
#define STACKSET(th,i,val) TABSET((th)->stack,(th)->pp-(i),val)
#define STACKSETINT(th,i,val) TABSETINT((th)->stack,(th)->pp-(i),val)
#define STACKSETFLOAT(th,i,val) TABSETFLOAT((th)->stack,(th)->pp-(i),val)
#define STACKSETNIL(th,i) TABSETNIL((th)->stack,(th)->pp-(i))

#define STACKSETSAFE(th,i,val) TABSETSAFE((th)->stack,(th)->pp-(i),val)	// idem STACKSET, use only when val is not a pointer (NIL or integer or float)
#define STACKPULL(th) TABGET((th)->stack,(th)->pp--)
#define STACKDROP(th) ((th)->pp--)
#define STACKDROPN(th,n) ((th)->pp-=(n))
#define STACKPUSH(th,val) (stackPush(th,val))
#define STACKPUSH_OM(th,val,err) { \
	TABSET((th)->stack,++(th)->pp,val); \
	if (((th)->pp >= TABLEN((th)->stack) - 1)&&threadBigger(th)) return err; \
}

#define STACKREF(th) ((th)->pp)
#define STACKGETFROMREF(th,size,i) TABGET((th)->stack,(size)-(i))
#define STACKSETFROMREF(th,size,i,val) TABSET((th)->stack,(size)-(i),val)
#define STACKSETFROMREFSAFE(th,size,i,val) TABSETSAFE((th)->stack,(size)-(i),val)
#define STACKSKIP(th,n) { STACKSETSAFE(th, n, STACKGET(th, 0));	(th)->pp -= (n);}

#define TABLEPUSH(th,size,dbg) stackPushTable(th,size,dbg)
#define DEFTAB(th,size,dbg) (stackDeftab(th,size,dbg))


Thread* threadCreate(Thread* th, Mem* mem);
int threadBigger(Thread* th);
void stackReset(Thread* th);

int stackPush(Thread* th,LW val);

int stackPushTable(Thread* th,LINT size,LW dbg);
int stackDeftab(Thread* th,LINT size,LW dbg);
int stackPushStr(Thread* th,char* src,LINT size);

void threadPrintCallstack(Thread* t);

int coreThreadInit(Thread* th, Pkg *system);

#endif
