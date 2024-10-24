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
#include"minimacy.h"



void eventNotify(int c, int x, int y, int v)
{
	char out[16];
	int i = 0;
//	PRINTF(LOG_DEV,"eventNotify %d %d %d %d\n",c,x,y,v);

	out[i++] = (char)c;

	out[i++] = (char)x; x >>= 8;
	out[i++] = (char)x; x >>= 8;
	out[i++] = (char)x; x >>= 8;
	out[i++] = (char)x; x >>= 8;

	out[i++] = (char)y; y >>= 8;
	out[i++] = (char)y; y >>= 8;
	out[i++] = (char)y; y >>= 8;
	out[i++] = (char)y; y >>= 8;

	out[i++] = (char)v; v >>= 8;
	out[i++] = (char)v; v >>= 8;
	out[i++] = (char)v; v >>= 8;
	out[i++] = (char)v; v >>= 8;

	out[i++] = 0;
	out[i++] = 0;
	out[i++] = 0;
	internalSend(out, 16);
}

void internalPoke(void)
{
	eventNotify(EVENT_POKE,0,0,0);
}

int coreEventInit(Pkg* system)
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


