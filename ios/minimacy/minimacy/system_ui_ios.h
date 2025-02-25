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
