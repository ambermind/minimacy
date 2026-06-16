// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _SYSTEM_WORKER_
#define _SYSTEM_WORKER_

#ifdef USE_WORKER_ASYNC
#define WORKER_START MTHREAD_START
#define WORKER_RETURN_TYPE MTHREAD_RETURN_TYPE
#define WORKER_RETURN MTHREAD_RETURN
#else
#define WORKER_START int
#define WORKER_RETURN_TYPE int
#define WORKER_RETURN 0
#endif

#define WORKER_READY 0
#define WORKER_RUN 1
#define WORKER_DONE 2
#define WORKER_BIGGER_BUFFER 3

#define WORKER_RESULT_NIL 0
#define WORKER_RESULT_INT 1
#define WORKER_RESULT_BOOL 2

#ifndef USE_WORKER_ASYNC
extern char WorkerScratchpad[1024 * 34];
#endif

int workerStart(Thread* th, int argc, void* start);
Thread* workerThread(Worker* w);
WORKER_RETURN_TYPE workerDoneNil(Worker* w);
WORKER_RETURN_TYPE workerDoneBool(Worker* w, LINT result);
WORKER_RETURN_TYPE workerDoneInt(Worker* w, LINT result);
int workerBiggerBuffer(LINT fixedRootBuffer, LINT newSize);
void workerWaitUntilAllInactive(void);

int systemWorkerInit(Pkg* system);
#endif
