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

//--------------- ON_WINDOWS
#define KEYBUFFER_LENGTH 32

typedef struct {
	HINSTANCE thisInstance;
	int classRegistered;
	HWND win;
	HDC paintdc;
	int virtkey[KEYBUFFER_LENGTH];
	int scankey[KEYBUFFER_LENGTH];
	int ikey;
	int flags;
	int w;
	int h;
}UIstruct;
UIstruct UI;

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
0,0,0,XK_Pause,XK_Caps_Lock,0,0,0,
0,0,0,0,0,0,0,0,
0,XK_Prior,XK_Next,XK_End,XK_Home,XK_Left,XK_Up,XK_Right,
XK_Down,0,0,0,XK_Sys_Req,XK_Insert,XK_Delete,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0, /*4.*/
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
XK_F1,XK_F2,XK_F3,XK_F4,XK_F5,XK_F6,XK_F7,XK_F8,
XK_F9,XK_F10,XK_F11,XK_F12,0,0,0,0,
0,0,0,0,0,0,0,0, /*8.*/
0,0,0,0,0,0,0,0,
XK_Num_Lock,XK_Scroll_Lock,0,0,0,0,0,0,
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
//	printf("uiTouch %d\n", n);
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
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int c;
//	printf("windowproc %d "LSX" "LSX"\n", msg, wParam, lParam);
	if (hwnd != UI.win) {
//		printf("unknown window!!!!!!!!!!!!!!!!! %x, "LSX"\n", msg,hwnd);
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
		eventNotify(EVENT_PAINT, 0, 0, 0);
		return(DefWindowProc(hwnd, msg, wParam, lParam));
		break;
	case WM_LBUTTONDOWN:
		SetCapture(hwnd);
		eventNotify(EVENT_CLICK, LOWORD(lParam), HIWORD(lParam), 0);
		break;
	case WM_RBUTTONDOWN:
		SetCapture(hwnd);
		eventNotify(EVENT_CLICK, LOWORD(lParam), HIWORD(lParam), 1);
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
	case WM_RBUTTONUP:
		ReleaseCapture();
		eventNotify(EVENT_UNCLICK, LOWORD(lParam), HIWORD(lParam), 1);
		break;
	case WM_MOUSEMOVE:
		eventNotify(EVENT_MOUSEMOVE, LOWORD(lParam), HIWORD(lParam), (int)wParam);
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
	case WM_MOVE:
		eventNotify(EVENT_MOVE, LOWORD(lParam), HIWORD(lParam), 0);
		break;
	case WM_SIZE:
		UI.w = LOWORD(lParam);
		UI.h = HIWORD(lParam);
		eventNotify(EVENT_SIZE, LOWORD(lParam), HIWORD(lParam), 0);
		break;
	case WM_CLOSE:
		eventNotify(EVENT_CLOSE, 0, 0, 0);
		break;
	case WM_DROPFILES:
//		if (uiDrop(th, vp, wi, (HDROP)wParam)) return 0;
//		DragFinish((HDROP)wParam);
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
//	printf("start hostloop\n");
	while(win==UI.win)
	{
		if (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
//		else return (int)msg.wParam;
	}
//	printf("exit hostloop\n");
	return 0;
}

BOOL DeclareWindow(HINSTANCE this_inst)
{
	WNDCLASS    wc;
	BOOL        rc;
	HICON IconMain;
	
	if (UI.classRegistered) return 0;
	IconMain = NULL;// LoadIcon(this_inst, (LPCTSTR)IDI_ICON1);

	wc.style = CS_HREDRAW | CS_VREDRAW; // | CS_DBLCLKS;
	wc.lpfnWndProc = (WNDPROC)WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = sizeof(DWORD);
	wc.hInstance = this_inst;
	wc.hIcon = IconMain;
	wc.hCursor = NULL;
	wc.hbrBackground = GetSysColorBrush(COLOR_MENU);
	wc.lpszMenuName = "GenericMenu";
	wc.lpszClassName = TEXT("WindowClass");

	rc = RegisterClass(&wc);
	UI.classRegistered = 1;
	return(rc);
}

int fullscreen_modes[] = { 32,24,16,15 };

int fullscreen(HWND hwnd, int w, int h)
{
	int i;
	for (i = 0; i < 4; i++)
	{
		long value;
		DEVMODE dev;
		memset(&dev, 0, sizeof(dev));
		dev.dmSize = sizeof(dev);
		dev.dmBitsPerPel = fullscreen_modes[i];
		dev.dmPelsHeight = h;
		dev.dmPelsWidth = w;
		dev.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		value = ChangeDisplaySettings(&dev, CDS_FULLSCREEN);
		if (value == DISP_CHANGE_SUCCESSFUL) return TRUE;
	}
	return 0;
}

MTHREAD_START _uiStart(Thread* th)
{
	LW result = NIL;

	LW vx, vy;
	LINT type, y, x;
	DWORD flags;
	char* name;
	RECT r;

	LB* p = VALTOPNT(STACKGET(th, 0));
	type = VALTOINT(STACKGET(th, 1));
	r.bottom = (LONG)VALTOINT(STACKGET(th, 2));
	r.right = (LONG)VALTOINT(STACKGET(th, 3));
	vy = STACKGET(th, 4);
	vx = STACKGET(th, 5);

	if (!p) name = "Minimacy";
	else name = STRSTART(p);

	if (type & UI_RESIZE)
	{
		if (type & UI_FRAME) flags = WS_POPUP | WS_THICKFRAME;
		else flags = (WS_OVERLAPPEDWINDOW);
	}
	else
	{
		if (type & UI_FRAME) flags = WS_POPUP;
		else flags = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
	}

	flags |= WS_CLIPCHILDREN;
	if (r.bottom < 0) r.bottom = 0;
	if (r.right < 0) r.right = 0;
	UI.w = r.right;
	UI.h = r.bottom;
	r.left = 0;
	r.top = 0;
	AdjustWindowRect(&r, flags, FALSE);

	y = (vy == NIL) ? CW_USEDEFAULT : VALTOINT(vy);
	x = (vx == NIL) ? CW_USEDEFAULT : VALTOINT(vx);
	DeclareWindow(UI.thisInstance);
	UI.win = CreateWindowEx(WS_EX_ACCEPTFILES, TEXT("WindowClass"), name, flags,
		(int)x, (int)y, r.right - r.left, r.bottom - r.top,
		NULL, NULL, UI.thisInstance, NULL);
	uiRegisterTouchWindow(UI.win);
	UI.paintdc= GetDC(UI.win);
	UI.flags = (int)flags;
#ifdef WITH_GLES2
	_windowInitGL(UI.win);
#endif

//	printf("UI.win "LSX"\n", UI.win);
//	uiRegisterTouchWindow(th, UI.win);
	ShowWindow(UI.win, (type & UI_MAXIMIZE) ? SW_MAXIMIZE : SW_SHOW);
	UpdateWindow(UI.win);
	result = MM.trueRef;
	workerDone(th, result);
	hostLoop();
	return MTHREAD_RETURN;

//cleanup:
//	return workerDone(th, result);
}
int fun_uiStart(Thread* th) { return workerStart(th, 6, _uiStart); }

int fun_uiResize(Thread* th)
{
	WINDOWPLACEMENT wnd;
	RECT r;

	LINT NDROP = 4 - 1;
	LW result = NIL;

	LINT y, x;
	LINT h = VALTOINT(STACKGET(th, 0));
	LINT w = VALTOINT(STACKGET(th, 1));
	LW vy = STACKGET(th, 2);
	LW vx = STACKGET(th, 3);

	if (!UI.win) goto cleanup;

	wnd.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(UI.win, &wnd);

	y = (vy == NIL) ? wnd.rcNormalPosition.top : VALTOINT(vy);
	x = (vx == NIL) ? wnd.rcNormalPosition.left : VALTOINT(vx);

	r.left = 0; r.top = 0; r.right = (LONG)w;	r.bottom = (LONG)h;
	AdjustWindowRect(&r, UI.flags, FALSE);
	w = (LINT)( r.right - r.left);
	h = (LINT)(r.bottom - r.top);

	if ((w < 0) || (h < 0)) goto cleanup;
	UI.w = (int)w;
	UI.h = (int)h;
	wnd.rcNormalPosition.left = (LONG)x;
	wnd.rcNormalPosition.top = (LONG)y;
	wnd.rcNormalPosition.right = (LONG)(x + w);
	wnd.rcNormalPosition.bottom = (LONG)(y + h);

	SetWindowPlacement(UI.win, &wnd);
	UpdateWindow(UI.win);

cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;

}

int fun_uiStop(Thread* th)
{
	HWND win = UI.win;
	if (!win) return STACKPUSH(th, MM.falseRef);
#ifdef WITH_GLES2
	_windowReleaseGL();
#endif
	UI.win = 0;
	PostMessage(win, WM_CLOSE, 0, 0);
	return STACKPUSH(th, MM.trueRef);
}

int fun_uiW(Thread* th)
{
	if (UI.win == 0) return STACKPUSH(th, NIL);
	return STACKPUSH(th,INTTOVAL(UI.w));
}
int fun_uiH(Thread* th)
{
	if (UI.win == 0) return STACKPUSH(th, NIL);
	return STACKPUSH(th,INTTOVAL(UI.h));
}
int fun_uiX(Thread* th)
{
	RECT r;
	if (UI.win == 0) return STACKPUSH(th, NIL);
	GetClientRect(UI.win, &r);
	MapWindowPoints(UI.win, GetParent(UI.win), (LPPOINT)&r, 2);
	return STACKPUSH(th,INTTOVAL(r.left));
}
int fun_uiY(Thread* th)
{
	RECT r;
	if (UI.win == 0) return STACKPUSH(th, NIL);
	GetClientRect(UI.win, &r);
	MapWindowPoints(UI.win, GetParent(UI.win), (LPPOINT)&r, 2);
	return STACKPUSH(th,INTTOVAL(r.top));
}
int fun_uiUpdate(Thread* th)
{
	HDC Dcb;
	LINT h, w;

	LW vh = STACKPULL(th);
	LW vw = STACKPULL(th);
	LINT y = VALTOINT(STACKPULL(th));
	LINT x = VALTOINT(STACKPULL(th));
	LBitmap* b = (LBitmap*)VALTOPNT(STACKGET(th, 0));
	if (!b) return 0;
	if (UI.win == 0) return 0;
	if ((b->w != UI.w) || (b->h != UI.h)) return 0;

	w = (vw == NIL) ? b->w : VALTOINT(vw);
	h = (vh == NIL) ? b->h : VALTOINT(vh);

	if (_clip1D(0, b->w, &x, &w, NULL)) return 0;
	if (_clip1D(0, b->h, &y, &h, NULL)) return 0;

	bitmapRequireDib(b);
	Dcb = CreateCompatibleDC(UI.paintdc);
	SelectObject(Dcb, b->bmp);
	BitBlt(UI.paintdc, (int)x, (int)y, (int)w, (int)h, Dcb, (int)x, (int)y, SRCCOPY);
	DeleteDC(Dcb);
	return 0;
}

int fun_screenW(Thread* th)
{
	RECT r;
	GetWindowRect(GetDesktopWindow(), &r);
	return STACKPUSH(th, INTTOVAL(r.right - r.left));
}
int fun_screenH(Thread* th)
{
	RECT r;
	GetWindowRect(GetDesktopWindow(), &r);
	return STACKPUSH(th, INTTOVAL(r.bottom - r.top));
}

int fun_uiSetName(Thread* th)
{
	LB* name = VALTOPNT(STACKGET(th, 0));
	if ((!name) || (!UI.win)) return 0;
	SetWindowText(UI.win, STRSTART(name));
	return 0;
}

int fun_uiFocus(Thread* th)
{
	if (UI.win) SetFocus(UI.win);
	return STACKPUSH(th, NIL);
}

int fun_keyboardState(Thread* th)
{
	int k = 0;
	if (GetKeyState(VK_SHIFT) < 0) k += KS_Shift;
	if (GetKeyState(VK_CONTROL) < 0) k += KS_Control;
	if (GetKeyState(VK_MENU) < 0) k += KS_Alt;
	//	if (GetKeyState(VK_MENU)<0) k+=KS_Meta;
	return STACKPUSH(th, INTTOVAL(k));
}

int fun_clipboardCopy(Thread* th)
{
	LINT len;
	HANDLE hdata;
	char* lpdata;
	LB* data = VALTOPNT(STACKGET(th, 0));
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
				if (stackPushStr(th, lpdata, -1)) return EXEC_OM;
				GlobalUnlock(hdata);
				CloseClipboard();
				return 0;
			}
		}
		CloseClipboard();
	}
	return STACKPUSH(th, NIL);
}

int fun_keyboardShow(Thread* th) { return fun_empty(th, 0); }
int fun_keyboardHide(Thread* th) { return fun_empty(th, 0); }
int fun_keyboardHeight(Thread* th) { return fun_empty(th, 0); }
int fun_orientationGet(Thread* th) { return fun_empty(th, 0); }
int fun_accelerometerX(Thread* th) { return fun_empty(th, 0); }
int fun_accelerometerY(Thread* th) { return fun_empty(th, 0); }
int fun_accelerometerZ(Thread* th) { return fun_empty(th, 0); }
int fun_accelerometerInit(Thread* th){ return fun_empty(th, 1); }

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

	LINT NDROP = 3 - 1;
	LW result = NIL;

	LINT flags = VALTOINT(STACKGET(th, 0));
	LINT size = VALTOINT(STACKGET(th, 1));
	LB* name = VALTOPNT(STACKGET(th, 2));
	if (!name) goto cleanup;

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
	if (!hfont) goto cleanup;
	f = (Font*)memoryAllocExt(th, sizeof(Font), DBG_BIN, _fontForget, NULL);
	if (!f) {
		DeleteObject(hfont);
		return EXEC_OM;
	}
	f->hfont = hfont;
	result = PNTTOVAL(f);
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}

int fun_fontH(Thread* th)
{
	HDC DC;
	TEXTMETRIC tm;

	Font* f= (Font*)VALTOPNT(STACKGET(th, 0));
	if (!f) return 0;

	DC = GetDC(NULL);
	SelectFont(DC, f->hfont);
	GetTextMetrics(DC, &tm);
	ReleaseDC(NULL, DC);
	STACKSET(th, 0,INTTOVAL(tm.tmHeight));
	return 0;
}

int _fontStringW(Thread* th,int u16)
{
	LINT size;
	HDC DC;

	LINT NDROP = 2 - 1;
	LW result = NIL;

	LB* str = VALTOPNT(STACKGET(th, 0));
	Font* f = (Font*)VALTOPNT(STACKGET(th, 1));
	if ((!f) || (!str)) goto cleanup;
	
	DC = GetDC(NULL);
	SelectFont(DC, f->hfont);
	if (u16) size= LOWORD(GetTabbedTextExtentW(DC, (LPCWSTR)STRSTART(str), (int)STRLEN(str) >> 1, 0, NULL));
	else size = LOWORD(GetTabbedTextExtentA(DC, STRSTART(str),(int) STRLEN(str), 0, NULL));
	ReleaseDC(NULL, DC);
	result = INTTOVAL(size);
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}
int fun_fontStringW(Thread* th) { return _fontStringW(th, 0); }
int fun_fontStringU16W(Thread* th) { return _fontStringW(th, 1); }

int _fontDraw(Thread* th, int u16)
{
	HDC dcb;

	LINT NDROP = 5 - 1;
	LW result = NIL;

	LB* str = VALTOPNT(STACKGET(th, 0));
	Font* f = (Font*)VALTOPNT(STACKGET(th, 1));
	LINT y = VALTOINT(STACKGET(th, 2));
	LINT x = VALTOINT(STACKGET(th, 3));
	LBitmap* b=(LBitmap*)VALTOPNT(STACKGET(th, 4));
	if ((!f) || (!str) || (!b)) goto cleanup;

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
	result = MM.trueRef;
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}
int fun_fontDraw(Thread* th) { return _fontDraw(th, 0);}
int fun_fontDrawU16(Thread* th) { return _fontDraw(th, 1); }

int coreUiHwInit(Thread* th, Pkg* system)
{
	Ref* font = pkgAddType(th, system, "Font");

	pkgAddConst(th, system, "FONT_BOLD", INTTOVAL(FONT_BOLD), MM.I);
	pkgAddConst(th, system, "FONT_ITALIC", INTTOVAL(FONT_ITALIC), MM.I);
	pkgAddConst(th, system, "FONT_UNDERLINE", INTTOVAL(FONT_UNDERLINE), MM.I);
	pkgAddConst(th, system, "FONT_STRIKED", INTTOVAL(FONT_STRIKED), MM.I);
	pkgAddConst(th, system, "FONT_PIXEL", INTTOVAL(FONT_PIXEL), MM.I);

	pkgAddFun(th, system, "fontFromHost", fun_fontFromHost, typeAlloc(th, TYPECODE_FUN, NULL, 4, MM.S, MM.I, MM.I, font->type));
	pkgAddFun(th, system, "fontH", fun_fontH, typeAlloc(th, TYPECODE_FUN, NULL, 2, font->type, MM.I));
	pkgAddFun(th, system, "fontStringW", fun_fontStringW, typeAlloc(th, TYPECODE_FUN, NULL, 3, font->type, MM.S, MM.I));
	pkgAddFun(th, system, "fontStringU16W", fun_fontStringU16W, typeAlloc(th, TYPECODE_FUN, NULL, 3, font->type, MM.S, MM.I));
	pkgAddFun(th, system, "fontDraw", fun_fontDraw, typeAlloc(th, TYPECODE_FUN, NULL, 6, MM.Bitmap, MM.I, MM.I, font->type, MM.S, MM.Boolean));
	pkgAddFun(th, system, "fontDrawU16", fun_fontDrawU16, typeAlloc(th, TYPECODE_FUN, NULL, 6, MM.Bitmap, MM.I, MM.I, font->type, MM.S, MM.Boolean));

	UI.thisInstance = NULL;
	UI.classRegistered = 0;
	UI.win = 0;
	UI.ikey = 0;
	return 0;
}
#endif
