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
#ifdef ON_WINDOWS
#include <io.h>
#include"system_ui_keydef.h"
   
//--------------- ON_WINDOWS
#define KEYBUFFER_LENGTH 32

typedef struct {
	HINSTANCE thisInstance;
	int classRegistered;
	HCURSOR defaultCursor;
	HCURSOR currentCursor;
	HWND win;
	HDC paintdc;
	HDROP hdrop;
	int virtkey[KEYBUFFER_LENGTH];
	int scankey[KEYBUFFER_LENGTH];
	int ikey;
	int flags;
	int w;
	int h;
	int type;
#ifdef WITH_GL
	HDC hDCgl;
	HGLRC hRCgl;
#endif
}UIstruct;
UIstruct UI;

#ifdef WITH_GL

FUN_ENUM_UINT glCreateShader = NULL;
FUN_UINT glCreateProgram = NULL;
FUN_UINT_VOID glDeleteShader = NULL;
FUN_UINT_VOID glUseProgram = NULL;
FUN_UINT_VOID glLinkProgram = NULL;
FUN_UINT_VOID glDeleteProgram = NULL;
FUN_UINT_VOID glCompileShader = NULL;
FUN_UINT_UINT_VOID glAttachShader = NULL;
FUN_UINT_SIZEI_PPCHAR_PINT_VOID glShaderSource = NULL;
FUN_UINT_VOID glEnableVertexAttribArray = NULL;
FUN_UINT_INT_ENUM_BOOL_SIZEI_PVOID_VOID glVertexAttribPointer = NULL;
FUN_UINT_ENUM_PINT_VOID glGetShaderiv = NULL;
FUN_UINT_SIZEI_PSIZEI_PCHAR_VOID glGetShaderInfoLog = NULL;

FUN_UINT_PCHAR_INT glGetUniformLocation = NULL;
FUN_UINT_PCHAR_INT glGetAttribLocation = NULL;
FUN_ENUM_VOID glDisableVertexAttribArray = NULL;
FUN_INT_VOID glActiveTexture = NULL;
FUN_INT_SIZEI_PFLOAT_VOID glUniform1fv = NULL;
FUN_INT_SIZEI_PFLOAT_VOID glUniform2fv = NULL;
FUN_INT_SIZEI_PFLOAT_VOID glUniform3fv = NULL;
FUN_INT_SIZEI_PFLOAT_VOID glUniform4fv = NULL;
FUN_INT_SIZEI_BOOL_PFLOAT_VOID glUniformMatrix4fv = NULL;
FUN_INT_SIZEI_BOOL_PFLOAT_VOID glUniformMatrix3fv = NULL;
FUN_INT_SIZEI_BOOL_PFLOAT_VOID glUniformMatrix2fv = NULL;

void* myWglGetProcAddress(char* name)
{
	void* ad = wglGetProcAddress(name);
	//	PRINTF(LOG_DEV,"wglGetProcAddress %s -> %llx", name, ad);
	if (!ad) PRINTF(LOG_SYS, "cannot locate %s\n", name);
	return ad;
}

int myGetGlReady()
{
	if (!(glCreateShader = (FUN_ENUM_UINT)myWglGetProcAddress("glCreateShader"))) return 0;
	if (!(glCreateProgram = (FUN_UINT)myWglGetProcAddress("glCreateProgram"))) return 0;
	if (!(glDeleteShader = (FUN_UINT_VOID)myWglGetProcAddress("glDeleteShader"))) return 0;
	if (!(glUseProgram = (FUN_UINT_VOID)myWglGetProcAddress("glUseProgram"))) return 0;
	if (!(glLinkProgram = (FUN_UINT_VOID)myWglGetProcAddress("glLinkProgram"))) return 0;
	if (!(glDeleteProgram = (FUN_UINT_VOID)myWglGetProcAddress("glDeleteProgram"))) return 0;
	if (!(glCompileShader = (FUN_UINT_VOID)myWglGetProcAddress("glCompileShader"))) return 0;
	if (!(glAttachShader = (FUN_UINT_UINT_VOID)myWglGetProcAddress("glAttachShader"))) return 0;
	if (!(glShaderSource = (FUN_UINT_SIZEI_PPCHAR_PINT_VOID)myWglGetProcAddress("glShaderSource"))) return 0;
	if (!(glEnableVertexAttribArray = (FUN_UINT_VOID)myWglGetProcAddress("glEnableVertexAttribArray"))) return 0;
	if (!(glVertexAttribPointer = (FUN_UINT_INT_ENUM_BOOL_SIZEI_PVOID_VOID)myWglGetProcAddress("glVertexAttribPointer"))) return 0;
	if (!(glGetShaderiv = (FUN_UINT_ENUM_PINT_VOID)myWglGetProcAddress("glGetShaderiv"))) return 0;
	if (!(glGetShaderInfoLog = (FUN_UINT_SIZEI_PSIZEI_PCHAR_VOID)myWglGetProcAddress("glGetShaderInfoLog"))) return 0;

	if (!(glGetUniformLocation = (FUN_UINT_PCHAR_INT)myWglGetProcAddress("glGetUniformLocation"))) return 0;
	if (!(glGetAttribLocation = (FUN_UINT_PCHAR_INT)myWglGetProcAddress("glGetAttribLocation"))) return 0;
	if (!(glDisableVertexAttribArray = (FUN_ENUM_VOID)myWglGetProcAddress("glDisableVertexAttribArray"))) return 0;
	if (!(glActiveTexture = (FUN_INT_VOID)myWglGetProcAddress("glActiveTexture"))) return 0;
	if (!(glUniform1fv = (FUN_INT_SIZEI_PFLOAT_VOID)myWglGetProcAddress("glUniform1fv"))) return 0;
	if (!(glUniform2fv = (FUN_INT_SIZEI_PFLOAT_VOID)myWglGetProcAddress("glUniform2fv"))) return 0;
	if (!(glUniform3fv = (FUN_INT_SIZEI_PFLOAT_VOID)myWglGetProcAddress("glUniform3fv"))) return 0;
	if (!(glUniform4fv = (FUN_INT_SIZEI_PFLOAT_VOID)myWglGetProcAddress("glUniform4fv"))) return 0;
	if (!(glUniformMatrix4fv = (FUN_INT_SIZEI_BOOL_PFLOAT_VOID)myWglGetProcAddress("glUniformMatrix4fv"))) return 0;
	if (!(glUniformMatrix3fv = (FUN_INT_SIZEI_BOOL_PFLOAT_VOID)myWglGetProcAddress("glUniformMatrix3fv"))) return 0;
	if (!(glUniformMatrix2fv = (FUN_INT_SIZEI_BOOL_PFLOAT_VOID)myWglGetProcAddress("glUniformMatrix2fv"))) return 0;
	return 1;
}

void _windowInitGL()
{
	UI.hDCgl = NULL;
	UI.hRCgl = NULL;
}

void _windowReleaseGL()
{
	if (UI.hRCgl)
	{
		//		PRINTF(LOG_DEV,"_windowReleaseGL\n");
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(UI.hRCgl);
		ReleaseDC(UI.win, UI.hDCgl);
		UI.hDCgl = NULL;
		UI.hRCgl = NULL;
	}
	UI.win = NULL;
}

void _glMakeGLcontextInitGL()
{
	PIXELFORMATDESCRIPTOR pfd;
	int format;
	UI.hDCgl = NULL;
	UI.hRCgl = NULL;
	//	PRINTF(LOG_DEV,"_windowInitGL %x\n",UI.win);
		// get the device context (DC)
	GLinstance++;
	UI.hDCgl = GetDC(UI.win);

	// set the pixel format for the DC
	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 32;
	pfd.iLayerType = PFD_MAIN_PLANE;
	format = ChoosePixelFormat(UI.hDCgl, &pfd);
	SetPixelFormat(UI.hDCgl, format, &pfd);
	// create and enable the render context (RC)
	UI.hRCgl = wglCreateContext(UI.hDCgl);
	wglMakeCurrent(UI.hDCgl, UI.hRCgl);
	GLready= myGetGlReady();	// we need a rendering context before this call
}

int fun_glMakeContext(Thread* th)
{
	//	PRINTF(LOG_DEV,"===============fun_glMakeContext\n");

	if (!UI.win) FUN_RETURN_NIL;
	_glMakeGLcontextInitGL();
	FUN_RETURN_INT(0);
}
int fun_glRefreshContext(Thread* th) FUN_RETURN_NIL
int fun_glSwapBuffers(Thread* th)
{
	if (!UI.hRCgl) FUN_RETURN_NIL;
	//	PRINTF(LOG_DEV,"SwapBuffers\n");
	SwapBuffers(UI.hDCgl);
	FUN_RETURN_INT(0);
}
int viewPortScale(int u) { return u; }
#endif






//-----------------------------------
int bitmapForget(LB* user)
{
    LBitmap* b = (LBitmap*)user;
	if (b->bmp) DeleteObject(b->bmp);
	return 0;
}

void uiRegisterkey(int virt, int scan)
{
	int i;
	for (i = 0; i < UI.ikey; i++) if (UI.scankey[i] == scan)
	{
		UI.virtkey[i] = virt;
		return;
	}
	if (UI.ikey >= KEYBUFFER_LENGTH)
	{
		for (i = 0; i < UI.ikey - 1; i++)
		{
			UI.virtkey[i] = UI.virtkey[i + 1];
			UI.scankey[i] = UI.scankey[i + 1];
		}
		UI.ikey--;
	}
	UI.virtkey[UI.ikey] = virt;
	UI.scankey[UI.ikey++] = scan;
}

int uiReleasekey(int scan)
{
	int i, j, virt;
	for (i = 0; i < UI.ikey; i++) if (UI.scankey[i] == scan)
	{
		virt = UI.virtkey[i];
		for (j = i; j < UI.ikey - 1; j++)
		{
			UI.virtkey[j] = UI.virtkey[j + 1];
			UI.scankey[j] = UI.scankey[j + 1];
		}
		UI.ikey--;
		return virt;
	}
	return 0;
}


int KeyCodes[160] =
{
0,0,0,0,0,0,0,0, /*0.*/
0,0,0,0,0,0,0,0,
0,0,0,XKey_Pause,XKey_Caps_Lock,0,0,0,
0,0,0,0,0,0,0,0,
0,XKey_Prior,XKey_Next,XKey_End,XKey_Home,XKey_Left,XKey_Up,XKey_Right,
XKey_Down,0,0,0,XKey_Sys_Req,XKey_Insert,XKey_Delete,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0, /*4.*/
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
XKey_F1,XKey_F2,XKey_F3,XKey_F4,XKey_F5,XKey_F6,XKey_F7,XKey_F8,
XKey_F9,XKey_F10,XKey_F11,XKey_F12,0,0,0,0,
0,0,0,0,0,0,0,0, /*8.*/
0,0,0,0,0,0,0,0,
XKey_Num_Lock,XKey_Scroll_Lock,0,0,0,0,0,0,
0,0,0,0,0,0,0,0
};

int convertvirtcode(int c)
{
	if ((c < 0) || (c >= 160)) return 0;
	return KeyCodes[c];
}

void mouseCommonWheel(int event, short delta, short keys, short x, short y)
{
	RECT r, t;
	GetWindowRect(UI.win, &r);
	t.bottom = 300;
	t.right = 300;
	t.left = 100;
	t.top = 100;
	AdjustWindowRect(&t, UI.flags, FALSE);
	eventNotify(event, x - r.left + t.left - 100, y - r.top + t.top - 100, delta);
}

#define MULTITOUCH_MAX 16
typedef BOOL(WINAPI* GTIF)(HTOUCHINPUT, UINT, PTOUCHINPUT, int);
typedef BOOL(WINAPI* CTIH)(HTOUCHINPUT);
typedef BOOL(WINAPI* RTW)(HWND, ULONG);
GTIF gtif = NULL;
CTIH ctih = NULL;
RTW rtw = NULL;

void uiRegisterTouchWindow(HWND win)
{
	if (rtw == NULL) rtw = (RTW)GetProcAddress(GetModuleHandle("USER32.dll"), "RegisterTouchWindow");
	if (rtw) rtw(win, 0);
}
void uiTouch(HTOUCHINPUT h, int n)
{
	TOUCHINPUT pInputs[MULTITOUCH_MAX];
//	PRINTF(LOG_DEV,"uiTouch %d\n", n);
	if (gtif == NULL) gtif = (GTIF)GetProcAddress(GetModuleHandle("USER32.dll"), "GetTouchInputInfo");
	if (ctih == NULL) ctih = (CTIH)GetProcAddress(GetModuleHandle("USER32.dll"), "CloseTouchInputHandle");
	if ((gtif == NULL) || (ctih == NULL)) return;

	if (!n) return;
	if (n > MULTITOUCH_MAX) n = MULTITOUCH_MAX;
	if (gtif(h, n, pInputs, sizeof(TOUCHINPUT)))
	{
		int i;
		RECT r;
		GetClientRect(UI.win, &r); MapWindowPoints(UI.win, GetParent(UI.win), (LPPOINT)&r, 2);
		for (i = 0; i < n; i++)
		{
			TOUCHINPUT ti = pInputs[i];
			if (!(ti.dwFlags & TOUCHEVENTF_UP))
				eventNotify(EVENT_MULTITOUCH, (int)ti.x / 100 - r.left, (int)ti.y / 100 - r.top, 0);
		}
		eventNotify(EVENT_MULTITOUCH, 0, 0, 1);
		ctih(h);
	}
}

void uiOnDrop(HDROP hdrop)
{
	POINT p;
	if (UI.hdrop) DragFinish(UI.hdrop);
	UI.hdrop = hdrop;
	DragQueryPoint(hdrop, &p);
	eventNotify(EVENT_DROPFILES, p.x, p.y, 0);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int c;
//	PRINTF(LOG_DEV,"windowproc %d "LSX" "LSX"\n", msg, wParam, lParam);
	if (hwnd != UI.win) {
//		PRINTF(LOG_DEV,"unknown window!!!!!!!!!!!!!!!!! %x, "LSX"\n", msg,hwnd);
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	switch (msg) {
	case WM_CHAR:
		uiRegisterkey((int)wParam, (lParam >> 16) & 0x1ff);
		eventNotify(EVENT_KEYDOWN, (int)wParam,0,0);
		break;
	case WM_KEYDOWN:
		c = convertvirtcode((int)wParam);
		if (c) eventNotify(EVENT_KEYDOWN, c, 0, 0);
		break;
	case WM_KEYUP:
		c = convertvirtcode((int)wParam);
		if (!c) c = uiReleasekey((lParam >> 16) & 0x1ff);
		if (c) eventNotify(EVENT_KEYUP, c, 0, 0);
		break;
	case WM_PAINT:
//		printf("paint.");
		eventNotify(EVENT_PAINT, 0, 0, 0);
		return(DefWindowProc(hwnd, msg, wParam, lParam));
		break;
	case WM_LBUTTONDOWN:
		SetCapture(hwnd);
		eventNotify(EVENT_CLICK, LOWORD(lParam), HIWORD(lParam), 0);
		break;
	case WM_MBUTTONDOWN:
		SetCapture(hwnd);
		eventNotify(EVENT_CLICK, LOWORD(lParam), HIWORD(lParam), 1);
		break;
	case WM_RBUTTONDOWN:
		SetCapture(hwnd);
		eventNotify(EVENT_CLICK, LOWORD(lParam), HIWORD(lParam), 2);
		break;
/*	case WM_LBUTTONDBLCLK:
		SetCapture(hwnd);
		if (uiDclick(th, vp, wi, LOWORD(lParam), HIWORD(lParam), 0)) return 0;
		break;
	case WM_RBUTTONDBLCLK:
		SetCapture(hwnd);
		if (uiDclick(th, vp, wi, LOWORD(lParam), HIWORD(lParam), 1)) return 0;
		break;
*/	case WM_LBUTTONUP:
		ReleaseCapture();
		eventNotify(EVENT_UNCLICK, LOWORD(lParam), HIWORD(lParam), 0);
		break;
	case WM_MBUTTONUP:
		ReleaseCapture();
		eventNotify(EVENT_UNCLICK, LOWORD(lParam), HIWORD(lParam), 1);
		break;
	case WM_RBUTTONUP:
		ReleaseCapture();
		eventNotify(EVENT_UNCLICK, LOWORD(lParam), HIWORD(lParam), 2);
		break;
	case WM_MOUSEMOVE:
		eventNotify(EVENT_MOUSEMOVE, (int)signExtend16(LOWORD(lParam)), (int)signExtend16(HIWORD(lParam)), (int)wParam);
		break;
	case WM_TOUCH:
		uiTouch((HTOUCHINPUT)lParam, LOWORD(wParam));
		break;
	case WM_MOUSEWHEEL:
		mouseCommonWheel(EVENT_VWHEEL, HIWORD(wParam), LOWORD(wParam), LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_MOUSEHWHEEL:
		mouseCommonWheel(EVENT_HWHEEL, HIWORD(wParam), LOWORD(wParam), LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_SIZE:
//		printf("size.");
		UI.w = LOWORD(lParam);
		UI.h = HIWORD(lParam);
		eventNotify(EVENT_SIZE, LOWORD(lParam), HIWORD(lParam), 0);
		break;
	case WM_CLOSE:
		eventNotify(EVENT_CLOSE, 0, 0, 0);
		break;
	case WM_DROPFILES:
		uiOnDrop((HDROP)wParam);
		break;
	default:
		return(DefWindowProc(hwnd, msg, wParam, lParam));
	}
	return 0;
}

int hostLoop()
{
	MSG msg;
	HWND win = UI.win;
//	PRINTF(LOG_DEV,"start hostloop\n");
	while(win==UI.win)
	{
		if (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
//		else return (int)msg.wParam;
	}
//	PRINTF(LOG_DEV,"exit hostloop\n");
	return 0;
}


LPCTSTR IDI_ICON;

BOOL DeclareWindow(HINSTANCE this_inst)
{
	WNDCLASS    wc;
	BOOL        rc;
	HICON IconMain;
	
	if (UI.classRegistered) return 0;
	IconMain = LoadIcon(this_inst, IDI_ICON);
	UI.defaultCursor = LoadCursor(NULL, IDC_ARROW);
	UI.currentCursor = UI.defaultCursor;
	wc.style = CS_HREDRAW | CS_VREDRAW; // | CS_DBLCLKS;
	wc.lpfnWndProc = (WNDPROC)WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = sizeof(DWORD);
	wc.hInstance = this_inst;
	wc.hIcon = IconMain;
	wc.hCursor = UI.defaultCursor;
	wc.hbrBackground = GetSysColorBrush(COLOR_MENU);
	wc.lpszMenuName = "GenericMenu";
	wc.lpszClassName = TEXT("WindowClass");

	rc = RegisterClass(&wc);
	UI.classRegistered = 1;
	return(rc);
}


MTHREAD_START _uiStart(Thread* th)
{
	int xIsNil, yIsNil;
	LINT y, x;
	DWORD flags;
	char* name;
	RECT r;
	int typeStyle;

	LB* p = STACKPNT(th, 0);
	LINT type = STACKINT(th, 1);
	r.bottom = (LONG)STACKINT(th, 2);
	r.right = (LONG)STACKINT(th, 3);
	yIsNil= STACKISNIL(th, 4);
	y = STACKINT(th, 4);
	xIsNil= STACKISNIL(th, 5);
	x = STACKINT(th, 5);

	if (!p) name = "Minimacy";
	else name = STRSTART(p);

	UI.type = (int)type;
	typeStyle= UI.type & UI_TYPE_MASK;

	if (typeStyle == UI_FULLSCREEN) flags = WS_POPUP;
	else if (typeStyle == UI_RESIZE) flags = (WS_OVERLAPPEDWINDOW);
	else flags = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);

	flags |= WS_CLIPCHILDREN;
	if (r.bottom < 0) r.bottom = 0;
	if (r.right < 0) r.right = 0;
	r.left = 0;
	r.top = 0;
	if (typeStyle == UI_FULLSCREEN) GetWindowRect(GetDesktopWindow(), &r);
	else AdjustWindowRect(&r, flags, FALSE);

	UI.w = r.right-r.left;
	UI.h = r.bottom-r.top;

	if (yIsNil) y = CW_USEDEFAULT;
	if (xIsNil) x = CW_USEDEFAULT;
	if (typeStyle & UI_FULLSCREEN) x = y = 0;

	DeclareWindow(UI.thisInstance);
	UI.win = CreateWindowEx(WS_EX_ACCEPTFILES, TEXT("WindowClass"), name, flags,
		(int)x, (int)y, r.right - r.left, r.bottom - r.top,
		NULL, NULL, UI.thisInstance, NULL);
	uiRegisterTouchWindow(UI.win);
	UI.paintdc= GetDC(UI.win);
	UI.flags = (int)flags;
	DragAcceptFiles(UI.win, TRUE);
	UI.hdrop = NULL;
#ifdef WITH_GL
	_windowInitGL();
#endif

//	PRINTF(LOG_DEV,"UI.win "LSX"\n", UI.win);
//	uiRegisterTouchWindow(th, UI.win);
	ShowWindow(UI.win, SW_SHOW);
	UpdateWindow(UI.win);
	workerDonePnt(th, MM._true);
	hostLoop();
#ifdef WITH_GL
//	_windowReleaseGL();
#endif
	return MTHREAD_RETURN;
}
int fun_uiStart(Thread* th) { return workerStart(th, 6, _uiStart); }

int fun_uiResize(Thread* th)
{
	WINDOWPLACEMENT wnd;
	RECT r;
	LINT y, x;

	LINT h = STACKINT(th, 0);
	LINT w = STACKINT(th, 1);
	if (!UI.win) FUN_RETURN_NIL;

	wnd.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(UI.win, &wnd);

	y = wnd.rcNormalPosition.top;
	x = wnd.rcNormalPosition.left;

	r.left = 0; r.top = 0; r.right = (LONG)w;	r.bottom = (LONG)h;
	AdjustWindowRect(&r, UI.flags, FALSE);
	w = (LINT)( r.right - r.left);
	h = (LINT)(r.bottom - r.top);

	if ((w < 0) || (h < 0)) FUN_RETURN_NIL;
	UI.w = (int)w;
	UI.h = (int)h;
	wnd.rcNormalPosition.left = (LONG)x;
	wnd.rcNormalPosition.top = (LONG)y;
	wnd.rcNormalPosition.right = (LONG)(x + w);
	wnd.rcNormalPosition.bottom = (LONG)(y + h);

	SetWindowPlacement(UI.win, &wnd);
	UpdateWindow(UI.win);
	FUN_RETURN_INT(0);
}

int fun_uiStop(Thread* th)
{
	HWND win = UI.win;
	if (!win) FUN_RETURN_FALSE;
#ifdef WITH_GL
	_windowReleaseGL();
#endif
	UI.win = 0;
	PostMessage(win, WM_CLOSE, 0, 0);
	FUN_RETURN_TRUE;
}

int fun_uiW(Thread* th)
{
	if (UI.win == 0) FUN_RETURN_NIL;
	FUN_RETURN_INT(UI.w);
}
int fun_uiH(Thread* th)
{
	if (UI.win == 0) FUN_RETURN_NIL;
	FUN_RETURN_INT(UI.h);
}
int fun_screenW(Thread* th)
{
	RECT r;
	GetWindowRect(GetDesktopWindow(), &r);
	FUN_RETURN_INT(r.right - r.left);
}
int fun_screenH(Thread* th)
{
	RECT r;
	GetWindowRect(GetDesktopWindow(), &r);
	FUN_RETURN_INT(r.bottom - r.top);
}

int fun_uiSetTitle(Thread* th)
{
	LB* name = STACKPNT(th, 0);
	if ((!name) || (!UI.win)) FUN_RETURN_NIL;
	SetWindowText(UI.win, STRSTART(name));
	return 0;
}

int fun_keyboardState(Thread* th)
{
	int k = 0;
	if (GetKeyState(VK_SHIFT) < 0) k += KeyMask_Shift;
	if (GetKeyState(VK_CONTROL) < 0) k += KeyMask_Control;
	if (GetKeyState(VK_MENU) < 0) k += KeyMask_Alt;
	//	if (GetKeyState(VK_MENU)<0) k+=KeyMask_Meta;
	FUN_RETURN_INT(k);
}

int fun_clipboardCopy(Thread* th)
{
	LINT len;
	HANDLE hdata;
	char* lpdata;
	LB* data = STACKPNT(th, 0);
	if (!data) return 0;
	len = STRLEN(data);
	if ((hdata = GlobalAlloc(GMEM_DDESHARE, sizeof(char) * (len + 1))) &&
		(lpdata = (char*)GlobalLock(hdata)))
	{
		memcpy(lpdata, STRSTART(data), sizeof(char) * (len + 1));
		GlobalUnlock(hdata);

		if (OpenClipboard(NULL))
		{
			EmptyClipboard();
			SetClipboardData(CF_TEXT, hdata);
			CloseClipboard();
		}
	}
	return 0;
}
int fun_clipboardPaste(Thread* th)
{
	if (OpenClipboard(NULL))
	{
		if (IsClipboardFormatAvailable(CF_TEXT) || IsClipboardFormatAvailable(CF_OEMTEXT))
		{
			HANDLE hdata;
			char* lpdata;

			if ((hdata = GetClipboardData(CF_TEXT)) && (lpdata = (char*)GlobalLock(hdata)))
			{
				FUN_PUSH_STR( lpdata, -1);
				GlobalUnlock(hdata);
				CloseClipboard();
				return 0;
			}
		}
		CloseClipboard();
	}
	FUN_RETURN_NIL;
}

int fun_uiDrop(Thread* th)
{
	int nbDroppedFiles, nbReturnedFiles, i;
	if (!UI.hdrop || !UI.win) FUN_RETURN_NIL;

	nbDroppedFiles = DragQueryFile(UI.hdrop, 0xFFFFFFFF, NULL, 0);
	nbReturnedFiles = 0;
	for (i = 0; i < nbDroppedFiles; i++)
	{
		int j;
		LINT h;
		struct _finddata_t fileinfo;
		char* fileName;

		int len = DragQueryFile(UI.hdrop, i, NULL, 0);
		FUN_PUSH_STR( NULL, len + 1);
		fileName = STRSTART(STACKPNT(th, 0));

		DragQueryFile(UI.hdrop, i, fileName + 1, len + 1);
		h = _findfirst(fileName+1, &fileinfo);
		if (h != -1)	// should not happen ?
		{
			fileName[0] = (fileinfo.attrib & 16) ? 'D' : 'F';
			for (j = 0; j <= len; j++) if (fileName[j] == '\\') fileName[j] = '/';
			nbReturnedFiles++;
			_findclose(h);
		}
		else STACKDROP(th);
	}
	FUN_PUSH_NIL;
	while ((nbReturnedFiles--) > 0) FUN_MAKE_TABLE( LIST_LENGTH, DBG_LIST);
	DragFinish(UI.hdrop);
	UI.hdrop = NULL;
	return 0;
}


typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	HCURSOR cursor;
}Cursor;

int _cursorForget(LB* p)
{
	Cursor* c = (Cursor*)p;
	if (c) DestroyCursor(c->cursor);
	return 0;
}

int fun_cursorSize(Thread* th)
{
	FUN_PUSH_INT(GetSystemMetrics(SM_CXCURSOR));
	FUN_PUSH_INT(GetSystemMetrics(SM_CYCURSOR));
	FUN_MAKE_TABLE( 2, DBG_TUPLE);
	return 0;
}

int fun_cursorCreate(Thread* th)
{
	HCURSOR hc;
	int i, j, w, h, lenbmp;
	int* p;
	Cursor* d;
	char tand[32 * 4*4];	// up to 64x64
	char txor[32 * 4*4];

	LINT yhot = STACKINT(th, 0);
	LINT xhot = STACKINT(th, 1);
	LBitmap* bmp = (LBitmap*)STACKPNT(th, 2);
	if ((!bmp)||(!UI.win)) FUN_RETURN_NIL;	// on X11 we need a window before creating the cursor

	w = GetSystemMetrics(SM_CXCURSOR);
	h = GetSystemMetrics(SM_CYCURSOR);
	if ((bmp->w != w) || (bmp->h != h)) FUN_RETURN_NIL;
	lenbmp = ((w + 7) >> 3) * h;
	if (lenbmp > 32 * 4 * 4) FUN_RETURN_NIL;
	for (i = 0; i < lenbmp; i++) tand[i] = txor[i] = 0;

	p = bmp->start32;
	for (j = 0; j < h; j++)
		for (i = 0; i < w; i++)
		{
			int color = p[j * bmp->next32 + i];
			if (!(color&0xff)) tand[(i >> 3) + ((w + 7) >> 3) * j] |= 128 >> (i & 7);
			if (color&0xff0000) txor[(i >> 3) + ((w + 7) >> 3) * j] |= 128 >> (i & 7);
		}
	hc = CreateCursor(UI.thisInstance,(int)xhot, (int)yhot, w, h, tand, txor);
	d = (Cursor*)memoryAllocExt(th, sizeof(Cursor), DBG_BIN, _cursorForget, NULL);
	if (!d) {
		DestroyCursor(hc);
		return EXEC_OM;
	}
	d->cursor = hc;
	FUN_RETURN_PNT((LB*)d);
}
int fun_cursorShow(Thread* th)
{
	HCURSOR cursor = NULL;

	Cursor* d = (Cursor*) STACKPNT(th, 0);
	if (d) cursor = d->cursor;
	if (!cursor) cursor = UI.defaultCursor;
//	PRINTF(LOG_DEV,"fun_cursorShow %llx -> %llx %llx\n", UI.currentCursor, cursor, UI.defaultCursor);
	if (cursor != UI.currentCursor)
	{
		POINT P;
		SetClassLongPtr(UI.win, GCLP_HCURSOR, (LONG_PTR)cursor);
		if (GetCapture() == UI.win) SetCursor(cursor);
		GetCursorPos(&P);
		SetCursorPos(P.x, P.y);
		UI.currentCursor = cursor;
	}
	return 0;
}

int fun_keyboardShow(Thread* th) FUN_RETURN_NIL
int fun_keyboardHide(Thread* th) FUN_RETURN_NIL
int fun_keyboardHeight(Thread* th) FUN_RETURN_NIL
int fun_orientationGet(Thread* th) FUN_RETURN_NIL
int fun_accelerometerX(Thread* th) FUN_RETURN_NIL
int fun_accelerometerY(Thread* th) FUN_RETURN_NIL
int fun_accelerometerZ(Thread* th) FUN_RETURN_NIL
int fun_accelerometerInit(Thread* th)FUN_RETURN_NIL

#define FONT_BOLD 1
#define FONT_ITALIC 2
#define FONT_UNDERLINE 4
#define FONT_STRIKED 8
#define FONT_PIXEL 16

typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	HFONT hfont;
}Font;

int _fontForget(LB* p)
{
	Font* f = (Font*)p;
	if (f) DeleteObject(f->hfont);
	return 0;
}

int fun_fontFromHost(Thread* th)
{
	HFONT hfont;
	Font* f;
	LINT h;

	LINT flags = STACKINT(th, 0);
	LINT size = STACKINT(th, 1);
	LB* name = STACKPNT(th, 2);
	if (!name) FUN_RETURN_NIL;

	h = size;
	if (flags & FONT_PIXEL)
	{
		HDC DC = GetDC(NULL);
		h = MulDiv((int)size, GetDeviceCaps(DC, LOGPIXELSY), 72);
		ReleaseDC(NULL, DC);
	}

	hfont = CreateFont((int)h, 0, 0, 0, (flags & FONT_BOLD) ? 700 : 0, (flags & FONT_ITALIC) ? 1 : 0,
		(flags & FONT_UNDERLINE) ? 1 : 0, (flags & FONT_STRIKED) ? 1 : 0,
		DEFAULT_CHARSET, 0, 0, 0, 0, STRSTART(name));
	if (!hfont) FUN_RETURN_NIL;
	f = (Font*)memoryAllocExt(th, sizeof(Font), DBG_BIN, _fontForget, NULL);
	if (!f) {
		DeleteObject(hfont);
		return EXEC_OM;
	}
	f->hfont = hfont;
	FUN_RETURN_PNT((LB*)f);
}

int fun_fontH(Thread* th)
{
	HDC DC;
	TEXTMETRIC tm;

	Font* f= (Font*)STACKPNT(th, 0);
	if (!f) FUN_RETURN_NIL;

	DC = GetDC(NULL);
	SelectFont(DC, f->hfont);
	GetTextMetrics(DC, &tm);
	ReleaseDC(NULL, DC);
	FUN_RETURN_INT(tm.tmHeight);
}

int _fontStringW(Thread* th,int u16)
{
	LINT size;
	HDC DC;

	LB* str = STACKPNT(th, 0);
	Font* f = (Font*)STACKPNT(th, 1);
	if ((!f) || (!str)) FUN_RETURN_NIL;
	
	DC = GetDC(NULL);
	SelectFont(DC, f->hfont);
	if (u16) size= LOWORD(GetTabbedTextExtentW(DC, (LPCWSTR)STRSTART(str), (int)STRLEN(str) >> 1, 0, NULL));
	else size = LOWORD(GetTabbedTextExtentA(DC, STRSTART(str),(int) STRLEN(str), 0, NULL));
	ReleaseDC(NULL, DC);
	FUN_RETURN_INT(size);
}
int fun_fontStringW(Thread* th) { return _fontStringW(th, 0); }
int fun_fontStringU16W(Thread* th) { return _fontStringW(th, 1); }

void bitmapRequireDib(LBitmap* d)
{
	lchar* newstart;
	int newnextline, j;
	BITMAPINFO Bi;
	DIBSECTION bmpinfo;
	lchar* src;
	lchar* dest;
	if (d->bmp) return;        // Dib already created

	Bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	Bi.bmiHeader.biWidth = (LONG)d->w;
	Bi.bmiHeader.biHeight = (LONG)-d->h;
	Bi.bmiHeader.biPlanes = 1;
	Bi.bmiHeader.biBitCount = 32;
	Bi.bmiHeader.biCompression = BI_RGB;
	Bi.bmiHeader.biSizeImage = 0;
	Bi.bmiHeader.biXPelsPerMeter = 0;
	Bi.bmiHeader.biYPelsPerMeter = 0;
	Bi.bmiHeader.biClrUsed = 0;
	Bi.bmiHeader.biClrImportant = Bi.bmiHeader.biClrUsed;

	d->bmp = CreateDIBSection(NULL, (BITMAPINFO*)&Bi, DIB_RGB_COLORS, (void**)&newstart, NULL, 0);

	GetObject(d->bmp, sizeof(DIBSECTION), &bmpinfo);
	newnextline = bmpinfo.dsBm.bmWidthBytes;

	src = d->start8;
	dest = newstart;
	for (j = 0; j < d->h; j++)
	{
		memcpy(dest, src, d->w * 4);
		src += d->next8;
		dest += newnextline;
	}
	d->bytes = NULL;
	d->start8 = newstart;
	d->next8 = newnextline;
	d->start32 = (int*)d->start8;
	d->next32 = d->next8 >> 2;
	d->forget=bitmapForget;
}

int _fontDraw(Thread* th, int u16)
{
	HDC dcb;

	LB* str = STACKPNT(th, 0);
	Font* f = (Font*)STACKPNT(th, 1);
	LINT y = STACKINT(th, 2);
	LINT x = STACKINT(th, 3);
	LBitmap* b=(LBitmap*)STACKPNT(th, 4);
	if ((!f) || (!str) || (!b)) FUN_RETURN_NIL;

	bitmapRequireDib(b);
	dcb = CreateCompatibleDC(NULL);
	SelectObject(dcb, b->bmp);
	SetBkMode(dcb, TRANSPARENT);
	SetTextColor(dcb, 0xffffff);
	SelectFont(dcb, f->hfont);
	SetTextAlign(dcb, 0);
	if (u16) TextOutW(dcb, (int)x, (int)y, (LPCWSTR)STRSTART(str), (int)STRLEN(str) >> 1);
	else TextOutA(dcb, (int)x, (int)y, STRSTART(str), (int)STRLEN(str));
	DeleteDC(dcb);
	FUN_RETURN_PNT(MM._true);
}
int fun_fontDraw(Thread* th) { return _fontDraw(th, 0);}
int fun_fontDrawU16(Thread* th) { return _fontDraw(th, 1); }

int coreUiHwInit(Thread* th, Pkg* system)
{
	Def* font = pkgAddType(th, system, "Font");

	pkgAddConstInt(th, system, "FONT_BOLD", FONT_BOLD, MM.I);
	pkgAddConstInt(th, system, "FONT_ITALIC", FONT_ITALIC, MM.I);
	pkgAddConstInt(th, system, "FONT_UNDERLINE", FONT_UNDERLINE, MM.I);
	pkgAddConstInt(th, system, "FONT_STRIKED", FONT_STRIKED, MM.I);
	pkgAddConstInt(th, system, "FONT_PIXEL", FONT_PIXEL, MM.I);

	pkgAddFun(th, system, "fontFromHost", fun_fontFromHost, typeAlloc(th, TYPECODE_FUN, NULL, 4, MM.S, MM.I, MM.I, font->type));
	pkgAddFun(th, system, "fontH", fun_fontH, typeAlloc(th, TYPECODE_FUN, NULL, 2, font->type, MM.I));
	pkgAddFun(th, system, "fontStringW", fun_fontStringW, typeAlloc(th, TYPECODE_FUN, NULL, 3, font->type, MM.S, MM.I));
	pkgAddFun(th, system, "fontStringU16W", fun_fontStringU16W, typeAlloc(th, TYPECODE_FUN, NULL, 3, font->type, MM.S, MM.I));
	pkgAddFun(th, system, "fontDraw", fun_fontDraw, typeAlloc(th, TYPECODE_FUN, NULL, 6, MM.Bitmap, MM.I, MM.I, font->type, MM.S, MM.Boolean));
	pkgAddFun(th, system, "fontDrawU16", fun_fontDrawU16, typeAlloc(th, TYPECODE_FUN, NULL, 6, MM.Bitmap, MM.I, MM.I, font->type, MM.S, MM.Boolean));

	UI.thisInstance = GetModuleHandle(NULL);
	UI.classRegistered = 0;
	UI.win = 0;
	UI.ikey = 0;
	UI.hdrop = NULL;
	return 0;
}
#endif
