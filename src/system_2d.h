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

int core2dInit(Pkg* system);
#endif
