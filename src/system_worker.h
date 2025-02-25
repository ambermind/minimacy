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

int workerStart(Thread* th, int argc, void* start);
WORKER_RETURN_TYPE workerDoneNil(volatile Thread* th);
WORKER_RETURN_TYPE workerDonePnt(volatile Thread* th, LB* result);
WORKER_RETURN_TYPE workerDoneInt(volatile Thread* th, LINT result);
int workerAllocExt(volatile Thread** pth, LINT sizeofExt, LW dbg, FORGET forget, MARK mark);
int workerBiggerBuffer(volatile Thread** pth, Buffer* buffer, LINT newSize);
void workerWaitUntilAllInactive(void);

int systemWorkerInit(Pkg* system);
#endif
