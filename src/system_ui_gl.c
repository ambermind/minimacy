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
#include"system_ui_gl.h"

#ifdef ON_WINDOWS
FUN_ENUM_UINT glCreateShader = NULL;
FUN_UINT glCreateProgram = NULL;
FUN_UINT_VOID glDeleteShader = NULL;
FUN_UINT_VOID glUseProgram = NULL;
FUN_UINT_VOID glLinkProgram = NULL;
FUN_UINT_VOID glDeleteProgram = NULL;
FUN_UINT_VOID glCompileShader = NULL;
FUN_UINT_UINT_VOID glAttachShader = NULL;
FUN_UINT_SIZEI_PPCHAR_PINT_VOID glShaderSource = NULL;
FUN_UINT_VOID glEnableVertexAttribArray = NULL;
FUN_UINT_UINT_PCHAR_VOID glBindAttribLocation = NULL;
FUN_UINT_INT_ENUM_BOOL_SIZEI_PVOID_VOID glVertexAttribPointer = NULL;
FUN_UINT_ENUM_PINT_VOID glGetShaderiv = NULL;
FUN_UINT_SIZEI_PSIZEI_PCHAR_VOID glGetShaderInfoLog = NULL;
FUN_UINT_ENUM_PINT_VOID glGetProgramiv = NULL;
FUN_UINT_SIZEI_PSIZEI_PCHAR_VOID glGetProgramInfoLog = NULL;

FUN_UINT_PCHAR_INT glGetUniformLocation = NULL;
FUN_UINT_PCHAR_INT glGetAttribLocation = NULL;
FUN_ENUM_VOID glDisableVertexAttribArray = NULL;
FUN_INT_VOID glActiveTexture = NULL;
FUN_INT_INT_VOID glUniform1i = NULL;
FUN_INT_SIZEI_PFLOAT_VOID glUniform1fv = NULL;
FUN_INT_SIZEI_PFLOAT_VOID glUniform2fv = NULL;
FUN_INT_SIZEI_PFLOAT_VOID glUniform3fv = NULL;
FUN_INT_SIZEI_PFLOAT_VOID glUniform4fv = NULL;
FUN_INT_SIZEI_BOOL_PFLOAT_VOID glUniformMatrix4fv = NULL;
FUN_INT_SIZEI_BOOL_PFLOAT_VOID glUniformMatrix3fv = NULL;
FUN_INT_SIZEI_BOOL_PFLOAT_VOID glUniformMatrix2fv = NULL;
FUN_ENUM_UINT_VOID glBindBuffer = NULL;
FUN_SIZEI_PUINT_VOID glGenBuffers = NULL;
FUN_ENUM_SIZEIPTR_PVOID_ENUM_VOID glBufferData = NULL;
FUN_SIZEI_PUINT_VOID glDeleteBuffers = NULL;

FUN_SIZEI_PUINT_VOID glGenFramebuffers = NULL;
FUN_SIZEI_PUINT_VOID glGenRenderbuffers = NULL;
FUN_ENUM_UINT_VOID glBindFramebuffer = NULL;
FUN_ENUM_UINT_VOID glBindRenderbuffer = NULL;
FUN_SIZEI_PUINT_VOID glDeleteFramebuffers = NULL;
FUN_SIZEI_PUINT_VOID glDeleteRenderbuffers = NULL;

FUN_ENUM_ENUM_SIZEI_SIZEI glRenderbufferStorage = NULL;
FUN_ENUM_ENUM_ENUM_UINT glFramebufferRenderbuffer = NULL;
FUN_ENUM_ENUM_ENUM_UINT_INT glFramebufferTexture2D = NULL;

void* myWglGetProcAddress(char* name)
{
	void* ad = wglGetProcAddress(name);
//	printf("wglGetProcAddress %s -> %llx", name, ad);
	if (!ad) PRINTF(MM.scheduler, LOG_ERR, "cannot locate %s\n", name);
	return ad;
}

void _GLES2Link()
{
//	printf("_GLES2Link\n");
//	if (glCreateShader) return;
	glCreateShader = (FUN_ENUM_UINT)myWglGetProcAddress("glCreateShader");
	glCreateProgram = (FUN_UINT)myWglGetProcAddress("glCreateProgram");
	glDeleteShader = (FUN_UINT_VOID)myWglGetProcAddress("glDeleteShader");
	glUseProgram = (FUN_UINT_VOID)myWglGetProcAddress("glUseProgram");
	glLinkProgram = (FUN_UINT_VOID)myWglGetProcAddress("glLinkProgram");
	glDeleteProgram = (FUN_UINT_VOID)myWglGetProcAddress("glDeleteProgram");
	glCompileShader = (FUN_UINT_VOID)myWglGetProcAddress("glCompileShader");
	glAttachShader = (FUN_UINT_UINT_VOID)myWglGetProcAddress("glAttachShader");
	glShaderSource = (FUN_UINT_SIZEI_PPCHAR_PINT_VOID)myWglGetProcAddress("glShaderSource");
	glEnableVertexAttribArray = (FUN_UINT_VOID)myWglGetProcAddress("glEnableVertexAttribArray");
	glBindAttribLocation = (FUN_UINT_UINT_PCHAR_VOID)myWglGetProcAddress("glBindAttribLocation");	// unused
	glVertexAttribPointer = (FUN_UINT_INT_ENUM_BOOL_SIZEI_PVOID_VOID)myWglGetProcAddress("glVertexAttribPointer");
	glGetShaderiv = (FUN_UINT_ENUM_PINT_VOID)myWglGetProcAddress("glGetShaderiv");
	glGetShaderInfoLog = (FUN_UINT_SIZEI_PSIZEI_PCHAR_VOID)myWglGetProcAddress("glGetShaderInfoLog");
	glGetProgramiv = (FUN_UINT_ENUM_PINT_VOID)myWglGetProcAddress("glGetProgramiv");	// unused
	glGetProgramInfoLog = (FUN_UINT_SIZEI_PSIZEI_PCHAR_VOID)myWglGetProcAddress("glGetProgramInfoLog");	// unused

	glGetUniformLocation = (FUN_UINT_PCHAR_INT)myWglGetProcAddress("glGetUniformLocation");
	glGetAttribLocation = (FUN_UINT_PCHAR_INT)myWglGetProcAddress("glGetAttribLocation");
	glDisableVertexAttribArray = (FUN_ENUM_VOID)myWglGetProcAddress("glDisableVertexAttribArray");
	glActiveTexture = (FUN_INT_VOID)myWglGetProcAddress("glActiveTexture");
	glUniform1i = (FUN_INT_INT_VOID)myWglGetProcAddress("glUniform1i");
	glUniform1fv = (FUN_INT_SIZEI_PFLOAT_VOID)myWglGetProcAddress("glUniform1fv");
	glUniform2fv = (FUN_INT_SIZEI_PFLOAT_VOID)myWglGetProcAddress("glUniform2fv");
	glUniform3fv = (FUN_INT_SIZEI_PFLOAT_VOID)myWglGetProcAddress("glUniform3fv");
	glUniform4fv = (FUN_INT_SIZEI_PFLOAT_VOID)myWglGetProcAddress("glUniform4fv");
	glUniformMatrix4fv = (FUN_INT_SIZEI_BOOL_PFLOAT_VOID)myWglGetProcAddress("glUniformMatrix4fv");
	glUniformMatrix3fv = (FUN_INT_SIZEI_BOOL_PFLOAT_VOID)myWglGetProcAddress("glUniformMatrix3fv");
	glUniformMatrix2fv = (FUN_INT_SIZEI_BOOL_PFLOAT_VOID)myWglGetProcAddress("glUniformMatrix2fv");
	glBindBuffer = (FUN_ENUM_UINT_VOID)myWglGetProcAddress("glBindBuffer");
	glGenBuffers = (FUN_SIZEI_PUINT_VOID)myWglGetProcAddress("glGenBuffers");
	glBufferData = (FUN_ENUM_SIZEIPTR_PVOID_ENUM_VOID)myWglGetProcAddress("glBufferData");
	glDeleteBuffers = (FUN_SIZEI_PUINT_VOID)myWglGetProcAddress("glDeleteBuffers");

	glGenFramebuffers = (FUN_SIZEI_PUINT_VOID)myWglGetProcAddress("glGenFramebuffers");
	glGenRenderbuffers = (FUN_SIZEI_PUINT_VOID)myWglGetProcAddress("glGenRenderbuffers");
	glBindFramebuffer = (FUN_ENUM_UINT_VOID)myWglGetProcAddress("glBindFramebuffer");
	glBindRenderbuffer = (FUN_ENUM_UINT_VOID)myWglGetProcAddress("glBindRenderbuffer");
	glDeleteFramebuffers = (FUN_SIZEI_PUINT_VOID)myWglGetProcAddress("glDeleteFramebuffers");
	glDeleteRenderbuffers = (FUN_SIZEI_PUINT_VOID)myWglGetProcAddress("glDeleteRenderbuffers");

	glRenderbufferStorage = (FUN_ENUM_ENUM_SIZEI_SIZEI)myWglGetProcAddress("glRenderbufferStorage");
	glFramebufferRenderbuffer = (FUN_ENUM_ENUM_ENUM_UINT)myWglGetProcAddress("glFramebufferRenderbuffer");
	glFramebufferTexture2D = (FUN_ENUM_ENUM_ENUM_UINT_INT)myWglGetProcAddress("glFramebufferTexture2D");
}

void _windowInitGL(HWND win)
{
	GL.win = win;
	GL.hDCgl = NULL;
	GL.hRCgl = NULL;
}

void _windowReleaseGL()
{
	if (GL.hRCgl)
	{
//		printf("_windowReleaseGL\n");
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(GL.hRCgl);
		ReleaseDC(GL.win, GL.hDCgl);
		GL.hDCgl = NULL;
		GL.hRCgl = NULL;
	}
	GL.win=NULL;
}

void _glMakeGLcontextInitGL()
{
	PIXELFORMATDESCRIPTOR pfd;
	int format;
	GL.hDCgl = NULL;
	GL.hRCgl = NULL;
//	printf("_windowInitGL %x\n",GL.win);
	// get the device context (DC)
	GL.instance++;
	GL.hDCgl = GetDC(GL.win);

	// set the pixel format for the DC
	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 32;
	pfd.iLayerType = PFD_MAIN_PLANE;
	format = ChoosePixelFormat(GL.hDCgl, &pfd);
	SetPixelFormat(GL.hDCgl, format, &pfd);
	// create and enable the render context (RC)
	GL.hRCgl = wglCreateContext(GL.hDCgl);
	wglMakeCurrent(GL.hDCgl, GL.hRCgl);
	_GLES2Link();	// we need a rendering context before this call
}

int fun_glMakeContext(Thread* th)
{
//	printf("===============fun_glMakeContext\n");

	if (!GL.win) return STACKPUSH(th, NIL);
	_glMakeGLcontextInitGL();
	return STACKPUSH(th, INTTOVAL(0));
}
int fun_glSwapBuffers(Thread* th)
{
	if (!GL.hRCgl) return STACKPUSH(th, NIL);
//	printf("SwapBuffers\n");
	SwapBuffers(GL.hDCgl);
	return STACKPUSH(th,INTTOVAL(0));
}
int viewPortScale() { return 1; }

#endif
#ifdef USE_COCOA
int fun_glMakeContext(Thread* th)
{
    // context is already made on COCOA!
    GL.instance++;
    return STACKPUSH(th, INTTOVAL(0));
}
int fun_glSwapBuffers(Thread* th);
int viewPortScale(void);

#endif

#ifdef USE_IOS
int fun_glMakeContext(Thread* th)
{
    // context is already made on COCOA!
    GL.instance++;
    return STACKPUSH(th, INTTOVAL(0));
}
int fun_glSwapBuffers(Thread* th);
int viewPortScale(void);

#endif

#ifdef USE_X11
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

void _windowInitGL(HWND win)
{
	GL.win = win;
	GL.display=NULL;
	GL.hRCgl = NULL;
}

void _windowReleaseGL()
{
	if (GL.hRCgl) glXDestroyContext(GL.display,GL.hRCgl);
	if (GL.display) XCloseDisplay(GL.display);
	GL.win= 0;
	GL.display = NULL;
	GL.hRCgl = NULL;
}

void _glMakeGLcontextInitGL()
{
	GLXContext context;
	Display* d=GL.display=XOpenDisplay(NULL);	// we need a second connection to run opengl in a thread different from the Window thread
	XVisualInfo* vInfo=glXChooseVisual(d,DefaultScreen(d),testAttributesA);
	if (!vInfo) vInfo=glXChooseVisual(d,DefaultScreen(d),testAttributesB);
	if (!vInfo) vInfo=glXChooseVisual(d,DefaultScreen(d),testAttributesC);
	context=glXCreateContext(d,vInfo,NULL,True);
	glXMakeCurrent(d,GL.win,context);
	GL.hRCgl=context;
	GL.instance++;
}
int fun_glMakeContext(Thread* th)
{	printf("===============fun_glMakeContext\n");

	if (!GL.win) return STACKPUSH(th, NIL);
	_glMakeGLcontextInitGL();
	return STACKPUSH(th, INTTOVAL(0));
}
int fun_glSwapBuffers(Thread* th)
{
	if (!GL.hRCgl) return STACKPUSH(th, NIL);
	glXSwapBuffers(GL.display,GL.win);
	return STACKPUSH(th,INTTOVAL(0));
}
int viewPortScale() { return 1; }

#endif

int fun_glES(Thread* th)
{
#ifdef USE_GLES
    return STACKPUSH(th,MM.trueRef);
#else
    return STACKPUSH(th,MM.falseRef);
#endif
}


//---------------- shaders
// the following function is not used
// it prevents the user to keep a link to it after attaching to a program
// it will be released with the graphic context
int _shaderForget(LB* p)
{
	if (glDeleteShader) glDeleteShader(((lglShader*)p)->shader);
	return 0;
}
int fun_glCreateShader(Thread* th)
{
	LINT type = VALTOINT(STACKPULL(th));
	lglShader* d = (lglShader*)memoryAllocExt(th, sizeof(lglShader), DBG_BIN, NULL, NULL); if (!d) return EXEC_OM;
	if (!glCreateShader) return STACKPUSH(th, NIL); 
	d->shader= glCreateShader((GLenum)type);
	d->instance = GL.instance;
	return STACKPUSH(th, PNTTOVAL(d));
}

int fun_glShaderSource(Thread* th)
{
	char* p;
	int NDROP = 2 - 1;
	LW result = NIL;

	LB* src = VALTOPNT(STACKGET(th, 0));
	lglShader* s = (lglShader*)VALTOPNT(STACKGET(th, 1));
	if ((!src) || (!s)) goto cleanup;
	p = STRSTART(src);
	if (glShaderSource)
	{
		glShaderSource(s->shader, 1, (const GLchar**)&p, NULL);
		result = INTTOVAL(0);
	}
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}
int fun_glCompileShader(Thread* th)
{
	GLint compiled;
	int NDROP = 1 - 1;
	LW result = NIL;

	lglShader* s = (lglShader*)VALTOPNT(STACKGET(th, 0));
	if (!s) goto cleanup;
	if (!glCompileShader) goto cleanup;
	glCompileShader(s->shader);
	glGetShaderiv(s->shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		GLint infoLen = 0;
		glGetShaderiv(s->shader, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1)
		{
			char* infoLog = (char*)malloc(sizeof(char) * infoLen);
			glGetShaderInfoLog(s->shader, infoLen, NULL, infoLog);
			PRINTF(th, LOG_ERR, "Error compiling shader:\n%s\n", infoLog);
			free(infoLog);
		}
		result = MM.falseRef;
	}
	else result = MM.trueRef;
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}

//---------------- programs
int _programForget(LB* p)
{
	lglProgram* d = (lglProgram*)p;
	if ((d->instance == GL.instance)&&glDeleteProgram) glDeleteProgram(d->program);
	return 0;
}
int fun_glCreateProgram(Thread* th)
{
	lglProgram* d = (lglProgram*)memoryAllocExt(th, sizeof(lglProgram), DBG_BIN, _programForget, NULL); if (!d) return EXEC_OM;
	if (!glCreateProgram)  return STACKPUSH(th, NIL);
	d->program = glCreateProgram();
	d->instance = GL.instance;
	return STACKPUSH(th, PNTTOVAL(d));
}

int fun_glAttachShader(Thread* th)
{
	int NDROP = 2 - 1;
	LW result = NIL;

	lglShader* s = (lglShader*)VALTOPNT(STACKGET(th, 0));
	lglProgram* p = (lglProgram*)VALTOPNT(STACKGET(th, 1));
	if ((!s) || (!p)) goto cleanup;
	if (!glAttachShader) goto cleanup;
	glAttachShader(p->program, s->shader);
	result = INTTOVAL(0);
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}

//---------------- textures
int _textureForget(LB* p)
{
	lglTexture* d = (lglTexture*)p;
	if ((d->instance == GL.instance) && glDeleteTextures) glDeleteTextures(1, &d->Texture);
	return 0;
}
int _glTextureTexImage2D(lglTexture* t, int vtarget, int vlevel, int vformat, int vborder, int w, int h, lchar* src, int nextline)
{
	if (glTexImage2D) glTexImage2D(vtarget, vlevel, vformat, w, h, vborder, GL_BGRA, GL_UNSIGNED_BYTE, src);
	return 0;
}

int fun_glCreateTexture(Thread* th)
{
	lglTexture* d = (lglTexture*)memoryAllocExt(th, sizeof(lglTexture), DBG_BIN, _textureForget, NULL); if (!d) return EXEC_OM;
	if (!glCreateShader) return STACKPUSH(th, NIL);
	d->data0 = NULL;
	glGenTextures(1, &d->Texture);
	d->instance = GL.instance;
	return STACKPUSH(th, PNTTOVAL(d));
}

int fun_glBindTexture(Thread* th)
{
	int NDROP = 2 - 1;
	LW result = INTTOVAL(-1);

	lglTexture* d = (lglTexture*)VALTOPNT(STACKGET(th, 0));
	LINT v0 = VALTOINT(STACKGET(th, 1));
	if (glBindTexture==NULL) goto cleanup;
	glBindTexture((int)v0, d ? (d->Texture) : 0);
	result = INTTOVAL(0);
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}
int fun_glTexImage2D(Thread* th)
{
	int NDROP = 6 - 1;
	LW result = INTTOVAL(-1);

	LBitmap* bmp = (LBitmap*)VALTOPNT(STACKGET(th, 0));
	int vborder = (int)VALTOINT(STACKGET(th, 1));
	int vformat = (int)VALTOINT(STACKGET(th, 2));
	int vlevel = (int)VALTOINT(STACKGET(th, 3));
	int vtarget = (int)VALTOINT(STACKGET(th, 4));
	lglTexture* d = (lglTexture*)VALTOPNT(STACKGET(th, 5));
	if ((!d) || (!bmp)) goto cleanup;
	if (glBindTexture==NULL) goto cleanup;
	glBindTexture(vtarget, d->Texture);
	_glTextureTexImage2D(d, vtarget, vlevel, vformat, vborder, (int)bmp->w, (int)bmp->h, bmp->start8, (int)bmp->next8);
	result = INTTOVAL(0);
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}


//---------------- floats

int fun_floatsFromArray(Thread* th)
{
	LINT n;
	LB* result;
	LB* src = VALTOPNT(STACKGET(th,0));
	if (!src) return 0;
	n = TABLEN(src);
	result = memoryAllocBin(th, NULL, sizeof(float) * n, DBG_BIN);
	if (result)
	{
		LINT i;
		float* p = (float*)BINSTART(result);
		for (i = 0; i < n; i++) p[i] = (float)VALTOFLOAT(TABGET(src, i));
	}
	STACKSET(th,0, PNTTOVAL(result));
	return 0;
}

int fun_floatsLength(Thread* th)
{
	LB* floats = VALTOPNT(STACKGET(th, 0));
	if (floats) STACKSET(th, 0, INTTOVAL(BINLEN(floats)/sizeof(float)));
	return 0;
}
int fun_floatsGet(Thread* th)
{
	LINT i=VALTOINT(STACKPULL(th));
	LB* floats = VALTOPNT(STACKGET(th, 0));
	if (floats)
	{
		LFLOAT f;
		LW result= NIL;
		LINT n = BINLEN(floats) / sizeof(float);
		float* p = (float*)BINSTART(floats);
		if ((i >= 0) && (i < n))
		{
			f = p[i];
			result = FLOATTOVAL(f);
		}
		STACKSET(th, 0, result);
	}
	return 0;
}
int fun_glVertexAttribPointer(Thread* th)
{
	float* p;
	int NDROP = 6 - 1;
	LW result = NIL;

	int offset = (int)VALTOINT(STACKGET(th, 0));
	LB* floats = VALTOPNT(STACKGET(th, 1));
	int stride = (int)VALTOINT(STACKGET(th, 2));
	int normalized = (int)VALTOINT(STACKGET(th, 3));
	int size = (int)VALTOINT(STACKGET(th, 4));
	int index = (int)VALTOINT(STACKGET(th, 5));
	p = floats ? (float*)(BINSTART(floats)) : NULL;
	if (!glVertexAttribPointer) goto cleanup;

	glVertexAttribPointer(index, size, GL_FLOAT, normalized, stride * sizeof(float), p + offset);
	result = INTTOVAL(0);
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}

GLNONE(fun_glFlush, glFlush)
GLI(fun_glClear, glClear)
GLI(fun_glEnable, glEnable)
GLI(fun_glDisable, glDisable)
GLI(fun_glCullFace, glCullFace)
GLI(fun_glDepthFunc, glDepthFunc)
GLI(fun_glDepthMask, glDepthMask)
GLII(fun_glBlendFunc, glBlendFunc)
GLII(fun_glHint, glHint)
GLIII(fun_glTexParameteri, glTexParameteri)
GLIII(fun_glDrawArrays, glDrawArrays)
GLIIII(fun_glViewport, glViewport)
GLIIII(fun_glScissor, glScissor)
GLIIIIIIII(fun_glCopyTexImage2D, glCopyTexImage2D)
GLIIIIIIII(fun_glCopyTexSubImage2D, glCopyTexSubImage2D)
GLF(fun_glLineWidth, glLineWidth)
GLFFFF(fun_glClearColor, glClearColor)
GLIIF(fun_glTexParameterf, glTexParameterf)
GLI(fun_glEnableVertexAttribArray, glEnableVertexAttribArray)
GLI(fun_glActiveTexture, glActiveTexture)
GLI(fun_glDisableVertexAttribArray, glDisableVertexAttribArray)
GLI(fun_glFrontFace, glFrontFace)
GLP(fun_glUseProgram, glUseProgram)
GLP(fun_glLinkProgram, glLinkProgram)
GLIs(fun_glGetString, glGetString)
GLPSi(fun_glGetUniformLocation, glGetUniformLocation)
GLPSi(fun_glGetAttribLocation, glGetAttribLocation)
GLII(fun_glUniform1i, glUniform1i)
GLIIFloats(fun_glUniform1fv, glUniform1fv)
GLIIFloats(fun_glUniform2fv, glUniform2fv)
GLIIFloats(fun_glUniform3fv, glUniform3fv)
GLIIFloats(fun_glUniform4fv, glUniform4fv)
GLIIIFloats(fun_glUniformMatrix2fv, glUniformMatrix2fv)
GLIIIFloats(fun_glUniformMatrix3fv, glUniformMatrix3fv)
GLIIIFloats(fun_glUniformMatrix4fv, glUniformMatrix4fv)

int coreUiGLES2Init(Thread* th, Pkg* system)
{
	Ref* floats = pkgAddType(th, system, "Floats");
	Ref* shader = pkgAddType(th, system, "GlShader");
	Ref* program = pkgAddType(th, system, "GlProgram");
	Ref* texture = pkgAddType(th, system, "GlTexture");
	Type* GLNONE = typeAlloc(th, TYPECODE_FUN, NULL, 1, MM.I);
    Type* GLBOOL=typeAlloc(th, TYPECODE_FUN, NULL, 1, MM.Boolean);
	Type* GLI = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.I, MM.I);
	Type* GLII = typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.I, MM.I, MM.I);
	Type* GLIII = typeAlloc(th, TYPECODE_FUN, NULL, 4, MM.I, MM.I, MM.I, MM.I);
	Type* GLIIII = typeAlloc(th, TYPECODE_FUN, NULL, 5, MM.I, MM.I, MM.I, MM.I, MM.I);
	Type* GLIIIIIIII = typeAlloc(th, TYPECODE_FUN, NULL, 9, MM.I, MM.I, MM.I, MM.I, MM.I, MM.I, MM.I, MM.I, MM.I);
	Type* GLF = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.F, MM.I);
	Type* GLFFFF = typeAlloc(th, TYPECODE_FUN, NULL, 5, MM.F, MM.F, MM.F, MM.F, MM.I);
	Type* GLIIF = typeAlloc(th, TYPECODE_FUN, NULL, 4, MM.I, MM.I, MM.F, MM.I);
	Type* GLP = typeAlloc(th, TYPECODE_FUN, NULL, 2, program->type, MM.I);
	Type* GLIs = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.I, MM.S);
	Type* GLPSi = typeAlloc(th, TYPECODE_FUN, NULL, 3, program->type, MM.S, MM.I);
	Type* GLIIFloats = typeAlloc(th, TYPECODE_FUN, NULL, 4, MM.I, MM.I, floats->type, MM.I);
	Type* GLIIIFloats = typeAlloc(th, TYPECODE_FUN, NULL, 5, MM.I, MM.I, MM.I, floats->type, MM.I);
	Type* GLFloatsI = typeAlloc(th, TYPECODE_FUN, NULL, 3, floats->type, MM.I, MM.F);

	GL.hRCgl = NULL;
	GL.instance = 0;

	pkgAddConst(th, system, "GL_COLOR_BUFFER_BIT", INTTOVAL(GL_COLOR_BUFFER_BIT), MM.I);
	pkgAddConst(th, system, "GL_TRIANGLES", INTTOVAL(GL_TRIANGLES), MM.I);
	pkgAddConst(th, system, "GL_LINE_LOOP", INTTOVAL(GL_LINE_LOOP), MM.I);
	pkgAddConst(th, system, "GL_SRC_ALPHA", INTTOVAL(GL_SRC_ALPHA), MM.I);
	pkgAddConst(th, system, "GL_ONE_MINUS_SRC_ALPHA", INTTOVAL(GL_ONE_MINUS_SRC_ALPHA), MM.I);
	pkgAddConst(th, system, "GL_DONT_CARE", INTTOVAL(GL_DONT_CARE), MM.I);
	pkgAddConst(th, system, "GL_BLEND", INTTOVAL(GL_BLEND), MM.I);
	pkgAddConst(th, system, "GL_TEXTURE_2D", INTTOVAL(GL_TEXTURE_2D), MM.I);
	pkgAddConst(th, system, "GL_TEXTURE_MAG_FILTER", INTTOVAL(GL_TEXTURE_MAG_FILTER), MM.I);
	pkgAddConst(th, system, "GL_LINEAR", INTTOVAL(GL_LINEAR), MM.I);
	pkgAddConst(th, system, "GL_TEXTURE_MIN_FILTER", INTTOVAL(GL_TEXTURE_MIN_FILTER), MM.I);
	pkgAddConst(th, system, "GL_NEAREST", INTTOVAL(GL_NEAREST), MM.I);
	pkgAddConst(th, system, "GL_FRONT_AND_BACK", INTTOVAL(GL_FRONT_AND_BACK), MM.I);
	pkgAddConst(th, system, "GL_POINTS", INTTOVAL(GL_POINTS), MM.I);
	pkgAddConst(th, system, "GL_LINES", INTTOVAL(GL_LINES), MM.I);
	pkgAddConst(th, system, "GL_LINE_STRIP", INTTOVAL(GL_LINE_STRIP), MM.I);
	pkgAddConst(th, system, "GL_TRIANGLE_STRIP", INTTOVAL(GL_TRIANGLE_STRIP), MM.I);
	pkgAddConst(th, system, "GL_TRIANGLE_FAN", INTTOVAL(GL_TRIANGLE_FAN), MM.I);
	pkgAddConst(th, system, "GL_FRONT", INTTOVAL(GL_FRONT), MM.I);
	pkgAddConst(th, system, "GL_BACK", INTTOVAL(GL_BACK), MM.I);
	pkgAddConst(th, system, "GL_RGB", INTTOVAL(GL_RGB), MM.I);
	pkgAddConst(th, system, "GL_RGBA", INTTOVAL(GL_RGBA), MM.I);
	pkgAddConst(th, system, "GL_LUMINANCE", INTTOVAL(GL_LUMINANCE), MM.I);
	pkgAddConst(th, system, "GL_CULL_FACE", INTTOVAL(GL_CULL_FACE), MM.I);
	pkgAddConst(th, system, "GL_DEPTH_TEST", INTTOVAL(GL_DEPTH_TEST), MM.I);
	pkgAddConst(th, system, "GL_LESS", INTTOVAL(GL_LESS), MM.I);
	pkgAddConst(th, system, "GL_GREATER", INTTOVAL(GL_GREATER), MM.I);
	pkgAddConst(th, system, "GL_LEQUAL", INTTOVAL(GL_LEQUAL), MM.I);
	pkgAddConst(th, system, "GL_GEQUAL", INTTOVAL(GL_GEQUAL), MM.I);
	pkgAddConst(th, system, "GL_NOTEQUAL", INTTOVAL(GL_NOTEQUAL), MM.I);
	pkgAddConst(th, system, "GL_EQUAL", INTTOVAL(GL_EQUAL), MM.I);
	pkgAddConst(th, system, "GL_TEXTURE_WRAP_S", INTTOVAL(GL_TEXTURE_WRAP_S), MM.I);
	pkgAddConst(th, system, "GL_TEXTURE_WRAP_T", INTTOVAL(GL_TEXTURE_WRAP_T), MM.I);
	pkgAddConst(th, system, "GL_REPEAT", INTTOVAL(GL_REPEAT), MM.I);
	pkgAddConst(th, system, "GL_CLAMP_TO_EDGE", INTTOVAL(GL_CLAMP_TO_EDGE), MM.I);
	pkgAddConst(th, system, "GL_NEAREST_MIPMAP_NEAREST", INTTOVAL(GL_NEAREST_MIPMAP_NEAREST), MM.I);
	pkgAddConst(th, system, "GL_NEAREST_MIPMAP_LINEAR", INTTOVAL(GL_NEAREST_MIPMAP_LINEAR), MM.I);
	pkgAddConst(th, system, "GL_LINEAR_MIPMAP_NEAREST", INTTOVAL(GL_LINEAR_MIPMAP_NEAREST), MM.I);
	pkgAddConst(th, system, "GL_LINEAR_MIPMAP_LINEAR", INTTOVAL(GL_LINEAR_MIPMAP_LINEAR), MM.I);
	pkgAddConst(th, system, "GL_REPLACE", INTTOVAL(GL_REPLACE), MM.I);
	pkgAddConst(th, system, "GL_ALWAYS", INTTOVAL(GL_ALWAYS), MM.I);
	pkgAddConst(th, system, "GL_DEPTH_BUFFER_BIT", INTTOVAL(GL_DEPTH_BUFFER_BIT), MM.I);
	pkgAddConst(th, system, "GL_NICEST", INTTOVAL(GL_NICEST), MM.I);
	pkgAddConst(th, system, "GL_DITHER", INTTOVAL(GL_DITHER), MM.I);
	pkgAddConst(th, system, "GL_FASTEST", INTTOVAL(GL_FASTEST), MM.I);
	pkgAddConst(th, system, "GL_ONE", INTTOVAL(GL_ONE), MM.I);
	pkgAddConst(th, system, "GL_ZERO", INTTOVAL(GL_ZERO), MM.I);
	pkgAddConst(th, system, "GL_DST_COLOR", INTTOVAL(GL_DST_COLOR), MM.I);
	pkgAddConst(th, system, "GL_ONE_MINUS_DST_COLOR", INTTOVAL(GL_ONE_MINUS_DST_COLOR), MM.I);
	pkgAddConst(th, system, "GL_DST_ALPHA", INTTOVAL(GL_DST_ALPHA), MM.I);
	pkgAddConst(th, system, "GL_ONE_MINUS_DST_ALPHA", INTTOVAL(GL_ONE_MINUS_DST_ALPHA), MM.I);
	pkgAddConst(th, system, "GL_SRC_ALPHA_SATURATE", INTTOVAL(GL_SRC_ALPHA_SATURATE), MM.I);
	pkgAddConst(th, system, "GL_SRC_COLOR", INTTOVAL(GL_SRC_COLOR), MM.I);
	pkgAddConst(th, system, "GL_ONE_MINUS_SRC_COLOR", INTTOVAL(GL_ONE_MINUS_SRC_COLOR), MM.I);
	pkgAddConst(th, system, "GL_SCISSOR_TEST", INTTOVAL(GL_SCISSOR_TEST), MM.I);
	pkgAddConst(th, system, "GL_TRUE", INTTOVAL(GL_TRUE), MM.I);
	pkgAddConst(th, system, "GL_FALSE", INTTOVAL(GL_FALSE), MM.I);
	pkgAddConst(th, system, "GL_VERSION", INTTOVAL(GL_VERSION), MM.I);
	pkgAddConst(th, system, "GL_FRAGMENT_SHADER", INTTOVAL(GL_FRAGMENT_SHADER), MM.I);
	pkgAddConst(th, system, "GL_VERTEX_SHADER", INTTOVAL(GL_VERTEX_SHADER), MM.I);
	pkgAddConst(th, system, "GL_TEXTURE0", INTTOVAL(GL_TEXTURE0), MM.I);
	pkgAddConst(th, system, "GL_ARRAY_BUFFER", INTTOVAL(GL_ARRAY_BUFFER), MM.I);
	pkgAddConst(th, system, "GL_STATIC_DRAW", INTTOVAL(GL_STATIC_DRAW), MM.I);
	pkgAddConst(th, system, "GL_FRAMEBUFFER", INTTOVAL(GL_FRAMEBUFFER), MM.I);
	pkgAddConst(th, system, "GL_VENDOR", INTTOVAL(GL_VENDOR), MM.I);
	pkgAddConst(th, system, "GL_RENDERER", INTTOVAL(GL_RENDERER), MM.I);
	pkgAddConst(th, system, "GL_SHADING_LANGUAGE_VERSION", INTTOVAL(GL_SHADING_LANGUAGE_VERSION), MM.I);
	pkgAddConst(th, system, "GL_EXTENSIONS", INTTOVAL(GL_EXTENSIONS), MM.I);
	pkgAddConst(th, system, "GL_CW", INTTOVAL(GL_CW), MM.I);
	pkgAddConst(th, system, "GL_CCW", INTTOVAL(GL_CCW), MM.I);

	// generic functions
	pkgAddFun(th, system, "glFlush", fun_glFlush, GLNONE);
	pkgAddFun(th, system, "glClear", fun_glClear, GLI);
	pkgAddFun(th, system, "glEnable", fun_glEnable, GLI);
	pkgAddFun(th, system, "glDisable", fun_glDisable, GLI);
	pkgAddFun(th, system, "glCullFace", fun_glCullFace, GLI);
	pkgAddFun(th, system, "glDepthFunc", fun_glDepthFunc, GLI);
	pkgAddFun(th, system, "glDepthMask", fun_glDepthMask, GLI);
	pkgAddFun(th, system, "glBlendFunc", fun_glBlendFunc, GLII);
	pkgAddFun(th, system, "glHint", fun_glHint, GLII);
	pkgAddFun(th, system, "glTexParameteri", fun_glTexParameteri, GLIII);
	pkgAddFun(th, system, "glDrawArrays", fun_glDrawArrays, GLIII);
	pkgAddFun(th, system, "glViewport", fun_glViewport, GLIIII);
	pkgAddFun(th, system, "glScissor", fun_glScissor, GLIIII);
	pkgAddFun(th, system, "glCopyTexImage2D", fun_glCopyTexImage2D, GLIIIIIIII);
	pkgAddFun(th, system, "glCopyTexSubImage2D", fun_glCopyTexSubImage2D, GLIIIIIIII);
	pkgAddFun(th, system, "glLineWidth", fun_glLineWidth, GLF);
	pkgAddFun(th, system, "glClearColor", fun_glClearColor, GLFFFF);
	pkgAddFun(th, system, "glTexParameterf", fun_glTexParameterf, GLIIF);
	pkgAddFun(th, system, "glEnableVertexAttribArray", fun_glEnableVertexAttribArray, GLI);
	pkgAddFun(th, system, "glActiveTexture", fun_glActiveTexture, GLI);
	pkgAddFun(th, system, "glDisableVertexAttribArray", fun_glDisableVertexAttribArray, GLI);
	pkgAddFun(th, system, "glFrontFace", fun_glFrontFace, GLI);
	pkgAddFun(th, system, "glUseProgram", fun_glUseProgram, GLP);
	pkgAddFun(th, system, "glLinkProgram", fun_glLinkProgram, GLP);
	pkgAddFun(th, system, "glGetString", fun_glGetString, GLIs);
	pkgAddFun(th, system, "glGetUniformLocation", fun_glGetUniformLocation, GLPSi);
	pkgAddFun(th, system, "glGetAttribLocation", fun_glGetAttribLocation, GLPSi);
	pkgAddFun(th, system, "glUniform1i", fun_glUniform1i, GLII);
	pkgAddFun(th, system, "glUniform1fv", fun_glUniform1fv, GLIIFloats);
	pkgAddFun(th, system, "glUniform2fv", fun_glUniform2fv, GLIIFloats);
	pkgAddFun(th, system, "glUniform3fv", fun_glUniform3fv, GLIIFloats);
	pkgAddFun(th, system, "glUniform4fv", fun_glUniform4fv, GLIIFloats);
	pkgAddFun(th, system, "glUniformMatrix2fv", fun_glUniformMatrix2fv, GLIIIFloats);
	pkgAddFun(th, system, "glUniformMatrix3fv", fun_glUniformMatrix3fv, GLIIIFloats);
	pkgAddFun(th, system, "glUniformMatrix4fv", fun_glUniformMatrix4fv, GLIIIFloats);

	// non generic functions
    pkgAddFun(th, system, "glES", fun_glES, GLBOOL);
	pkgAddFun(th, system, "glSwapBuffers", fun_glSwapBuffers, GLNONE);
	pkgAddFun(th, system, "_glMakeContext", fun_glMakeContext, GLNONE);
	pkgAddFun(th, system, "glCreateShader", fun_glCreateShader, typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.I, shader->type));
	pkgAddFun(th, system, "glCreateProgram", fun_glCreateProgram, typeAlloc(th, TYPECODE_FUN, NULL, 1, program->type));
	pkgAddFun(th, system, "glCreateTexture", fun_glCreateTexture, typeAlloc(th, TYPECODE_FUN, NULL, 1, texture->type));

	pkgAddFun(th, system, "glAttachShader", fun_glAttachShader, typeAlloc(th, TYPECODE_FUN, NULL, 3, program->type, shader->type,MM.I));
	pkgAddFun(th, system, "glShaderSource", fun_glShaderSource, typeAlloc(th, TYPECODE_FUN, NULL, 3, shader->type, MM.S, MM.I));
	pkgAddFun(th, system, "glCompileShader", fun_glCompileShader, typeAlloc(th, TYPECODE_FUN, NULL, 2, shader->type, MM.Boolean));

	pkgAddFun(th, system, "glVertexAttribPointer", fun_glVertexAttribPointer, typeAlloc(th, TYPECODE_FUN, NULL, 7, MM.I, MM.I, MM.I, MM.I, floats->type, MM.I, MM.I));

	pkgAddFun(th, system, "glBindTexture", fun_glBindTexture, typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.I, texture->type, MM.I));
	pkgAddFun(th, system, "glTexImage2D", fun_glTexImage2D, typeAlloc(th, TYPECODE_FUN, NULL, 7, texture->type, MM.I, MM.I, MM.I, MM.I, MM.Bitmap, MM.I));

	pkgAddFun(th, system, "floatsFromArray", fun_floatsFromArray, typeAlloc(th, TYPECODE_FUN, NULL, 2, typeAlloc(th, TYPECODE_ARRAY, NULL, 1, MM.F), floats->type));
	pkgAddFun(th, system, "floatsLength", fun_floatsLength, typeAlloc(th, TYPECODE_FUN, NULL, 2, floats->type, MM.I));
	pkgAddFun(th, system, "floatsGet", fun_floatsGet, GLFloatsI);
	return 0;
}
#endif
