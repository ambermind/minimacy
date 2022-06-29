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

#ifdef WITH_GLES2
#ifdef ON_UNIX

#ifdef USE_COCOA
#import <OpenGL/gl.h>
#import <OpenGL/glext.h>
#define HWND int
#define Display int
#define HGLRC int
#endif

#ifdef USE_X11
#include <GL/gl.h>
#include <GL/glx.h>
#define HGLRC GLXContext
#endif

#ifdef USE_IOS
#define USE_GLES
#import <OpenGLES/ES3/gl.h>
#import <OpenGLES/ES3/glext.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#define HWND int
#define Display int
#define HGLRC int
#endif

#endif

#ifdef ON_WINDOWS
#include<GL/gl.h>
#include<GL/glu.h>
#include"libs/glext.h"
#endif

typedef struct
{
	HWND win;
#ifdef ON_UNIX
	Display* display;
#endif
#ifdef ON_WINDOWS
	HDC hDCgl;
#endif
	HGLRC hRCgl;
	LINT instance;
}GLstruct;
GLstruct GL;

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

#endif

#define GLNONE(name,fun)	\
int name(Thread* th) \
{	\
	if (fun) fun();	\
	return STACKPUSH(th,INTTOVAL(0));	\
}

#define GLI(name,fun)	\
int name(Thread* th) \
{	\
	int v0=(int)VALTOINT(STACKPULL(th));	\
	if (fun) fun(v0);	\
	return STACKPUSH(th,INTTOVAL(0));	\
}

#define GLIi(name,fun)	\
int name(Thread* th) \
{	\
	int v0=(int)VALTOINT(STACKPULL(th));	\
	int i=fun?fun(v0):-1;	\
	STACKPUSH(th,INTTOVAL(i));	\
	return 0;	\
}
#define GLIs(name,fun)	\
int name(Thread* th) \
{	\
	int v0=(int)VALTOINT(STACKPULL(th));	\
	char* p=(char*)(fun?fun(v0):NULL);	\
	if (p) return stackPushStr(th,p,-1); \
	return STACKPUSH(th,NIL);	\
}
#define GLII(name,fun)	\
int name(Thread* th) \
{	\
	int v1=(int)VALTOINT(STACKPULL(th));	\
	int v0=(int)VALTOINT(STACKPULL(th));	\
	if (fun) fun(v0,v1);	\
	return STACKPUSH(th,INTTOVAL(0));	\
}

#define GLIII(name,fun)	\
int name(Thread* th) \
{	\
	int v2=(int)VALTOINT(STACKPULL(th));	\
	int v1=(int)VALTOINT(STACKPULL(th));	\
	int v0=(int)VALTOINT(STACKPULL(th));	\
	if (fun) fun(v0,v1,v2);	\
	return STACKPUSH(th,INTTOVAL(0));	\
}

//viewport and scissor
#define GLIIII(name,fun)	\
int name(Thread* th) \
{	\
	int v3=(int)VALTOINT(STACKPULL(th));	\
	int v2=(int)VALTOINT(STACKPULL(th));	\
	int v1=(int)VALTOINT(STACKPULL(th));	\
	int v0=(int)VALTOINT(STACKPULL(th));	\
	int k=viewPortScale(); \
	if (fun) fun(k*v0,k*v1,k*v2,k*v3);	\
	return STACKPUSH(th,INTTOVAL(0));	\
}

#define GLIIIIIIII(name,fun)	\
int name(Thread* th) \
{	\
	int v7=(int)VALTOINT(STACKPULL(th));	\
	int v6=(int)VALTOINT(STACKPULL(th));	\
	int v5=(int)VALTOINT(STACKPULL(th));	\
	int v4=(int)VALTOINT(STACKPULL(th));	\
	int v3=(int)VALTOINT(STACKPULL(th));	\
	int v2=(int)VALTOINT(STACKPULL(th));	\
	int v1=(int)VALTOINT(STACKPULL(th));	\
	int v0=(int)VALTOINT(STACKPULL(th));	\
	if (fun) fun(v0,v1,v2,v3,v4,v5,v6,v7);	\
	return STACKPUSH(th,INTTOVAL(0));	\
}

#define GLF(name,fun)	\
int name(Thread* th) \
{	\
	LW w0=STACKPULL(th);	\
	float v0=(float)VALTOFLOAT(w0);	\
	if (fun) fun(v0);	\
	return STACKPUSH(th,INTTOVAL(0));	\
}

#define GLFF(name,fun)	\
int name(Thread* th) \
{	\
	LW w1=STACKPULL(th);	\
	LW w0=STACKPULL(th);	\
	float v1=(float)VALTOFLOAT(w1);	\
	float v0=(float)VALTOFLOAT(w0);	\
	if (fun) fun(v0,v1);	\
	return STACKPUSH(th,INTTOVAL(0));	\
}

#define GLFFF(name,fun)	\
int name(Thread* th) \
{	\
	LW w2=STACKPULL(th);	\
	LW w1=STACKPULL(th);	\
	LW w0=STACKPULL(th);	\
	float v2=(float)VALTOFLOAT(w2);	\
	float v1=(float)VALTOFLOAT(w1);	\
	float v0=(float)VALTOFLOAT(w0);	\
	if (fun) fun(v0,v1,v2);	\
	return STACKPUSH(th,INTTOVAL(0));	\
}
#define GLFFFF(name,fun)	\
int name(Thread* th) \
{	\
	LW w3=STACKPULL(th);	\
	LW w2=STACKPULL(th);	\
	LW w1=STACKPULL(th);	\
	LW w0=STACKPULL(th);	\
	float v3=(float)VALTOFLOAT(w3);	\
	float v2=(float)VALTOFLOAT(w2);	\
	float v1=(float)VALTOFLOAT(w1);	\
	float v0=(float)VALTOFLOAT(w0);	\
	if (fun) fun(v0,v1,v2,v3);	\
	return STACKPUSH(th,INTTOVAL(0));	\
}
#define GLFFFFFF(name,fun)	\
int name(Thread* th) \
{	\
	LW w5=STACKPULL(th);	\
	LW w4=STACKPULL(th);	\
	LW w3=STACKPULL(th);	\
	LW w2=STACKPULL(th);	\
	LW w1=STACKPULL(th);	\
	LW w0=STACKPULL(th);	\
	float v5=(float)VALTOFLOAT(w5);	\
	float v4=(float)VALTOFLOAT(w4);	\
	float v3=(float)VALTOFLOAT(w3);	\
	float v2=(float)VALTOFLOAT(w2);	\
	float v1=(float)VALTOFLOAT(w1);	\
	float v0=(float)VALTOFLOAT(w0);	\
	if (fun) fun(v0,v1,v2,v3,v4,v5);	\
	return STACKPUSH(th,INTTOVAL(0));	\
}

#define GLIF(name,fun)	\
int name(Thread* th) \
{	\
	LW w1=STACKPULL(th);	\
	float v1=(float)VALTOFLOAT(w1);	\
	int v0=(int)VALTOINT(STACKPULL(th));	\
	if (fun) fun(v0,v1);	\
	return STACKPUSH(th,INTTOVAL(0));	\
}
#define GLIFF(name,fun)	\
int name(Thread* th) \
{	\
	LW w2=STACKPULL(th);	\
	LW w1=STACKPULL(th);	\
	float v2=(float)VALTOFLOAT(w2);	\
	float v1=(float)VALTOFLOAT(w1);	\
	int v0=(int)VALTOINT(STACKPULL(th));	\
	if (fun) fun(v0,v1,v2);	\
	return STACKPUSH(th,INTTOVAL(0));	\
}

#define GLIFFFF(name,fun)	\
int name(Thread* th) \
{	\
	float vec[4];	\
	LW w3=STACKPULL(th);	\
	LW w2=STACKPULL(th);	\
	LW w1=STACKPULL(th);	\
	LW w0=STACKPULL(th);	\
	int v0=(int)VALTOINT(STACKPULL(th));	\
	vec[3]=(float)VALTOFLOAT(w3);	\
	vec[2]=(float)VALTOFLOAT(w2);	\
	vec[1]=(float)VALTOFLOAT(w1);	\
	vec[0]=(float)VALTOFLOAT(w0);	\
	if (fun) fun(v0,vec);	\
	return STACKPUSH(th,INTTOVAL(0));	\
}
#define GLIIF(name,fun)	\
int name(Thread* th) \
{	\
	LW w2=STACKPULL(th);	\
	float v2=(float)VALTOFLOAT(w2);	\
	int v1=(int)VALTOINT(STACKPULL(th));	\
	int v0=(int)VALTOINT(STACKPULL(th));	\
	if (fun) fun(v0,v1,v2);	\
	return STACKPUSH(th,INTTOVAL(0));	\
}
#define GLIIFFFF(name,fun)	\
int name(Thread* th) \
{	\
	float vec[4];	\
	LW w3=STACKPULL(th);	\
	LW w2=STACKPULL(th);	\
	LW w1=STACKPULL(th);	\
	LW w0=STACKPULL(th);	\
	int v1=(int)VALTOINT(STACKPULL(th));	\
	int v0=(int)VALTOINT(STACKPULL(th));	\
	vec[3]=(float)VALTOFLOAT(w3);	\
	vec[2]=(float)VALTOFLOAT(w2);	\
	vec[1]=(float)VALTOFLOAT(w1);	\
	vec[0]=(float)VALTOFLOAT(w0);	\
	if (fun) fun(v0,v1,vec);	\
	return STACKPUSH(th,INTTOVAL(0));	\
}

#define GLP(name,fun)	\
int name(Thread* th) \
{	\
	lglProgram* d=(lglProgram*)VALTOPNT(STACKPULL(th));	\
	if (fun) fun(d?d->program:0);	\
	STACKPUSH(th,INTTOVAL(0));	\
	return 0;	\
}

#define GLPSi(name,fun)	\
int name(Thread* th) \
{	\
	int NDROP=2-1;	\
	LW result=NIL;	\
	LB* src=VALTOPNT(STACKGET(th,0));	\
	lglProgram* p=(lglProgram*)VALTOPNT(STACKGET(th,1));	\
	if ((!src)||(!p)) goto cleanup;	\
	result=INTTOVAL(fun?fun(p->program,STRSTART(src)):-1);	\
cleanup:	\
	STACKSET(th,NDROP,result);	\
	STACKDROPN(th,NDROP);	\
	return 0;	\
}

#define GLIIFloats(name,fun)	\
int name(Thread* th) \
{	\
	int NDROP=3-1;	\
	LW result=NIL;	\
	LB* floats=VALTOPNT(STACKGET(th,0));	\
	int count=(int)VALTOINT(STACKGET(th,1)); \
	int location=(int)VALTOINT(STACKGET(th,2)); \
	if (!floats) goto cleanup;	\
	if (fun) fun(location,count,(float*)BINSTART(floats));	\
	result=INTTOVAL(0); \
cleanup:	\
	STACKSET(th,NDROP,result);	\
	STACKDROPN(th,NDROP);	\
	return 0;	\
}
#define GLIIIFloats(name,fun)	\
int name(Thread* th) \
{	\
	int NDROP=4-1;	\
	LW result=NIL;	\
	LB* floats=VALTOPNT(STACKGET(th,0));	\
	int transpose=(int)VALTOINT(STACKGET(th,1)); \
	int count=(int)VALTOINT(STACKGET(th,2)); \
	int location=(int)VALTOINT(STACKGET(th,3)); \
	if (!floats) goto cleanup;	\
	if (fun) fun(location,count,transpose,(float*)BINSTART(floats));	\
	result=INTTOVAL(0); \
cleanup:	\
	STACKSET(th,NDROP,result);	\
	STACKDROPN(th,NDROP);	\
	return 0;	\
}
#endif
