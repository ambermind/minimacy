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
	MEMORYMARK(user, b->bytes);
}

int bitmapForget(LB* user)
{
#ifdef WITH_UI
#ifdef USE_X11
    LBitmap* b = (LBitmap*)user;
	if (b->bmp) XDestroyImage((XImage*)b->bmp);
#endif
#ifdef ON_WINDOWS
    LBitmap* b = (LBitmap*)user;
	if (b->bmp) DeleteObject(b->bmp);
#endif
#endif
	return 0;
}
LBitmap* _bitmapCreate(Thread* th, LINT w, LINT h)
{
	LBitmap* b;
	memoryEnterFast(th);
	b = (LBitmap*)memoryAllocExt(th, sizeof(LBitmap), DBG_LOCALS, bitmapForget, bitmapMark); if (!b) return NULL;
	b->w = w;
	b->h = h;
	b->next8 = w * 4;
	b->next32 = w;
	b->bytes = NULL;
	b->bytes = memoryAllocStr(th, NULL, h * b->next8); if (!b->bytes) return NULL;
	b->start8 = (lchar*)STRSTART(b->bytes);
	b->start32 = (int*)b->start8;
#ifdef WITH_UI
	b->bmp = NULL;
#endif
	memoryLeaveFast(th);
	return b;
}


int fun_bitmapCreate(Thread* th)
{
	LBitmap* b;
	LW wcolor = STACKPULL(th);
	LINT h = VALTOINT(STACKPULL(th));
	LINT w = VALTOINT(STACKGET(th,0));
	STACKSETNIL(th, 0);
	if ((h < 0) || (w < 0) || (w> BITMAP_MAX_LEN)||(h> BITMAP_MAX_LEN)) return 0;
	b = _bitmapCreate(th, w, h); if (!b) return EXEC_OM;
	if (wcolor != NIL) _bitmapFill(b,(int)VALTOINT(wcolor),NULL);
	STACKSET(th, 0, PNTTOVAL(b));
	return 0;
}

int fun_bitmapGet(Thread* th)
{
	LINT color;
	LINT y = VALTOINT(STACKPULL(th));
	LINT x = VALTOINT(STACKPULL(th));
	LBitmap* b = (LBitmap*)VALTOPNT(STACKGET(th, 0));
	if ((!b) || (x < 0) || (x >= b->w) || (y < 0) || (y >= b->h))
	{
		STACKSETNIL(th, 0);
		return 0;
	}
	color = 0xffffffff & b->start32[x + y * b->next32];
	STACKSETINT(th, 0, color);
	return 0;
}

int fun_bitmapSet(Thread* th)
{
	LINT color = VALTOINT(STACKPULL(th));
	LINT y = VALTOINT(STACKPULL(th));
	LINT x = VALTOINT(STACKPULL(th));
	LBitmap* b = (LBitmap*)VALTOPNT(STACKGET(th, 0));
	if ((!b) || (x < 0) || (x >= b->w) || (y < 0) || (y >= b->h)) return 0;
	b->start32[x + y * b->next32] = (int)color;
	return 0;
}
int fun_bitmapPlot(Thread* th)
{
	LINT blend = VALTOINT(STACKPULL(th));
	LINT color = VALTOINT(STACKPULL(th));
	LINT y = VALTOINT(STACKPULL(th));
	LINT x = VALTOINT(STACKPULL(th));
	LBitmap* b = (LBitmap*)VALTOPNT(STACKGET(th, 0));
	_bitmapPlot(b,x,y,(int)color, _blendFunction(blend));
	return 0;
}
int fun_bitmapComponents(Thread* th)
{
	LINT b = VALTOINT(STACKPULL(th));
	LINT g = VALTOINT(STACKPULL(th));
	LINT r = VALTOINT(STACKPULL(th));
	LINT a = VALTOINT(STACKPULL(th));
	LBitmap* d = (LBitmap*)VALTOPNT(STACKGET(th, 0));
	_bitmapComponents(d, r, g, b, a);
	return 0;
}
int fun_bitmapFill(Thread* th)
{
	LINT blend = VALTOINT(STACKPULL(th));
	LINT color = VALTOINT(STACKPULL(th));
	LBitmap* b = (LBitmap*)VALTOPNT(STACKGET(th, 0));
	_bitmapFill(b, (int)color, _blendFunction(blend));
	return 0;
}
int fun_bitmapMakeColorTransparent(Thread* th)
{
	LINT color = VALTOINT(STACKPULL(th));
	LBitmap* b = (LBitmap*)VALTOPNT(STACKGET(th, 0));
	_bitmapMakeColorTransparent(b, (int)color);
	return 0;
}

int fun_bitmapW(Thread* th)
{
	LBitmap* b = (LBitmap*)VALTOPNT(STACKGET(th, 0));
	if (!b) return 0;
	STACKSETINT(th, 0, b->w);
	return 0;
}
int fun_bitmapH(Thread* th)
{
	LBitmap* b = (LBitmap*)VALTOPNT(STACKGET(th, 0));
	if (!b) return 0;
	STACKSETINT(th, 0, b->h);
	return 0;
}

#define BITMAP_OPE_IIIII(name,ope) \
int name(Thread* th) \
{\
	LINT w,h;\
	LINT blend = VALTOINT(STACKPULL(th));\
	LINT color = VALTOINT(STACKPULL(th));\
	LW hh = STACKPULL(th);\
	LW ww = STACKPULL(th);\
	LINT y = VALTOINT(STACKPULL(th));\
	LINT x = VALTOINT(STACKPULL(th));\
	LBitmap* b = (LBitmap*)VALTOPNT(STACKGET(th, 0));\
	if (!b) return 0;\
	w= (ww==NIL)? b->w-x : VALTOINT(ww);\
	h= (hh==NIL)? b->h-y : VALTOINT(hh);\
	ope(b, x, y, w, h, (int)color, _blendFunction(blend));\
	return 0;\
}
BITMAP_OPE_IIIII(fun_bitmapRectangle, _bitmapRectangle)
BITMAP_OPE_IIIII(fun_bitmapFilledRectangle, _bitmapFilledRectangle)
BITMAP_OPE_IIIII(fun_bitmapLine, _bitmapLine)
BITMAP_OPE_IIIII(fun_bitmapFilledCircle, _bitmapFilledCircle)
BITMAP_OPE_IIIII(fun_bitmapCircle, _bitmapCircle)

int fun_bitmapScanline(Thread* th)
{
	LINT blend = VALTOINT(STACKPULL(th));
	LINT color = VALTOINT(STACKPULL(th));
	LINT y = VALTOINT(STACKPULL(th));
	LINT x2 = VALTOINT(STACKPULL(th));
	LINT x1 = VALTOINT(STACKPULL(th));
	LBitmap* b = (LBitmap*)VALTOPNT(STACKGET(th, 0));
	if (!b) return 0;
	_bitmapScanline(b, x1, x2, y, (int)color, _blendFunction(blend));
	return 0;
}
int fun_bitmapToBitmap(Thread* th)
{
	LINT w, h;
	LINT blend = VALTOINT(STACKPULL(th));
	LW hh = STACKPULL(th);
	LW ww = STACKPULL(th);
	LINT ysrc = VALTOINT(STACKPULL(th));
	LINT xsrc = VALTOINT(STACKPULL(th));
	LBitmap* a = (LBitmap*)VALTOPNT(STACKPULL(th));
	LINT ydst = VALTOINT(STACKPULL(th));
	LINT xdst = VALTOINT(STACKPULL(th));
	LBitmap* b = (LBitmap*)VALTOPNT(STACKGET(th, 0));
	if ((!b)||(!a)) return 0;
	w = (ww == NIL) ? a->w - xsrc : VALTOINT(ww);
	h = (hh == NIL) ? a->h - ysrc : VALTOINT(hh);
	_bitmapToBitmap(b, xdst, ydst, a, xsrc, ysrc, w, h, _blendFunction(blend));
	return 0;
}
int fun_bitmapToBitmapColored(Thread* th)
{
	LINT w, h;
	LINT coloredBlend = VALTOINT(STACKPULL(th));
	LINT colored = VALTOINT(STACKPULL(th));
	LINT blend = VALTOINT(STACKPULL(th));
	LW hh = STACKPULL(th);
	LW ww = STACKPULL(th);
	LINT ysrc = VALTOINT(STACKPULL(th));
	LINT xsrc = VALTOINT(STACKPULL(th));
	LBitmap* a = (LBitmap*)VALTOPNT(STACKPULL(th));
	LINT ydst = VALTOINT(STACKPULL(th));
	LINT xdst = VALTOINT(STACKPULL(th));
	LBitmap* b = (LBitmap*)VALTOPNT(STACKGET(th, 0));
	if ((!b) || (!a)) return 0;
	w = (ww == NIL) ? a->w - xsrc : VALTOINT(ww);
	h = (hh == NIL) ? a->h - ysrc : VALTOINT(hh);
	_bitmapToBitmapColored(b, xdst, ydst, a, xsrc, ysrc, w, h, _blendFunction(blend),(int)colored, _blendFunction(coloredBlend));
	return 0;
}
int fun_bitmapResize(Thread* th)
{
	LW smooth = STACKPULL(th);
	LBitmap* a = (LBitmap*)VALTOPNT(STACKPULL(th));
	LBitmap* b = (LBitmap*)VALTOPNT(STACKGET(th, 0));
	if (smooth==MM.trueRef) _bitmapResizeSmooth(b, a);
	else _bitmapResizeNearest(b, a);
	return 0;
}

int fun_bitmapCopy(Thread* th)
{
	int k;
	LBitmap* d = NULL;
	LINT w, h;
	LW hh = STACKPULL(th);
	LW ww = STACKPULL(th);
	LINT y = VALTOINT(STACKPULL(th));
	LINT x = VALTOINT(STACKPULL(th));
	LBitmap* b = (LBitmap*)VALTOPNT(STACKGET(th, 0));
	if (!b) return 0;
	w = (ww == NIL) ? b->w - x : VALTOINT(ww);
	h = (hh == NIL) ? b->h - y : VALTOINT(hh);
	if ((k= _bitmapCopy(th, b, x, y, w, h, &d))) return k;
	STACKSET(th, 0, PNTTOVAL(d));
	return 0;
}


int core2dInit(Thread* th, Pkg *system)
{
	Ref* BlendFunction=pkgAddType(th, system, "BlendFunction");
	Ref* Component=pkgAddType(th, system, "Component");
	Type* fun_B_I = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.Bitmap, MM.I);
	Type* fun_S_B = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.S, MM.Bitmap);
	Type* fun_B_Bool_S = typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.Bitmap, MM.Boolean, MM.S);
	Type* fun_B_I_S = typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.Bitmap, MM.I, MM.S);
	Type* fun_B_I_I_I_I_Blend_B = typeAlloc(th, TYPECODE_FUN, NULL, 7, MM.Bitmap, MM.I, MM.I, MM.I, MM.I, BlendFunction->type, MM.Bitmap);
	Type* fun_B_I_I_I_I_I_Blend_B = typeAlloc(th, TYPECODE_FUN, NULL, 8, MM.Bitmap, MM.I, MM.I, MM.I, MM.I, MM.I, BlendFunction->type, MM.Bitmap);
	Type* fun_B_I_I_B_I_I_I_I_Blend_B = typeAlloc(th, TYPECODE_FUN, NULL, 10, MM.Bitmap, MM.I, MM.I, MM.Bitmap, MM.I, MM.I, MM.I, MM.I, BlendFunction->type, MM.Bitmap);
	Type* fun_B_I_I_B_I_I_I_I_Blend_I_Blend_B = typeAlloc(th, TYPECODE_FUN, NULL, 12, MM.Bitmap, MM.I, MM.I, MM.Bitmap, MM.I, MM.I, MM.I, MM.I, BlendFunction->type, MM.I, BlendFunction->type, MM.Bitmap);
	
	_colorInit();

	pkgAddConst(th, system, "COMP_R", INTTOVAL(0), Component->type);
	pkgAddConst(th, system, "COMP_G", INTTOVAL(1), Component->type);
	pkgAddConst(th, system, "COMP_B", INTTOVAL(2), Component->type);
	pkgAddConst(th, system, "COMP_A", INTTOVAL(3), Component->type);
	pkgAddConst(th, system, "COMP_0", INTTOVAL(4), Component->type);
	pkgAddConst(th, system, "COMP_R_INV", INTTOVAL(5), Component->type);
	pkgAddConst(th, system, "COMP_G_INV", INTTOVAL(6), Component->type);
	pkgAddConst(th, system, "COMP_B_INV", INTTOVAL(7), Component->type);
	pkgAddConst(th, system, "COMP_A_INV", INTTOVAL(8), Component->type);
	pkgAddConst(th, system, "COMP_255", INTTOVAL(9), Component->type);

	pkgAddConst(th, system,"BLEND_NONE",INTTOVAL(0),BlendFunction->type);
	pkgAddConst(th, system,"BLEND_XOR",INTTOVAL(1),BlendFunction->type);
	pkgAddConst(th, system,"BLEND_OR",INTTOVAL(2),BlendFunction->type);
	pkgAddConst(th, system,"BLEND_AND",INTTOVAL(3),BlendFunction->type);
	pkgAddConst(th, system,"BLEND_MAX",INTTOVAL(4),BlendFunction->type);
	pkgAddConst(th, system,"BLEND_MIN",INTTOVAL(5),BlendFunction->type);
	pkgAddConst(th, system,"BLEND_ADD",INTTOVAL(6),BlendFunction->type);
	pkgAddConst(th, system,"BLEND_SUB",INTTOVAL(7),BlendFunction->type);
	pkgAddConst(th, system,"BLEND_MUL",INTTOVAL(8),BlendFunction->type);
	pkgAddConst(th, system,"BLEND_ALPHA",INTTOVAL(9),BlendFunction->type);

	pkgAddFun(th, system, "bitmapCreate",fun_bitmapCreate,typeAlloc(th, TYPECODE_FUN,NULL,4,MM.I, MM.I,MM.I, MM.Bitmap));
	pkgAddFun(th, system, "bitmapW", fun_bitmapW, fun_B_I);
	pkgAddFun(th, system, "bitmapH", fun_bitmapH, fun_B_I);
	pkgAddFun(th, system, "bitmapCopy", fun_bitmapCopy, typeAlloc(th, TYPECODE_FUN, NULL, 6, MM.Bitmap, MM.I, MM.I, MM.I, MM.I, MM.Bitmap));
	pkgAddFun(th, system, "bitmapGet", fun_bitmapGet, typeAlloc(th, TYPECODE_FUN, NULL, 4, MM.Bitmap, MM.I, MM.I, MM.I));
	pkgAddFun(th, system, "bitmapSet", fun_bitmapSet, typeAlloc(th, TYPECODE_FUN, NULL, 5, MM.Bitmap, MM.I, MM.I, MM.I, MM.Bitmap));
	pkgAddFun(th, system, "bitmapPlot", fun_bitmapPlot, typeAlloc(th, TYPECODE_FUN, NULL, 6, MM.Bitmap, MM.I, MM.I, MM.I, BlendFunction->type, MM.Bitmap));
	pkgAddFun(th, system, "bitmapFill", fun_bitmapFill, typeAlloc(th, TYPECODE_FUN, NULL, 4, MM.Bitmap, MM.I, BlendFunction->type, MM.Bitmap));
	pkgAddFun(th, system, "bitmapMakeColorTransparent", fun_bitmapMakeColorTransparent, typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.Bitmap, MM.I, MM.Bitmap));
	pkgAddFun(th, system, "bitmapRectangle", fun_bitmapRectangle, fun_B_I_I_I_I_I_Blend_B);
	pkgAddFun(th, system, "bitmapFilledRectangle", fun_bitmapFilledRectangle, fun_B_I_I_I_I_I_Blend_B);
	pkgAddFun(th, system, "bitmapScanline", fun_bitmapScanline, fun_B_I_I_I_I_Blend_B);
	pkgAddFun(th, system, "bitmapLine", fun_bitmapLine, fun_B_I_I_I_I_I_Blend_B);
	pkgAddFun(th, system, "bitmapFilledCircle", fun_bitmapFilledCircle, fun_B_I_I_I_I_I_Blend_B);
	pkgAddFun(th, system, "bitmapCircle", fun_bitmapCircle, fun_B_I_I_I_I_I_Blend_B);
	pkgAddFun(th, system, "bitmapToBitmap", fun_bitmapToBitmap, fun_B_I_I_B_I_I_I_I_Blend_B);
	pkgAddFun(th, system, "bitmapToBitmapColored", fun_bitmapToBitmapColored, fun_B_I_I_B_I_I_I_I_Blend_I_Blend_B);
	pkgAddFun(th, system, "bitmapResize", fun_bitmapResize, typeAlloc(th, TYPECODE_FUN, NULL, 4, MM.Bitmap, MM.Bitmap, MM.Boolean, MM.Bitmap));
	pkgAddFun(th, system, "bitmapComponents", fun_bitmapComponents, typeAlloc(th, TYPECODE_FUN, NULL, 6, MM.Bitmap, Component->type, Component->type, Component->type, Component->type, MM.Bitmap));

	return 0;
}
