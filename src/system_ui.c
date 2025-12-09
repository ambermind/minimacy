// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
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


int systemUiHwInit(Pkg* system) {return 0;}
#endif


#ifndef WITH_NATIVE_FONT
int fun_nativeFontCreate(Thread* th) FUN_RETURN_NIL
int fun_nativeFontH(Thread* th) FUN_RETURN_NIL
int fun_nativeFontBaseline(Thread* th) FUN_RETURN_NIL
int fun_nativeFontW(Thread* th) FUN_RETURN_NIL
int fun_nativeFontDraw(Thread* th) FUN_RETURN_NIL
int fun_nativeFontList(Thread* th) FUN_RETURN_NIL
#endif
int systemUiInit(Pkg* system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_INT, "UI_NORMAL", (void*)UI_NORMAL, "Int"},
		{ NATIVE_INT, "UI_RESIZE", (void*)UI_RESIZE, "Int" },
		{ NATIVE_INT, "UI_FULLSCREEN", (void*)UI_FULLSCREEN, "Int" },
		{ NATIVE_INT, "UI_GL", (void*)UI_GL, "Int" },

		{ NATIVE_FUN, "_uiStart", fun_uiStart, "fun Int Int Int Int Int Str -> Bool"},
		{ NATIVE_FUN, "_uiStop", fun_uiStop, "fun -> Bool" },
		{ NATIVE_FUN, "uiResize", fun_uiResize, "fun Int Int -> Bool" },
		{ NATIVE_FUN, "_uiW", fun_uiW, "fun -> Int" },
		{ NATIVE_FUN, "_uiH", fun_uiH, "fun -> Int" },
		{ NATIVE_FUN, "_uiDrop", fun_uiDrop, "fun -> list Str" },
		{ NATIVE_FUN, "screenW", fun_screenW, "fun -> Int" },
		{ NATIVE_FUN, "screenH", fun_screenH, "fun -> Int" },
		{ NATIVE_FUN, "uiSetTitle", fun_uiSetTitle, "fun Str -> Str" },
		{ NATIVE_FUN, "keyboardState", fun_keyboardState, "fun -> Int" },
		{ NATIVE_FUN, "clipboardCopy", fun_clipboardCopy, "fun Str -> Str" },
		{ NATIVE_FUN, "clipboardPaste", fun_clipboardPaste, "fun -> Str" },
		{ NATIVE_FUN, "cursorSize", fun_cursorSize, "fun -> [Int Int]" },
		{ NATIVE_FUN, "_cursorCreate", fun_cursorCreate, "fun Bitmap Int Int -> Cursor" },
		{ NATIVE_FUN, "_cursorShow", fun_cursorShow, "fun Cursor -> Cursor" },
		{ NATIVE_FUN, "keyboardShow", fun_keyboardShow, "fun -> Bool" },
		{ NATIVE_FUN, "keyboardHide", fun_keyboardHide, "fun -> Bool" },
		{ NATIVE_FUN, "keyboardHeight", fun_keyboardHeight, "fun -> Int" },
		{ NATIVE_FUN, "orientationGet", fun_orientationGet, "fun -> Int" },
		{ NATIVE_FUN, "accelerometerX", fun_accelerometerX, "fun -> Float" },
		{ NATIVE_FUN, "accelerometerY", fun_accelerometerY, "fun -> Float" },
		{ NATIVE_FUN, "accelerometerZ", fun_accelerometerZ, "fun -> Float" },
		{ NATIVE_FUN, "accelerometerInit", fun_accelerometerInit, "fun Float -> Float" },
		{ NATIVE_INT, "Key_Pause", (void*)XKey_Pause, "Int" },
		{ NATIVE_INT, "Key_Scroll_Lock", (void*)XKey_Scroll_Lock, "Int" },
		{ NATIVE_INT, "Key_Sys_Req", (void*)XKey_Sys_Req, "Int" },
		{ NATIVE_INT, "Key_Delete", (void*)XKey_Delete, "Int" },
		{ NATIVE_INT, "Key_Home", (void*)XKey_Home, "Int" },
		{ NATIVE_INT, "Key_Left", (void*)XKey_Left, "Int" },
		{ NATIVE_INT, "Key_Up", (void*)XKey_Up, "Int" },
		{ NATIVE_INT, "Key_Right", (void*)XKey_Right, "Int" },
		{ NATIVE_INT, "Key_Down", (void*)XKey_Down, "Int" },
		{ NATIVE_INT, "Key_Prior", (void*)XKey_Prior, "Int" },
		{ NATIVE_INT, "Key_Page_Up", (void*)XKey_Page_Up, "Int" },
		{ NATIVE_INT, "Key_Next", (void*)XKey_Next, "Int" },
		{ NATIVE_INT, "Key_Page_Down", (void*)XKey_Page_Down, "Int" },
		{ NATIVE_INT, "Key_End", (void*)XKey_End, "Int" },
		{ NATIVE_INT, "Key_Insert", (void*)XKey_Insert, "Int" },
		{ NATIVE_INT, "Key_Num_Lock", (void*)XKey_Num_Lock, "Int" },
		{ NATIVE_INT, "Key_F1", (void*)XKey_F1, "Int" },
		{ NATIVE_INT, "Key_F2", (void*)XKey_F2, "Int" },
		{ NATIVE_INT, "Key_F3", (void*)XKey_F3, "Int" },
		{ NATIVE_INT, "Key_F4", (void*)XKey_F4, "Int" },
		{ NATIVE_INT, "Key_F5", (void*)XKey_F5, "Int" },
		{ NATIVE_INT, "Key_F6", (void*)XKey_F6, "Int" },
		{ NATIVE_INT, "Key_F7", (void*)XKey_F7, "Int" },
		{ NATIVE_INT, "Key_F8", (void*)XKey_F8, "Int" },
		{ NATIVE_INT, "Key_F9", (void*)XKey_F9, "Int" },
		{ NATIVE_INT, "Key_F10", (void*)XKey_F10, "Int" },
		{ NATIVE_INT, "Key_F11", (void*)XKey_F11, "Int" },
		{ NATIVE_INT, "Key_F12", (void*)XKey_F12, "Int" },
		{ NATIVE_INT, "Key_Caps_Lock", (void*)XKey_Caps_Lock, "Int" },
		{ NATIVE_INT, "KeyMask_Shift", (void*)KeyMask_Shift, "Int" },
		{ NATIVE_INT, "KeyMask_Alt", (void*)KeyMask_Alt, "Int" },
		{ NATIVE_INT, "KeyMask_Control", (void*)KeyMask_Control, "Int" },
		{ NATIVE_INT, "KeyMask_Meta", (void*)KeyMask_Meta, "Int" },
		{ NATIVE_INT, "FONT_BOLD", (void*)FONT_BOLD, "Int" },
		{ NATIVE_INT, "FONT_ITALIC", (void*)FONT_ITALIC, "Int" },
		{ NATIVE_INT, "FONT_UNDERLINE", (void*)FONT_UNDERLINE, "Int" },
		{ NATIVE_INT, "FONT_STRIKED", (void*)FONT_STRIKED, "Int" },
		{ NATIVE_FUN, "_nativeFontList", fun_nativeFontList, "fun -> list Str"},
		{ NATIVE_FUN, "nativeFontCreate", fun_nativeFontCreate, "fun Str Int Int -> NativeFont" },
		{ NATIVE_FUN, "nativeFontH", fun_nativeFontH, "fun NativeFont -> Int" },
		{ NATIVE_FUN, "nativeFontBaseline", fun_nativeFontBaseline, "fun NativeFont -> Int" },
		{ NATIVE_FUN, "nativeFontW", fun_nativeFontW, "fun NativeFont Int -> Int" },
		{ NATIVE_FUN, "nativeFontDraw", fun_nativeFontDraw, "fun Bitmap Int Int NativeFont Int -> Bool" },
#ifdef USE_SOFT_CURSOR
		{ NATIVE_INT, "useSoftCursor", (void*)1, "Int" },
#endif
	};
	pkgAddType(system, "Cursor");
	pkgAddType(system, "NativeFont");
	NATIVE_DEF(nativeDefs);

	systemUiHwInit(system);
	systemUiGLInit(system);

	return 0;
}
