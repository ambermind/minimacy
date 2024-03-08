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
#ifndef _UTIL_2D_
#define _UTIL_2D_

typedef int (*BLEND_FUNCTION)(int src, int dst);
BLEND_FUNCTION _blendFunction(LINT num);

LINT _clip1D(LINT targetX, LINT targetW, LINT* x, LINT* w, LINT* dx);

void _colorInit(void);
void _bitmapFill(LBitmap* b, int color, BLEND_FUNCTION blend);
int _bitmapCopy(Thread* th, LBitmap* b, LINT x, LINT y, LINT w, LINT h, LBitmap** result);
void _bitmapPlot(LBitmap* b, LINT x, LINT y, int color, BLEND_FUNCTION blend);
void _bitmapMakeColorTransparent(LBitmap* b, int color);
void _bitmapToBitmap(LBitmap* b, LINT xdst, LINT ydst, LBitmap* a, LINT xsrc, LINT ysrc, LINT w, LINT h, BLEND_FUNCTION blend);
void _bitmapToBitmapColored(LBitmap* b, LINT xdst, LINT ydst, LBitmap* a, LINT xsrc, LINT ysrc, LINT w, LINT h, BLEND_FUNCTION blend, int colored, BLEND_FUNCTION coloredBlend);
void _bitmapFillRectangle(LBitmap* b, LINT x, LINT y, LINT w, LINT h, int color, BLEND_FUNCTION blend);
void _bitmapRectangle(LBitmap* b, LINT x, LINT y, LINT w, LINT h, int color, BLEND_FUNCTION blend);
void _bitmapScanline(LBitmap* b, LINT x1, LINT x2, LINT y, int color, BLEND_FUNCTION blend);
void _bitmapLine(LBitmap* b, LINT x1, LINT y1, LINT x2, LINT y2, int color, BLEND_FUNCTION blend);
void _bitmapFillCircle(LBitmap* b, LINT x0, LINT y0, LINT w, LINT h, int color, BLEND_FUNCTION blend);
void _bitmapCircle(LBitmap* b, LINT x0, LINT y0, LINT w, LINT h, int color, BLEND_FUNCTION blend);
void _bitmapResizeSmooth(LBitmap* d, LBitmap* src);
void _bitmapResizeNearest(LBitmap* dst, LBitmap* src);
void _bitmapComponents(LBitmap* d, LINT r, LINT g, LINT b, LINT a);
int _rgbFromYCrCb(LINT rgb);
int _yCrCbFromRgb(LINT ycrcb);
void _bitmapFromYCrCb(LBitmap* b);
void _bitmapToYCrCb(LBitmap* b);
void idct88(LB* array);
void dct88(LB* array);
#endif
