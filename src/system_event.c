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
//	printf("eventNotify %d %d %d %d\n",c,x,y,v);

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

void internalPoke()
{
	eventNotify(EVENT_POKE,0,0,0);
}

int coreEventInit(Thread* th, Pkg* system)
{
	pkgAddConst(th, system, "_EVENT_POKE", INTTOVAL(EVENT_POKE), MM.I);
	pkgAddConst(th, system, "_EVENT_PAINT", INTTOVAL(EVENT_PAINT), MM.I);
	pkgAddConst(th, system, "_EVENT_MOUSEMOVE", INTTOVAL(EVENT_MOUSEMOVE), MM.I);
	pkgAddConst(th, system, "_EVENT_CLICK", INTTOVAL(EVENT_CLICK), MM.I);
	pkgAddConst(th, system, "_EVENT_UNCLICK", INTTOVAL(EVENT_UNCLICK), MM.I);
	pkgAddConst(th, system, "_EVENT_VWHEEL", INTTOVAL(EVENT_VWHEEL), MM.I);
	pkgAddConst(th, system, "_EVENT_HWHEEL", INTTOVAL(EVENT_HWHEEL), MM.I);
	pkgAddConst(th, system, "_EVENT_KEYDOWN", INTTOVAL(EVENT_KEYDOWN), MM.I);
	pkgAddConst(th, system, "_EVENT_KEYUP", INTTOVAL(EVENT_KEYUP), MM.I);
	pkgAddConst(th, system, "_EVENT_SIZE", INTTOVAL(EVENT_SIZE), MM.I);
	pkgAddConst(th, system, "_EVENT_MOVE", INTTOVAL(EVENT_MOVE), MM.I);
	pkgAddConst(th, system, "_EVENT_CLOSE", INTTOVAL(EVENT_CLOSE), MM.I);
    pkgAddConst(th, system, "_EVENT_WILL_RESIZE", INTTOVAL(EVENT_WILL_RESIZE), MM.I);
    pkgAddConst(th, system, "_EVENT_SUSPEND", INTTOVAL(EVENT_SUSPEND), MM.I);
    pkgAddConst(th, system, "_EVENT_RESUME", INTTOVAL(EVENT_RESUME), MM.I);
    pkgAddConst(th, system, "_EVENT_MULTITOUCH", INTTOVAL(EVENT_MULTITOUCH), MM.I);

	return 0;
}


