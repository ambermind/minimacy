// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

#ifdef WITH_GL
#define GL_GLEXT_PROTOTYPES
#ifdef USE_COCOA
#import <OpenGL/gl.h>
#import <OpenGL/glext.h>
#endif

#ifdef USE_X11
#include <GL/gl.h>
#include <GL/glx.h>
#endif

#ifdef ON_IOS
#import <OpenGLES/ES3/gl.h>
#import <OpenGLES/ES3/glext.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#endif

#ifdef ON_ANDROID
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#define GL_BGRA	GL_RGBA
#define USE_RGBA_MISSING_BGRA
#endif

#ifdef ON_WINDOWS
#include<GL/gl.h>
#include<GL/glu.h>
#endif

extern int GLinstance;
extern int GLready;

typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	GLuint shader;
	LINT instance;
}lglShader;

typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	GLuint program;
	LINT instance;
}lglProgram;

typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	lchar* data0;
	GLuint Texture;
	LINT instance;
}lglTexture;

#ifdef ON_WINDOWS
#define APIENTRYP APIENTRY *

#define GL_BGRA                           0x80E1
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_TEXTURE0                       0x84C0

#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_SHADING_LANGUAGE_VERSION       0x8B8C
#define GL_ARRAY_BUFFER                   0x8892
#define GL_STATIC_DRAW                    0x88E4

#define GL_FRAMEBUFFER                    0x8D40


typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;

typedef GLuint(APIENTRYP FUN_ENUM_UINT) (GLenum);
typedef GLuint(APIENTRYP FUN_UINT) ();
typedef void (APIENTRYP FUN_UINT_VOID) (GLuint);
typedef void (APIENTRYP FUN_UINT_UINT_VOID) (GLuint, GLuint);
typedef void (APIENTRYP FUN_UINT_SIZEI_PPCHAR_PINT_VOID) (GLuint, GLsizei, const GLchar**, const GLint*);
typedef void (APIENTRYP FUN_UINT_UINT_PCHAR_VOID) (GLuint, GLuint, const GLchar*);
typedef void (APIENTRYP FUN_UINT_INT_ENUM_BOOL_SIZEI_PVOID_VOID) (GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*);
typedef void (APIENTRYP FUN_UINT_ENUM_PINT_VOID) (GLuint, GLenum, GLint*);
typedef void (APIENTRYP FUN_UINT_SIZEI_PSIZEI_PCHAR_VOID) (GLuint, GLsizei, GLsizei*, GLchar*);

typedef GLint(APIENTRYP FUN_UINT_PCHAR_INT)(GLuint, const GLchar*);
typedef void (APIENTRYP FUN_ENUM_VOID) (GLenum);
typedef void (APIENTRYP FUN_INT_VOID) (GLint);
typedef void (APIENTRYP FUN_INT_INT_VOID) (GLint, GLint);
typedef void (APIENTRYP FUN_INT_FLOAT_VOID) (GLint, GLfloat);
typedef void (APIENTRYP FUN_INT_SIZEI_PFLOAT_VOID) (GLint, GLsizei, const GLfloat*);
typedef void (APIENTRYP FUN_INT_SIZEI_BOOL_PFLOAT_VOID) (GLint, GLsizei, GLboolean, const GLfloat*);

typedef void (APIENTRYP FUN_ENUM_UINT_VOID) (GLenum, GLuint);
typedef void (APIENTRYP FUN_SIZEI_PUINT_VOID) (GLsizei, GLuint*);
typedef void (APIENTRYP FUN_ENUM_SIZEIPTR_PVOID_ENUM_VOID)(GLenum, GLsizeiptr, const GLvoid*, GLenum);

typedef void (APIENTRYP FUN_ENUM_ENUM_SIZEI_SIZEI) (GLenum, GLenum, GLsizei, GLsizei);
typedef void (APIENTRYP FUN_ENUM_ENUM_ENUM_UINT) (GLenum, GLenum, GLenum, GLuint);
typedef void (APIENTRYP FUN_ENUM_ENUM_ENUM_UINT_INT) (GLenum, GLenum, GLenum, GLuint, GLint);

extern FUN_ENUM_UINT glCreateShader;
extern FUN_UINT glCreateProgram;
extern FUN_UINT_VOID glDeleteShader;
extern FUN_UINT_VOID glUseProgram;
extern FUN_UINT_VOID glLinkProgram;
extern FUN_UINT_VOID glDeleteProgram;
extern FUN_UINT_VOID glCompileShader;
extern FUN_UINT_UINT_VOID glAttachShader;
extern FUN_UINT_SIZEI_PPCHAR_PINT_VOID glShaderSource;
extern FUN_UINT_VOID glEnableVertexAttribArray;
extern FUN_UINT_INT_ENUM_BOOL_SIZEI_PVOID_VOID glVertexAttribPointer;
extern FUN_UINT_ENUM_PINT_VOID glGetShaderiv;
extern FUN_UINT_SIZEI_PSIZEI_PCHAR_VOID glGetShaderInfoLog;
extern FUN_UINT_PCHAR_INT glGetUniformLocation;
extern FUN_UINT_PCHAR_INT glGetAttribLocation;
extern FUN_ENUM_VOID glDisableVertexAttribArray;
extern FUN_INT_VOID glActiveTexture;
extern FUN_INT_SIZEI_PFLOAT_VOID glUniform1fv;
extern FUN_INT_SIZEI_PFLOAT_VOID glUniform2fv;
extern FUN_INT_SIZEI_PFLOAT_VOID glUniform3fv;
extern FUN_INT_SIZEI_PFLOAT_VOID glUniform4fv;
extern FUN_INT_SIZEI_BOOL_PFLOAT_VOID glUniformMatrix4fv;
extern FUN_INT_SIZEI_BOOL_PFLOAT_VOID glUniformMatrix3fv;
extern FUN_INT_SIZEI_BOOL_PFLOAT_VOID glUniformMatrix2fv;

#endif

int fun_glMakeContext(Thread* th);
int fun_glRefreshContext(Thread* th);
int fun_glSwapBuffers(Thread* th);
int viewPortScale(int u);

#ifdef ON_WINDOWS
#define GLcheck if(GLready)
#define GLdefault(a,b) ((GLready)?(a):(b))
#else
#define GLcheck
#define GLdefault(a,b) (a)
#endif
#define GLNONE(name,fun)	\
int name(Thread* th) \
{	\
	GLcheck fun();	\
	FUN_RETURN_INT(0);	\
}

#define GLI(name,fun)	\
int name(Thread* th) \
{	\
	int v0=(int)STACK_INT(th,0);	\
	GLcheck fun(v0);	\
	FUN_RETURN_INT(0);	\
}

#define GLIi(name,fun)	\
int name(Thread* th) \
{	\
	int v0=(int)STACK_INT(th,0);	\
	int i=GLdefault(fun(v0),-1);	\
	FUN_RETURN_INT(i);	\
}
#define GLIs(name,fun)	\
int name(Thread* th) \
{	\
	int v0=(int)STACK_INT(th,0);	\
	char* p=(char*)(GLdefault(fun(v0),NULL));	\
	if (!p) FUN_RETURN_NIL;	\
	FUN_RETURN_STR(p,-1); \
}
#define GLII(name,fun)	\
int name(Thread* th) \
{	\
	int v1=(int)STACK_PULL_INT(th);	\
	int v0=(int)STACK_INT(th,0);	\
	GLcheck fun(v0,v1);	\
	FUN_RETURN_INT(0);	\
}

#define GLIII(name,fun)	\
int name(Thread* th) \
{	\
	int v2=(int)STACK_PULL_INT(th);	\
	int v1=(int)STACK_PULL_INT(th);	\
	int v0=(int)STACK_INT(th,0);	\
	GLcheck fun(v0,v1,v2);	\
	FUN_RETURN_INT(0);	\
}

//viewport and scissor
#define GLIIII(name,fun)	\
int name(Thread* th) \
{	\
	int v3=(int)STACK_PULL_INT(th);	\
	int v2=(int)STACK_PULL_INT(th);	\
	int v1=(int)STACK_PULL_INT(th);	\
	int v0=(int)STACK_INT(th,0);	\
	GLcheck fun(viewPortScale(v0),viewPortScale(v1),viewPortScale(v2),viewPortScale(v3));	\
	FUN_RETURN_INT(0);	\
}

#define GLIIIIIIII(name,fun)	\
int name(Thread* th) \
{	\
	int v7=(int)STACK_PULL_INT(th);	\
	int v6=(int)STACK_PULL_INT(th);	\
	int v5=(int)STACK_PULL_INT(th);	\
	int v4=(int)STACK_PULL_INT(th);	\
	int v3=(int)STACK_PULL_INT(th);	\
	int v2=(int)STACK_PULL_INT(th);	\
	int v1=(int)STACK_PULL_INT(th);	\
	int v0=(int)STACK_INT(th,0);	\
	GLcheck fun(v0,v1,v2,v3,v4,v5,v6,v7);	\
	FUN_RETURN_INT(0);	\
}

#define GLF(name,fun)	\
int name(Thread* th) \
{	\
	float v0=(float)STACK_FLOAT(th,0);	\
	GLcheck fun(v0);	\
	FUN_RETURN_INT(0);	\
}

#define GLFF(name,fun)	\
int name(Thread* th) \
{	\
	float v1=(float)STACK_PULL_FLOAT(th);	\
	float v0=(float)STACK_FLOAT(th,0);	\
	GLcheck fun(v0,v1);	\
	FUN_RETURN_INT(0);	\
}

#define GLFFF(name,fun)	\
int name(Thread* th) \
{	\
	float v2=(float)STACK_PULL_FLOAT(th);	\
	float v1=(float)STACK_PULL_FLOAT(th);	\
	float v0=(float)STACK_FLOAT(th,0);	\
	GLcheck fun(v0,v1,v2);	\
	FUN_RETURN_INT(0);	\
}
#define GLFFFF(name,fun)	\
int name(Thread* th) \
{	\
	float v3=(float)STACK_PULL_FLOAT(th);	\
	float v2=(float)STACK_PULL_FLOAT(th);	\
	float v1=(float)STACK_PULL_FLOAT(th);	\
	float v0=(float)STACK_FLOAT(th,0);	\
	GLcheck fun(v0,v1,v2,v3);	\
	FUN_RETURN_INT(0);	\
}
#define GLFFFFFF(name,fun)	\
int name(Thread* th) \
{	\
	float v5=(float)STACK_PULL_FLOAT(th);	\
	float v4=(float)STACK_PULL_FLOAT(th);	\
	float v3=(float)STACK_PULL_FLOAT(th);	\
	float v2=(float)STACK_PULL_FLOAT(th);	\
	float v1=(float)STACK_PULL_FLOAT(th);	\
	float v0=(float)STACK_FLOAT(th,0);	\
	GLcheck fun(v0,v1,v2,v3,v4,v5);	\
	FUN_RETURN_INT(0);	\
}

#define GLIF(name,fun)	\
int name(Thread* th) \
{	\
	float v1=(float)STACK_PULL_FLOAT(th);	\
	int v0=(int)STACK_INT(th,0);	\
	GLcheck fun(v0,v1);	\
	FUN_RETURN_INT(0);	\
}
#define GLIFF(name,fun)	\
int name(Thread* th) \
{	\
	float v2=(float)STACK_PULL_FLOAT(th);	\
	float v1=(float)STACK_PULL_FLOAT(th);	\
	int v0=(int)STACK_INT(th,0);	\
	GLcheck fun(v0,v1,v2);	\
	FUN_RETURN_INT(0);	\
}

#define GLIFFFF(name,fun)	\
int name(Thread* th) \
{	\
	int v0;	\
	float vec[4];	\
	vec[3]=(float)STACK_PULL_FLOAT(th);	\
	vec[2]=(float)STACK_PULL_FLOAT(th);	\
	vec[1]=(float)STACK_PULL_FLOAT(th);	\
	vec[0]=(float)STACK_PULL_FLOAT(th);	\
	v0=(int)STACK_INT(th,0);	\
	GLcheck fun(v0,vec);	\
	FUN_RETURN_INT(0);	\
}
#define GLIIF(name,fun)	\
int name(Thread* th) \
{	\
	float v2=(float)STACK_PULL_FLOAT(th);	\
	int v1=(int)STACK_PULL_INT(th);	\
	int v0=(int)STACK_INT(th,0);	\
	GLcheck fun(v0,v1,v2);	\
	FUN_RETURN_INT(0);	\
}
#define GLIIFFFF(name,fun)	\
int name(Thread* th) \
{	\
	int v0,v1;	\
	float vec[4];	\
	vec[3]=(float)STACK_PULL_FLOAT(th);	\
	vec[2]=(float)STACK_PULL_FLOAT(th);	\
	vec[1]=(float)STACK_PULL_FLOAT(th);	\
	vec[0]=(float)STACK_PULL_FLOAT(th);	\
	v1=(int)STACK_PULL_INT(th);	\
	v0=(int)STACK_INT(th,0);	\
	GLcheck fun(v0,v1,vec);	\
	FUN_RETURN_INT(0);	\
}

#define GLP(name,fun)	\
int name(Thread* th) \
{	\
	lglProgram* d=(lglProgram*)STACK_PNT(th,0);	\
	GLcheck fun(d?d->program:0);	\
	FUN_RETURN_INT(0);	\
}

#define GLPSi(name,fun)	\
int name(Thread* th) \
{	\
	LB* src=STACK_PNT(th,0);	\
	lglProgram* p=(lglProgram*)STACK_PNT(th,1);	\
	if ((!src)||(!p)) FUN_RETURN_NIL;	\
	FUN_RETURN_INT(GLdefault(fun(p->program,STR_START(src)),-1));	\
}

#define GLIIFloats(name,fun)	\
int name(Thread* th) \
{	\
	LB* floats=STACK_PNT(th,0);	\
	int count=(int)STACK_INT(th,1); \
	int location=(int)STACK_INT(th,2); \
	if (!floats) FUN_RETURN_NIL;	\
	GLcheck fun(location,count,(float*)BIN_START(floats));	\
	FUN_RETURN_INT(0); \
}
#define GLIIIFloats(name,fun)	\
int name(Thread* th) \
{	\
	LB* floats=STACK_PNT(th,0);	\
	int transpose=(int)STACK_INT(th,1); \
	int count=(int)STACK_INT(th,2); \
	int location=(int)STACK_INT(th,3); \
	if (!floats) FUN_RETURN_NIL;	\
	GLcheck fun(location,count,transpose,(float*)BIN_START(floats));	\
	FUN_RETURN_INT(0); \
}
#endif
