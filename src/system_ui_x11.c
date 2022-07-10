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
#ifdef USE_X11

typedef struct {
	Display *display;
	int screen;
	Visual *visual;
	int depth;
	HDC dc;
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

	HWND win;
	HDC paintdc;
	int type;
	int x;
	int y;
	int w;
	int h;
	int state;

}UIstruct;
UIstruct UI;

int hwindowStartX(char* station)
{
	if (UI.display) return 0;
//	printf("X11: connect now\n");
	XInitThreads();
	UI.display=XOpenDisplay(station);
	if (!UI.display)
	{
		PRINTF(NULL,LOG_ERR,"X11: cannot open display\n");
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

void bitmapRequireDib(LBitmap* d)
{
	lchar* newstart;
//	printf("bitmapRequireDib %llx %llx\n",d,d->bmp);
	if (d->bmp) return;        // already created ?
	if (hwindowStartX(NULL)) return;

	// we allocate another block of memory outside the managed memory
	// because it will be deallocated by the XDestroyImage
	newstart=malloc(d->w*d->h*4); if (!newstart) return;
	memcpy(newstart,d->start8,d->w*d->h*4);

	d->bytes=NULL; // we forget the initial buffer
	d->start8 = newstart;
	d->next8 = d->w*4;
	d->start32 = (int*)d->start8;
	d->next32 = d->next8 >> 2;
	d->bmp = (void*)XCreateImage(UI.display, UI.visual, UI.depth, ZPixmap, 0, (char*)d->start8, d->w, d->h, 8, 0);

//	printf("done %llx\n",d->bmp);
}

int hostLoop()
{
	HWND win = UI.win;
//	printf("start hostloop\n");
	while(win==UI.win)
	{
		XEvent ev;
		int key;
		char c;
		KeySym ks;

//printf("wait event\n");
		XNextEvent(UI.display,&ev);

		if (ev.xany.window != UI.win) {
//			printf("unknown window!!!!!!!!!!!!!!!!! %x, "LSX"\n", ev.type,ev.xany.window);
			continue;
//			return 0;
		}
//		printf("event %d\n",ev.type);
		switch(ev.type)	{
		case ClientMessage:
//			printf("clientmessage %d %d\n",ev.xclient.message_type,UI.wmProtocols);
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
//					printf("dndposition %d,%d\n",UI.dndx,UI.dndy);
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
//					printf("xdnddrop\n");
				XConvertSelection(UI.display, UI.XdndSelection, XA_STRING, UI.xselection, hwnd, ev.xclient.data.l[2]);
			}
*/			break;
		case SelectionNotify:
//			printf("SelectionNotify\n");
/*			{
				XClientMessageEvent m;
				unsigned char* prop;
				prop=read_property(UI.display, hwnd, UI.xselection);
				if (prop) printf("data=%s\n",prop);
		
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
//			printf("KeyPress=%d\n",ev.xkey.keycode);	
			UI.state=ev.xkey.state;
			key=ev.xkey.keycode&255;
			c=-1;
			XLookupString((XKeyEvent*)&ev,&c,1,&ks,NULL);
			if ((c)&&(c!=-1)) key=c;
			else key=ks&0xffff;
//			printf("key=%d\n",key);	
			if (key==127) key=XK_Delete;	//HACK sur clavier macintosh
			if (key==0xff6a) key=XK_Insert;	//HACK sur clavier macintosh (Insert s'obtient par Control+Insert)
			eventNotify(EVENT_KEYDOWN, key,0,0);
			break;
		case KeyRelease:
//			printf("KeyRelease=%d\n",ev.xkey.keycode);	
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
//			printf("Expose\n");	
			if (win==UI.win) eventNotify(EVENT_PAINT, 0, 0, 0);
			break;
		case ButtonPress:
//			printf("ButtonPress %d\n",ev.xbutton.button);
			UI.state=ev.xbutton.state;
			if (ev.xbutton.button<=3)
				eventNotify(EVENT_CLICK, ev.xbutton.x,ev.xbutton.y,ev.xbutton.button);
			if ((ev.xbutton.button==4)||(ev.xbutton.button==5))
				eventNotify(EVENT_VWHEEL, ev.xbutton.x,ev.xbutton.y, (ev.xbutton.button==4)?1:-1);
			if ((ev.xbutton.button==6)||(ev.xbutton.button==7))
				eventNotify(EVENT_HWHEEL, ev.xbutton.x,ev.xbutton.y, (ev.xbutton.button==4)?7:-1);
			break;
		case ButtonRelease:
//			printf("ButtonRelease %d\n",ev.xbutton.button);
			UI.state=ev.xbutton.state;
			if (ev.xbutton.button<=3)
				eventNotify(EVENT_UNCLICK, ev.xbutton.x,ev.xbutton.y,ev.xbutton.button);
			break;
		case MotionNotify:
//			printf("MotionNotify %d\n",ev.xbutton.button);
			UI.state=ev.xbutton.state;
			while(XCheckMaskEvent(UI.display,PointerMotionMask,&ev)) {/*printf("remove motion\n");*/};
			eventNotify(EVENT_MOUSEMOVE, ev.xbutton.x,ev.xbutton.y,0);
			break;
		case ConfigureNotify:
//			printf("ConfigureNotify %d,%d %dx%d\n",ev.xconfigure.x,ev.xconfigure.y,ev.xconfigure.width,ev.xconfigure.height);
			UI.x=ev.xconfigure.x;
			UI.y=ev.xconfigure.y;
			UI.w=ev.xconfigure.width;
			UI.h=ev.xconfigure.height;
			eventNotify(EVENT_SIZE, ev.xconfigure.width,ev.xconfigure.height, 0);
			eventNotify(EVENT_MOVE, ev.xconfigure.x,ev.xconfigure.y, 0);
			break;
		}

		XFlush(UI.display);
	}
	XDestroyWindow(UI.display,win);
	XFlush(UI.display);

//	printf("exit hostloop\n");
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
void _uiApplyHints(LINT x, LINT y, LINT w, LINT h)
{
	XSizeHints* hints = XAllocSizeHints();
	hints->flags = PPosition | PWinGravity;
	if (!(UI.type&UI_RESIZE)) hints->flags|= PSize | PMinSize | PMaxSize;
	hints->x = x; hints->y = y;
	hints->width = w; hints->height = h;
	hints->min_width = hints->max_width = w;
	hints->min_height = hints->max_height = h;
	hints->win_gravity = StaticGravity;
	XSetWMNormalHints(UI.display, UI.win, hints);
	XFree(hints);

	if (UI.type&UI_FRAME)
	{
		Atom mwmHintsProperty = XInternAtom(UI.display, "_MOTIF_WM_HINTS", 0);
		struct MwmHints hints;
		hints.flags = MWM_HINTS_DECORATIONS;
		hints.decorations = 0;
		XChangeProperty(UI.display, UI.win, mwmHintsProperty, mwmHintsProperty, 32,
    	    PropModeReplace, (unsigned char *)&hints, 5);
	}
	UI.x=x; UI.y=y; UI.w=w; UI.h=h;
}

MTHREAD_START _uiStart(Thread* th)
{
	LW result = NIL;

	LINT x, y;
	char* name;

	LB* p = VALTOPNT(STACKGET(th, 0));
	LINT type = VALTOINT(STACKGET(th, 1));
	LINT h=VALTOINT(STACKGET(th,2));
	LINT w=VALTOINT(STACKGET(th,3));
	LW vy = STACKGET(th, 4);
	LW vx = STACKGET(th, 5);

	if (hwindowStartX(NULL)) goto cleanup;

	if (!p) name = "Minimacy";
	else name = STRSTART(p);

	y=(vy==NIL)?0:VALTOINT(vy);
	x=(vx==NIL)?0:VALTOINT(vx);

	UI.win=XCreateSimpleWindow(UI.display, RootWindow(UI.display,UI.screen),
			x,y,w,h,0,WhitePixel(UI.display,UI.screen),
			BlackPixel(UI.display,UI.screen));
	XSetStandardProperties(UI.display, UI.win, name,name,0,NULL,0,NULL);
	UI.type=type;
	#ifdef WITH_GLES2
		_windowInitGL(UI.win);
	#endif

	_uiApplyHints(x,y,w,h);

	XSetWMProtocols(UI.display,UI.win, &UI.wmDeleteWindow, 1);
	XChangeProperty(UI.display,UI.win, UI.XdndAware, XA_ATOM, 32, PropModeReplace, (unsigned char*)&UI.dndversion, 1);

	UI.paintdc=DefaultGC(UI.display,UI.screen);
	XSelectInput(UI.display,UI.win,ExposureMask|ButtonPressMask|ButtonReleaseMask
		|PointerMotionMask|KeyPressMask|KeyReleaseMask|StructureNotifyMask);
	XMapWindow(UI.display,UI.win);
	XFlush(UI.display);

	result = MM.trueRef;
	workerDone(th, result);
	hostLoop();
	return MTHREAD_RETURN;

cleanup:
	return workerDone(th, result);
}
int fun_uiStart(Thread* th) { return workerStart(th, 6, _uiStart); }

int fun_uiResize(Thread* th) {
	LINT NDROP = 4 - 1;
	LW result = NIL;
	
	LINT y, x;
	LINT h=VALTOINT(STACKGET(th, 0));
	LINT w=VALTOINT(STACKGET(th, 1));
	LW vy = STACKGET(th, 2);
	LW vx = STACKGET(th, 3);
	
	if (!UI.win) goto cleanup;
	
	y = (vy == NIL) ? UI.y: VALTOINT(vy);
	x = (vx == NIL) ? UI.x : VALTOINT(vx);

	_uiApplyHints(x,y,w,h);
	XMoveResizeWindow(UI.display,UI.win,x,y,w,h);
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;

}

int fun_uiStop(Thread* th)
{
	XEvent	evhack;
	HWND win = UI.win;
	if (!win) return STACKPUSH(th, MM.falseRef);
	#ifdef WITH_GLES2
		_windowReleaseGL();
	#endif
	UI.win = 0;
	// we send an event to free the hostloop, which in turn will destroy the window
	evhack.type=Expose;
	XSendEvent(UI.display,win,False,ExposureMask,&evhack);
	XFlush(UI.display);
	return STACKPUSH(th, MM.trueRef);
}
int fun_uiW(Thread* th) { return STACKPUSH(th,INTTOVAL(UI.w));}
int fun_uiH(Thread* th) { return STACKPUSH(th,INTTOVAL(UI.h));}
int fun_uiX(Thread* th) { return STACKPUSH(th,INTTOVAL(UI.x));}
int fun_uiY(Thread* th) { return STACKPUSH(th,INTTOVAL(UI.y));}

int fun_uiUpdate(Thread* th)
{
	LINT h, w;

	LW vh = STACKPULL(th);
	LW vw = STACKPULL(th);
	LINT y = VALTOINT(STACKPULL(th));
	LINT x = VALTOINT(STACKPULL(th));

	LBitmap* b = (LBitmap*)VALTOPNT(STACKGET(th, 0));
	if (!b) return 0;
//printf("uiUpdate now %llx\n",UI.win);
	if (UI.win == 0) return 0;
	if ((b->w != UI.w) || (b->h != UI.h)) return 0;

	w = (vw == NIL) ? b->w : VALTOINT(vw);
	h = (vh == NIL) ? b->h : VALTOINT(vh);

	if (_clip1D(0, b->w, &x, &w, NULL)) return 0;
	if (_clip1D(0, b->h, &y, &h, NULL)) return 0;

	bitmapRequireDib(b);

	XPutImage(UI.display,UI.win,UI.paintdc,(XImage*)b->bmp,x,y,x,y,w,h);
	XFlush(UI.display);
	return 0;
}

int fun_screenW(Thread* th)
{
	if (hwindowStartX(NULL)) return STACKPUSH(th,NIL);
	return STACKPUSH(th, INTTOVAL(DisplayWidth(UI.display,UI.screen)));
}
int fun_screenH(Thread* th)
{
	if (hwindowStartX(NULL)) return STACKPUSH(th,NIL);
	return STACKPUSH(th, INTTOVAL(DisplayHeight(UI.display,UI.screen)));
}

int fun_uiSetName(Thread* th)
{
	LB* name = VALTOPNT(STACKGET(th, 0));
	if ((!name) || (!UI.win)) return 0;
	XSetStandardProperties(UI.display, UI.win, STRSTART(name),STRSTART(name),0,NULL,0,NULL);
	return 0;
}

int fun_uiFocus(Thread* th)
{
	if (UI.win) XSetInputFocus(UI.display,UI.win,RevertToNone,CurrentTime);
	return STACKPUSH(th, NIL);
}

int fun_keyboardState(Thread* th)
{
	int k=0;
	int state=UI.state;

	if (hwindowStartX(NULL)) return STACKPUSH(th,NIL);
	if (state&ShiftMask) k+=KS_Shift;
	if (state&ControlMask) k+=KS_Control;
	if (state&Mod1Mask) k+=KS_Alt;
	if (state&Mod2Mask) k+=KS_Meta;
	return STACKPUSH(th, INTTOVAL(k));
}

int fun_clipboardCopy(Thread* th)
{
	LB* data = VALTOPNT(STACKGET(th, 0));
	if (!data) return 0;

	if (hwindowStartX(NULL)) return 0;
	XStoreBytes(UI.display,STRSTART(data),STRLEN(data));
	return 0;
}
int fun_clipboardPaste(Thread* th)
{
	int len;
	char* p;
	if (hwindowStartX(NULL)) return 0;
	p=XFetchBytes(UI.display,&len);
	if (p) return stackPushStr(th, p, -1);
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
int coreUiHwInit(Thread* th, Pkg* system)
{
	UI.display=NULL;
	UI.win = 0;
	UI.state=0;
	return 0;
}
#endif
