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

void bitmapMark(LB* user)
{
	LBitmap* b = (LBitmap*)user;
	MEMORY_MARK(user, b->bytes);
}

LBitmap* _bitmapCreate(Thread* th, LINT w, LINT h)
{
	LBitmap* b;
	memoryEnterFast();
	b = (LBitmap*)memoryAllocExt(th, sizeof(LBitmap), DBG_LOCALS, NULL, bitmapMark); if (!b) return NULL;
	b->w = w;
	b->h = h;
	b->next8 = w * 4;
	b->next32 = w;
	b->bytes = NULL;
	b->bytes = memoryAllocStr(th, NULL, h * b->next8); if (!b->bytes) return NULL;
	b->start8 = (lchar*)STR_START(b->bytes);
	b->start32 = (int*)b->start8;
#ifdef ON_WINDOWS
	b->bmp = NULL;
#endif
	memoryLeaveFast();
	return b;
}


int fun_bitmapCreate(Thread* th)
{
	LBitmap* b;

	LINT color = STACK_PULL_INT(th);
	LINT h = STACK_PULL_INT(th);
	LINT w = STACK_INT(th,0);
	if ((h < 0) || (w < 0) || (w> BITMAP_MAX_LENGTH)||(h> BITMAP_MAX_LENGTH)) FUN_RETURN_NIL;
	b = _bitmapCreate(th, w, h); if (!b) return EXEC_OM;
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
	if ((k= _bitmapCopy(th, b, x, y, w, h, &d))) return k;
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
int core2dInit(Thread* th, Pkg *system)
{
	Def* BlendFunction=pkgAddType(th, system, "BlendFunction");
	Def* Component=pkgAddType(th, system, "Component");
	Type* fun_I_I = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.Int, MM.Int);
	Type* fun_B_I = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.Bitmap, MM.Int);
	Type* fun_B_I_I_I_I_Blend_B = typeAlloc(th, TYPECODE_FUN, NULL, 7, MM.Bitmap, MM.Int, MM.Int, MM.Int, MM.Int, BlendFunction->type, MM.Bitmap);
	Type* fun_B_I_I_I_I_I_Blend_B = typeAlloc(th, TYPECODE_FUN, NULL, 8, MM.Bitmap, MM.Int, MM.Int, MM.Int, MM.Int, MM.Int, BlendFunction->type, MM.Bitmap);
	Type* fun_B_I_I_B_I_I_I_I_Blend_B = typeAlloc(th, TYPECODE_FUN, NULL, 10, MM.Bitmap, MM.Int, MM.Int, MM.Bitmap, MM.Int, MM.Int, MM.Int, MM.Int, BlendFunction->type, MM.Bitmap);
	Type* fun_B_I_I_B_I_I_I_I_Blend_I_Blend_B = typeAlloc(th, TYPECODE_FUN, NULL, 12, MM.Bitmap, MM.Int, MM.Int, MM.Bitmap, MM.Int, MM.Int, MM.Int, MM.Int, BlendFunction->type, MM.Int, BlendFunction->type, MM.Bitmap);
	Type* arrayF = typeAlloc(th, TYPECODE_ARRAY, NULL, 1, MM.Float);
	_colorInit();

	pkgAddConstInt(th, system, "COMP_R", 0, Component->type);
	pkgAddConstInt(th, system, "COMP_G", 1, Component->type);
	pkgAddConstInt(th, system, "COMP_B", 2, Component->type);
	pkgAddConstInt(th, system, "COMP_A", 3, Component->type);
	pkgAddConstInt(th, system, "COMP_0", 4, Component->type);
	pkgAddConstInt(th, system, "COMP_R_INV", 5, Component->type);
	pkgAddConstInt(th, system, "COMP_G_INV", 6, Component->type);
	pkgAddConstInt(th, system, "COMP_B_INV", 7, Component->type);
	pkgAddConstInt(th, system, "COMP_A_INV", 8, Component->type);
	pkgAddConstInt(th, system, "COMP_255", 9, Component->type);

	pkgAddConstInt(th, system,"BLEND_NONE",0,BlendFunction->type);
	pkgAddConstInt(th, system,"BLEND_XOR",1,BlendFunction->type);
	pkgAddConstInt(th, system,"BLEND_OR",2,BlendFunction->type);
	pkgAddConstInt(th, system,"BLEND_AND",3,BlendFunction->type);
	pkgAddConstInt(th, system,"BLEND_MAX",4,BlendFunction->type);
	pkgAddConstInt(th, system,"BLEND_MIN",5,BlendFunction->type);
	pkgAddConstInt(th, system,"BLEND_ADD",6,BlendFunction->type);
	pkgAddConstInt(th, system,"BLEND_SUB",7,BlendFunction->type);
	pkgAddConstInt(th, system,"BLEND_MUL",8,BlendFunction->type);
	pkgAddConstInt(th, system,"BLEND_ALPHA",9,BlendFunction->type);
	pkgAddConstInt(th, system,"BLEND_DESTINATION",10,BlendFunction->type);

	pkgAddFun(th, system, "bitmapCreate",fun_bitmapCreate,typeAlloc(th, TYPECODE_FUN,NULL,4,MM.Int, MM.Int,MM.Int, MM.Bitmap));
	pkgAddFun(th, system, "bitmapW", fun_bitmapW, fun_B_I);
	pkgAddFun(th, system, "bitmapH", fun_bitmapH, fun_B_I);
	pkgAddFun(th, system, "bitmapCopy", fun_bitmapCopy, typeAlloc(th, TYPECODE_FUN, NULL, 6, MM.Bitmap, MM.Int, MM.Int, MM.Int, MM.Int, MM.Bitmap));
	pkgAddFun(th, system, "bitmapGet", fun_bitmapGet, typeAlloc(th, TYPECODE_FUN, NULL, 4, MM.Bitmap, MM.Int, MM.Int, MM.Int));
	pkgAddFun(th, system, "bitmapSet", fun_bitmapSet, typeAlloc(th, TYPECODE_FUN, NULL, 5, MM.Bitmap, MM.Int, MM.Int, MM.Int, MM.Bitmap));
	pkgAddFun(th, system, "bitmapPlot", fun_bitmapPlot, typeAlloc(th, TYPECODE_FUN, NULL, 6, MM.Bitmap, MM.Int, MM.Int, MM.Int, BlendFunction->type, MM.Bitmap));
	pkgAddFun(th, system, "bitmapFill", fun_bitmapFill, typeAlloc(th, TYPECODE_FUN, NULL, 4, MM.Bitmap, MM.Int, BlendFunction->type, MM.Bitmap));
	pkgAddFun(th, system, "bitmapMakeColorTransparent", fun_bitmapMakeColorTransparent, typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.Bitmap, MM.Int, MM.Bitmap));
	pkgAddFun(th, system, "bitmapRectangle", fun_bitmapRectangle, fun_B_I_I_I_I_I_Blend_B);
	pkgAddFun(th, system, "bitmapFillRectangle", fun_bitmapFillRectangle, fun_B_I_I_I_I_I_Blend_B);
	pkgAddFun(th, system, "bitmapScanline", fun_bitmapScanline, fun_B_I_I_I_I_Blend_B);
	pkgAddFun(th, system, "bitmapLine", fun_bitmapLine, fun_B_I_I_I_I_I_Blend_B);
	pkgAddFun(th, system, "bitmapFillCircle", fun_bitmapFillCircle, fun_B_I_I_I_I_I_Blend_B);
	pkgAddFun(th, system, "bitmapCircle", fun_bitmapCircle, fun_B_I_I_I_I_I_Blend_B);
	pkgAddFun(th, system, "bitmapBlit", fun_bitmapBlit, fun_B_I_I_B_I_I_I_I_Blend_B);
	pkgAddFun(th, system, "bitmapColoredBlit", fun_bitmapColoredBlit, fun_B_I_I_B_I_I_I_I_Blend_I_Blend_B);
	pkgAddFun(th, system, "bitmapResize", fun_bitmapResize, typeAlloc(th, TYPECODE_FUN, NULL, 4, MM.Bitmap, MM.Bitmap, MM.Boolean, MM.Bitmap));
	pkgAddFun(th, system, "bitmapComponents", fun_bitmapComponents, typeAlloc(th, TYPECODE_FUN, NULL, 6, MM.Bitmap, Component->type, Component->type, Component->type, Component->type, MM.Bitmap));

	pkgAddFun(th, system, "rgbFromYCrCb", fun_rgbFromYCrCb, fun_I_I);
	pkgAddFun(th, system, "yCrCbFromRgb", fun_yCrCbFromRgb, fun_I_I);
	pkgAddFun(th, system, "bitmapFromYCrCb", fun_bitmapFromYCrCb, fun_B_I);
	pkgAddFun(th, system, "bitmapToYCrCb", fun_bitmapToYCrCb, fun_B_I);
	pkgAddFun(th, system, "idct88", fun_idct88, typeAlloc(th, TYPECODE_FUN, NULL, 2, arrayF, arrayF));
	pkgAddFun(th, system, "dct88", fun_dct88, typeAlloc(th, TYPECODE_FUN, NULL, 2, arrayF, arrayF));

	
	return 0;
}
