// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#include"../../src/minimacy.h"
#include"raspberry-pi.h"

void consoleWrite(int user, char* src, int len)
{
	uartPut(src,len);
}
void consoleVPrint(int user, char* format, va_list arglist)
{
	vsnprintf(NULL,0, format, arglist);
}
#ifdef USE_TIME_RPI
LINT hwTimeMs()
{
	register unsigned long f, t;
	asm volatile ("mrs %0, cntfrq_el0" : "=r"(f));
	asm volatile ("mrs %0, cntpct_el0" : "=r"(t));
	return t*1000/f;
}
LINT hwTime()
{
	return hwTimeMs()/1000;
}
void hwTimeInit() { }
#endif
void hwSleepMs(LINT ms)
{
	unsigned long long t0=hwTimeMs();
	while((hwTimeMs()-t0)<ms);
}

#ifdef ON_RPI3
#define RNG_CTRL        ((volatile unsigned int*)(MMIO_BASE+0x00104000))
#define RNG_STATUS      ((volatile unsigned int*)(MMIO_BASE+0x00104004))
#define RNG_DATA        ((volatile unsigned int*)(MMIO_BASE+0x00104008))
#define RNG_INT_MASK    ((volatile unsigned int*)(MMIO_BASE+0x00104010))

void hwRandomInit(void)
{
	*RNG_STATUS=0x40000;
	// mask interrupt
	*RNG_INT_MASK|=1;
	// enable
	*RNG_CTRL|=1;
}

void hwRandomBytes(char* dst, LINT len)
{
	while(len>0)
	{
		int i;
		unsigned long val;
	    while(!((*RNG_STATUS)>>24)) asm volatile("nop");
		val=*RNG_DATA;	// looks like it is only 32 bits
		for(i=0;i<4;i++) {
			*(dst++)=(char)val;
			val>>=8;
			len--;
			if (!len) return;
		}
	}
}
int hwHasRandom()
{
	return 1;
}
#endif


#ifdef USE_RANDOM_RNG200

#ifdef ON_RPI4
#define RNG_BASE (MMIO_BASE+0x00104000)
#endif
#ifdef ON_RPI5
#define RNG_BASE (MMIO_BASE+0x01208000)
#endif

#define RNG_REG(reg) ((volatile unsigned int*)(unsigned long long)(RNG_BASE+(reg)))

#define RNG_CTRL				RNG_REG(0x00)
#define RNG_TOTAL_BIT_COUNT		RNG_REG(0x0C)
#define RNG_TOTAL_BIT_COUNT_THR	RNG_REG(0x10)
#define RNG_FIFO_DATA			RNG_REG(0x20)
#define RNG_FIFO_COUNT			RNG_REG(0x24)

void hwRandomInit(void)
{
	*RNG_TOTAL_BIT_COUNT_THR= 0x40000;
	*RNG_FIFO_COUNT= 0x200;
	*RNG_CTRL= 0x00007FFF;
	// ensure warm up period has elapsed
	while((*RNG_TOTAL_BIT_COUNT)<=16) asm volatile("nop");
}

void hwRandomBytes(char* dst, LINT len)
{
	while(len>0)
	{
		int i;
		unsigned long val;
		while (((*RNG_FIFO_COUNT) & 0x000000FF) == 0) asm volatile("nop");
		val=*RNG_FIFO_DATA;
		for(i=0;i<4;i++) {
			*(dst++)=(char)val;
			val>>=8;
			len--;
			if (!len) return;
		}
	}
}
int hwHasRandom()
{
	return 1;
}
#endif


#ifdef ON_RPI3
void hwLedInit()
{

}
void hwLedSet(int val)
{
	mboxSetLed(val?1:0, GPIO_LED);
}
#endif
#ifdef ON_RPI4
void hwLedInit()
{
	int selreg=GPIO_LED/10;	// 10 gpios per register
	int selshift=(GPIO_LED%10)*3;	// 3 bits per gpio
	GPFSEL0[selreg] &= ~(7<<selshift);	// clear the 3 bits
	GPFSEL0[selreg] |= 1<<selshift;	// set 1 for output gpio
}
void hwLedSet(int val)
{
	volatile unsigned int* reg=val?GPSET0:GPCLR0;
	reg[GPIO_LED>>5]= 1<<(GPIO_LED&31);	// 32 gpios per register, 1 bit per gpio
}
#endif
#ifdef ON_RPI5
#define GPIO1_BASE			(MMIO_BASE + 0x1508500)
#define GPIO1_REG(reg) ((volatile unsigned int*)(unsigned long long)(GPIO1_BASE+(reg)))
#define GPIO1_DATA0		GPIO1_REG(0x04)
#define GPIO1_IODIR0	GPIO1_REG(0x08)
int hwPsw(void)
{
	if ((*GPIO1_DATA0)&(1<<20)) return 0;
	hwSleepMs(5);	
	if ((*GPIO1_DATA0)&(1<<20)) return 0;
	return 1;
}

#define GPIO2_BASE			(MMIO_BASE + 0x1517C00)
#define GPIO2_REG(reg) ((volatile unsigned int*)(unsigned long long)(GPIO2_BASE+(reg)))

#define GPIO2_DATA0		GPIO2_REG(0x04)
#define GPIO2_IODIR0	GPIO2_REG(0x08)

void hwLedInit()
{
	*GPIO2_IODIR0&= ~(1<<9);
}
void hwLedSet(int val)
{
	if (val) *GPIO2_DATA0|= (1<<9);
	else *GPIO2_DATA0&= ~(1<<9);
}
#endif
