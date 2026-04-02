// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#include"../../src/minimacy.h"
#include "raspberry-pi.h"

#ifdef ON_RPI5
#define UART0_BASE (MMIO_BASE+0x01001000)
#else
#define UART0_BASE (MMIO_BASE+0x00201000)
#define UART1_BASE (UART0_BASE+0xA00)
#endif

#define UART_REG(base,reg) ((volatile unsigned int*)(unsigned long long)((base)+(reg)))

// UART0 registers, used as console, gpio 14, 15
#define UART0_DR        UART_REG(UART0_BASE, 0x00)
#define UART0_FR        UART_REG(UART0_BASE, 0x18)
#define UART0_IBRD      UART_REG(UART0_BASE, 0x24)
#define UART0_FBRD      UART_REG(UART0_BASE, 0x28)
#define UART0_LCRH      UART_REG(UART0_BASE, 0x2C)
#define UART0_CR        UART_REG(UART0_BASE, 0x30)
#define UART0_ICR       UART_REG(UART0_BASE, 0x44)

// UART1 registers, used as serial, gpio 12,13
#define UART1_DR        UART_REG(UART1_BASE, 0x00)
#define UART1_FR        UART_REG(UART1_BASE, 0x18)
#define UART1_IBRD      UART_REG(UART1_BASE, 0x24)
#define UART1_FBRD      UART_REG(UART1_BASE, 0x28)
#define UART1_LCRH      UART_REG(UART1_BASE, 0x2C)
#define UART1_CR        UART_REG(UART1_BASE, 0x30)
#define UART1_ICR       UART_REG(UART1_BASE, 0x44)

unsigned long UartClock=0;
#ifdef ON_RPI5
void uartInit()
{
	UartClock=mboxGetClockRate(CLOCK_ID_UART);
}
#else
void uartInit()
{
	register unsigned int r;
	unsigned int period;

	*UART0_CR = 0;         // turn off UART0

	UartClock=mboxGetClockRate(CLOCK_ID_UART);

	// map UART0 to GPIO pins (10 gpio per register, 3bits per gpio)
	r=*GPFSEL1;
	r&=~((7<<12)|(7<<15)); // gpio14, gpio15
	r|=(4<<12)|(4<<15);    // alt0
	*GPFSEL1 = r;
	*GPPUD = 0;            // pull mode off gpio 0-15 (rpi4)

#ifdef ON_RPI3
	r=150; while(r--) { asm volatile("nop"); }	// rpi3 ?
	*GPPUDCLK0 = (1<<14)|(1<<15);	// rpi3 ?
	r=150; while(r--) { asm volatile("nop"); }	// rpi3 ?
	*GPPUDCLK0 = 0;        // flush GPIO setup		// rpi3 ?
#endif
	period= ((UartClock<<2)+(115200>>1))/115200;
	*UART0_ICR = 0x7ff;    // clear interrupts
	*UART0_IBRD = period>>6;
	*UART0_FBRD = period&0x3f;
	*UART0_LCRH = 0x70;  // 8n1, enable FIFOs
	*UART0_CR = 0x301;     // enable Tx, Rx, UART
}
#endif


// check whether there is something to read
int uartReadable() {
	if (*UART0_FR&0x10) return 0;
	return 1;
}
int uartGet() {
	unsigned char r;
	if (*UART0_FR&0x10) return -1;
	r=(unsigned char)(*UART0_DR);
	return r;
}
int uartWritable() {
	if (*UART0_FR&0x20) return 0;
	return 1;
}
void uartPutChar(unsigned int c) {
	if (c==10) {
		while(*UART0_FR&0x20);
		*UART0_DR=13;
	}
	while(*UART0_FR&0x20);
	*UART0_DR=c;
}
//----------------------
void uartPut(char *s,int len) {
	while(len--) uartPutChar(*s++);
}


#ifndef USE_SERIAL_STUB

int Uart1FD=0;

int _serialCheckReadable(Socket* s)
{
	if (s && s->fd==Uart1FD) return (*UART1_FR&0x10)?0:1;
	return 0;
}
int _serialCheckWritable(Socket* s)
{
	if (s && s->fd==Uart1FD) return (*UART1_FR&0x20)?0:1;
	return 0;
}

int fun_serialOpen(Thread* th)
{
	register unsigned int r;
	unsigned int period;
	Socket* s;
	LINT stop = STACK_INT(th, 0);
	LINT parity = STACK_INT(th, 1);
	LINT bits = STACK_INT(th, 2);
	LINT bds = STACK_INT(th, 3);
	LB* v = STACK_PNT(th, 4);
	if ((!bds)||(!v)) FUN_RETURN_NIL;
	if (strcmp(STR_START(v),"uart1")) FUN_RETURN_NIL;

	if (Uart1FD) FUN_RETURN_NIL;	// already open

	stop>>=1;	// 0, 2 -> 0, 1
	if (parity==2) parity=3;	// 0, 1, 2 -> 0, 1, 3

	r=*GPFSEL1;
	r&=~((7<<6)|(7<<9)); // gpio12, gpio13
	r|=(3<<6)|(3<<9);    // alt0
	*GPFSEL1 = r;

	period= ((UartClock<<2)+(bds>>1))/bds;

	*UART1_ICR = 0x7ff;    // clear interrupts
	*UART1_IBRD = period>>6;
	*UART1_FBRD = period&0x3f;
	*UART1_LCRH = (((bits-5)&3)<<5)|(1<<4)|(stop<<3)|(parity<<1);
	*UART1_CR = 0x301;     // enable Tx, Rx, UART

	Uart1FD=socketNextFd();
	s = _socketCreate(Uart1FD); if (!s) return EXEC_OM;
	s->checkReadable= _serialCheckReadable;
	s->checkWritable= _serialCheckWritable;
	FUN_RETURN_PNT((LB*)s);
}
int fun_serialClose(Thread* th)
{
	Socket* s = (Socket*)STACK_PNT(th, 0);
	if (s && s->fd==Uart1FD) {
		s->fd=INVALID_SOCKET;
		Uart1FD=0;
	}
	return 0;
}
int fun_serialWrite(Thread* th)
{
	LINT sent,len=0;
	char *p;

	LINT start = STACK_INT(th, 0);
	LB* src = STACK_PNT(th, 1);
	Socket* s = (Socket*)STACK_PNT(th, 2);
	if ((!s)|| (s->fd != Uart1FD)) FUN_RETURN_NIL;
	FUN_SUBSTR(src, start, len, 1, STR_LENGTH(src));

	if (len==0) FUN_RETURN_INT(start);
	sent=len;
	p=STR_START(src) + start;
	while(len--) {
		while(*UART1_FR&0x20);
		*UART1_DR=*(p++);
	}
	FUN_RETURN_INT(start + sent);
}
int fun_serialRead(Thread* th) 
{
	char buffer[32];	// RX fifo is 32 bytes wide
	int len=0;
	Socket* s = (Socket*)STACK_PNT(th, 0);
	if ((!s)|| (s->fd !=Uart1FD)) FUN_RETURN_NIL;

	while((len<32) && !(*UART1_FR&0x10)) buffer[len++]=(*UART1_DR);
	FUN_RETURN_STR(buffer, len);
}
int fun_serialSocket(Thread* th) {
	return 0;
}
int fun_serialList(Thread* th) {
	char label[32];
	int n=0;
	snprintf(label,31,"TX:gpio%d RX:gpio%d",12, 13);
	STACK_PUSH_STR_ERR(th, "uart1", -1, EXEC_OM);
	STACK_PUSH_STR_ERR(th, label, -1, EXEC_OM);
	FUN_MAKE_ARRAY(2, DBG_TUPLE);
	n++;
	FUN_PUSH_NIL;
	while (n--) FUN_MAKE_ARRAY(LIST_LENGTH, DBG_LIST);
	return 0;
}

#endif