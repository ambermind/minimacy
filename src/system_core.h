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

int coreCryptoInit(Pkg *system);
int coreStrInit(Pkg *system);
int coreBytesInit(Pkg *system);
int coreBinaryInit(Pkg *system);
int coreConvertInit(Pkg *system);
int coreBufferInit(Pkg *system);
int coreLzwInit(Pkg* system);
int coreInflateInit(Pkg* system);
int coreInit(Pkg *system);

#endif