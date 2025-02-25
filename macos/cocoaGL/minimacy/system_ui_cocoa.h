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
