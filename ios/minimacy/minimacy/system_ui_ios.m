// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#import <Foundation/Foundation.h>

#import "system_ui_ios.h"
#import "system_ble.h"
#import "system_location.h"


SystemUIIOS* UI;
void sendEvent(int c, int x, int y, int v);

@implementation SystemUIIOS


- (void)insertText:(NSString *)text {
    const char* p=[text cStringUsingEncoding:NSISOLatin1StringEncoding];
//    printf("insert '");
//    if (p) for(int i=0;i<strlen(p);i++) printf("%02x:",p[i]&255);
//    printf("'\n");
    if (!p) return;
    sendEvent(EVENT_KEYDOWN,p[0],0,0);
    sendEvent(EVENT_KEYUP,p[0],0,0);
}
- (void)deleteBackward {
    printf("backspace\n");
    sendEvent(EVENT_KEYDOWN,8,0,0);
    sendEvent(EVENT_KEYUP,8,0,0);
}
- (BOOL)hasText {
    return YES;
}
- (BOOL)canBecomeFirstResponder {
    return YES;
}
-(void)showKeyboard {
    [self becomeFirstResponder];
}
-(void)hideKeyboard {
    [self resignFirstResponder];
}

-(void)checksize
{
    CGRect bounds=self.bounds;
//    printf("current %d,%d\n",w,h);
//    printf("view bounds %fx%f %f\n",bounds.size.width,bounds.size.height,scale);
    if((w!=(int)bounds.size.width)||(h!=(int)bounds.size.height))
    {
        w=(int)bounds.size.width;
        h=(int)bounds.size.height;
//        printf("check %d,%d\n",w,h);
        sendEvent(EVENT_SIZE,w,h,0);
        sendEvent(EVENT_PAINT,w,h,0);
    }
}
-(void)flip:(UIInterfaceOrientation)interfaceOrientation
{
    orientation=(int)interfaceOrientation-1;
    printf("orientation=%d\n",orientation);
    [self checksize];
}


-(void)touchCommon:(UIEvent *)event
{
	int touchCount=0;
	CGPoint anyUp;
	float sx=0;
	float sy=0;
    NSArray *touches=[[event allTouches] allObjects];
    int n=(int)[touches count];
	if (!n) return;
    for(int i=0;i<n;i++)
    {
        UITouch* touch=[touches objectAtIndex:i];
        CGPoint p=[touch locationInView:self];
		
//		printf("---touch %fx%f up=%d\n",(float)p.x,(float)p.y,(touch.phase!=UITouchPhaseEnded)?0:1);
        if (touch.phase!=UITouchPhaseEnded)
		{
			touchCount++;
			sx+=p.x; sy+= p.y;
			sendEvent(EVENT_MULTITOUCH,(int)p.x,(int)p.y,0);
		}
		else anyUp=p;
    }
	sendEvent(EVENT_MULTITOUCH,0,0,1);
	if (!touchCount) {
//		printf("unclick %fx%f\n",anyUp.x, anyUp.y);
		sendEvent(EVENT_UNCLICK,(int)anyUp.x,(int)anyUp.y,1);
	}
	else {
		sx/=touchCount;
		sy/=touchCount;
		if (lastTouchCount) {
//			printf("move %fx%f\n",sx,sy);
			sendEvent(EVENT_MOUSEMOVE,(int)sx,(int)sy,1);
		}
		else {
//			printf("click %fx%f\n",sx,sy);
			sendEvent(EVENT_CLICK,(int)sx,(int)sy,1);
		}
	}
	lastTouchCount=touchCount;
}
-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self touchCommon:event];
}
-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self touchCommon:event];
}
-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self touchCommon:event];
}

// ---------------------------------
- (id)initWithCoder:(NSCoder *)decoder
{
//    NSLog(@"initWithCoder");
    self = [super initWithCoder:decoder];
    UI=self;
    keyboardHeight=0;
    
	self.drawableDepthFormat=GLKViewDrawableDepthFormat16;
//    self.delegate=self;
    scale=[[UIScreen mainScreen] nativeScale];

    CGRect bounds=self.bounds;
    w=(int)bounds.size.width;
    h=(int)bounds.size.height;

//    CGRect bounds=[[UIScreen mainScreen] nativeBounds];
//    printf("view bounds %fx%f %f\n",bounds.size.width,bounds.size.height,scale);
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWasShown:) name:UIKeyboardDidShowNotification object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillHide:) name:UIKeyboardWillHideNotification object:nil];

    startInThread(0,NULL);

    return self;
}

- (void)drawRect:(CGRect)dirtyRect {
    [super drawRect:dirtyRect];
//    glClearColor(1.0, 0.0, 0.0, 1.0);
//    glClear(GL_COLOR_BUFFER_BIT);
}

-(void)startGl  // this one is run in the minimacy thread
{
    open=true;
    [EAGLContext setCurrentContext:self.context];
}
-(void)showFrame
{
    [self checksize];
    [self.context presentRenderbuffer:GL_RENDERBUFFER];
}

-(void)keyboardWasShown:(NSNotification *)notification
{
//    CGRect keyboardFrame = [[[notification userInfo] objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
//    printf("keyboardWasShown %fx%f\n",keyboardFrame.size.width,keyboardFrame.size.height);
    CGRect keyboardFrame2 = [[[notification userInfo] objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue];
    printf("keyboardWasShown2 %fx%f\n",keyboardFrame2.size.width,keyboardFrame2.size.height);
    if ([[UIScreen mainScreen] respondsToSelector:@selector(scale)] == YES)
        printf("scale=%f\n",[[UIScreen mainScreen] scale]);
    
//    float hk=(orientation>=2)?keyboardFrame2.size.width:keyboardFrame2.size.height;
    float hk=scale*keyboardFrame2.size.height;
	keyboardHeight=(int)hk;
    printf("---->viewKeyboardH=%d scale=%f orientation %d\n",keyboardHeight,scale,orientation);
    
}
-(void)keyboardWillHide:(NSNotification *)notification
{
//    CGRect keyboardFrame = [[[notification userInfo] objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
//    printf("keyboardWillHide %fx%f\n",keyboardFrame.size.width,keyboardFrame.size.height);
//    CGRect keyboardFrame2 = [[[notification userInfo] objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue];
//    printf("keyboardWillHide2 %fx%f\n",keyboardFrame2.size.width,keyboardFrame2.size.height);
    keyboardHeight=0;
    printf("---->viewKeyboardH=%d\n",keyboardHeight);
}

-(void)accelerometer:(UIAccelerometer *)accelerometer didAccelerate:(UIAcceleration *)acceleration
{
    accX=acceleration.x;
    accY=acceleration.y;
    accZ=acceleration.z;
//    printf("accelerometer %f %f %f\n",accX,accY,accZ);
}
-(void)setAccelerometer:(float)freq
{
    accX=accY=accZ=0.0;
	if (freq==0) {
		[[UIAccelerometer sharedAccelerometer] setDelegate:nil];
		return;
	}
    [[UIAccelerometer sharedAccelerometer] setUpdateInterval:1.0 / freq];
    [[UIAccelerometer sharedAccelerometer] setDelegate:self];
}
@end

void sendEvent(int c, int x, int y, int v)
{
//    printf("sendEvent started %d -> %d\n",UI->open,c);
    if (UI->open) eventNotify(c,x,y,v);
}

int fun_uiStart(Thread* th) {
    // we ignore most of the arguments as we have no control on the size of the screen and the ui is always fullscreen
    // at this stage only the type could be useful for a non GL ui, but we need to fix uiUpdate first
    LINT type = STACK_INT(th, 1);
    if (UI->open) FUN_RETURN_NIL;
        
    UI->type=type;
    [UI startGl];

    FUN_RETURN_PNT(MM._true);
}

int fun_uiResize(Thread* th)  FUN_RETURN_NIL

int fun_uiStop(Thread* th) {
    if (!UI->open) FUN_RETURN_FALSE;
    UI->open = false;   // this will stop the incoming events
    //TODO when would we possibly stop ui on iphone?
	FUN_RETURN_TRUE;
}
int fun_uiW(Thread* th) { FUN_RETURN_INT(UI->w);}
int fun_uiH(Thread* th) { FUN_RETURN_INT(UI->h);}

int fun_screenW(Thread* th) {
    FUN_RETURN_INT((int)[[UIScreen mainScreen] bounds].size.width);
}
int fun_screenH(Thread* th) {
    FUN_RETURN_INT((int)[[UIScreen mainScreen] bounds].size.height);
}

int fun_uiSetName(Thread* th) FUN_RETURN_NIL

int fun_uiFocus(Thread* th) FUN_RETURN_NIL// TODO open virtual keyboard?
int fun_uiSetTitle(Thread* th) FUN_RETURN_NIL

int fun_keyboardState(Thread* th) { FUN_RETURN_INT(0);}

int fun_clipboardCopy(Thread* th) FUN_RETURN_NIL

int fun_clipboardPaste(Thread* th) FUN_RETURN_NIL
int fun_uiDrop(Thread* th) FUN_RETURN_NIL

int fun_keyboardShow(Thread* th) {
    [UI performSelectorOnMainThread:@selector(showKeyboard) withObject:nil waitUntilDone:true];
    FUN_RETURN_TRUE;
}
int fun_keyboardHide(Thread* th) {
    [UI performSelectorOnMainThread:@selector(hideKeyboard) withObject:nil waitUntilDone:true];
    FUN_RETURN_TRUE;
}
int fun_keyboardHeight(Thread* th) { FUN_RETURN_INT(UI->keyboardHeight);}
int fun_orientationGet(Thread* th) { FUN_RETURN_INT(UI->orientation);}
int fun_accelerometerX(Thread* th) { FUN_RETURN_FLOAT(UI->accX);}
int fun_accelerometerY(Thread* th) { FUN_RETURN_FLOAT(UI->accY);}
int fun_accelerometerZ(Thread* th) { FUN_RETURN_FLOAT(UI->accZ);}
int fun_accelerometerInit(Thread* th) {
    LFLOAT f = STACK_FLOAT(th,0);
    [UI setAccelerometer:f];
    return 0;
}

int fun_cursorSize(Thread* th) FUN_RETURN_NIL
int fun_cursorCreate(Thread* th) FUN_RETURN_NIL
int fun_cursorShow(Thread* th) FUN_RETURN_NIL

int fun_glMakeContext(Thread* th)
{
    // context is already made on IOS!
    GLinstance++;
    FUN_RETURN_INT(0);
}
int fun_glRefreshContext(Thread* th) FUN_RETURN_NIL
int fun_glSwapBuffers(Thread* th)
{
//    printf("glSwapBuffers\n");
//    [UI showFrame];
    [UI performSelectorOnMainThread:@selector(showFrame) withObject:nil waitUntilDone:true];

    FUN_RETURN_INT(0);
}
int viewPortScale(int u) { float v=u; v*=UI->scale; return (int)v; }

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

void notifySuspend(void)
{
    if (UI && !UI->suspended)
    {
        sendEvent(EVENT_SUSPEND,0,0,0);
        UI->suspended=true;
    }
}
void notifyResume(void)
{
    if (UI && UI->suspended)
    {
        sendEvent(EVENT_RESUME,0,0,0);
        UI->suspended=false;
    }
}

void systemCurrentDir(char* path, int len) { path[0] = 0; }
void systemUserDir(char* path, int len) { path[0] = 0; }

int systemUiHwInit(Pkg* system) {
    systemBleInit(system);
    systemLocationInit(system);
    return 0;
}
