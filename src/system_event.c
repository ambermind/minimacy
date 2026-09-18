// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

int NextExtEvent=0x8000;

int eventGetNextID(void) { return NextExtEvent++; }

void eventNotify(int c, int x, int y, int v)
{
	char out[16];
	int i = 0;
//	PRINTF(LOG_DEV,"eventNotify %d %d %d %d\n",c,x,y,v);

	*(short*)(&out[i]) = LSBW(12); i+=2;
	*(short*)(&out[i]) = LSBW(c); i+=2;
	*(int*)(&out[i]) = LSBL(x); i+=4;
	*(int*)(&out[i]) = LSBL(y); i+=4;
	*(int*)(&out[i]) = LSBL(v); i+=4;
	lockEnter(&MM.lock);
	internalSend(out, 16);
	lockLeave(&MM.lock);
}
void eventBinary(int c, char* data, int dataLen)
{
    char out[4];
    int i = 0;
    int eventSize=dataLen;
//    PRINTF(LOG_DEV,"eventNotify %d %d %d %d\n",c,x,y,v);
    c&=0xffff;
	*(short*)(&out[i]) = LSBW(eventSize); i += 2;
	*(short*)(&out[i]) = LSBW(c); i += 2;
	lockEnter(&MM.lock);
	internalSend(out, 4);
    internalSend(data, dataLen);
	lockLeave(&MM.lock);
}

void internalPoke(void)
{
	eventNotify(EVENT_POKE,0,0,0);
}

int systemEventInit(Pkg* system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_INT, "_EVENT_POKE", (void*)EVENT_POKE, "Int"},
		{ NATIVE_INT, "_EVENT_PAINT", (void*)EVENT_PAINT, "Int"},
		{ NATIVE_INT, "_EVENT_MOUSEMOVE", (void*)EVENT_MOUSEMOVE, "Int"},
		{ NATIVE_INT, "_EVENT_CLICK", (void*)EVENT_CLICK, "Int"},
		{ NATIVE_INT, "_EVENT_UNCLICK", (void*)EVENT_UNCLICK, "Int"},
		{ NATIVE_INT, "_EVENT_VWHEEL", (void*)EVENT_VWHEEL, "Int"},
		{ NATIVE_INT, "_EVENT_HWHEEL", (void*)EVENT_HWHEEL, "Int"},
		{ NATIVE_INT, "_EVENT_KEYDOWN", (void*)EVENT_KEYDOWN, "Int"},
		{ NATIVE_INT, "_EVENT_KEYUP", (void*)EVENT_KEYUP, "Int"},
		{ NATIVE_INT, "_EVENT_SIZE", (void*)EVENT_SIZE, "Int"},
		{ NATIVE_INT, "_EVENT_CLOSE", (void*)EVENT_CLOSE, "Int"},
		{ NATIVE_INT, "_EVENT_WILL_RESIZE", (void*)EVENT_WILL_RESIZE, "Int"},
		{ NATIVE_INT, "_EVENT_SUSPEND", (void*)EVENT_SUSPEND, "Int"},
		{ NATIVE_INT, "_EVENT_RESUME", (void*)EVENT_RESUME, "Int"},
		{ NATIVE_INT, "_EVENT_MULTITOUCH", (void*)EVENT_MULTITOUCH, "Int"},
		{ NATIVE_INT, "_EVENT_DROPFILES", (void*)EVENT_DROPFILES, "Int"},
	};

	NATIVE_DEF(nativeDefs);

	return 0;
}


