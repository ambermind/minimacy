// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _SYSTEM_2D_
#define _SYSTEM_2D_

#define BITMAP_MAX_LENGTH (1024*16)

typedef unsigned char lchar;

typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	LB* bytes;
	lchar* start8;
	LINT next8;
	int* start32;
	LINT next32;
	LINT w;
	LINT h;
#ifdef ON_WINDOWS
	HBITMAP bmp;
#endif
}LBitmap;

LBitmap* _bitmapCreate(LINT w, LINT h);

int system2dInit(Pkg* system);
#endif
