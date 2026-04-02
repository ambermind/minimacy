// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

typedef unsigned long uintptr;
typedef unsigned long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

// rasp3:
#ifdef ON_RPI3
#define MMIO_BASE 0x3F000000
#define GPIO_LED 130 // through mbox
#endif
// rasp4:
#ifdef ON_RPI4
#define MMIO_BASE 0xFE000000
#define GPIO_LED 42 // direct gpio
#define USE_RANDOM_RNG200
#endif
// rasp5:
#ifdef ON_RPI5
#define MMIO_BASE 0x107C000000
#define USE_RANDOM_RNG200
#endif

#define SHARED_MEM_START 0x3b000000
#define SHARED_MEM_END 0x3b400000

#define GP_REG(reg) ((volatile unsigned int *)(unsigned long long)(MMIO_BASE + (reg)))

#define GPFSEL0 GP_REG(0x200000)
#define GPFSEL1 GP_REG(0x200004)
#define GPSET0 GP_REG(0x20001C)
#define GPCLR0 GP_REG(0x200028)
#define GPPUD GP_REG(0x200094)
#define GPPUDCLK0 GP_REG(0x200098)

#define CLOCK_ID_EMMC 1
#define CLOCK_ID_UART 2
#define CLOCK_ID_ARM 3
#define CLOCK_ID_CORE 4
#define CLOCK_ID_EMMC2 12
#define CLOCK_ID_PIXEL_BVB 14

//------------------------------MMU
void mmu_init();

//------------------------------MBOX
unsigned int mboxBoardModel();
unsigned int mboxBoardRevision();
unsigned long mboxMac();
unsigned long mboxSerial();
unsigned long mboxGetClockRate(int id);
unsigned long mboxGetMaxClockRate(int id);
void mboxSetClockRate(int id, unsigned long rate);
unsigned int mboxVcMemory(unsigned int *start);
unsigned int mboxArmMemory(unsigned int *start);
void mboxSetLed(unsigned int on, int led);
int mboxFrameBuffer(int width, int height);
int fun_mbox(Thread *th);

//------------------------------UART
void uartInit();
void uartPutChar(unsigned int c);
void uartPut(char *s, int len);
int uartReadable();
int uartGet();

//------------------------------LED
void hwLedInit();
void hwLedSet(int val);

//------------------------------DTB
void hwDtbInit();

int systemSharedMemInit(Pkg *system);
