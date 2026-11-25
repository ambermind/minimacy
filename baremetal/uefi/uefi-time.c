// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"inc/efi.h"
#include"inc/efilib.h"
#include"../../src/minimacy.h"
#include"uefi.h"

typedef UINT64 (EFIAPI *TIMESTAMP_GET) (
	VOID
);
typedef struct {
	UINT64 Frequency;
	UINT64 EndValue;
} EFI_TIMESTAMP_PROPERTIES;
typedef EFI_STATUS (EFIAPI *TIMESTAMP_GET_PROPERTIES) (
	OUT EFI_TIMESTAMP_PROPERTIES *Properties
);
typedef struct {
	TIMESTAMP_GET GetTimestamp;
	TIMESTAMP_GET_PROPERTIES GetProperties;
} EFI_TIMESTAMP_PROTOCOL;

EFI_TIMESTAMP_PROTOCOL* EfiTimestamp=NULL;

LINT TsClock0=0;
LINT TsClockFreq=1;

unsigned long long rdtsc(void)
{
	unsigned hi, lo;
	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
	return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

unsigned long long tscFreqCompute()
{
	long long t0=rdtsc();
	BS->Stall(1000);
	t0=rdtsc()-t0;
	return t0;
}

const int _MonthsFirstDay[]={0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
int _YearCache=0;;
long long _YearTimeCache=0;;
long long uefiSystemTime(EFI_TIME *tm)
{
	long long t=_YearTimeCache;
	if (tm->Year!=_YearCache) {
		int year=tm->Year;
		t=0;
		year-=1968;
		while(year>2) t+=86400*((year--)&3?365:366);
		_YearCache=tm->Year;
		_YearTimeCache=t;
	}
	t+=(tm->Day-1)*86400 + tm->Hour*3600 + tm->Minute*60 + tm->Second;
	if (tm->Month<1 || tm->Month>12) return 0;
	t+=86400*_MonthsFirstDay[tm->Month-1];
	if ((tm->Month>=3) && !(tm->Year&3)) t+=86400;
	return t;
}

LINT hwTimeMs()
{
	return ((EfiTimestamp?EfiTimestamp->GetTimestamp():rdtsc())-TsClock0)/TsClockFreq;
}

LINT hwTime()
{
	EFI_STATUS Status;
	EFI_TIME Time;
	Status=ST->RuntimeServices->GetTime(&Time,NULL);
	if (!Status) return uefiSystemTime(&Time);
	return hwTimeMs()/1000;
}

void hwTimeInit(){}

void timeInit()
{
	EFI_STATUS Status;
	EFI_TIME Time;
	EfiTimestamp=NULL;
	// Timestamp is a free counter of clock ticks, not the real date
	Status = BS->LocateProtocol(&gEfiTimestampProtocolGuid,NULL,(void**)&EfiTimestamp);
	if (!Status) {
		EFI_TIMESTAMP_PROPERTIES prop;
		EfiTimestamp->GetProperties(&prop);
		TsClock0    = EfiTimestamp->GetTimestamp();
		TsClockFreq = prop.Frequency/1000;
		PRINTF(LOG_SYS,"> UEFI: system timeMs(), freq= "LSD" kHz\n",TsClockFreq);
	}
	else 
	{
		TsClock0    = rdtsc();
		TsClockFreq = tscFreqCompute();
		PRINTF(LOG_SYS,"> UEFI: software timeMs(), freq= "LSD" kHz\n",TsClockFreq);
	}
	if (TsClockFreq<=0) TsClockFreq=1;

	// GetTime has the system date
	Status=ST->RuntimeServices->GetTime(&Time,NULL);
	if (!Status) PRINTF(LOG_SYS,"> UEFI: system time(): %d\n",uefiSystemTime(&Time));
	else PRINTF(LOG_SYS,"> UEFI: no system time()\n");
}
