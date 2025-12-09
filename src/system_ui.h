// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _SYSTEM_UI_
#define _SYSTEM_UI_

#define KeyMask_Shift	1
#define KeyMask_Alt	2
#define KeyMask_Control	4
#define KeyMask_Meta 8

#define UI_NORMAL 0
#define UI_RESIZE 1
#define UI_FULLSCREEN 2
#define UI_TYPE_MASK 3

#define UI_GL 8

int fun_uiStart(Thread* th);
int fun_uiStop(Thread* th);
int fun_uiResize(Thread* th);
int fun_uiW(Thread* th);
int fun_uiH(Thread* th);
int fun_uiDrop(Thread* th);
int fun_screenW(Thread* th);
int fun_screenH(Thread* th);
int fun_uiSetTitle(Thread* th);
int fun_keyboardState(Thread* th);
int fun_clipboardCopy(Thread* th);
int fun_clipboardPaste(Thread* th);

int fun_cursorSize(Thread* th);
int fun_cursorCreate(Thread* th);
int fun_cursorShow(Thread* th);

int fun_keyboardShow(Thread* th);
int fun_keyboardHide(Thread* th);
int fun_keyboardHeight(Thread* th);
int fun_orientationGet(Thread* th);
int fun_accelerometerX(Thread* th);
int fun_accelerometerY(Thread* th);
int fun_accelerometerZ(Thread* th);
int fun_accelerometerInit(Thread* th);

#define FONT_BOLD 1
#define FONT_ITALIC 2
#define FONT_UNDERLINE 4
#define FONT_STRIKED 8
#define FONT_PIXEL 16
int fun_nativeFontCreate(Thread* th);
int fun_nativeFontH(Thread* th);
int fun_nativeFontBaseline(Thread* th);
int fun_nativeFontW(Thread* th);
int fun_nativeFontDraw(Thread* th);
int fun_nativeFontList(Thread* th);

int systemUiHwInit(Pkg* system);
int systemUiGLInit(Pkg* system);
int systemUiInit(Pkg* system);

#endif
