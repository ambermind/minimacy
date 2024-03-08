/* Copyright (c) 2022, Sylvain Huet, Ambermind
   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License, version 2.0, as
   published by the Free Software Foundation.
   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License,
   version 2.0, for more details.
   You should have received a copy of the GNU General Public License along
   with this program. If not, see <https://www.gnu.org/licenses/>. */
#ifndef _CORE_
#define _CORE_

#define FIFO_START 0
#define FIFO_END 1
#define FIFO_COUNT 2

#define SUFFIX_CODE ".mcy"
#define SUFFIX_NONE ""

int coreCryptoInit(Thread* th, Pkg *system);
int coreStrInit(Thread* th, Pkg *system);
int coreBytesInit(Thread* th, Pkg *system);
int coreBinaryInit(Thread* th, Pkg *system);
int coreConvertInit(Thread* th, Pkg *system);
int coreBufferInit(Thread* th, Pkg *system);
int coreLzwInit(Thread* th, Pkg* system);
int coreInflateInit(Thread* th, Pkg* system);
int coreInit(Thread* th, Pkg *system);

#endif