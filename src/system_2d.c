// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

void bitmapMark(LB* user)
{
	LBitmap* b = (LBitmap*)user;
	MARK_OR_MOVE(b->bytes);
	if (MOVING_BLOCKS) {
		b->start8 = (lchar*)STR_START(b->bytes);
		b->start32 = (int*)b->start8;
	}
}

LBitmap* _bitmapCreate(LINT w, LINT h)
{
	LBitmap* b;
	LB* p;
	memoryEnterSafe();
	b = (LBitmap*)memoryAllocNative(sizeof(LBitmap), DBG_LOCALS, NULL, bitmapMark); if (!b) return NULL;
	b->w = w;
	b->h = h;
	b->next8 = w * 4;
	b->next32 = w;
	b->bytes = NULL;
	p = memoryAllocStr(NULL, h * b->next8); if (!p) return NULL;
	b->bytes = p;
	b->start8 = (lchar*)STR_START(b->bytes);
	b->start32 = (int*)b->start8;
#ifdef ON_WINDOWS
	b->bmp = NULL;
#endif
	memoryLeaveSafe();
	return b;
}


int fun_bitmapCreate(Thread* th)
{
	LBitmap* b;

	LINT color = STACK_PULL_INT(th);
	LINT h = STACK_PULL_INT(th);
	LINT w = STACK_INT(th,0);
	if ((h < 0) || (w < 0) || (w> BITMAP_MAX_LENGTH)||(h> BITMAP_MAX_LENGTH)) FUN_RETURN_NIL;
	b = _bitmapCreate(w, h); if (!b) return EXEC_OM;
	_bitmapFill(b, (int)color, NULL);
	FUN_RETURN_PNT((LB*)b);
}

int fun_bitmapGet(Thread* th)
{
	LINT color;
	LINT y = STACK_PULL_INT(th);
	LINT x = STACK_PULL_INT(th);
	LBitmap* b = (LBitmap*)STACK_PNT(th, 0);
	if ((!b) || (x < 0) || (x >= b->w) || (y < 0) || (y >= b->h)) FUN_RETURN_NIL;
	color = 0xffffffff & b->start32[x + y * b->next32];
	FUN_RETURN_INT(color);
}

int fun_bitmapSet(Thread* th)
{
	LINT color = STACK_PULL_INT(th);
	LINT y = STACK_PULL_INT(th);
	LINT x = STACK_PULL_INT(th);
	LBitmap* b = (LBitmap*)STACK_PNT(th, 0);
	if ((!b) || (x < 0) || (x >= b->w) || (y < 0) || (y >= b->h)) return 0;
	b->start32[x + y * b->next32] = (int)color;
	return 0;
}
int fun_bitmapPlot(Thread* th)
{
	LINT blend = STACK_PULL_INT(th);
	LINT color = STACK_PULL_INT(th);
	LINT y = STACK_PULL_INT(th);
	LINT x = STACK_PULL_INT(th);
	LBitmap* b = (LBitmap*)STACK_PNT(th, 0);
	_bitmapPlot(b,x,y,(int)color, _blendFunction(blend));
	return 0;
}
int fun_bitmapComponents(Thread* th)
{
	LINT b = STACK_PULL_INT(th);
	LINT g = STACK_PULL_INT(th);
	LINT r = STACK_PULL_INT(th);
	LINT a = STACK_PULL_INT(th);
	LBitmap* d = (LBitmap*)STACK_PNT(th, 0);
	_bitmapComponents(d, r, g, b, a);
	return 0;
}
int fun_bitmapCorners(Thread* th)
{
	LINT mask = STACK_PULL_INT(th);
	LINT w = STACK_PULL_INT(th);
	LBitmap* d = (LBitmap*)STACK_PNT(th, 0);
	_bitmapCorners(d, w, mask);
	return 0;
}
int fun_bitmapGradient(Thread* th)
{
	LINT col0h = STACK_PULL_INT(th);
	LINT colw0 = STACK_PULL_INT(th);
	LINT col00 = STACK_PULL_INT(th);
	LBitmap* d = (LBitmap*)STACK_PNT(th, 0);
	_bitmapGradient(d, col00, colw0, col0h);
	return 0;
}
int fun_bitmapFill(Thread* th)
{
	LINT blend = STACK_PULL_INT(th);
	LINT color = STACK_PULL_INT(th);
	LBitmap* b = (LBitmap*)STACK_PNT(th, 0);
	_bitmapFill(b, (int)color, _blendFunction(blend));
	return 0;
}
int fun_bitmapMakeColorTransparent(Thread* th)
{
	LINT color = STACK_PULL_INT(th);
	LBitmap* b = (LBitmap*)STACK_PNT(th, 0);
	_bitmapMakeColorTransparent(b, (int)color);
	return 0;
}

int fun_bitmapW(Thread* th)
{
	LBitmap* b = (LBitmap*)STACK_PNT(th, 0);
	if (!b) FUN_RETURN_NIL;
	FUN_RETURN_INT(b->w);
}
int fun_bitmapH(Thread* th)
{
	LBitmap* b = (LBitmap*)STACK_PNT(th, 0);
	if (!b) FUN_RETURN_NIL;
	FUN_RETURN_INT(b->h);
}

#define BITMAP_OPE_IIIII(name,ope) \
int name(Thread* th) \
{\
	LINT blend = STACK_PULL_INT(th);\
	LINT color = STACK_PULL_INT(th);\
	int hIsNil= STACK_IS_NIL(th,0);\
	LINT h = STACK_PULL_INT(th);\
	int wIsNil= STACK_IS_NIL(th,0);\
	LINT w = STACK_PULL_INT(th);\
	LINT y = STACK_PULL_INT(th);\
	LINT x = STACK_PULL_INT(th);\
	LBitmap* b = (LBitmap*)STACK_PNT(th, 0);\
	if (!b) return 0;\
	if (wIsNil) w=b->w-x;\
	if (hIsNil) h=b->h-y;\
	ope(b, x, y, w, h, (int)color, _blendFunction(blend));\
	return 0;\
}
BITMAP_OPE_IIIII(fun_bitmapRectangle, _bitmapRectangle)
BITMAP_OPE_IIIII(fun_bitmapFillRectangle, _bitmapFillRectangle)
BITMAP_OPE_IIIII(fun_bitmapLine, _bitmapLine)
BITMAP_OPE_IIIII(fun_bitmapFillCircle, _bitmapFillCircle)
BITMAP_OPE_IIIII(fun_bitmapCircle, _bitmapCircle)

int fun_bitmapScanline(Thread* th)
{
	LINT blend = STACK_PULL_INT(th);
	LINT color = STACK_PULL_INT(th);
	LINT y = STACK_PULL_INT(th);
	LINT x2 = STACK_PULL_INT(th);
	LINT x1 = STACK_PULL_INT(th);
	LBitmap* b = (LBitmap*)STACK_PNT(th, 0);
	if (!b) return 0;
	_bitmapScanline(b, x1, x2, y, (int)color, _blendFunction(blend));
	return 0;
}
int fun_bitmapBlit(Thread* th)
{
	LINT blend = STACK_PULL_INT(th);
	int hIsNil= STACK_IS_NIL(th,0);
	LINT h = STACK_PULL_INT(th);
	int wIsNil= STACK_IS_NIL(th,0);
	LINT w = STACK_PULL_INT(th);
	LINT ysrc = STACK_PULL_INT(th);
	LINT xsrc = STACK_PULL_INT(th);
	LBitmap* a = (LBitmap*)STACK_PULL_PNT(th);
	LINT ydst = STACK_PULL_INT(th);
	LINT xdst = STACK_PULL_INT(th);
	LBitmap* b = (LBitmap*)STACK_PNT(th, 0);
	if ((!b)||(!a)) return 0;
	if (wIsNil) w= a->w - xsrc;
	if (hIsNil) h= a->h - ysrc;
	_bitmapToBitmap(b, xdst, ydst, a, xsrc, ysrc, w, h, _blendFunction(blend));
	return 0;
}
int fun_bitmapColoredBlit(Thread* th)
{
	LINT coloredBlend = STACK_PULL_INT(th);
	LINT colored = STACK_PULL_INT(th);
	LINT blend = STACK_PULL_INT(th);
	int hIsNil= STACK_IS_NIL(th,0);
	LINT h = STACK_PULL_INT(th);
	int wIsNil= STACK_IS_NIL(th,0);
	LINT w = STACK_PULL_INT(th);
	LINT ysrc = STACK_PULL_INT(th);
	LINT xsrc = STACK_PULL_INT(th);
	LBitmap* a = (LBitmap*)STACK_PULL_PNT(th);
	LINT ydst = STACK_PULL_INT(th);
	LINT xdst = STACK_PULL_INT(th);
	LBitmap* b = (LBitmap*)STACK_PNT(th, 0);
	if ((!b) || (!a)) return 0;
	if (wIsNil) w= a->w - xsrc;
	if (hIsNil) h= a->h - ysrc;
	_bitmapToBitmapColored(b, xdst, ydst, a, xsrc, ysrc, w, h, _blendFunction(blend),(int)colored, _blendFunction(coloredBlend));
	return 0;
}
int fun_bitmapResize(Thread* th)
{
	LB* smooth = STACK_PULL_PNT(th);
	LBitmap* a = (LBitmap*)STACK_PULL_PNT(th);
	LBitmap* b = (LBitmap*)STACK_PNT(th, 0);
	if (smooth==MM._true) _bitmapResizeSmooth(b, a);
	else _bitmapResizeNearest(b, a);
	return 0;
}
int fun_bitmapCopy(Thread* th)
{
	int k;
	LBitmap* d = NULL;
	int hIsNil= STACK_IS_NIL(th,0);
	LINT h = STACK_PULL_INT(th);
	int wIsNil= STACK_IS_NIL(th,0);
	LINT w = STACK_PULL_INT(th);
	LINT y = STACK_PULL_INT(th);
	LINT x = STACK_PULL_INT(th);
	LBitmap* b = (LBitmap*)STACK_PNT(th, 0);
	if (!b) FUN_RETURN_NIL;
	if (wIsNil) w= b->w - x;
	if (hIsNil) h= b->h - y;
	if ((k= _bitmapCopy(b, x, y, w, h, &d))) return k;
	FUN_RETURN_PNT((LB*)d);
}
int fun_rgbFromYCrCb(Thread* th)
{
	LINT rgb = STACK_INT(th, 0);
	FUN_RETURN_INT(_rgbFromYCrCb(rgb));
}
int fun_yCrCbFromRgb(Thread* th)
{
	LINT ycrcb = STACK_INT(th, 0);
	FUN_RETURN_INT(_yCrCbFromRgb(ycrcb));
}
int fun_bitmapFromYCrCb(Thread* th)
{
	LBitmap* b = (LBitmap*)STACK_PNT(th, 0);
	if (!b) FUN_RETURN_NIL;
	_bitmapFromYCrCb(b);
	FUN_RETURN_INT(0);
}
int fun_bitmapToYCrCb(Thread* th)
{
	LBitmap* b = (LBitmap*)STACK_PNT(th, 0);
	if (!b) FUN_RETURN_NIL;
	_bitmapToYCrCb(b);
	FUN_RETURN_INT(0);
}
int fun_idct88(Thread* th)
{
	LB* array = STACK_PNT(th, 0);
	if (ARRAY_LENGTH(array) == 64) idct88(array);
	return 0;
}
int fun_dct88(Thread* th)
{
	LB* array = STACK_PNT(th, 0);
	if (ARRAY_LENGTH(array) == 64) dct88(array);
	return 0;
}
int fun_bitmapExportMono(Thread* th)
{
	LB* output;
	LINT len;
	LINT background = STACK_PULL_INT(th);
	LBitmap* b = (LBitmap*)STACK_PNT(th, 0);
	if ((!b) || (b->w & 7)) FUN_RETURN_NIL;
	len = (b->w >> 3) * b->h;
	output = memoryAllocStr(NULL, len); if (!output) return EXEC_OM;
	_bitmapExportMono(b, (int)background, STR_START(output), len);
	FUN_RETURN_PNT(output);
}
int fun_bitmapExportMonoBytes(Thread* th)
{
	LINT background = STACK_PULL_INT(th);
	LINT offset = STACK_PULL_INT(th);
	LB* output = STACK_PULL_PNT(th);
	LBitmap* b = (LBitmap*)STACK_PNT(th, 0);
	if ((!b) || (b->w & 7) || (offset < 0)) FUN_RETURN_NIL;
	_bitmapExportMono(b, (int)background, STR_START(output) + offset, STR_LENGTH(output) - offset);
	FUN_RETURN_PNT(output);
}
int fun_bitmapImportMono(Thread* th)
{
	LINT background = STACK_PULL_INT(th);
	LINT color = STACK_PULL_INT(th);
	LB* src = STACK_PULL_PNT(th);
	LBitmap* b = (LBitmap*)STACK_PNT(th, 0);
	if ((!b) || (!src)) return 0;
	_bitmapImportMono(b, (int)color, (int)background, STR_START(src), STR_LENGTH(src));
	return 0;
}
int fun_bitmapExportMonoVertical(Thread* th)
{
	LB* output;
	LINT len;
	LINT background = STACK_PULL_INT(th);
	LBitmap* b = (LBitmap*)STACK_PNT(th, 0);
	if ((!b) || (b->w & 7)) FUN_RETURN_NIL;
	len = (b->w >> 3) * b->h;
	output = memoryAllocStr(NULL, len); if (!output) return EXEC_OM;
	_bitmapExportMonoVertical(b, (int)background, STR_START(output), len);
	FUN_RETURN_PNT(output);
}
int fun_bitmapExportMonoVerticalBytes(Thread* th)
{
	LINT background = STACK_PULL_INT(th);
	LINT offset = STACK_PULL_INT(th);
	LB* output = STACK_PULL_PNT(th);
	LBitmap* b = (LBitmap*)STACK_PNT(th, 0);
	if ((!b) || (b->w & 7) || (offset<0)) FUN_RETURN_NIL;
	_bitmapExportMonoVertical(b, (int)background, STR_START(output)+offset, STR_LENGTH(output)-offset);
	FUN_RETURN_PNT(output);
}
int fun_bitmapImportMonoVertical(Thread* th)
{
	LINT background = STACK_PULL_INT(th);
	LINT color = STACK_PULL_INT(th);
	LB* src = STACK_PULL_PNT(th);
	LBitmap* b = (LBitmap*)STACK_PNT(th, 0);
	if ((!b) || (!src)) return 0;
	_bitmapImportMonoVertical(b, (int)color, (int)background, STR_START(src), STR_LENGTH(src));
	return 0;
}

int system2dInit(Pkg *system)
{
	pkgAddType(system, "BlendFunction");
	pkgAddType(system, "Component");

	static const Native nativeDefs[] = {
		{ NATIVE_INT, "COMP_R", (void*)0, "Component"},
		{ NATIVE_INT, "COMP_G", (void*)1, "Component"},
		{ NATIVE_INT, "COMP_B", (void*)2, "Component"},
		{ NATIVE_INT, "COMP_A", (void*)3, "Component"},
		{ NATIVE_INT, "COMP_0", (void*)4, "Component"},
		{ NATIVE_INT, "COMP_R_INV", (void*)5, "Component"},
		{ NATIVE_INT, "COMP_G_INV", (void*)6, "Component"},
		{ NATIVE_INT, "COMP_B_INV", (void*)7, "Component"},
		{ NATIVE_INT, "COMP_A_INV", (void*)8, "Component"},
		{ NATIVE_INT, "COMP_255", (void*)9, "Component"},
		{ NATIVE_INT, "BLEND_NONE", (void*)0, "BlendFunction"},
		{ NATIVE_INT, "BLEND_XOR", (void*)1, "BlendFunction"},
		{ NATIVE_INT, "BLEND_OR", (void*)2, "BlendFunction"},
		{ NATIVE_INT, "BLEND_AND", (void*)3, "BlendFunction"},
		{ NATIVE_INT, "BLEND_MAX", (void*)4, "BlendFunction"},
		{ NATIVE_INT, "BLEND_MIN", (void*)5, "BlendFunction"},
		{ NATIVE_INT, "BLEND_ADD", (void*)6, "BlendFunction"},
		{ NATIVE_INT, "BLEND_SUB", (void*)7, "BlendFunction"},
		{ NATIVE_INT, "BLEND_MUL", (void*)8, "BlendFunction"},
		{ NATIVE_INT, "BLEND_ALPHA", (void*)9, "BlendFunction"},
		{ NATIVE_INT, "BLEND_DESTINATION", (void*)10, "BlendFunction"},
		{ NATIVE_INT, "CORNER_TOP_LEFT", (void*)CORNER_TOP_LEFT, "Int"},
		{ NATIVE_INT, "CORNER_TOP_RIGHT", (void*)CORNER_TOP_RIGHT, "Int"},
		{ NATIVE_INT, "CORNER_BOTTOM_LEFT", (void*)CORNER_BOTTOM_LEFT, "Int"},
		{ NATIVE_INT, "CORNER_BOTTOM_RIGHT", (void*)CORNER_BOTTOM_RIGHT, "Int"},
		{ NATIVE_FUN, "bitmapCreate", fun_bitmapCreate, "fun Int Int Int -> Bitmap"},
		{ NATIVE_FUN, "bitmapW", fun_bitmapW, "fun Bitmap -> Int"},
		{ NATIVE_FUN, "bitmapH", fun_bitmapH, "fun Bitmap -> Int"},
		{ NATIVE_FUN, "bitmapCopy", fun_bitmapCopy, "fun Bitmap Int Int Int Int -> Bitmap"},
		{ NATIVE_FUN, "bitmapGet", fun_bitmapGet, "fun Bitmap Int Int -> Int"},
		{ NATIVE_FUN, "bitmapSet", fun_bitmapSet, "fun Bitmap Int Int Int -> Bitmap"},
		{ NATIVE_FUN, "bitmapPlot", fun_bitmapPlot, "fun Bitmap Int Int Int BlendFunction -> Bitmap"},
		{ NATIVE_FUN, "bitmapFill", fun_bitmapFill, "fun Bitmap Int BlendFunction -> Bitmap"},
		{ NATIVE_FUN, "bitmapMakeColorTransparent", fun_bitmapMakeColorTransparent, "fun Bitmap Int -> Bitmap"},
		{ NATIVE_FUN, "bitmapRectangle", fun_bitmapRectangle, "fun Bitmap Int Int Int Int Int BlendFunction -> Bitmap"},
		{ NATIVE_FUN, "bitmapFillRectangle", fun_bitmapFillRectangle, "fun Bitmap Int Int Int Int Int BlendFunction -> Bitmap"},
		{ NATIVE_FUN, "bitmapScanline", fun_bitmapScanline, "fun Bitmap Int Int Int Int BlendFunction -> Bitmap"},
		{ NATIVE_FUN, "bitmapLine", fun_bitmapLine, "fun Bitmap Int Int Int Int Int BlendFunction -> Bitmap"},
		{ NATIVE_FUN, "bitmapFillCircle", fun_bitmapFillCircle, "fun Bitmap Int Int Int Int Int BlendFunction -> Bitmap"},
		{ NATIVE_FUN, "bitmapCircle", fun_bitmapCircle, "fun Bitmap Int Int Int Int Int BlendFunction -> Bitmap"},
		{ NATIVE_FUN, "bitmapBlit", fun_bitmapBlit, "fun Bitmap Int Int Bitmap Int Int Int Int BlendFunction -> Bitmap"},
		{ NATIVE_FUN, "bitmapColoredBlit", fun_bitmapColoredBlit, "fun Bitmap Int Int Bitmap Int Int Int Int BlendFunction Int BlendFunction -> Bitmap"},
		{ NATIVE_FUN, "bitmapResize", fun_bitmapResize, "fun Bitmap Bitmap Bool -> Bitmap"},
		{ NATIVE_FUN, "bitmapComponents", fun_bitmapComponents, "fun Bitmap Component Component Component Component -> Bitmap"},
		{ NATIVE_FUN, "bitmapCorners", fun_bitmapCorners, "fun Bitmap Int Int -> Bitmap"},
		{ NATIVE_FUN, "bitmapGradient", fun_bitmapGradient, "fun Bitmap Int Int Int -> Bitmap"},
		{ NATIVE_FUN, "rgbFromYCrCb", fun_rgbFromYCrCb, "fun Int -> Int"},
		{ NATIVE_FUN, "yCrCbFromRgb", fun_yCrCbFromRgb, "fun Int -> Int"},
		{ NATIVE_FUN, "bitmapFromYCrCb", fun_bitmapFromYCrCb, "fun Bitmap -> Int"},
		{ NATIVE_FUN, "bitmapToYCrCb", fun_bitmapToYCrCb, "fun Bitmap -> Int"},
		{ NATIVE_FUN, "idct88", fun_idct88, "fun array Float -> array Float"},
		{ NATIVE_FUN, "dct88", fun_dct88, "fun array Float -> array Float"},
		{ NATIVE_FUN, "bitmapExportMono", fun_bitmapExportMono, "fun Bitmap Int -> Str"},
		{ NATIVE_FUN, "bitmapExportMonoBytes", fun_bitmapExportMonoBytes, "fun Bitmap Bytes Int Int -> Bitmap"},
		{ NATIVE_FUN, "bitmapImportMono", fun_bitmapImportMono, "fun Bitmap Str Int Int -> Bitmap"},
		{ NATIVE_FUN, "bitmapExportMonoVertical", fun_bitmapExportMonoVertical, "fun Bitmap Int -> Str"},
		{ NATIVE_FUN, "bitmapExportMonoVerticalBytes", fun_bitmapExportMonoVerticalBytes, "fun Bitmap Bytes Int Int -> Bitmap"},
		{ NATIVE_FUN, "bitmapImportMonoVertical", fun_bitmapImportMonoVertical, "fun Bitmap Str Int Int -> Bitmap"},
	};
	NATIVE_DEF(nativeDefs);
	return 0;
}
