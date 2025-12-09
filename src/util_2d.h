// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _UTIL_2D_
#define _UTIL_2D_

#define CORNER_TOP_LEFT 1
#define CORNER_TOP_RIGHT 2
#define CORNER_BOTTOM_LEFT 4
#define CORNER_BOTTOM_RIGHT 8

typedef int (*BLEND_FUNCTION)(int src, int dst);
BLEND_FUNCTION _blendFunction(LINT num);

LINT _clip1D(LINT targetX, LINT targetW, LINT* x, LINT* w, LINT* dx);

void _bitmapFill(LBitmap* b, int color, BLEND_FUNCTION blend);
int _bitmapCopy(LBitmap* b, LINT x, LINT y, LINT w, LINT h, LBitmap** result);
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
void _bitmapCorners(LBitmap* d, LINT w, LINT mask);
void _bitmapGradient(LBitmap* d, LINT col00, LINT colw0, LINT col0h);
int _rgbFromYCrCb(LINT rgb);
int _yCrCbFromRgb(LINT ycrcb);
void _bitmapFromYCrCb(LBitmap* b);
void _bitmapToYCrCb(LBitmap* b);
void idct88(LB* array);
void dct88(LB* array);
void _bitmapExportMono(LBitmap* b, int background, char* dst, LINT len);
void _bitmapImportMono(LBitmap* b, int color, int background, char* src, LINT len);
void _bitmapExportMonoVertical(LBitmap* b, int background, char* dst, LINT len);
void _bitmapImportMonoVertical(LBitmap* b, int color, int background, char* src, LINT len);

#endif
