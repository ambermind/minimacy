// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
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
int eventGetNextID(void);
void eventNotify(int c, int x, int y, int v);
void eventBinary(int c, char* data, int dataLen);
int systemEventInit(Pkg* system);

#endif
