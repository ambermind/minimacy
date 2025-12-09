// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#import <Cocoa/Cocoa.h>

#import <OpenGL/gl.h>
#import <OpenGL/glext.h>
#import <OpenGL/glu.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/CGLContext.h>

#import "minimacy.h"
#import "system_ui_keydef.h"
NS_ASSUME_NONNULL_BEGIN

#define RESIZE_IDLE 0
#define RESIZE_EVENT 1
#define RESIZE_WAIT_DONE 2

@interface SystemUIcocoa : NSOpenGLView<NSWindowDelegate>
{
@public bool started;
@public bool open;
@public int x;
@public int y;
@public int w;
@public int h;
@public float scale;
@public LINT modifiers;
@public LINT type;
@public int resizeState;
@public NSString *title;
@public CGRect shape;   // contains the requested coordinates (y=0 at the top)
@public NSCursor *cursor;
@public NSArray *droppedFiles;
    int lastTouchCount;
}
- (id)initWithCoder:(NSCoder *)decoder;

@end

NS_ASSUME_NONNULL_END
