// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

.section ".text.boot"

_start:
    mrs     x1, mpidr_el1
    and     x1, x1, #3  // read cpu id
    cbnz    x1, 1f      // if cpu id<>0 then jump to 1:

    ldr     x1, =_start
    mov     sp, x1      // set stack pointer
    bl      main        // call C main function
1:  wfe                 // turn cpu off
    b       1b
