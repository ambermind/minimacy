// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

#ifdef USE_TIME_ANSI
LINT hwTime(void)
{
	time_t x;
	time(&x);
	return (LINT)x;
}
#endif
#ifdef USE_TIME_MS_WIN
#define LocalTickCount GetTickCount64
LINT DeltaTimeMs = 0;
LARGE_INTEGER CurrentTimer;
LARGE_INTEGER FreqTimer;

LINT hwTimeMs(void)
{
	long long p;
	if (!FreqTimer.QuadPart) return DeltaTimeMs + (LINT)LocalTickCount();
	QueryPerformanceCounter(&CurrentTimer);
	p=DeltaTimeMs + CurrentTimer.QuadPart * 1000 / FreqTimer.QuadPart;
	return (LINT)p;

}
void hwSleepMs(LINT ms)
{
	Sleep((DWORD)ms);
}
void hwTimeInit()
{
	long long p;
	if (!QueryPerformanceFrequency(&FreqTimer)) FreqTimer.QuadPart = 0;
	if (FreqTimer.QuadPart)
	{
		QueryPerformanceCounter(&CurrentTimer);
		p = hwTime() * 1000 - CurrentTimer.QuadPart * 1000 / FreqTimer.QuadPart;
	}
	else p = hwTime() * 1000 - LocalTickCount();
	DeltaTimeMs = (LINT)p;
}
#endif

#ifdef USE_TIME_MS_ANSI
LINT hwTimeMs(void)
{
	LINT t;
	struct timeval tm;

	gettimeofday(&tm, NULL);
	t = (tm.tv_sec * 1000) + (tm.tv_usec / 1000);
	return t;
}
void hwSleepMs(LINT ms)
{
	struct timeval tm;
	int d = (int)ms;
	tm.tv_sec = d / 1000;
	tm.tv_usec = (int)((d - (1000 * tm.tv_sec)) * 1000);
	select(1, NULL, NULL, NULL, &tm);
}
void hwTimeInit(void)
{
}
#endif

#ifdef USE_TIME_STUB
LINT fakeTime = 0;
LINT hwTime()
{
	fakeTime += 10;
	return fakeTime/1000;
}
#endif
#ifdef USE_TIME_MS_STUB
LINT hwTimeMs()
{
	fakeTime+=10;
	return fakeTime;
}
void hwTimeInit()
{
}
#endif
