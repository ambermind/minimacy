// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#ifndef system_ui_ios_h
#define system_ui_ios_h

#import "minimacy.h"

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

#import <OpenGLES/ES3/gl.h>
#import <OpenGLES/ES3/glext.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>

#define RESIZE_IDLE 0
#define RESIZE_EVENT 1
#define RESIZE_WAIT_DONE 2


@interface SystemUIIOS : GLKView<UIKeyInput> 
{
@public bool open;
@public bool suspended;
@public int w;
@public int h;
@public int orientation;
@public float scale;
@public int keyboardHeight;
@public LFLOAT accX;
@public LFLOAT accY;
@public LFLOAT accZ;
@public LINT modifiers;
@public LINT type;
@public int lastTouchCount;
}
- (id)initWithCoder:(NSCoder *)decoder;
-(void)flip:(UIInterfaceOrientation)interfaceOrientation;

@end

#endif /* system_ui_ios_h */
