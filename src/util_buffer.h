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
	volatile Thread** pth;
	volatile Buffer** link;
};

int _bufferBiggerFinalize(Buffer* b, LINT newsize);
Buffer* bufferCreateWithSize(LINT size);
Buffer* bufferCreate(void);
void bufferReinit(Buffer* b);
void bufferRemove(Buffer* b, int delta);
void bufferSetWorkerThread(volatile Buffer** pb, volatile Thread** pth);
void bufferUnsetWorkerThread(volatile Buffer** pb, volatile Thread** pth);
int bufferAddCharWorker(volatile Buffer** pb,char c);
int bufferAddChar(Buffer* b,char c);
int bufferAddZero(Buffer* b,LINT n);
int bufferAddInt(Buffer* b,LINT i);
int bufferAddIntN(Buffer* b,LINT i,LINT n);
char bufferGetChar(Buffer* b,LINT index);
void bufferSetChar(Buffer* b,LINT index,char c);
void bufferDelete(Buffer* b, LINT index, LINT remove);
void bufferSetInt(Buffer* b,LINT index,LINT i);
void bufferSetIntN(Buffer* b,LINT index,LINT i,LINT n);
LINT bufferGetIntN(Buffer* b,LINT index,LINT n);
char* bufferRequireWorker(volatile Buffer** pb, LINT len);
int bufferAddBinWorker(volatile Buffer** pb, char* src, LINT len);
int bufferAddBin(Buffer* b,char *src,LINT len);
int bufferAddStr(Buffer* b,char *src);
int bufferVPrintf(Buffer* b, char* format, va_list arglist);
int bufferPrintf(Buffer* b,char *format, ...);
void bufferCut(Buffer* b,LINT len);
LINT bufferSize(Buffer* b);
char* bufferStart(Buffer* b);
int bufferItem(Buffer* b, LW v, int type, LB* join);
int bufferFormat(Buffer* b, Thread* th, LINT argc);
#endif
