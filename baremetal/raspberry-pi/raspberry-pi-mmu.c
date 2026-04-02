// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#include "../../src/minimacy.h"
#include "raspberry-pi.h"

// reference documentation:
// - https://documentation-service.arm.com/static/5efa1d23dbdee951c1ccdec5?token=
// - https://developer.arm.com/documentation/101811/latest/
// - https://developer.arm.com/documentation/101811/0104/Translation-granule
// - https://log.martinatkins.me/2023/12/25/armv8-virtual-memory-vmsav8/

// this information is not official, it may be wrong but leads to a working program
// reminder of hex values:
//     20.0000 is 2M
//   4000.0000 is 1G, is 512 blocks of 2M
// 1.0000.0000 is 4G

// RPi3 memory map is:
//         0 -   3b40.0000 : ARM ram memory
// 3b40.0000 -   3f00.0000 : VideoCore (VC) memory
// 3f00.0000 -   4000.0000 : devices

// RPi4 memory map is (32GB):
//           0 -   3b40.0000 : ARM ram memory
//   3b40.0000 -   4000.0000 : VideoCore (VC) memory
//   4000.0000 -           * : ram (if more than 1GB)
//   fe00.0000 - 1.0000.0000 : devices
// 1.0000.0000 - 3.ffff.ffff : ram (if more than 4GB)
// 6.0000.0000 - 7.ffff.ffff : Pcie

// RPi5 memory map is (128GB):
//            0 -    3b40.0000 : ARM ram memory
//    3b40.0000 -    4000.0000 : VideoCore (VC) memory
//    4000.0000 -  3.ffff.ffff : ram (if more than 4GB)
// 10.0000.0000 - 10.1fff.ffff : Axi
// 10.6000.0000 - 10.7fff.ffff : Soc
// 1f.0000.0000 - 1f.1fff.ffff : Pcie

// our specific configuration (for RPi5, replace 33 with 97):
//        _end - _end+4096*33: mmu tables (1+32)
// _end+4096*33- ...         : minimacy memory strip (size 920MB: 3980.0000) about 399d.c000
//         ... -   3aff.ffff : not used (around 22MB)
//   3b00.0000 -   3b3f.0000 : pcie exchange buffers (4032 KB)
//   3b3f.0000 -   3b40.0000 : mbox

// Our MMU configuration:
//           0 -    3b00.0000 : cacheable  (512-8*5 blocks)
//   3b00.0000 -    4000.0000 : non cacheable (8*5 blocks)
//   4000.0000 -    f000.0000 : cacheable  (512*2+384 blocks)
//   f000.0000 -  1.0000.0000 : non cacheable (128 blocks)
// 1.0000.0000 -  4.0000.0000 : cacheable  (512*12 blocks)
// 4.0000.0000 -  8.0000.0000 : non cacheable
// on RPi5 we add:
// 8.0000.0000 - 20.0000.0000 : non cacheable

// we use 4KB granule, with the following cascading tables:
// - level 0: 512 GB -> we don't use it
// - level 1: 1 GB -> it is our root table, each entry references a level 2 table wich addresse 1GB
// - level 2: 2 MB -> it is our pages tables, each entry is a block of 2MB
// - level 3: 4KB -> we don't use it
// => each mmu page is 4096 bytes, contains 512 blocks, and specifies access to 1GB 
// this leads to 32 pages of 1Gb each:
// page 0: 0 - 512-8*5 : cacheable
// page 0: 512-8*5 - 511 : non cacheable
// pages 1,2 : 0 - 511 : cacheable
// pages 3 : 0 - 383 : cacheable
// pages 3 : 384 - 511 : non cacheable
// pages 4-31 : 0 - 511 : cacheable
// on RPi5 we add:
// pages 32-127 : 0 - 511 : non cacheable


// block descriptor flags:
#define BD_BLOCK  1	     	// 2M granule
#define BD_AF_SET (1<<10)	// access flag is pre-set
#define BD_OSH    (2<<8) 	// outter shareable
#define BD_ISH    (3<<8) 	// inner shareable

#define BD_PXN    (((unsigned long)1)<<53) 	// Privileged Execute-never
#define BD_UXN    (((unsigned long)1)<<54) 	// Unprivileged Execute-never

// custom Memory Attribute Indexes
#define MAI_NORMAL (0<<2)	// normal memory
#define MAI_DEVICE (1<<2)	// device MMIO
#define MAI_NC     (2<<2)	// non-cacheable

// get addresses from linker
extern volatile unsigned char _end;
unsigned long *root_table=(unsigned long*)&_end;

#ifdef ON_RPI5
#define NPAGES 128
#else
#define NPAGES 32
#endif
// starting at root_table we have 1+NPAGES tables with 512 unsigned long entries each
// root table: only the first NPAGES entries are not null and reference the next NPAGES tables.
//             Null values are ignored, there is no need to advertise the value of NPAGES
// page table 0
// page table ...
// page table NPAGES-1
// these NPAGES tables contain continuous "1 on 1" address translations, from 0 to NPAGES Gb

#define BLOCK_NORMAL (MAI_NORMAL | BD_AF_SET | BD_ISH | BD_BLOCK)
#define BLOCK_DEVICE (MAI_DEVICE | BD_AF_SET | BD_BLOCK | BD_PXN | BD_UXN)

void mmu_init()
{
	unsigned long base,i;
	unsigned long *pages_tables=&root_table[512];

	// set everything to zero
	for(i=0;i<512;i++) root_table[i]=0;
	for(i=0;i<NPAGES*512;i++) pages_tables[i]=0;

	for(i=0;i<NPAGES;i++) root_table[i] = (0x8000000000000000) | (unsigned long)(&pages_tables[i*512]) | 3;

	base=0;
	for(;base<512-5*8;base++)	// up to 0x3b00.0000
		pages_tables[base]= (base<<21) | BLOCK_NORMAL;

	for(;base<512;base++)	// up to 0x4000.0000
		pages_tables[base]= (base<<21) | BLOCK_DEVICE;

	for(;base<512*3+384;base++)	// almost 3 cacheable pages (pages 1 and 2, and 384 entries in page 3)
		pages_tables[base]= (base<<21) | BLOCK_NORMAL;

	for(;base<512*4;base++)	// non cacheable pages (page 3- entries 384-511)
		pages_tables[base]= (base<<21) | BLOCK_DEVICE;

	for(;base<512*16;base++)	// cacheable pages (pages 4-15)
		pages_tables[base]= (base<<21) | BLOCK_NORMAL;

	for(;base<512*NPAGES;base++)	// non cacheable pages (pages 8-31)
		pages_tables[base]= (base<<21) | BLOCK_DEVICE;

	asm volatile ("tlbi alle2");

// https://developer.arm.com/documentation/102670/0301/AArch64-registers/AArch64-register-descriptions/AArch64-Generic-System-control-register-description/MAIR-EL2--Memory-Attribute-Indirection-Register--EL2-
	unsigned long mair=
		(0xFF << 0) |	// MAI_NORMAL -> AttrIdx=0: normal, IWBWA, OWBWA, NTR
		(0x00 << 8) |	// MAI_DEVICE -> AttrIdx=1: device, nGnRnE
		(0x44 <<16) ;	// MAI_NC	 -> AttrIdx=2: non cacheable
	asm volatile ("msr mair_el2, %0" : : "r" (mair));

// https://developer.arm.com/documentation/ddi0601/2024-12/AArch64-Registers/TCR-EL2--Translation-Control-Register--EL2-
	unsigned long tcr=
#ifdef ON_RPI5
		(2LL    << 16)|	// PS=2 (1TB)
#else
		(1LL    << 16)|	// PS=1 (64GB)
#endif
		(0b00LL << 14)|	// TG0=4k
		(0b11LL << 12)|	// SH0=3 inner
		(0b01LL << 10)|	// ORGN0=1 write back
		(0b01LL << 8) |	// IRGN0=1 write back
#ifdef ON_RPI5
		(27LL   << 0) ;	// T0SZ=27 (128G)
#else
		(28LL   << 0) ;	// T0SZ=28 (64G)
#endif

	asm volatile ("msr tcr_el2, %0; isb" : : "r" (tcr));

// https://developer.arm.com/documentation/ddi0601/2024-12/AArch64-Registers/TTBR0-EL2--Translation-Table-Base-Register-0--EL2-
	asm volatile ("msr ttbr0_el2, %0" : : "r" ((unsigned long)root_table));
	
// https://developer.arm.com/documentation/ddi0601/2024-12/AArch64-Registers/SCTLR-EL2--System-Control-Register--EL2-
	unsigned long sctlr;
	asm volatile ("dsb ish; isb; mrs %0, sctlr_el2" : "=r" (sctlr));

	sctlr&=~(
		(1 << 4) |	// SA0, Stack Alignment Check Enable for EL0
		(1 << 3) |	// SA, Stack Alignment Check Enable
		(1 << 1)	// A, Alignment Check Enable
	);
	sctlr|=
		0xC00800 |	// mandatory reserved bits
		(1 << 12)|	// I, Instruction cache enable
		(1 << 2) |	// C, Data cache enable
		(1 << 0) ;	// M, MMU Enable
	asm volatile ("msr sctlr_el2, %0; isb" : : "r" (sctlr));
}
