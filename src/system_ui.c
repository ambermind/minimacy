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
#include"system_ui_keydef.h"

#ifdef WITH_UI
#else
int fun_uiStart(Thread* th) FUN_RETURN_NIL
int fun_uiStop(Thread* th) FUN_RETURN_NIL
int fun_uiResize(Thread* th) FUN_RETURN_NIL

int fun_uiW(Thread* th) FUN_RETURN_NIL
int fun_uiH(Thread* th) FUN_RETURN_NIL
int fun_uiDrop(Thread* th) FUN_RETURN_NIL

int fun_screenW(Thread* th) FUN_RETURN_NIL
int fun_screenH(Thread* th) FUN_RETURN_NIL

int fun_uiSetTitle(Thread* th) FUN_RETURN_NIL
int fun_keyboardState(Thread* th) FUN_RETURN_NIL

int fun_clipboardCopy(Thread* th) FUN_RETURN_NIL
int fun_clipboardPaste(Thread* th) FUN_RETURN_NIL

int fun_cursorSize(Thread* th) FUN_RETURN_NIL
int fun_cursorCreate(Thread* th) FUN_RETURN_NIL
int fun_cursorShow(Thread* th) FUN_RETURN_NIL

int fun_keyboardShow(Thread* th) FUN_RETURN_NIL
int fun_keyboardHide(Thread* th) FUN_RETURN_NIL
int fun_keyboardHeight(Thread* th) FUN_RETURN_NIL
int fun_orientationGet(Thread* th) FUN_RETURN_NIL
int fun_accelerometerX(Thread* th) FUN_RETURN_NIL
int fun_accelerometerY(Thread* th) FUN_RETURN_NIL
int fun_accelerometerZ(Thread* th) FUN_RETURN_NIL
int fun_accelerometerInit(Thread* th)FUN_RETURN_NIL


int coreUiHwInit(Thread* th, Pkg* system) {return 0;}
#endif


#ifndef WITH_GL
int fun_glMakeContext(Thread* th) FUN_RETURN_NIL
int fun_glRefreshContext(Thread* th) FUN_RETURN_NIL
#endif
int coreUiInit(Thread* th, Pkg *system)
{
	Type* fun_I_I_I_I_I_S_B = typeAlloc(th, TYPECODE_FUN, NULL, 7, MM.I, MM.I, MM.I, MM.I, MM.I, MM.S, MM.Boolean);
	Type* fun_I_I_B = typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.I, MM.I, MM.Boolean);
	Type* fun_B = typeAlloc(th, TYPECODE_FUN, NULL, 1, MM.Boolean);
	Type* fun_F = typeAlloc(th, TYPECODE_FUN, NULL, 1, MM.F);
    Type* fun_F_F = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.F, MM.F);
	Type* fun_S = typeAlloc(th, TYPECODE_FUN, NULL, 1, MM.S);
	Type* fun_S_S = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.S, MM.S);
	Type* fun_I = typeAlloc(th, TYPECODE_FUN, NULL, 1, MM.I);
	Type* fun_list_S = typeAlloc(th, TYPECODE_FUN, NULL, 1, typeAlloc(th, TYPECODE_LIST, NULL, 1, MM.S));
	Def* Cursor= pkgAddType(th, system, "Cursor");
	Type* fun_II = typeAlloc(th, TYPECODE_FUN, NULL, 1, typeAlloc(th, TYPECODE_TUPLE, NULL, 2, MM.I, MM.I));
	Type* fun_Bmp_I_I_Cursor = typeAlloc(th, TYPECODE_FUN, NULL, 4, MM.Bitmap, MM.I, MM.I, Cursor->type);
	Type* fun_Cursor_Cursor = typeAlloc(th, TYPECODE_FUN, NULL, 2, Cursor->type, Cursor->type);

	coreUiHwInit(th,system);
#ifdef WITH_GL
	coreUiGLInit(th, system);
#else
	pkgAddFun(th, system, "_glMakeContext", fun_glMakeContext, fun_I);
	pkgAddFun(th, system, "_glRefreshContext", fun_glRefreshContext, fun_I);
#endif
	pkgAddConstInt(th, system, "UI_NORMAL", UI_NORMAL, MM.I);
	pkgAddConstInt(th, system, "UI_RESIZE", UI_RESIZE, MM.I);
	pkgAddConstInt(th, system, "UI_FULLSCREEN", UI_FULLSCREEN, MM.I);
	pkgAddConstInt(th, system, "UI_GL", UI_GL, MM.I);

	pkgAddFun(th, system, "_uiStart", fun_uiStart, fun_I_I_I_I_I_S_B);
	pkgAddFun(th, system, "_uiStop", fun_uiStop, fun_B);
	pkgAddFun(th, system, "uiResize", fun_uiResize, fun_I_I_B);
	pkgAddFun(th, system, "_uiW", fun_uiW, fun_I);
	pkgAddFun(th, system, "_uiH", fun_uiH, fun_I);
	pkgAddFun(th, system, "_uiDrop", fun_uiDrop, fun_list_S);
	
	pkgAddFun(th, system, "screenW", fun_screenW, fun_I);
	pkgAddFun(th, system, "screenH", fun_screenH, fun_I);

	pkgAddFun(th, system, "uiSetTitle", fun_uiSetTitle, fun_S_S);
	pkgAddFun(th, system, "keyboardState", fun_keyboardState, fun_I);

	pkgAddFun(th, system, "clipboardCopy", fun_clipboardCopy, fun_S_S);
	pkgAddFun(th, system, "clipboardPaste", fun_clipboardPaste, fun_S);


	pkgAddFun(th, system, "cursorSize", fun_cursorSize, fun_II);
	pkgAddFun(th, system, "_cursorCreate", fun_cursorCreate, fun_Bmp_I_I_Cursor);
	pkgAddFun(th, system, "_cursorShow", fun_cursorShow, fun_Cursor_Cursor);

	
	pkgAddFun(th, system, "keyboardShow", fun_keyboardShow, fun_B);
	pkgAddFun(th, system, "keyboardHide", fun_keyboardHide, fun_B);
	pkgAddFun(th, system, "keyboardHeight", fun_keyboardHeight, fun_I);

	pkgAddFun(th, system, "orientationGet", fun_orientationGet, fun_I);
	pkgAddFun(th, system, "accelerometerX", fun_accelerometerX, fun_F);
	pkgAddFun(th, system, "accelerometerY", fun_accelerometerY, fun_F);
	pkgAddFun(th, system, "accelerometerZ", fun_accelerometerZ, fun_F);
    pkgAddFun(th, system, "accelerometerInit", fun_accelerometerInit, fun_F_F);

    
	pkgAddConstInt(th, system, "Key_Pause", XKey_Pause, MM.I);
	pkgAddConstInt(th, system, "Key_Scroll_Lock", XKey_Scroll_Lock, MM.I);
	pkgAddConstInt(th, system, "Key_Sys_Req", XKey_Sys_Req, MM.I);
	pkgAddConstInt(th, system, "Key_Delete", XKey_Delete, MM.I);
	pkgAddConstInt(th, system, "Key_Home", XKey_Home, MM.I);
	pkgAddConstInt(th, system, "Key_Left", XKey_Left, MM.I);
	pkgAddConstInt(th, system, "Key_Up", XKey_Up, MM.I);
	pkgAddConstInt(th, system, "Key_Right", XKey_Right, MM.I);
	pkgAddConstInt(th, system, "Key_Down", XKey_Down, MM.I);
	pkgAddConstInt(th, system, "Key_Prior", XKey_Prior, MM.I);
	pkgAddConstInt(th, system, "Key_Page_Up", XKey_Page_Up, MM.I);
	pkgAddConstInt(th, system, "Key_Next", XKey_Next, MM.I);
	pkgAddConstInt(th, system, "Key_Page_Down", XKey_Page_Down, MM.I);
	pkgAddConstInt(th, system, "Key_End", XKey_End, MM.I);
	pkgAddConstInt(th, system, "Key_Insert", XKey_Insert, MM.I);
	pkgAddConstInt(th, system, "Key_Num_Lock", XKey_Num_Lock, MM.I);
	pkgAddConstInt(th, system, "Key_F1", XKey_F1, MM.I);
	pkgAddConstInt(th, system, "Key_F2", XKey_F2, MM.I);
	pkgAddConstInt(th, system, "Key_F3", XKey_F3, MM.I);
	pkgAddConstInt(th, system, "Key_F4", XKey_F4, MM.I);
	pkgAddConstInt(th, system, "Key_F5", XKey_F5, MM.I);
	pkgAddConstInt(th, system, "Key_F6", XKey_F6, MM.I);
	pkgAddConstInt(th, system, "Key_F7", XKey_F7, MM.I);
	pkgAddConstInt(th, system, "Key_F8", XKey_F8, MM.I);
	pkgAddConstInt(th, system, "Key_F9", XKey_F9, MM.I);
	pkgAddConstInt(th, system, "Key_F10", XKey_F10, MM.I);
	pkgAddConstInt(th, system, "Key_F11", XKey_F11, MM.I);
	pkgAddConstInt(th, system, "Key_F12", XKey_F12, MM.I);
	pkgAddConstInt(th, system, "Key_Caps_Lock", XKey_Caps_Lock, MM.I);
	pkgAddConstInt(th, system, "KeyMask_Shift", KeyMask_Shift, MM.I);
	pkgAddConstInt(th, system, "KeyMask_Alt", KeyMask_Alt, MM.I);
	pkgAddConstInt(th, system, "KeyMask_Control", KeyMask_Control, MM.I);
	pkgAddConstInt(th, system, "KeyMask_Meta", KeyMask_Meta, MM.I);

	return 0;
}
