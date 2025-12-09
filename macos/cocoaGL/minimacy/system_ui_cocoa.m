// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#import "system_ui_cocoa.h"
#import "system_ble.h"
#import "system_location.h"

#include<sys/stat.h>
const int TraceOn=0;
SystemUIcocoa* UI;
void sendEvent(int c, int x, int y, int v);

@implementation SystemUIcocoa

// pixel format definition
+ (NSOpenGLPixelFormat*) basicPixelFormat
{
    NSOpenGLPixelFormatAttribute attributes [] = {
        NSOpenGLPFAWindow,
        NSOpenGLPFADoubleBuffer,	// double buffered
        NSOpenGLPFADepthSize, (NSOpenGLPixelFormatAttribute)16, // 16 bit depth buffer
        (NSOpenGLPixelFormatAttribute)nil
    };
    return [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
}

- (void)getGeometry {
	w=self.bounds.size.width;
	h=self.bounds.size.height;
	x=self.window.frame.origin.x;
	y=self.window.screen.frame.size.height-self.window.frame.origin.y-h;
//    scale=(int)[self.window backingScaleFactor];
//    if (!scale) scale=1;
	if (TraceOn) NSLog(@"geometry %d %d %d %d",x,y,w,h);
}

- (void)drawRect:(NSRect)dirtyRect {
	[super drawRect:dirtyRect];
	if (!started)
	{
		self.window.delegate=self;
		[[self window] setAcceptsMouseMovedEvents: YES];
		started=true;
	}
}

-(void)resizeWindow
{
    CGPoint invertedOrigin;
	if (TraceOn) NSLog(@"resize %@",NSStringFromRect(shape));
    invertedOrigin.x=shape.origin.x;
    invertedOrigin.y=self.window.screen.frame.size.height-shape.origin.y-shape.size.height;
//    if (TraceOn) printf("%f %f %f\n",self.window.screen.frame.size.height,invertedShape.size.height, invertedShape.origin.y);
    [[self window] setContentSize:shape.size];
    [[self window] setFrameOrigin:invertedOrigin];
	[self getGeometry];
    if (TraceOn) printf("resizeWindow\n");
}

// ---------------------------------

- (BOOL)windowShouldClose:(NSWindow *)sender
{
//    printf("windowShouldClose\n");
    sendEvent(EVENT_CLOSE, 0, 0, 0);
    return FALSE;
}

- (NSSize)windowWillResize:(NSWindow *)sender
                    toSize:(NSSize)frameSize
{
    NSSize current=[[self window]frame].size;
    if (TraceOn) printf("windowWillResize %fx%f -> %fx%f state=%d\n",current.width, current.height, frameSize.width,frameSize.height,resizeState);
    [self getGeometry];
    if (resizeState==RESIZE_WAIT_DONE) return frameSize;
    if (resizeState==RESIZE_IDLE)
    {
        resizeState=RESIZE_EVENT;
        if (TraceOn) printf("send event\n");
        sendEvent(EVENT_WILL_RESIZE, frameSize.width,frameSize.height, 0);
    }
    return current;
}
- (void) windowDidResize:(NSWindow *)sender
{
    [self getGeometry];
    if (TraceOn) printf("windowDidResize %d %d\n",w,h);
}
- (void) windowWillEnterFullScreen:(NSNotification *)notification
{
    if (TraceOn) printf("windowWillEnterFullScreen\n");
}
- (void) windowDidEnterFullScreen:(NSNotification *)notification
{
//    [self getGeometry];
    if (TraceOn) printf("windowDidEnterFullScreen\n");
}
- (void) windowWillExitFullScreen:(NSNotification *)notification
{
//    [self getGeometry];
    if (TraceOn) printf("windowWillExitFullScreen\n");
}
- (void) windowDidExitFullScreen:(NSNotification *)notification
{
//    [self getGeometry];
    if (TraceOn) printf("windowDidExitFullScreen\n");
}
-(void)renameWindow
{
	[[UI window] setTitle:title];
}
-(void)startWindow
{
	[self renameWindow];
    
    int typeStyle=type&UI_TYPE_MASK;
    if (typeStyle==UI_FULLSCREEN){
        shape.origin.x=0;
        shape.origin.y=0;
        shape.size.width=self.window.screen.frame.size.width;
        shape.size.height=self.window.screen.frame.size.height;
    }

	if (typeStyle==UI_RESIZE || typeStyle==UI_FULLSCREEN)
		[self window].styleMask |= NSWindowStyleMaskResizable;
	else
		[self window].styleMask &= ~NSWindowStyleMaskResizable;

    open=true;
    [self resizeWindow];
    [self show];
    int fullscreen=([self window].styleMask & NSWindowStyleMaskFullScreen)?1:0;
    int newFullscreen= (typeStyle==UI_FULLSCREEN)?1:0;
    if (fullscreen!=newFullscreen) [[self window] toggleFullScreen:nil];

    NSOpenGLPixelFormat * format = [SystemUIcocoa basicPixelFormat];
//    printf("context before=%llx\n",[self openGLContext]);
    [self clearGLContext];
    [self setPixelFormat:format];
    NSOpenGLContext *context=[self openGLContext];
//    printf("context afterrun demo.fun.pacman=%llx\n",context);
    [context setView:self];
    sendEvent(EVENT_PAINT,0,0,0);

}
// ---------------------------------

-(void)mouseHandle:(NSEvent *)theEvent button:(int)b state:(int)state
{
	NSPoint location = [self convertPoint:[theEvent locationInWindow] fromView:nil];
	location.y=h-location.y;
//	printf("mouse %d: %d,%d %d\n",state,(int)location.x,(int)location.y,b);
	sendEvent(state,(int)location.x,(int)location.y,b);
	
}

- (void)mouseDown:(NSEvent *)theEvent
{
	[self mouseHandle:theEvent button:1 state:EVENT_CLICK];
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
	[self mouseHandle:theEvent button:3 state:EVENT_CLICK];
}

- (void)mouseUp:(NSEvent *)theEvent
{
	[self mouseHandle:theEvent button:1 state:EVENT_UNCLICK];
}

- (void)rightMouseUp:(NSEvent *)theEvent
{
	[self mouseHandle:theEvent button:3 state:EVENT_UNCLICK];
}

- (void)mouseDragged:(NSEvent *)theEvent
{
	[self mouseHandle:theEvent button:1 state:EVENT_MOUSEMOVE];
}
- (void)rightMouseDragged:(NSEvent *)theEvent
{
	[self mouseHandle:theEvent button:3 state:EVENT_MOUSEMOVE];
}
- (void)otherMouseDragged:(NSEvent *)theEvent
{
	NSPoint location = [self convertPoint:[theEvent locationInWindow] fromView:nil];
	location.y=h-location.y;
    if (TraceOn) printf("otherMouseDragged %d %d\n",(int)location.x,(int)location.y);
}
- (void)mouseMoved:(NSEvent *)theEvent
{
	NSPoint location = [self convertPoint:[theEvent locationInWindow] fromView:nil];
	location.y=h-location.y;
	if (location.x>=0 && location.x<w &&location.y>=0 && location.y<h) [self mouseHandle:theEvent button:0 state:EVENT_MOUSEMOVE];
}


// not working: touches Events not fired with espresso display
-(void)touchCommon:(NSEvent *)event
{
    int touchCount=0;
    CGPoint anyUp;
    float sx=0;
    float sy=0;
    NSArray *touches=[[event allTouches] allObjects];
    int n=(int)[touches count];
    printf("touchCommon %d\n",n);
    if (!n) return;
    for(int i=0;i<n;i++)
    {
        NSTouch* touch=[touches objectAtIndex:i];
        CGPoint p=[touch locationInView:self];
        
       printf("---touch %fx%f up=%d\n",(float)p.x,(float)p.y,(touch.phase!=NSTouchPhaseEnded)?0:1);
        if (touch.phase!=NSTouchPhaseEnded)
        {
            touchCount++;
            sx+=p.x; sy+= p.y;
            sendEvent(EVENT_MULTITOUCH,(int)p.x,(int)p.y,0);
        }
        else anyUp=p;
    }
    sendEvent(EVENT_MULTITOUCH,0,0,1);
    if (!touchCount) {
//        printf("unclick %fx%f\n",anyUp.x, anyUp.y);
        sendEvent(EVENT_UNCLICK,(int)anyUp.x,(int)anyUp.y,1);
    }
    else {
        sx/=touchCount;
        sy/=touchCount;
        if (lastTouchCount) {
//            printf("move %fx%f\n",sx,sy);
            sendEvent(EVENT_MOUSEMOVE,(int)sx,(int)sy,1);
        }
        else {
//            printf("click %fx%f\n",sx,sy);
            sendEvent(EVENT_CLICK,(int)sx,(int)sy,1);
        }
    }
    lastTouchCount=touchCount;
}
-(void)touchesBegan:(NSSet *)touches withEvent:(NSEvent *)event
{
    [self touchCommon:event];
}
-(void)touchesMoved:(NSSet *)touches withEvent:(NSEvent *)event
{
    [self touchCommon:event];
}
-(void)touchesEnded:(NSSet *)touches withEvent:(NSEvent *)event
{
    [self touchCommon:event];
}

- (void)scrollWheel:(NSEvent *)theEvent
{
	float fx=[theEvent deltaX];
	float fy=[theEvent deltaY];
	int x=fx;
	int y=fy;
	//    printf("wheel %d %d\n",x,y);
	if (x)[self mouseHandle:theEvent button:x state:EVENT_HWHEEL];
	if (y)[self mouseHandle:theEvent button:y state:EVENT_VWHEEL];
}


// ---------------------------------
- (void)flagsChanged:(NSEvent *)theEvent
{
	modifiers=[theEvent modifierFlags];
	//    printf("modifiers=%x\n",modifiers);
}

-(int)keyVal:(NSEvent *)theEvent
{
	int keycode=[theEvent keyCode];
	modifiers=[theEvent modifierFlags];
	NSString *text = [theEvent characters];
	const char* p=[text cStringUsingEncoding:NSISOLatin1StringEncoding];
	if (p)
	{
		int v=p[0]&255;
		//        printf("key %d : %d\n",keycode,v);
		if (v==127) return 8;
		if ((v==118)&&(modifiers&NSCommandKeyMask)) return 22;
		if ((v==111)&&(modifiers&NSCommandKeyMask)) return XKey_F2;
		if ((v==83)&&(modifiers&NSCommandKeyMask)&&(modifiers&NSShiftKeyMask)) return XKey_F5;
		if ((v==115)&&(modifiers&NSCommandKeyMask)) return XKey_F3;
		if ((v==110)&&(modifiers&NSCommandKeyMask)) return XKey_F4;
		return v;
	}
	//    printf("key %d\n",keycode);
	switch(keycode)
	{
		case 123 : return XKey_Left;
		case 124 : return XKey_Right;
		case 125 : return XKey_Down;
		case 126 : return XKey_Up;
	}
	return 0;
}
-(void)keyDown:(NSEvent *)theEvent
{
	int v=[self keyVal:theEvent];
//	printf("keyDown=%d\n",v);
	if (v) sendEvent(EVENT_KEYDOWN,v,0,0);
}
-(void)keyUp:(NSEvent *)theEvent
{
	int v=[self keyVal:theEvent];
	//	printf("keyUp=%d\n",v);
	if (v) sendEvent(EVENT_KEYUP,v,0,0);
}

-(LINT)keyboardState
{
	LINT f=0;
	if (modifiers&NSShiftKeyMask) f|=KeyMask_Shift;
	if (modifiers&NSAlternateKeyMask) f|=KeyMask_Alt;
	if (modifiers&NSControlKeyMask) f|=KeyMask_Control;
	if (modifiers&NSCommandKeyMask) f|=KeyMask_Meta;
	return f;
}

- (char*)getPaste
{
	NSPasteboard *thePasteboard = [NSPasteboard generalPasteboard];
	NSString* text=[thePasteboard stringForType:NSStringPboardType];
	return (char*)[text cStringUsingEncoding:NSISOLatin1StringEncoding];
}

// ---------------------------------

- (BOOL)acceptsFirstResponder
{
	return YES;
}

// ---------------------------------

- (BOOL)becomeFirstResponder
{
	return  YES;
}

// ---------------------------------

- (BOOL)resignFirstResponder
{
	return YES;
}

- (void) hide
{
	[[self window] orderOut:self];
}
- (void) show
{
	[[self window] makeKeyAndOrderFront:self];
	[NSApp activateIgnoringOtherApps:YES];
}

// ---------------------------------
- (id)initWithCoder:(NSCoder *)decoder
{
//	NSLog(@"initWithCoder");
	self = [super initWithCoder:decoder];
	UI=self;
    scale=[[NSScreen mainScreen]backingScaleFactor];
//	scale=(int)[self.window backingScaleFactor];
//	if ([[NSScreen mainScreen] respondsToSelector:@selector(backingScaleFactor)]==YES
//		&&[[NSScreen mainScreen]backingScaleFactor]==2.00) scale=2;
//	printf("scale=%d\n",scale);
	
	[self registerForDraggedTypes: [NSArray arrayWithObjects:
									NSFilenamesPboardType, nil]];

	[self getGeometry];
    self.acceptsTouchEvents = true;

	startInThread(0,NULL);
	return self;
}

-(void)startGl  // this one is run in the minimacy thread
{
	[[self openGLContext] makeCurrentContext];
}
-(void)drawFrame
{
	[[self openGLContext] flushBuffer];
}
- (void)setCursor
{
    if (cursor) [cursor set];
    else [[NSCursor arrowCursor] set];
}

// ---------------------------------
- (NSDragOperation)draggingEntered:(id )sender
{
//	printf("draggingEntered\n");
	NSDragOperation sourceDragMask = [sender draggingSourceOperationMask];
	NSPasteboard* pboard = [sender draggingPasteboard];
	if ( [[pboard types] containsObject:NSFilenamesPboardType] ) {
		if (sourceDragMask & NSDragOperationLink) {
			return NSDragOperationLink;
		} else if (sourceDragMask & NSDragOperationCopy) {
			return NSDragOperationCopy;
		}
	}
	return NSDragOperationNone;
} // end draggingEntered

/*- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
	NSPoint p=[sender draggingLocation];
	return NSDragOperationCopy;
}
*/
- (BOOL)performDragOperation:(id )sender {
	NSPoint p=[sender draggingLocation];
	p.y=h-p.y;
//	printf("performDragOperation %f, %f\n",p.x,p.y);
	NSPasteboard* pboard = [sender draggingPasteboard];
	if ( [[pboard types] containsObject:NSFilenamesPboardType] )
	{
		droppedFiles = [pboard propertyListForType:NSFilenamesPboardType];
		sendEvent(EVENT_DROPFILES,(int)p.x,(int)p.y,0);
	}
	return YES;
	
} // end performDragOperation

- (BOOL)prepareForDragOperation:(id )sender {
//	printf("prepareForDragOperation\n");
	return YES;
} // end prepareForDragOperation

- (void)concludeDragOperation:(id )sender {
//	printf("concludeDragOperation\n");
//    [self setNeedsDisplay:YES];
}

NSCursor* cursorCreate(char* start,int w,int h,int xHot,int yHot)
{
	CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
	CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, start, h*w*4, NULL);
	CGImageRef imageRef = CGImageCreate(w, h, 8, 32, w*4, rgb,
				  kCGBitmapByteOrderDefault | kCGImageAlphaLast,
				  provider, NULL, false, kCGRenderingIntentDefault);
	NSImage* img=[[NSImage alloc] initWithCGImage:imageRef size:NSZeroSize];
	NSPoint p;
	p.x=xHot; p.y=yHot;
	NSCursor* cursor=[[NSCursor alloc] initWithImage:img hotSpot:p];
	return cursor;
}

@end

typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	char data[32 * 32*4];
	NSCursor* cursor;
}LCursor;

int _cursorForget(LB* p)
{
//    LCursor* c = (LCursor*)p;
//    if (c) XFreeCursor(UI.display,c->cursor);
	return 0;
}


int fun_cursorSize(Thread* th)
{
	FUN_PUSH_INT( 32);
	FUN_PUSH_INT( 32);
	FUN_MAKE_ARRAY( 2, DBG_TUPLE);
	return 0;
}

int fun_cursorShow(Thread* th)
{
	LCursor* d = (LCursor*) STACK_PNT(th, 0);
	NSCursor* cursor=d?d->cursor:NULL;
	if (cursor!=UI->cursor)
	{
		UI->cursor=cursor;
		[UI performSelectorOnMainThread:@selector(setCursor) withObject:nil waitUntilDone:true];
	}
	return 0;
}

int fun_cursorCreate(Thread* th)
{
	int i, j, w, h;
	LCursor* d;

	LINT yhot = STACK_INT(th, 0);
	LINT xhot = STACK_INT(th, 1);
	LBitmap* bmp = (LBitmap*)STACK_PNT(th, 2);
	if (!bmp) FUN_RETURN_NIL;

	w = 32;
	h = 32;
	if ((bmp->w != w) || (bmp->h != h)) FUN_RETURN_NIL;
	
	d = (LCursor*)memoryAllocNative(sizeof(LCursor), DBG_BIN, _cursorForget, NULL);
	if (!d) return EXEC_OM;
	for(j=0;j<h;j++)
		for(i=0;i<w;i++)
		{
			lchar* p= &bmp->start8[j*bmp->next8+i*4];
			char* q=&d->data[(j*w+i)*4];
			q[0]=p[2];
			q[1]=p[2];
			q[2]=p[2];
			q[3]=p[0];
		}
	d->cursor=cursorCreate(d->data,w,h,(int)xhot,(int)yhot);
	FUN_RETURN_PNT((LB*)d);
}

void systemMainDir(char* path, int len, const char* argv0)
{
	NSString* file=[[NSBundle mainBundle]pathForResource:@"topLevel" ofType:@"mcy"];
	if (file) [file getFileSystemRepresentation:path maxLength:1023];
	else strcpy(path,"");
	for(int i=(int)strlen(path)-1;i>=0;i--) if (path[i]=='/') {
		path[i+1]=0;
		break;
	}
//    printf("minimacy dir=%s\n%d\n",path,strlen(path));
}

void sendEvent(int c, int x, int y, int v)
{
//    printf("sendEvent started %d -> %d\n",UI->open,c);
	if (UI->open) eventNotify(c,x,y,v);
}

int fun_uiStart(Thread* th) {
	char* name;
	
	LB* p = STACK_PNT(th, 0);
	LINT type = STACK_INT(th, 1);
	LINT h=STACK_INT(th, 2);
	LINT w=STACK_INT(th, 3);
	int yIsNil= STACK_IS_NIL(th, 4);
	LINT y = STACK_INT(th, 4);
	int xIsNil= STACK_IS_NIL(th, 5);
	LINT x = STACK_INT(th, 5);

//	printf("===============fun_uiStart\n");

	if (!p) name = "Minimacy";
	else name = STR_START(p);
	if (UI->open) FUN_RETURN_NIL;
	UI->title=[NSString stringWithUTF8String:name];
	if (yIsNil) y = 40;
	if (xIsNil) x = 40;
	UI->type=type;
	UI->shape.origin.x=x;
	UI->shape.origin.y=y;
	UI->shape.size.width=w;
	UI->shape.size.height=h;
    UI->resizeState=RESIZE_IDLE;

	[UI performSelectorOnMainThread:@selector(startWindow) withObject:nil waitUntilDone:true];
    [UI startGl];

	FUN_RETURN_TRUE
}

int fun_uiResize(Thread* th)
{	
	LINT h=STACK_INT(th, 0);
	LINT w=STACK_INT(th, 1);
	
	if (!UI->open) FUN_RETURN_NIL;

	UI->shape.origin.x=-1;
	UI->shape.origin.y=-1;	
    UI->shape.size.width=w;
	UI->shape.size.height=h;
    
    if (TraceOn) NSLog(@"fun_resize %lld %lld :%d",w,h, UI->resizeState);

    int initialState=UI->resizeState;
    UI->resizeState=RESIZE_WAIT_DONE;
    
    if (w>=0)   // cas manuel
    {
        w=UI->w; h=UI->h;
        [UI performSelectorOnMainThread:@selector(resizeWindow) withObject:nil waitUntilDone:true];
    }
    if (TraceOn) printf("loopStart\n");
    w=h=0;
    while(w!=UI->w || h!=UI->h)
    {
        w=UI->w; h=UI->h;
        usleep(1000*500);
    }
    UI->resizeState=RESIZE_IDLE;
    if (TraceOn) printf("loopEnd\n");
    if (initialState!=RESIZE_WAIT_DONE) {
        sendEvent(EVENT_SIZE,(int)UI->w,(int)UI->h, 0);
        sendEvent(EVENT_PAINT, 0, 0, 0);
    }
	FUN_RETURN_INT(0);
}
int fun_uiStop(Thread* th) {
	if (!UI->open) FUN_RETURN_FALSE;
	UI->open = false;
	[UI performSelectorOnMainThread:@selector(hide) withObject:nil waitUntilDone:true];
	FUN_RETURN_TRUE
}
int fun_uiW(Thread* th) { FUN_RETURN_INT(UI->w);}
int fun_uiH(Thread* th) { FUN_RETURN_INT(UI->h);}

int fun_screenW(Thread* th) { FUN_RETURN_INT((int)[[NSScreen mainScreen]frame].size.width);}
int fun_screenH(Thread* th) { FUN_RETURN_INT((int)[[NSScreen mainScreen]frame].size.height);}

int fun_uiSetTitle(Thread* th)
{
	LB* name = STACK_PNT(th, 0);
	if (!name) return 0;
	UI->title=[NSString stringWithUTF8String:STR_START(name)];
	[UI performSelectorOnMainThread:@selector(renameWindow) withObject:nil waitUntilDone:true];
	return 0;
}

int fun_keyboardState(Thread* th) { FUN_RETURN_INT([UI keyboardState]);}

int fun_clipboardCopy(Thread* th) FUN_RETURN_NIL

int fun_clipboardPaste(Thread* th)
{
	char* p=[UI getPaste];
	if (!p) FUN_RETURN_NIL;
	FUN_RETURN_STR(p, -1);
}
int fun_uiDrop(Thread* th) {
	if (!UI->droppedFiles) FUN_RETURN_NIL;

	LINT nbDroppedFiles = [UI->droppedFiles count];
	LINT nbReturnedFiles = 0;
	for (int i = 0; i < nbDroppedFiles; i++)
	{
		struct stat info;
		char buf[1024];
		
		NSString* file=[UI->droppedFiles objectAtIndex:i];
		[file getFileSystemRepresentation:buf+1 maxLength:1022];
		stat(buf+1, &info);
		buf[0]=S_ISDIR(info.st_mode)?'D':'F';
		FUN_PUSH_STR( buf, -1);
		nbReturnedFiles++;
	}
	FUN_PUSH_NIL;
	while ((nbReturnedFiles--) > 0) FUN_MAKE_ARRAY( LIST_LENGTH, DBG_LIST);
	UI->droppedFiles = NULL;
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

int fun_glMakeContext(Thread* th)
{
    // context is already made on COCOA!
    GLinstance++;
    FUN_RETURN_INT(0);
}
int fun_glRefreshContext(Thread* th) FUN_RETURN_NIL
int fun_glSwapBuffers(Thread* th)
{
	[UI drawFrame];
	FUN_RETURN_INT(0);
}
int viewPortScale(int u) { float v=u; v*=UI->scale; return (int)v; }

void systemCurrentDir(char* path, int len) { path[0] = 0; }
void systemUserDir(char* path, int len) { path[0] = 0; }

int systemUiHwInit(Pkg* system) {
    systemBleInit(system);
    systemLocationInit(system);
    return 0;
}
