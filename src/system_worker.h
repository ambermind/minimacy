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

int workerStart(Thread* th, int argc, void* start);
MTHREAD_RETURN_TYPE workerDoneNil(Thread* th);
MTHREAD_RETURN_TYPE workerDonePnt(Thread* th, LB* result);
MTHREAD_RETURN_TYPE workerDoneInt(Thread* th, LINT result);
LB* workerAllocExt(Thread* th, LINT sizeofExt, LW dbg, FORGET forget, MARK mark);
int workerBiggerBuffer(Thread* th, Buffer* buffer, LINT newSize);

int sysWorkerInit(Thread* th, Pkg* system);
#endif
