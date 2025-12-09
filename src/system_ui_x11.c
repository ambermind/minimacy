// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"
#ifdef WITH_UI
#ifdef USE_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/Xatom.h>

typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	int xhot;
	int yhot;
	Cursor cursor;
	char tand[32 * 4*4];
	char txor[32 * 4*4];
}LCursor;

typedef struct {
	Display *display;
	int screen;
	Visual *visual;
	int depth;
	GC dc;
	int dndversion;
	int dndx;
	int dndy;
	int dndsource;
	Atom wmDeleteWindow;
	Atom wmProtocols;
	Atom XdndAware;
	Atom XdndEnter;
	Atom XdndPosition;
	Atom XdndStatus;
	Atom XdndAction;
	Atom XdndDrop;
	Atom XdndFinished;
	Atom XdndSelection;
	Atom xselection;

	Window win;
	GC paintdc;
	LCursor* cursorToMake;	// there is no need to update this during gcCompact as there can't be any gcCompact when in use
//	Cursor cursor;
	int type;
	int w;
	int h;
	int state;

#ifdef WITH_GL
	Display *displayGl;
	GLXContext contextGl;
#endif

}UIstruct;
UIstruct UI;

#ifdef WITH_GL
int testAttributesA[] = {
GLX_RGBA,
GLX_DOUBLEBUFFER,
GLX_DEPTH_SIZE, 32,
None
};
int testAttributesB[] = {
GLX_RGBA,
GLX_DOUBLEBUFFER,
GLX_DEPTH_SIZE, 24,
None
};
int testAttributesC[] = {
GLX_RGBA,
GLX_DOUBLEBUFFER,
GLX_DEPTH_SIZE, 16,
None
};

void _windowInitGL()
{
	UI.displayGl = NULL;
	UI.contextGl = NULL;
}

void _windowReleaseGL()
{
	if (UI.contextGl) glXDestroyContext(UI.displayGl, UI.contextGl);
	if (UI.displayGl) XCloseDisplay(UI.displayGl);
	UI.displayGl = NULL;
	UI.contextGl = NULL;
}

int _glMakeGLcontextInitGL()
{
	UI.displayGl = XOpenDisplay(NULL);	// we need a second connection to run opengl in a thread different from the Window thread
	XVisualInfo* vInfo = glXChooseVisual(UI.displayGl, DefaultScreen(UI.displayGl), testAttributesA);
	if (!vInfo) vInfo = glXChooseVisual(UI.displayGl, DefaultScreen(UI.displayGl), testAttributesB);
	if (!vInfo) vInfo = glXChooseVisual(UI.displayGl, DefaultScreen(UI.displayGl), testAttributesC);
	if (!vInfo) {
		PRINTF(LOG_SYS,"> Error: glXChooseVisual failed\n");
		return -1;
	}
	UI.contextGl = glXCreateContext(UI.displayGl, vInfo, NULL, True);
	glXMakeCurrent(UI.displayGl, UI.win, UI.contextGl);
	GLinstance++;
	return 0;
}
int fun_glMakeContext(Thread* th)
{
	//	PRINTF(LOG_DEV,"===============fun_glMakeContext\n");
	if (!UI.win) FUN_RETURN_NIL;
	if (!UI.displayGl) {
		if (_glMakeGLcontextInitGL()) FUN_RETURN_NIL;
	}
	FUN_RETURN_INT(0);
}
int fun_glRefreshContext(Thread* th)
{
	if (!UI.win) FUN_RETURN_NIL;
	if (UI.displayGl) {	// we have an option to 'refresh' the link with the context: mandatory on macX11GL after a resize
#ifdef ON_MACOS_CMDLINE
		glXMakeCurrent(UI.displayGl,UI.win,NULL);
		glXMakeCurrent(UI.displayGl,UI.win,UI.contextGl);
#endif
	}
	FUN_RETURN_INT(0);
}
int fun_glSwapBuffers(Thread* th)
{
	if (!UI.contextGl) FUN_RETURN_NIL;
	glXSwapBuffers(UI.displayGl, UI.win);
	FUN_RETURN_INT(0);
}
int viewPortScale(int u) { return u; }
#endif
//----------------------------------------------

int hwindowStartX(char* station)
{
	if (UI.display) return 0;
//	PRINTF(LOG_DEV,"X11: connect now\n");
	XInitThreads();
	UI.display=XOpenDisplay(station);
	if (!UI.display)
	{
		PRINTF(LOG_SYS,"> Error: cannot open X11 display\n");
		return -1;
	}
	UI.screen=DefaultScreen(UI.display);
	UI.visual=DefaultVisual(UI.display,UI.screen);
	UI.depth=DefaultDepth(UI.display,UI.screen);
	UI.dc=DefaultGC(UI.display,UI.screen);
	UI.wmDeleteWindow = XInternAtom (UI.display, "WM_DELETE_WINDOW", False);
	UI.wmProtocols=XInternAtom(UI.display, "WM_PROTOCOLS", False);
	UI.dndversion=4;
	UI.XdndAware=XInternAtom(UI.display,"XdndAware",False);
	UI.XdndEnter = XInternAtom(UI.display, "XdndEnter", False);
	UI.XdndPosition = XInternAtom(UI.display, "XdndPosition", False);
	UI.XdndStatus = XInternAtom(UI.display, "XdndStatus", False);
	UI.XdndAction = XInternAtom(UI.display, "XdndActionMove"/*"XdndActionCopy"*/, False);
	UI.XdndDrop = XInternAtom(UI.display, "XdndDrop", False);
	UI.XdndFinished = XInternAtom(UI.display, "XdndFinished", False);
	UI.XdndSelection = XInternAtom(UI.display, "XdndSelection", False);
	UI.xselection=XInternAtom(UI.display, "PRIMARY", 0);
	return 0;
}

void _sendSimpleEvent()
{
	XEvent	evhack;
	memset(&evhack,0,sizeof(XEvent));
	evhack.type=Expose;
	evhack.xexpose.window=UI.win;
	XSendEvent(UI.display,UI.win,False,ExposureMask,&evhack);
	XFlush(UI.display);
}
void _mkCursor(LCursor* d)
{
	Window root;
	Pixmap back,fore;
	XColor white,black;

	root=RootWindow(UI.display,UI.screen);
	back=XCreateBitmapFromData(UI.display,root,d->tand,32,32);
	fore=XCreateBitmapFromData(UI.display,root,d->txor,32,32);

	white.pixel=WhitePixel(UI.display,DefaultScreen(UI.display)); XQueryColor(UI.display,DefaultColormap(UI.display,UI.screen),&white);
	black.pixel=BlackPixel(UI.display,DefaultScreen(UI.display)); XQueryColor(UI.display,DefaultColormap(UI.display,UI.screen),&black);
	
	d->cursor=XCreatePixmapCursor(UI.display,fore,back,&white,&black,d->xhot,d->yhot);
	XFreePixmap(UI.display,fore);
	XFreePixmap(UI.display,back);
}

int hostLoop()
{
	Window win = UI.win;
	int lastButton=0;
	UI.cursorToMake=NULL;
//	PRINTF(LOG_DEV,"start hostloop\n");
	while(win==UI.win)
	{
		XEvent ev;
		int key;
		char c;
		KeySym ks;

//PRINTF(LOG_DEV,"wait event\n");
		XNextEvent(UI.display,&ev);

		if (ev.xany.window != UI.win) {
//			PRINTF(LOG_DEV,"unknown window!!!!!!!!!!!!!!!!! %x, "LSX"\n", ev.type,ev.xany.window);
			continue;
//			return 0;
		}
		if (UI.cursorToMake)
		{
//			printf("make cursor\n");
			_mkCursor(UI.cursorToMake);
			UI.cursorToMake=NULL;
		}
//		PRINTF(LOG_DEV,"event %d\n",ev.type);
		switch(ev.type)	{
		case ClientMessage:
//			PRINTF(LOG_DEV,"clientmessage %d %d\n",ev.xclient.message_type,UI.wmProtocols);
			if(ev.xclient.message_type==UI.wmProtocols)
			{
				if (ev.xclient.data.l[0]==UI.wmDeleteWindow)
				{
					eventNotify(EVENT_CLOSE, 0, 0, 0);
				}
			}
/*			else if (ev.xclient.message_type==UI.XdndPosition)
			{
				XClientMessageEvent m;
				UI.dndx=(ev.xclient.data.l[2]>>16)&0xffff;
				UI.dndy=ev.xclient.data.l[2]&0xffff;
//					PRINTF(LOG_DEV,"dndposition %d,%d\n",UI.dndx,UI.dndy);
				UI.dndsource=ev.xclient.data.l[0];
				memset(&m, sizeof(m), 0);
				m.type = ClientMessage;
				m.display = ev.xclient.display;
				m.window = ev.xclient.data.l[0];
				m.message_type = UI.XdndStatus;
				m.format=32;
				m.data.l[0] = hwnd;
				m.data.l[1] = 1;
				m.data.l[2] = 0; //Specify an empty rectangle
				m.data.l[3] = 1;
				m.data.l[4] = UI.XdndAction;

				XSendEvent(UI.display, ev.xclient.data.l[0], False, NoEventMask, (XEvent*)&m);
				XFlush(UI.display);
			}
			else if (ev.xclient.message_type==UI.XdndDrop)
			{
//					PRINTF(LOG_DEV,"xdnddrop\n");
				XConvertSelection(UI.display, UI.XdndSelection, XA_STRING, UI.xselection, hwnd, ev.xclient.data.l[2]);
			}
*/			break;
		case SelectionNotify:
//			PRINTF(LOG_DEV,"SelectionNotify\n");
/*			{
				XClientMessageEvent m;
				unsigned char* prop;
				prop=read_property(UI.display, hwnd, UI.xselection);
				if (prop) PRINTF(LOG_DEV,"data=%s\n",prop);
		
				memset(&m, sizeof(m), 0);
				m.type = ClientMessage;
				m.display = UI.display;
				m.window = UI.dndsource;
				m.message_type = UI.XdndFinished;
				m.format=32;
				m.data.l[0] = hwnd;
				m.data.l[1] = 1;
				m.data.l[2] = UI.XdndAction;

				XSendEvent(UI.display, UI.dndsource, False, NoEventMask, (XEvent*)&m);
				XFlush(UI.display);

				if (prop)
				{
					hwindowDrop(mw,th,vp,wi,prop);
					XFree(prop);
				}
			}
*/			break;
		case KeyPress:
//			PRINTF(LOG_DEV,"KeyPress=%d\n",ev.xkey.keycode);	
			UI.state=ev.xkey.state;
			key=ev.xkey.keycode&255;
			c=-1;
			XLookupString((XKeyEvent*)&ev,&c,1,&ks,NULL);
			if ((c)&&(c!=-1)) key=c;
			else key=ks&0xffff;
//			PRINTF(LOG_DEV,"key=%d\n",key);	
			if (key==127) key=XK_Delete;	//HACK sur clavier macintosh
			if (key==0xff6a) key=XK_Insert;	//HACK sur clavier macintosh (Insert s'obtient par Control+Insert)
			eventNotify(EVENT_KEYDOWN, key,0,0);
			break;
		case KeyRelease:
//			PRINTF(LOG_DEV,"KeyRelease=%d\n",ev.xkey.keycode);	
			UI.state=ev.xkey.state;
			key=ev.xkey.keycode&255;
			c=-1;
			XLookupString((XKeyEvent*)&ev,&c,1,&ks,NULL);
			if ((c)&&(c!=-1)) key=c;
			else key=ks&0xffff;
			if (key==127) key=XK_Delete;	//HACK sur clavier macintosh
			if (key==0xff6a) key=XK_Insert;	//HACK sur clavier macintosh (Insert s'obtient par Control+Insert)
			eventNotify(EVENT_KEYUP, key,0,0);
			break;
		case Expose:
//			PRINTF(LOG_DEV,"Expose\n");	
			if (win==UI.win) eventNotify(EVENT_PAINT, 0, 0, 0);
			break;
		case ButtonPress:
//			PRINTF(LOG_DEV,"ButtonPress %d\n",ev.xbutton.button);
			UI.state=ev.xbutton.state;
			if (ev.xbutton.button<=3) {
				eventNotify(EVENT_CLICK, ev.xbutton.x,ev.xbutton.y,ev.xbutton.button);
				lastButton=ev.xbutton.button;
			}
			if ((ev.xbutton.button==4)||(ev.xbutton.button==5))
				eventNotify(EVENT_VWHEEL, ev.xbutton.x,ev.xbutton.y, (ev.xbutton.button==4)?10:-10);
			if ((ev.xbutton.button==6)||(ev.xbutton.button==7))
				eventNotify(EVENT_HWHEEL, ev.xbutton.x,ev.xbutton.y, (ev.xbutton.button==7)?10:-10);
			break;
		case ButtonRelease:
//			PRINTF(LOG_DEV,"ButtonRelease %d\n",ev.xbutton.button);
			UI.state=ev.xbutton.state;
			if (ev.xbutton.button<=3)
				eventNotify(EVENT_UNCLICK, ev.xbutton.x,ev.xbutton.y,ev.xbutton.button);
			lastButton=0;
			break;
		case MotionNotify:
//			PRINTF(LOG_DEV,"MotionNotify %d\n",ev.xbutton.button);
			UI.state=ev.xbutton.state;
			while(XCheckMaskEvent(UI.display,PointerMotionMask,&ev)) {/*PRINTF(LOG_DEV,"remove motion\n");*/};
			eventNotify(EVENT_MOUSEMOVE, ev.xbutton.x,ev.xbutton.y,lastButton);
			break;
		case ConfigureNotify:
//			PRINTF(LOG_DEV,"ConfigureNotify %d,%d %dx%d\n",ev.xconfigure.x,ev.xconfigure.y,ev.xconfigure.width,ev.xconfigure.height);
			UI.w=ev.xconfigure.width;
			UI.h=ev.xconfigure.height;
			eventNotify(EVENT_SIZE, ev.xconfigure.width,ev.xconfigure.height, 0);

			break;
		}

		XFlush(UI.display);
	}
	XDestroyWindow(UI.display,win);
	XFlush(UI.display);

//	PRINTF(LOG_DEV,"exit hostloop\n");
    return 0;
}

struct MwmHints {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long input_mode;
    unsigned long status;
};
enum {
    MWM_HINTS_FUNCTIONS = (1L << 0),
    MWM_HINTS_DECORATIONS =  (1L << 1),

    MWM_FUNC_ALL = (1L << 0),
    MWM_FUNC_RESIZE = (1L << 1),
    MWM_FUNC_MOVE = (1L << 2),
    MWM_FUNC_MINIMIZE = (1L << 3),
    MWM_FUNC_MAXIMIZE = (1L << 4),
    MWM_FUNC_CLOSE = (1L << 5)
};
void _uiApplyHints(LINT w, LINT h)
{
	int typeStyle= UI.type & UI_TYPE_MASK;
	XSizeHints* hints = XAllocSizeHints();
	hints->flags = PPosition | PWinGravity;
	if (typeStyle!=UI_RESIZE) hints->flags|= PSize | PMinSize | PMaxSize;	// no effect on XQuartz (known issue) beyond maximizing button
	hints->width = hints->min_width = hints->max_width = w;
	hints->height = hints->min_height = hints->max_height = h;
	hints->win_gravity = StaticGravity;
	XSetWMNormalHints(UI.display, UI.win, hints);
	XFree(hints);

	if (typeStyle== UI_FULLSCREEN)
	{
		Atom mwmHintsProperty = XInternAtom(UI.display, "_MOTIF_WM_HINTS", 0);
		struct MwmHints hints;
		hints.flags = MWM_HINTS_DECORATIONS;
		hints.decorations = 0;
		XChangeProperty(UI.display, UI.win, mwmHintsProperty, mwmHintsProperty, 32,
    	    PropModeReplace, (unsigned char *)&hints, 5);
	}
	UI.w=w; UI.h=h;
}

WORKER_START _uiStart(volatile Thread* th)
{
	int xIsNil, yIsNil;
	LINT x, y;
	int typeStyle;
	char* name;

	LB* p = STACK_PNT(th, 0);
	LINT type = STACK_INT(th, 1);
	LINT h=STACK_INT(th,2);
	LINT w=STACK_INT(th,3);
	y = STACK_INT(th, 4);
	x = STACK_INT(th, 5);

	if (hwindowStartX(NULL)) workerDoneNil(th);

	if (!p) name = "Minimacy";
	else name = STR_START(p);

	UI.type=(int)type;
	typeStyle= UI.type & UI_TYPE_MASK;

	if (typeStyle == UI_FULLSCREEN) {
		x = y = 0;
		w=DisplayWidth(UI.display,UI.screen);
		h=DisplayHeight(UI.display,UI.screen);
//		PRINTF(LOG_DEV,"size type %lld : %lld, %lld\n",type,w,h);
	}
	UI.win=XCreateSimpleWindow(UI.display, RootWindow(UI.display,UI.screen),
			x,y,w,h,0,WhitePixel(UI.display,UI.screen),
			BlackPixel(UI.display,UI.screen));
	XSetStandardProperties(UI.display, UI.win, name,name,0,NULL,0,NULL);
	#ifdef WITH_GL
		_windowInitGL();
	#endif

	_uiApplyHints(w,h);

	XSetWMProtocols(UI.display,UI.win, &UI.wmDeleteWindow, 1);
	XChangeProperty(UI.display,UI.win, UI.XdndAware, XA_ATOM, 32, PropModeReplace, (unsigned char*)&UI.dndversion, 1);

	UI.paintdc=DefaultGC(UI.display,UI.screen);
	XSelectInput(UI.display,UI.win,ExposureMask|ButtonPressMask|ButtonReleaseMask
		|PointerMotionMask|KeyPressMask|KeyReleaseMask|StructureNotifyMask);
	XMapWindow(UI.display,UI.win);
	XFlush(UI.display);

	workerDonePnt(th, MM._true);
	hostLoop();
	return WORKER_RETURN;
}
int fun_uiStart(Thread* th) {
#ifdef USE_WORKER_ASYNC
	return workerStart(th, 6, _uiStart);
#else
	PRINTF(LOG_SYS, "> UI can't start without asynchronous workers\n");
	FUN_RETURN_NIL
#endif
}

int fun_uiResize(Thread* th)
{
	LINT h=STACK_INT(th, 0);
	LINT w=STACK_INT(th, 1);
	if (!UI.win) FUN_RETURN_NIL;
	
	_uiApplyHints(w,h);
	XResizeWindow(UI.display,UI.win,w,h);
	FUN_RETURN_INT(0);
}

int fun_uiStop(Thread* th)
{
	XEvent	evhack;
	Window win = UI.win;
	if (!win) FUN_RETURN_FALSE;
#ifdef WITH_GL
	_windowReleaseGL();
#endif
	UI.win = 0;
	// we send an event to free the hostloop, which in turn will destroy the window
	_sendSimpleEvent();
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
	if (hwindowStartX(NULL)) FUN_RETURN_NIL;
	FUN_RETURN_INT(DisplayWidth(UI.display,UI.screen));
}
int fun_screenH(Thread* th)
{
	if (hwindowStartX(NULL)) FUN_RETURN_NIL;
	FUN_RETURN_INT(DisplayHeight(UI.display,UI.screen));
}

int fun_uiSetTitle(Thread* th)
{
	LB* name = STACK_PNT(th, 0);
	if ((!name) || (!UI.win)) FUN_RETURN_NIL;
	XSetStandardProperties(UI.display, UI.win, STR_START(name),STR_START(name),0,NULL,0,NULL);
	return 0;
}

int fun_keyboardState(Thread* th)
{
	int k=0;
	int state=UI.state;

	if (hwindowStartX(NULL)) FUN_RETURN_NIL;
	if (state&ShiftMask) k+=KeyMask_Shift;
	if (state&ControlMask) k+=KeyMask_Control;
	if (state&Mod1Mask) k+=KeyMask_Alt;
	if (state&Mod2Mask) k+=KeyMask_Meta;
	FUN_RETURN_INT(k);
}

int fun_clipboardCopy(Thread* th)
{
	LB* data = STACK_PNT(th, 0);
	if (!data) return 0;

	if (hwindowStartX(NULL)) FUN_RETURN_NIL;
	XStoreBytes(UI.display,STR_START(data),STR_LENGTH(data));
	return 0;
}
int fun_clipboardPaste(Thread* th)
{
	int len;
	char* p;
	if (hwindowStartX(NULL)) FUN_RETURN_NIL;
	p=XFetchBytes(UI.display,&len);
	if (!p) FUN_RETURN_NIL;
	FUN_RETURN_STR(p, -1);
}
int fun_uiDrop(Thread* th) FUN_RETURN_NIL

int _cursorForget(LB* p)
{
	LCursor* c = (LCursor*)p;
	if (!UI.win) return 0;
	if (c) XFreeCursor(UI.display,c->cursor);
	return 0;
}

int fun_cursorSize(Thread* th)
{
	FUN_PUSH_INT(32);
	FUN_PUSH_INT(32);
	FUN_MAKE_ARRAY( 2, DBG_TUPLE);
	return 0;
}
int fun_cursorCreate(Thread* th)
{
	Cursor hc;
	int i, j, w, h, lenbmp;
	int* p;
	LCursor* d;
	Window root;
	Pixmap back,fore;
	XColor white,black;

	LINT yhot = STACK_INT(th, 0);
	LINT xhot = STACK_INT(th, 1);
	LBitmap* bmp = (LBitmap*)STACK_PNT(th, 2);
	if ((!bmp)||(!UI.win)) FUN_RETURN_NIL;	// we need a window before creating the cursor

//printf("alloc cursor %d\n",sizeof(LCursor));
	d = (LCursor*)memoryAllocNative(sizeof(LCursor), DBG_BIN, _cursorForget, NULL); if (!d) return EXEC_OM;
//printf("alloc cursor done\n");
	d->cursor=0;
	w = 32;
	h = 32;
	if ((bmp->w != w) || (bmp->h != h)) FUN_RETURN_NIL;
	lenbmp = ((w + 7) >> 3) * h;
	for (i = 0; i < lenbmp; i++) d->tand[i] = d->txor[i] = 0;

	p = bmp->start32;
	for (j = 0; j < h; j++)
		for (i = 0; i < w; i++)
		{
			int color = p[j * bmp->next32 + i];
			if (color&0xff) d->tand[(i >> 3) + ((w + 7) >> 3) * j] |= 1<<(i&7);
			if (color&0xff0000) d->txor[(i >> 3) + ((w + 7) >> 3) * j] |= 1<<(i&7);
		}
	d->xhot=xhot;
	d->yhot=yhot;
	UI.cursorToMake=d;
	_sendSimpleEvent();
	while(UI.cursorToMake)
	{
		struct timeval tm;
		tm.tv_sec =  0;
		tm.tv_usec = 1000;
		select(1, NULL, NULL, NULL, &tm);
//		printf("..wait\n");
	}
	FUN_RETURN_PNT((LB*)d);
}
int fun_cursorShow(Thread* th)
{
	LCursor* d = (LCursor*) STACK_PNT(th, 0);

	if (!d) XUndefineCursor(UI.display,UI.win);
	else XDefineCursor(UI.display,UI.win, d->cursor);
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

	XFontStruct* hfont;
}NativeFont;

Display *_nativeFontDisplay()
{
	if (UI.displayGl) return UI.displayGl;
	return UI.display;
}

int _nativeFontForget(LB* p)
{
	NativeFont* f = (NativeFont*)p;
	if (f) XFreeFont(_nativeFontDisplay(),f->hfont);
	return 0;
}

int fun_nativeFontCreate(Thread* th)
{
	XFontStruct* hfont;
	NativeFont* f;

	LINT flags = STACK_INT(th, 0);
	LINT size = STACK_INT(th, 1);
	LB* name = STACK_PNT(th, 2);
	if (!name) FUN_RETURN_NIL;

	if (hwindowStartX(NULL)) FUN_RETURN_NIL;
	hfont = XLoadQueryFont(_nativeFontDisplay(),STR_START(name));
	if (!hfont) FUN_RETURN_NIL;
	f = (NativeFont*)memoryAllocNative(sizeof(NativeFont), DBG_BIN, _nativeFontForget, NULL);
	if (!f) {
		XFreeFont(_nativeFontDisplay(),hfont);
		return EXEC_OM;
	}
	f->hfont = hfont;
	FUN_RETURN_PNT((LB*)f);
}

int fun_nativeFontH(Thread* th)
{
	NativeFont* f= (NativeFont*)STACK_PNT(th, 0);
	if (!f) FUN_RETURN_NIL;
	FUN_RETURN_INT(f->hfont->ascent+f->hfont->descent);
}

int fun_nativeFontBaseline(Thread* th)
{
	NativeFont* f= (NativeFont*)STACK_PNT(th, 0);
	if (!f) FUN_RETURN_NIL;
	FUN_RETURN_INT(f->hfont->descent);
}

int fun_nativeFontW(Thread* th)
{
	LINT size;
	XChar2b c;

	LINT code = STACK_INT(th, 0);
	NativeFont* f = (NativeFont*)STACK_PNT(th, 1);
	if (!f) FUN_RETURN_NIL;

	c.byte2=code; code>>=8;
	c.byte1=code;
	size= XTextWidth16(f->hfont, &c, 1);
	FUN_RETURN_INT(size);
}

int fun_nativeFontDraw(Thread* th)
{
	char* newstart;
	XImage *image;
	Pixmap pixmap;
	XChar2b c;
	Display *display;
	int i,j;

	LINT code = STACK_INT(th, 0);
	NativeFont* f = (NativeFont*)STACK_PNT(th, 1);
	LINT y = STACK_INT(th, 2);
	LINT x = STACK_INT(th, 3);
	LBitmap* b=(LBitmap*)STACK_PNT(th, 4);
	if ((!f) || (!b)) FUN_RETURN_NIL;
	display=_nativeFontDisplay();
	UI.dc=DefaultGC(display,UI.screen);
	XSetFont(display,UI.dc,f->hfont->fid);
	XSetForeground(display,UI.dc,WhitePixel(display,UI.screen));
	newstart=malloc(b->w*b->h*4); if (!newstart) FUN_RETURN_NIL;
	memcpy(newstart,b->start8,b->w*b->h*4);
	image=XCreateImage(display, UI.visual, UI.depth, ZPixmap, 0, newstart, b->w, b->h, 8, 0);
	pixmap=XCreatePixmap(display,RootWindow(display,UI.screen),b->w, b->h,UI.depth);
	XPutImage(display, pixmap, UI.dc, image, 0,0,0,0,b->w, b->h);
	y+=f->hfont->ascent;
	c.byte2=code; code>>=8;
	c.byte1=code;
	XDrawString16(display,pixmap, UI.dc,x,y,&c,1);
	XGetSubImage(display,pixmap, 0,0, b->w, b->h, 0x00FFFFFF, ZPixmap,image,0,0);

	memcpy(b->start8,newstart,b->w*b->h*4);
	XDestroyImage(image);
	XFreePixmap(display,pixmap);
	FUN_RETURN_PNT(MM._true);
}

int fun_nativeFontList(Thread* th)
{
	char** fontList;
	int fontNumber=1000000;
	int i;

	if (hwindowStartX(NULL)) FUN_RETURN_NIL;
	fontList=XListFonts(_nativeFontDisplay(),"*",fontNumber,&fontNumber);
//	printf("found %d fonts\n",fontNumber);
	for(i=0;i<fontNumber;i++) {
//		printf("> %s\n",fontList[i]);
		FUN_PUSH_STR(fontList[i],-1);
	}
	XFreeFontNames(fontList);
	FUN_PUSH_NIL;
	while (fontNumber--) FUN_MAKE_ARRAY(LIST_LENGTH, DBG_LIST);
	return 0;
}
int systemUiHwInit(Pkg* system)
{
	UI.display=NULL;
	UI.win = 0;
	UI.state=0;
	return 0;
}
#endif
#endif
