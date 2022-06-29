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

#ifdef WITH_UI
#else
int fun_uiStart(Thread* th) { return fun_empty(th,6);}
int fun_uiStop(Thread* th) { return fun_empty(th,0);}
int fun_uiResize(Thread* th) { return fun_empty(th,4);}

int fun_uiW(Thread* th) { return fun_empty(th,0);}
int fun_uiH(Thread* th) { return fun_empty(th,0);}
int fun_uiX(Thread* th) { return fun_empty(th,0);}
int fun_uiY(Thread* th) { return fun_empty(th,0);}
int fun_uiUpdate(Thread* th) { return fun_empty(th,5);}

int fun_screenW(Thread* th) { return fun_empty(th, 0); }
int fun_screenH(Thread* th) { return fun_empty(th, 0); }

int fun_uiSetName(Thread* th) { return fun_empty(th, 1); }
int fun_uiFocus(Thread* th) { return fun_empty(th, 0); }
int fun_keyboardState(Thread* th) { return fun_empty(th, 0); }

int fun_clipboardCopy(Thread* th) { return fun_empty(th, 1); }
int fun_clipboardPaste(Thread* th) { return fun_empty(th, 0); }

int fun_keyboardShow(Thread* th) { return fun_empty(th, 0); }
int fun_keyboardHide(Thread* th) { return fun_empty(th, 0); }
int fun_keyboardHeight(Thread* th) { return fun_empty(th, 0); }
int fun_orientationGet(Thread* th) { return fun_empty(th, 0); }
int fun_accelerometerX(Thread* th) { return fun_empty(th, 0); }
int fun_accelerometerY(Thread* th) { return fun_empty(th, 0); }
int fun_accelerometerZ(Thread* th) { return fun_empty(th, 0); }
int fun_accelerometerInit(Thread* th){ return fun_empty(th, 1); }


int coreUiHwInit(Thread* th, Pkg* system) {return 0;}
#endif

#ifndef WITH_GLES2
int fun_glMakeContext(Thread* th) {return fun_empty(th,0);}
#endif
int coreUiInit(Thread* th, Pkg *system)
{
	Type* fun_I_I_I_I_I_S_B = typeAlloc(th, TYPECODE_FUN, NULL, 7, MM.I, MM.I, MM.I, MM.I, MM.I, MM.S, MM.Boolean);
	Type* fun_I_I_I_I_B = typeAlloc(th, TYPECODE_FUN, NULL, 5, MM.I, MM.I, MM.I, MM.I, MM.Boolean);
	Type* fun_B = typeAlloc(th, TYPECODE_FUN, NULL, 1, MM.Boolean);
	Type* fun_F = typeAlloc(th, TYPECODE_FUN, NULL, 1, MM.F);
    Type* fun_F_F = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.F, MM.F);
	Type* fun_S = typeAlloc(th, TYPECODE_FUN, NULL, 1, MM.S);
	Type* fun_S_S = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.S, MM.S);
	Type* fun_I = typeAlloc(th, TYPECODE_FUN, NULL, 1, MM.I);
	Type* fun_Bmp_I_I_I_I_Bmp = typeAlloc(th, TYPECODE_FUN, NULL, 6, MM.Bitmap, MM.I, MM.I, MM.I, MM.I, MM.Bitmap);

	coreUiHwInit(th,system);
#ifdef WITH_GLES2
	coreUiGLES2Init(th, system);
#else
	pkgAddFun(th, system, "_glMakeContext", fun_glMakeContext, fun_I);
#endif
	pkgAddConst(th, system, "UI_NORMAL", INTTOVAL(UI_NORMAL), MM.I);
	pkgAddConst(th, system, "UI_RESIZE", INTTOVAL(UI_RESIZE), MM.I);
	pkgAddConst(th, system, "UI_FRAME", INTTOVAL(UI_FRAME), MM.I);
	pkgAddConst(th, system, "UI_GL", INTTOVAL(UI_GL), MM.I);
	pkgAddConst(th, system, "UI_MAXIMIZE", INTTOVAL(UI_MAXIMIZE), MM.I);

	pkgAddFun(th, system, "_uiStart", fun_uiStart, fun_I_I_I_I_I_S_B);
	pkgAddFun(th, system, "_uiStop", fun_uiStop, fun_B);
	pkgAddFun(th, system, "uiResize", fun_uiResize, fun_I_I_I_I_B);
	pkgAddFun(th, system, "uiW", fun_uiW, fun_I);
	pkgAddFun(th, system, "uiH", fun_uiH, fun_I);
	pkgAddFun(th, system, "uiX", fun_uiX, fun_I);
	pkgAddFun(th, system, "uiY", fun_uiY, fun_I);
	pkgAddFun(th, system, "_uiUpdate", fun_uiUpdate, fun_Bmp_I_I_I_I_Bmp);
	pkgAddFun(th, system, "screenW", fun_screenW, fun_I);
	pkgAddFun(th, system, "screenH", fun_screenH, fun_I);

	pkgAddFun(th, system, "uiSetName", fun_uiSetName, fun_S_S);
	pkgAddFun(th, system, "uiFocus", fun_uiFocus, fun_I);
	pkgAddFun(th, system, "keyboardState", fun_keyboardState, fun_I);

	pkgAddFun(th, system, "clipboardCopy", fun_clipboardCopy, fun_S_S);
	pkgAddFun(th, system, "clipboardPaste", fun_clipboardPaste, fun_S);

	pkgAddFun(th, system, "keyboardShow", fun_keyboardShow, fun_B);
	pkgAddFun(th, system, "keyboardHide", fun_keyboardHide, fun_B);
	pkgAddFun(th, system, "keyboardHeight", fun_keyboardHeight, fun_I);

	pkgAddFun(th, system, "orientationGet", fun_orientationGet, fun_I);
	pkgAddFun(th, system, "accelerometerX", fun_accelerometerX, fun_F);
	pkgAddFun(th, system, "accelerometerY", fun_accelerometerY, fun_F);
	pkgAddFun(th, system, "accelerometerZ", fun_accelerometerZ, fun_F);
    pkgAddFun(th, system, "accelerometerInit", fun_accelerometerInit, fun_F_F);

    
	pkgAddConst(th, system, "XK_Pause", INTTOVAL(XK_Pause), MM.I);
	pkgAddConst(th, system, "XK_Scroll_Lock", INTTOVAL(XK_Scroll_Lock), MM.I);
	pkgAddConst(th, system, "XK_Sys_Req", INTTOVAL(XK_Sys_Req), MM.I);
	pkgAddConst(th, system, "XK_Delete", INTTOVAL(XK_Delete), MM.I);
	pkgAddConst(th, system, "XK_Home", INTTOVAL(XK_Home), MM.I);
	pkgAddConst(th, system, "XK_Left", INTTOVAL(XK_Left), MM.I);
	pkgAddConst(th, system, "XK_Up", INTTOVAL(XK_Up), MM.I);
	pkgAddConst(th, system, "XK_Right", INTTOVAL(XK_Right), MM.I);
	pkgAddConst(th, system, "XK_Down", INTTOVAL(XK_Down), MM.I);
	pkgAddConst(th, system, "XK_Prior", INTTOVAL(XK_Prior), MM.I);
	pkgAddConst(th, system, "XK_Page_Up", INTTOVAL(XK_Page_Up), MM.I);
	pkgAddConst(th, system, "XK_Next", INTTOVAL(XK_Next), MM.I);
	pkgAddConst(th, system, "XK_Page_Down", INTTOVAL(XK_Page_Down), MM.I);
	pkgAddConst(th, system, "XK_End", INTTOVAL(XK_End), MM.I);
	pkgAddConst(th, system, "XK_Insert", INTTOVAL(XK_Insert), MM.I);
	pkgAddConst(th, system, "XK_Num_Lock", INTTOVAL(XK_Num_Lock), MM.I);
	pkgAddConst(th, system, "XK_F1", INTTOVAL(XK_F1), MM.I);
	pkgAddConst(th, system, "XK_F2", INTTOVAL(XK_F2), MM.I);
	pkgAddConst(th, system, "XK_F3", INTTOVAL(XK_F3), MM.I);
	pkgAddConst(th, system, "XK_F4", INTTOVAL(XK_F4), MM.I);
	pkgAddConst(th, system, "XK_F5", INTTOVAL(XK_F5), MM.I);
	pkgAddConst(th, system, "XK_F6", INTTOVAL(XK_F6), MM.I);
	pkgAddConst(th, system, "XK_F7", INTTOVAL(XK_F7), MM.I);
	pkgAddConst(th, system, "XK_F8", INTTOVAL(XK_F8), MM.I);
	pkgAddConst(th, system, "XK_F9", INTTOVAL(XK_F9), MM.I);
	pkgAddConst(th, system, "XK_F10", INTTOVAL(XK_F10), MM.I);
	pkgAddConst(th, system, "XK_F11", INTTOVAL(XK_F11), MM.I);
	pkgAddConst(th, system, "XK_F12", INTTOVAL(XK_F12), MM.I);
	pkgAddConst(th, system, "XK_Caps_Lock", INTTOVAL(XK_Caps_Lock), MM.I);
	pkgAddConst(th, system, "KS_Shift", INTTOVAL(KS_Shift), MM.I);
	pkgAddConst(th, system, "KS_Alt", INTTOVAL(KS_Alt), MM.I);
	pkgAddConst(th, system, "KS_Control", INTTOVAL(KS_Control), MM.I);
	pkgAddConst(th, system, "KS_Meta", INTTOVAL(KS_Meta), MM.I);

	return 0;
}
