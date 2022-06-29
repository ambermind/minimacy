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
#ifndef _SYSTEM_UI_
#define _SYSTEM_UI_

#ifdef WITH_UI

#ifdef ON_UNIX
#define HWND Window
#define HDC GC
#endif
#endif

#ifndef USE_X11
#include"system_ui_keydef.h"
#endif


#define KS_Shift	1
#define KS_Alt	2
#define KS_Control	4
#define KS_Meta 8

#define UI_NORMAL 0
#define UI_RESIZE 1
#define UI_FRAME 2
#define UI_GL 8
#define UI_MAXIMIZE 16

int fun_uiStart(Thread* th);
int fun_uiStop(Thread* th);
int fun_uiResize(Thread* th);
int fun_uiW(Thread* th);
int fun_uiH(Thread* th);
int fun_uiX(Thread* th);
int fun_uiY(Thread* th);
int fun_uiUpdate(Thread* th);
int fun_screenW(Thread* th);
int fun_screenH(Thread* th);
int fun_uiSetName(Thread* th);
int fun_uiFocus(Thread* th);
int fun_keyboardState(Thread* th);
int fun_clipboardCopy(Thread* th);
int fun_clipboardPaste(Thread* th);

int fun_keyboardShow(Thread* th);
int fun_keyboardHide(Thread* th);
int fun_keyboardHeight(Thread* th);
int fun_orientationGet(Thread* th);
int fun_accelerometerX(Thread* th);
int fun_accelerometerY(Thread* th);
int fun_accelerometerZ(Thread* th);
int fun_accelerometerInit(Thread* th);

#ifdef WITH_UI
#ifdef ON_WINDOWS
void _windowInitGL(HWND win);
void _windowReleaseGL();
#endif
#ifdef USE_X11
void _windowInitGL(HWND win);
void _windowReleaseGL();
#endif
#endif
int coreUiHwInit(Thread* th, Pkg* system);
int coreUiGLES2Init(Thread* th, Pkg* system);
int coreUiInit(Thread* th, Pkg* system);
#endif
