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
#ifndef _MTHREAD_
#define _MTHREAD_

#ifdef ON_UNIX
#include <pthread.h>
#include <semaphore.h>
#define MTHREAD pthread_t
#define MTHREAD_ID pthread_t
#define MTHREAD_START void*
#define MTHREAD_RETURN_TYPE void*
#define MTHREAD_RETURN NULL
#define MLOCK pthread_mutex_t
#define MSEM sem_t
#endif

#ifdef ON_WINDOWS
#define MTHREAD HANDLE
#define MTHREAD_ID DWORD
#define MTHREAD_START DWORD WINAPI
#define MTHREAD_RETURN_TYPE int
#define MTHREAD_RETURN 0
#define MLOCK CRITICAL_SECTION
#define MSEM HANDLE
#endif

MTHREAD hwThreadCreate(void* start,void* user);
MTHREAD_ID hwThreadId(void);

void lockCreate(MLOCK* lock);
void lockEnter(MLOCK* lock);
void lockLeave(MLOCK* lock);
void lockDelete(MLOCK* lock);

void semCreate(MSEM* sem);
void semP(MSEM* sem);
void semV(MSEM* sem);
void semDelete(MSEM* sem);

#endif
