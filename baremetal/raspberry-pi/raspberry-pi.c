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
