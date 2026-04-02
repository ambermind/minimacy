// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#include "../../src/minimacy.h"
#include "raspberry-pi.h"


// in order to have mbox running after mmu has been started, we need 
// to have the mbox buffer in a non cacheable memory
// we decided to have the mbox buffer at 3b3f.0000
// see pi_mmu.c for further information
volatile unsigned int *mbox=(void*)(0x3B3F0000);

#ifdef ON_RPI5
#define VIDEOCORE_MBOX  (MMIO_BASE+0x00013880)
#else
#define VIDEOCORE_MBOX  (MMIO_BASE+0x0000B880)
#endif
#define MBOX_READ       ((volatile unsigned int*)(VIDEOCORE_MBOX+0x0))
#define MBOX_POLL       ((volatile unsigned int*)(VIDEOCORE_MBOX+0x10))
#define MBOX_SENDER     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x14))
#define MBOX_STATUS     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x18))
#define MBOX_CONFIG     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x1C))
#define MBOX_WRITE      ((volatile unsigned int*)(VIDEOCORE_MBOX+0x20))

// channels
#define CH_POWER   0
#define CH_FB      1
#define CH_VUART   2
#define CH_VCHIQ   3
#define CH_LEDS    4
#define CH_BTNS    5
#define CH_TOUCH   6
#define CH_COUNT   7
#define CH_PROP    8

// tags (for CH_PROP)
#define TAG_GET_BOARD_MODEL	0x10001
#define TAG_GET_BOARD_REV   0x10002
#define TAG_GET_MAC         0x10003
#define TAG_GET_SERIAL      0x10004
#define TAG_GET_ARM_MEMORY	0x10005
#define TAG_GET_VC_MEMORY	0x10006
#define TAG_GET_CLOCK_RATE  0x30002
#define TAG_SET_CLOCK_RATE  0x38002
#define TAG_GET_CLOCK_MAX_RATE 0x30004
#define TAG_GET_GPIO        0x30041
#define TAG_SET_GPIO        0x38041

#define TAG_NOTIFY_XHCI_RESET	0x00030058


// special values
#define MBOX_REQUEST    0
#define MBOX_RESPONSE   0x80000000
#define MBOX_FULL       0x80000000
#define MBOX_EMPTY      0x40000000

int _mboxCall(int channel)
{
	unsigned int request = ((unsigned long)mbox) | channel;	// mbox address is assumed to align with 16

	while(*MBOX_STATUS & MBOX_FULL);

	*MBOX_WRITE = request;

	while(1) {
		while(*MBOX_STATUS & MBOX_EMPTY);
		if(request == *MBOX_READ) return (mbox[1]==MBOX_RESPONSE);
	}
}

int mboxCall(int channel, int n,...)
{
	int i;
	__builtin_va_list list;

	mbox[0]=(n+3)*4;
	mbox[1]=MBOX_REQUEST;
	__builtin_va_start(list, n);
	for(i=0;i<n;i++) mbox[2+i]=__builtin_va_arg(list,int);
	__builtin_va_end(list);
	mbox[n+2]=0;

	return _mboxCall(channel);
}

unsigned int _mboxGet(unsigned int tag)
{
	if (!mboxCall(CH_PROP,5,tag,8,0,0,0)) return 0;
	return mbox[5];
}
unsigned long _mboxGetLong(unsigned int tag)
{
	unsigned long result=0;
	if (!mboxCall(CH_PROP,5,tag,8,0,0,0)) return 0;
	result=mbox[6];
	return (result<<32)|(mbox[5]);
}
unsigned int _mboxGetTwo(unsigned int tag, unsigned int* first) {
	unsigned long result=_mboxGetLong(tag);
	if (first) *first=(unsigned int) result;
	result>>=32;
	return (unsigned int)result;
}

unsigned long mboxGetClockRate(int id)
{
	if (!mboxCall(CH_PROP,5,TAG_GET_CLOCK_RATE,8,0,id,0)) return 0;
	return mbox[6];
}
unsigned long mboxGetMaxClockRate(int id)
{
	if (!mboxCall(CH_PROP,5,TAG_GET_CLOCK_MAX_RATE,8,0,id,0)) return 0;
	return mbox[6];
}

void mboxSetClockRate(int id, unsigned long rate) {
	if (!mboxCall(CH_PROP,6,TAG_SET_CLOCK_RATE,12,12,CLOCK_ID_ARM,rate,0)) {
		PRINTF(LOG_SYS,"> Unable to set clock rate\n");
		return;
	}
}

unsigned int mboxBoardModel() { return _mboxGet(TAG_GET_BOARD_MODEL); }
unsigned int mboxBoardRevision() { return _mboxGet(TAG_GET_BOARD_REV); }

unsigned long mboxMac() { return _mboxGetLong(TAG_GET_MAC); }
unsigned long mboxSerial() { return _mboxGetLong(TAG_GET_SERIAL); }

unsigned int mboxVcMemory(unsigned int *start) { return _mboxGetTwo(TAG_GET_VC_MEMORY,start); }
unsigned int mboxArmMemory(unsigned int *start) { return _mboxGetTwo(TAG_GET_ARM_MEMORY,start); }

void mboxSetLed(unsigned int on,int led) {
	mboxCall(CH_PROP,5,TAG_SET_GPIO,8,8,led,on);
}


unsigned int fbWidth, fbHeight, fbWpl;
int *fbStart;

int mboxFrameBuffer(int w,int h)
{
	unsigned int bpl;

	fbStart= 0;

	mbox[0] = 35*4;
    mbox[1] = MBOX_REQUEST;

    mbox[2] = 0x48003;  // set physical size
    mbox[3] = 8;
    mbox[4] = 8;
    mbox[5] = w;        // width
    mbox[6] = h;        // height

    mbox[7] = 0x48004;  // set virtual size
    mbox[8] = 8;
    mbox[9] = 8;
    mbox[10] = w;       // virtual width
    mbox[11] = h;       // virtual height

    mbox[12] = 0x48009; //set virt offset
    mbox[13] = 8;
    mbox[14] = 8;
    mbox[15] = 0;       // x offset
    mbox[16] = 0;       // y offset

    mbox[17] = 0x48005; // set pixel depth
    mbox[18] = 4;
    mbox[19] = 4;
    mbox[20] = 32;      // depth

    mbox[21] = 0x48006; // set pixel order
    mbox[22] = 4;
    mbox[23] = 4;
    mbox[24] = 0;       // RGB

    mbox[25] = 0x40001; // get framebuffer, gets alignment on request
    mbox[26] = 8;
    mbox[27] = 8;
    mbox[28] = 0x1000;  // pointer
    mbox[29] = 0;       // size

    mbox[30] = 0x40008; // get bytes per line
    mbox[31] = 4;
    mbox[32] = 4;
    mbox[33] = 0;       // bpl

    mbox[34] = 0;

    if((!_mboxCall(CH_PROP))||(mbox[20]!=32)||(mbox[28]==0)) {
		PRINTF(LOG_SYS,"> Unable to set screen resolution %dx%d\n",w, h);
		return 0;
	}

	fbWidth =mbox[5];     // get actual physical width
	fbHeight=mbox[6];     // get actual physical height
	bpl     =mbox[33];    // get number of bytes per line
	mbox[28]&=0x3FFFFFFF; // convert GPU address to ARM address
	fbStart =(void*)((unsigned long)mbox[28]);
	fbWpl   = bpl/4;

//	PRINTF(LOG_SYS,"> graphic result %dx%d next:%d\n",fbWidth, fbHeight, pitch);
	memset(fbStart,fbHeight*fbWpl,0);
	return 1;
}

int fun_mbox(Thread* th)
{
	int n,i;
	int len=STACK_INT(th,0);
	LB* args=STACK_PNT(th,1);
	int channel=STACK_INT(th,2);
	if (len<0 || len>128 || !args) FUN_RETURN_NIL;
	n=ARRAY_LENGTH(args);
	if (n>128-3) FUN_RETURN_NIL;
	mbox[0]=(n+3)*4;
	mbox[1]=MBOX_REQUEST;
	for(i=0;i<n;i++) mbox[2+i]=ARRAY_INT(args,i);
	mbox[n+2]=0;
	if (!_mboxCall(channel)) FUN_RETURN_NIL;
	for(i=0;i<len;i++) STACK_PUSH_INT_ERR(th,(LINT)mbox[i],EXEC_OM);
	FUN_MAKE_ARRAY(len, DBG_ARRAY);
	return 0;
}
