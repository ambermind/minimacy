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

int coreEventInit(Thread* th, Pkg* system)
{
	pkgAddConstInt(th, system, "_EVENT_POKE", EVENT_POKE, MM.I);
	pkgAddConstInt(th, system, "_EVENT_PAINT", EVENT_PAINT, MM.I);
	pkgAddConstInt(th, system, "_EVENT_MOUSEMOVE", EVENT_MOUSEMOVE, MM.I);
	pkgAddConstInt(th, system, "_EVENT_CLICK", EVENT_CLICK, MM.I);
	pkgAddConstInt(th, system, "_EVENT_UNCLICK", EVENT_UNCLICK, MM.I);
	pkgAddConstInt(th, system, "_EVENT_VWHEEL", EVENT_VWHEEL, MM.I);
	pkgAddConstInt(th, system, "_EVENT_HWHEEL", EVENT_HWHEEL, MM.I);
	pkgAddConstInt(th, system, "_EVENT_KEYDOWN", EVENT_KEYDOWN, MM.I);
	pkgAddConstInt(th, system, "_EVENT_KEYUP", EVENT_KEYUP, MM.I);
	pkgAddConstInt(th, system, "_EVENT_SIZE", EVENT_SIZE, MM.I);
	pkgAddConstInt(th, system, "_EVENT_CLOSE", EVENT_CLOSE, MM.I);
    pkgAddConstInt(th, system, "_EVENT_WILL_RESIZE", EVENT_WILL_RESIZE, MM.I);
    pkgAddConstInt(th, system, "_EVENT_SUSPEND", EVENT_SUSPEND, MM.I);
    pkgAddConstInt(th, system, "_EVENT_RESUME", EVENT_RESUME, MM.I);
    pkgAddConstInt(th, system, "_EVENT_MULTITOUCH", EVENT_MULTITOUCH, MM.I);
    pkgAddConstInt(th, system, "_EVENT_DROPFILES", EVENT_DROPFILES, MM.I);

	return 0;
}


