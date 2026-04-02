/* SPDX-License-Identifier: GPL-3.0-only
   Copyright (c) 2022, Sylvain Huet, Ambermind
   Minimacy (r) System */
   
SECTIONS
{
    . = 0x80000;
    .text : { *(.text.boot) *(.text .text.* .gnu.linkonce.t*) }
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }
    .data ALIGN(4096) : { *(.data .data.* .gnu.linkonce.d*) }
    .bss ALIGN(16) : { *(.bss .bss.*) }
    _end = ALIGN(65536);
}
