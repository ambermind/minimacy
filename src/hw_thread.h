// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _MTHREAD_
#define _MTHREAD_

#ifdef USE_THREAD_STUB
#define MSEM int
#define MTHREAD int
#define MTHREAD_ID int
#define MLOCK int
#define MTHREAD_START void*
#define MTHREAD_RETURN_TYPE void*
#define MTHREAD_RETURN NULL
#endif
#ifdef USE_THREAD_UNIX
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

#ifdef USE_THREAD_WIN
#define MTHREAD HANDLE
#define MTHREAD_ID DWORD
#define MTHREAD_START int
//#define MTHREAD_START DWORD WINAPI
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
