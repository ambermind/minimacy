// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

#ifdef USE_THREAD_UNIX

MTHREAD hwThreadCreate(MTHREAD_START start, void* user)
{
	MTHREAD thread;
	pthread_attr_t attr;
	int ret;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	ret = pthread_create(&thread, &attr, start, user);
	//	ret = pthread_create(&thread, NULL, start, user);
	if (ret)
	{
//		PRINTF("unable to create thread err=%d\n", ret);
		return 0;
	}
	return (MTHREAD)thread;
}
MTHREAD_ID hwThreadId(void)
{
	return pthread_self();
}
void lockCreate(MLOCK* lock)
{
	pthread_mutexattr_t mta;
	pthread_mutexattr_init(&mta);
	//	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);	//TODO : creuser (necessite __USE_UNIX98, et autorise le lock d'un verrou deja locke par soi-meme)
	pthread_mutex_init(lock, &mta);
	pthread_mutexattr_destroy(&mta);
}
void lockEnter(MLOCK* lock)
{
	pthread_mutex_lock(lock);
}
void lockLeave(MLOCK* lock)
{
	pthread_mutex_unlock(lock);
}
void lockDelete(MLOCK* lock)
{
	pthread_mutex_destroy(lock);
}

void semCreate(MSEM* sem)
{
#ifndef DEPRECATED_SEM
	sem_init(sem, 0, 0);
#endif
}
void semP(MSEM* sem)
{
#ifndef DEPRECATED_SEM
	sem_wait(sem);
#endif
}
void semV(MSEM* sem)
{
#ifndef DEPRECATED_SEM
	sem_post(sem);
#endif
}
void semDelete(MSEM* sem)
{
#ifndef DEPRECATED_SEM
	sem_destroy(sem);
#endif
}

#endif
#ifdef USE_THREAD_WIN

MTHREAD hwThreadCreate(void* start, void* user)
{
	MTHREAD thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start, user, 0, NULL);
	return thread;
}
MTHREAD_ID hwThreadId()
{
	return GetCurrentThreadId();
}
void lockCreate(MLOCK* lock)
{
	InitializeCriticalSection(lock);
	//	PRINTF(LOG_DEV,"lockCreate %x\n",lock);
}
void lockEnter(MLOCK* lock)
{
	//	PRINTF(LOG_DEV,"lockEnter %x\n",lock);
	EnterCriticalSection(lock);
}
void lockLeave(MLOCK* lock)
{
	//	PRINTF(LOG_DEV,"lockLeave %x\n",lock);
	LeaveCriticalSection(lock);
}
void lockDelete(MLOCK* lock)
{
	//	PRINTF(LOG_DEV,"lockDelete %x\n",lock);
	DeleteCriticalSection(lock);
}
void semCreate(MSEM* sem)
{
	*sem = CreateSemaphore(NULL, 0, 1, NULL);
}
void semP(MSEM* sem)
{
	WaitForSingleObject(*sem, INFINITE);
}
void semV(MSEM* sem)
{
	ReleaseSemaphore(*sem, 1, NULL);
}
void semDelete(MSEM* sem)
{
	CloseHandle(*sem);
}
#endif
#ifdef USE_THREAD_STUB
MTHREAD hwThreadCreate(void* start, void* user) { return 0; }
MTHREAD_ID hwThreadId() { return 0; }

void lockCreate(MLOCK* lock) {}
void lockEnter(MLOCK* lock) {}
void lockLeave(MLOCK* lock) {}
void lockDelete(MLOCK* lock) {}

void semCreate(MSEM* sem) {}
void semP(MSEM* sem) {}
void semV(MSEM* sem) {}
void semDelete(MSEM* sem) {}
#endif
