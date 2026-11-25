// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include "../../src/minimacy.h"
#include "raspberry-pi.h"

// get addresses from linker
extern volatile unsigned char _end;

extern unsigned long UartClock;

void getBoardInfo()
{
	unsigned int start,size;
#ifndef ON_RPI5
	//PRINTF(LOG_SYS,"> Board model: %x\n",mboxBoardModel());
	PRINTF(LOG_SYS,"> Board rev  : %x\n",mboxBoardRevision());
	PRINTF(LOG_SYS,"> MAC        : %llx\n",mboxMac());
#endif
	PRINTF(LOG_SYS,"> Serial     : %llx\n",mboxSerial());
	size=mboxVcMemory(&start);
	PRINTF(LOG_SYS,"> VC memory  : "LSD" bytes (0x"LSX" - 0x"LSX")\n", size, start, start+size);
	//size=mboxArmMemory(&start);
	//PRINTF(LOG_SYS,"> ARM memory : %x (%x)\n",start, size);
	//PRINTF(LOG_SYS,"> Uart clock : %d Hz\n",UartClock);

//	PRINTF(LOG_SYS,"> _end       : %llx\n",(long long)(void*)&_end);
}

void setArmClocks()
{
	unsigned long rBefore=mboxGetClockRate(CLOCK_ID_ARM);
	unsigned long rMax=mboxGetMaxClockRate(CLOCK_ID_ARM);
//	mboxSetClockRate(CLOCK_ID_ARM, rMax);
	hwSleepMs(200);
	unsigned long rAfter=mboxGetClockRate(CLOCK_ID_ARM);
	PRINTF(LOG_SYS,"> Clock rate : %lld Hz (max %lld Hz) -> %lld Hz\n",rBefore, rMax, rAfter);
}

void ramSize(void)
{
	unsigned int ram;
	unsigned long long k=0x20000000;
	for(ram=0;ram<8;ram++) {
		volatile int* p;
//		PRINTF(LOG_SYS,"> store %d\n",i);
		p=(int*)k;
		p[0]=123;
//		PRINTF(LOG_SYS,"> read %d\n",i);
//		PRINTF(LOG_SYS,"> = %d\n",p[0]);
		if (p[0]!=123) break;		
		k+=0x40000000;
	}
	PRINTF(LOG_SYS,"> RAM        : %d GB\n",ram);
	bmmUpdateCount(1);
	switch(ram){
		case 2:
			bmmUpdateCount(2);
			bmmUpdateSize(1,0x40000000);
		break;
		case 4:
			bmmUpdateCount(2);
			bmmUpdateSize(1,0xb0000000);
		break;
		case 8:
			bmmUpdateCount(3);
			bmmUpdateSize(1,0xb0000000);
			bmmUpdateSize(2,0x100000000);
		break;
	}
}

void idleInit(void)
{
    unsigned long freq;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));          // 54 MHz on Pi 4

    // Event period = 2^(evnti+1) ticks: pick the largest period not above 1 ms
    unsigned evnti = 0;
    while (evnti < 15 && (1UL << (evnti + 2)) <= freq / 1000) evnti++;

    // Same layout in both registers: EVNTEN = bit 2, EVNTDIR = bit 3, EVNTI = bits 7:4
    unsigned long v;
	__asm__ volatile("mrs %0, cnthctl_el2" : "=r"(v));
	v = (v & ~0xF8UL) | (1UL << 2) | ((unsigned long)evnti << 4);
	__asm__ volatile("msr cnthctl_el2, %0; isb" : : "r"(v));
}

LINT lastActive=0;
void selectPollingTemporisation(LINT nbActive, LINT now, LINT maxWait)
{
	if (nbActive) lastActive=now;
	else if ((now-lastActive>10)&&(maxWait>=1)) {
		__asm__ volatile("wfe");
	}
}

#ifdef ON_RPI5
const char* Argv[]={"minimacy",""};
#else
//const char* Argv[]={"minimacy",""};
const char* Argv[]={"minimacy","/baremetal/baremetal.rpi4.boot.mcy"};
#endif
void main()
{
	//set up serial console
	uartInit();
	termInit();
	PRINTF(LOG_SYS,"\n");
	ramSize();	// must be called before MMU setup
	//PRINTF(LOG_SYS,"> Start MMU\n");
	mmu_init();
	getBoardInfo();
	setArmClocks();
	idleInit();
#ifdef ON_RPI5
	hwDtbInit();
#endif

	hwLedInit();
	start(2,Argv);
	// echo everything back
	PRINTF(LOG_SYS,"Done!\n");
	int on=1;
	while(1) {
		hwLedSet(on);
		on=1-on;
		while(!uartReadable());
		uartPutChar(1+uartGet());
	}
}
