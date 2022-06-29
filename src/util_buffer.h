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
#ifndef _BUFFER_
#define _BUFFER_

// assume BUFFER_SIZE0 multiple of LWLEN
#define BUFFER_SIZE0 128
#define BUFFER_DOUBLE_UNTIL (1024*1024*2)
#define BUFFER_FINAL_INCREMENT (1024*1024)

struct Buffer
{
	LB header;
	FORGET forget;
	MARK mark;
	LINT size;
	LINT index;
	LB* bloc;
	char* buffer;
};

int _bufferBiggerFinalize(Thread* th, Buffer* b, LINT newsize);
Buffer* bufferCreateWithSize(Thread* th, LINT size);
Buffer* bufferCreate(Thread* th);
void bufferReinit(Buffer* b);
int bufferAddchar(Thread* th, Buffer* b,char c);
int bufferAddZero(Thread* th, Buffer* b,LINT n);
int bufferAddint(Thread* th, Buffer* b,LINT i);
int bufferAddintN(Thread* th, Buffer* b,LINT i,LINT n);
char bufferGetchar(Buffer* b,LINT index);
void bufferSetchar(Buffer* b,LINT index,char c);
void bufferDelete(Buffer* b, LINT index, LINT remove);
void bufferSetint(Buffer* b,LINT index,LINT i);
void bufferSetintN(Buffer* b,LINT index,LINT i,LINT n);
int bufferAddBin(Thread* th, Buffer* b,char *src,LINT len);
int bufferAddStr(Thread* th, Buffer* b,char *src);
int bufferVPrintf(Thread* th, Buffer* b, char* format, va_list arglist);
int bufferPrintf(Thread* th, Buffer* b,char *format, ...);
void bufferCut(Buffer* b,LINT len);
LINT bufferSize(Buffer* b);
char* bufferStart(Buffer* b);
int bufferItem(Thread* th, Buffer* b, LW v, LB* join,int rec,LINT first);
int bufferFormat(Buffer* b, Thread* th, LINT argc);
#endif
