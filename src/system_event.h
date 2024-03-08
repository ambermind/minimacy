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
#ifndef _SYSTEM_EVENT_
#define _SYSTEM_EVENT_

enum UIevent {
	EVENT_POKE,
	EVENT_PAINT,
	EVENT_MOUSEMOVE,
	EVENT_CLICK,
	EVENT_UNCLICK,
	EVENT_VWHEEL,
	EVENT_HWHEEL,
	EVENT_KEYDOWN,
	EVENT_KEYUP,
	EVENT_SIZE,
	EVENT_CLOSE,
    EVENT_WILL_RESIZE,
	EVENT_SUSPEND,
	EVENT_RESUME,
	EVENT_MULTITOUCH,
	EVENT_DROPFILES
};

void internalPoke(void);
void eventNotify(int c, int x, int y, int v);
int coreEventInit(Thread* th, Pkg* system);

#endif
