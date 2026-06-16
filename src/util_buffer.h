// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
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
	Worker* worker;
};

int _bufferBiggerFinalize(Buffer* b, LINT newsize);
Buffer* bufferCreateWithSize(LINT size);
Buffer* bufferCreate(void);
Buffer* bufferReset(Buffer* b, LINT size);
void bufferReinit(Buffer* b);
void bufferRemove(Buffer* b, int delta);

int bufferAddChar(Buffer* b,char c);
int bufferAddZero(Buffer* b,LINT n);
int bufferAddInt(Buffer* b,LINT i);
int bufferAddIntN(Buffer* b,LINT i,LINT n);
char bufferGetChar(Buffer* b,LINT index);
char* bufferGetCharAddress(Buffer* b,LINT index);
void bufferSetChar(Buffer* b,LINT index,char c);
void bufferDelete(Buffer* b, LINT index, LINT remove);
void bufferSetInt(Buffer* b,LINT index,LINT i);
void bufferSetIntN(Buffer* b,LINT index,LINT i,LINT n);
LINT bufferGetIntN(Buffer* b,LINT index,LINT n);
int bufferAddBin(Buffer* b,char *src,LINT len);
int bufferAddStr(Buffer* b,char *src);
int bufferVPrintf(Buffer* b, char* format, va_list arglist);
int bufferPrintf(Buffer* b,char *format, ...);
void bufferCut(Buffer* b,LINT len);
LINT bufferSize(Buffer* b);
char* bufferStart(Buffer* b);
int bufferItem(Buffer* b, LW v, int type, LB* join);
int bufferFormat(Buffer* b, Thread* th, LINT argc);

Buffer* fixedBufferGet(LINT fixedRoot);
void bufferSetWorker(Buffer* b, Worker* w);
char* fixedBufferRequire(LINT fixedRoot, LINT len);
int fixedBufferAddChar(LINT fixedRoot, char c);
int fixedBufferAddBin(LINT fixedRoot, char* src, LINT len);
#endif
