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

#ifdef ON_WINDOWS
int GLready = 0;
#else
int GLready = 1;
#endif

#ifdef WITH_GL
int GLinstance=0;

int fun_glES(Thread* th)
{
#ifdef USE_GLES
    FUN_RETURN_TRUE;
#else
	FUN_RETURN_FALSE;
#endif
}


//---------------- shaders
// the following function is not used
// it prevents the user to keep a link to it after attaching to a program
// it will be released with the graphic context
int _shaderForget(LB* p)
{
	if (GLready) glDeleteShader(((lglShader*)p)->shader);
	return 0;
}
int fun_glCreateShader(Thread* th)
{
	LINT type = STACKPULLINT(th);
	lglShader* d = (lglShader*)memoryAllocExt(th, sizeof(lglShader), DBG_BIN, NULL, NULL); if (!d) return EXEC_OM;
	if (!GLready) FUN_RETURN_NIL;
	d->shader= glCreateShader((GLenum)type);
	d->instance = GLinstance;
	FUN_RETURN_PNT((LB*)d);
}

int fun_glShaderSource(Thread* th)
{
	char* p;

	LB* src = STACKPNT(th, 0);
	lglShader* s = (lglShader*)STACKPNT(th, 1);
	if ((!src) || (!s)) FUN_RETURN_NIL;
	p = STRSTART(src);
	if (GLready) glShaderSource(s->shader, 1, (const GLchar**)&p, NULL);
	FUN_RETURN_INT(0);
}
int fun_glCompileShader(Thread* th)
{
	GLint compiled;
	GLint infoLen = 0;

	lglShader* s = (lglShader*)STACKPNT(th, 0);
	if ((!s)||(!GLready)) FUN_RETURN_NIL;
	glCompileShader(s->shader);
	glGetShaderiv(s->shader, GL_COMPILE_STATUS, &compiled);

	if (compiled) FUN_RETURN_PNT(MM._true);

	glGetShaderiv(s->shader, GL_INFO_LOG_LENGTH, &infoLen);
	if (infoLen > 1)
	{
		char* infoLog = (char*)malloc(sizeof(char) * infoLen);
		glGetShaderInfoLog(s->shader, infoLen, NULL, infoLog);
		PRINTF(LOG_SYS, "Error compiling shader:\n%s\n", infoLog);
		free(infoLog);
	}
	FUN_RETURN_PNT(MM._false);
}

//---------------- programs
int _programForget(LB* p)
{
	lglProgram* d = (lglProgram*)p;
	if (GLready && d->instance == GLinstance) glDeleteProgram(d->program);
	return 0;
}
int fun_glCreateProgram(Thread* th)
{
	lglProgram* d = (lglProgram*)memoryAllocExt(th, sizeof(lglProgram), DBG_BIN, _programForget, NULL); if (!d) return EXEC_OM;
	if (!GLready) FUN_RETURN_NIL;
	d->program = glCreateProgram();
	d->instance = GLinstance;
	FUN_RETURN_PNT((LB*)(d));
}

int fun_glAttachShader(Thread* th)
{
	lglShader* s = (lglShader*)STACKPNT(th, 0);
	lglProgram* p = (lglProgram*)STACKPNT(th, 1);
	if ((!s) || (!p)||(!GLready)) FUN_RETURN_NIL;
	glAttachShader(p->program, s->shader);
	FUN_RETURN_INT(0);
}

//---------------- textures
int _textureForget(LB* p)
{
	lglTexture* d = (lglTexture*)p;
	if (GLready && d->instance == GLinstance) glDeleteTextures(1, &d->Texture);
	return 0;
}
int _glTextureTexImage2D(lglTexture* t, int vtarget, int vlevel, int vformat, int vborder, int w, int h, lchar* src)
{
	if (GLready) glTexImage2D(vtarget, vlevel, vformat, w, h, vborder, GL_BGRA, GL_UNSIGNED_BYTE, src);
	return 0;
}

int _glTexSubImage2D(lglTexture* t, int vtarget, int vlevel, int x, int y, int w, int h, lchar* src)
{
	if (GLready) glTexSubImage2D(vtarget, vlevel, x, y, w, h,  GL_BGRA, GL_UNSIGNED_BYTE, src);
	return 0;
}

int fun_glCreateTexture(Thread* th)
{
	lglTexture* d = (lglTexture*)memoryAllocExt(th, sizeof(lglTexture), DBG_BIN, _textureForget, NULL); if (!d) return EXEC_OM;
	d->data0 = NULL;
	glGenTextures(1, &d->Texture);
	d->instance = GLinstance;
	FUN_RETURN_PNT((LB*)(d));
}

int fun_glBindTexture(Thread* th)
{
	lglTexture* d = (lglTexture*)STACKPNT(th, 0);
	LINT v0 = STACKINT(th, 1);
	glBindTexture((int)v0, d ? (d->Texture) : 0);
	FUN_RETURN_INT(0);
}
int fun_glTexImage2D(Thread* th)
{
	LBitmap* bmp = (LBitmap*)STACKPNT(th, 0);
	int vborder = (int)STACKINT(th, 1);
	int vformat = (int)STACKINT(th, 2);
	int vlevel = (int)STACKINT(th, 3);
	int vtarget = (int)STACKINT(th, 4);
	lglTexture* d = (lglTexture*)STACKPNT(th, 5);
	if ((!d) || (!bmp)) FUN_RETURN_INT(1);
	glBindTexture(vtarget, d->Texture);
#ifdef USE_RGBA_MISSING_BGRA
	int N=bmp->w*bmp->h;
	int i;
	for(i=0;i<N;i++) bmp->start32[i]=(bmp->start32[i]&0xff00ff00)|((bmp->start32[i]&0xff0000)>>16)|((bmp->start32[i]&0xff)<<16);
#endif
	_glTextureTexImage2D(d, vtarget, vlevel, vformat, vborder, (int)bmp->w, (int)bmp->h, bmp->start8);
#ifdef USE_RGBA_MISSING_BGRA
	for(i=0;i<N;i++) bmp->start32[i]=(bmp->start32[i]&0xff00ff00)|((bmp->start32[i]&0xff0000)>>16)|((bmp->start32[i]&0xff)<<16);
#endif
	FUN_RETURN_INT(0);
}
int fun_glTexSubImage2D(Thread* th)
{
	LBitmap* bmp = (LBitmap*)STACKPNT(th, 0);
	int vh = (int)STACKINT(th, 1);
	int vw = (int)STACKINT(th, 2);
	int vy = (int)STACKINT(th, 3);
	int vx = (int)STACKINT(th, 4);
	int vlevel = (int)STACKINT(th, 5);
	int vtarget = (int)STACKINT(th, 6);
	lglTexture* d = (lglTexture*)STACKPNT(th, 7);
	if ((!d) || (!bmp)) FUN_RETURN_INT(1);
	if ((vx<0)||(vy<0)||(vw<=0)||(vh<=0)) FUN_RETURN_INT(1);
	if ((vx+vw>bmp->w)||(vy+vh>bmp->h)) FUN_RETURN_INT(1);
	glBindTexture(vtarget, d->Texture);
#ifdef USE_RGBA_MISSING_BGRA
	int N=vw*vh;
	int i;
	for(i=0;i<N;i++) bmp->start32[i]=(bmp->start32[i]&0xff00ff00)|((bmp->start32[i]&0xff0000)>>16)|((bmp->start32[i]&0xff)<<16);
#endif
	_glTexSubImage2D(d, vtarget, vlevel, vx, vy, vw, vh, bmp->start8);
#ifdef USE_RGBA_MISSING_BGRA
	for(i=0;i<N;i++) bmp->start32[i]=(bmp->start32[i]&0xff00ff00)|((bmp->start32[i]&0xff0000)>>16)|((bmp->start32[i]&0xff)<<16);
#endif
	FUN_RETURN_INT(0);
}
int fun_glTexImage2DUpdate(Thread* th)
{
	int* p;
	int* q;
	int j;

	LBitmap* bmp = (LBitmap*)STACKPNT(th, 0);
	int vh = (int)STACKINT(th, 1);
	int vw = (int)STACKINT(th, 2);
	int vy = (int)STACKINT(th, 3);
	int vx = (int)STACKINT(th, 4);
	int vlevel = (int)STACKINT(th, 5);
	int vtarget = (int)STACKINT(th, 6);
	LB* buffer = STACKPNT(th, 7);
	lglTexture* d = (lglTexture*)STACKPNT(th, 8);
	if ((!d) || (!bmp) || (!buffer)) FUN_RETURN_INT(1);
	if ((vw <= 0) || (vh <= 0)) FUN_RETURN_INT(1);
	vw += vx;
	if (vx < 0) vx = 0;
	if (vw > bmp->w) vw = (int)bmp->w;
	vw -= vx;

	vh += vy;
	if (vy < 0) vy = 0;
	if (vh > bmp->h) vh = (int)bmp->h;
	vh -= vy;
	if ((vw <= 0) || (vh <= 0)) FUN_RETURN_INT(1);
	if (STRLEN(buffer) < vw * vh * 4) FUN_RETURN_INT(1);

	p = (int*)STRSTART(buffer);
	q = &bmp->start32[vx + bmp->next32 * vy];
	for (j = 0; j < vh; j++) {
#ifdef USE_RGBA_MISSING_BGRA
		int i;
		for (i = 0; i < vw; i++) p[i] = (q[i] & 0xff00ff00) | ((q[i] & 0xff0000) >> 16) | ((q[i] & 0xff) << 16);
#else
		memcpy(p, q, vw<<2);
#endif
		p += vw;
		q += bmp->next32;
	}
	glBindTexture(vtarget, d->Texture);
	_glTexSubImage2D(d, vtarget, vlevel, vx, vy, vw, vh, (lchar*)STRSTART(buffer));

	FUN_RETURN_INT(0);
}


//---------------- floats

int fun_floatsFromArray(Thread* th)
{
	LINT n,i;
	float* p;
	LB* result;

	LB* src = STACKPNT(th,0);
	if (!src) FUN_RETURN_NIL;
	n = TABLEN(src);
	result = memoryAllocBin(th, NULL, sizeof(float) * n, DBG_BIN); if (!result) return EXEC_OM;
	p = (float*)BINSTART(result);
	for (i = 0; i < n; i++) p[i] = (float)TABFLOAT(src, i);
	FUN_RETURN_PNT(result);
}

int fun_floatsLength(Thread* th)
{
	LB* floats = STACKPNT(th, 0);
	if (!floats) FUN_RETURN_NIL;
	FUN_RETURN_INT(BINLEN(floats)/sizeof(float));
}
int fun_floatsGet(Thread* th)
{
	LFLOAT f;
	LINT n;
	float* p;
	LINT i=STACKPULLINT(th);
	LB* floats = STACKPNT(th, 0);
	if (!floats) FUN_RETURN_NIL;
	n = BINLEN(floats) / sizeof(float);
	if ((i < 0) || (i >= n)) FUN_RETURN_NIL;
	p = (float*)BINSTART(floats);
	f=p[i];
	FUN_RETURN_FLOAT(f);
}
int fun_glVertexAttribPointer(Thread* th)
{
	float* p;
	
	int offset = (int)STACKINT(th, 0);
	LB* floats = STACKPNT(th, 1);
	int stride = (int)STACKINT(th, 2);
	int normalized = (int)STACKINT(th, 3);
	int size = (int)STACKINT(th, 4);
	int index = (int)STACKINT(th, 5);
	p = floats ? (float*)BINSTART(floats) : NULL;
	if (!GLready) FUN_RETURN_NIL;

	glVertexAttribPointer(index, size, GL_FLOAT, normalized, stride * sizeof(float), p + offset);
	FUN_RETURN_INT(0);
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
GLIIFloats(fun_glUniform1fv, glUniform1fv)
GLIIFloats(fun_glUniform2fv, glUniform2fv)
GLIIFloats(fun_glUniform3fv, glUniform3fv)
GLIIFloats(fun_glUniform4fv, glUniform4fv)
GLIIIFloats(fun_glUniformMatrix2fv, glUniformMatrix2fv)
GLIIIFloats(fun_glUniformMatrix3fv, glUniformMatrix3fv)
GLIIIFloats(fun_glUniformMatrix4fv, glUniformMatrix4fv)

int coreUiGLInit(Thread* th, Pkg* system)
{
	Def* floats = pkgAddType(th, system, "Floats");
	Def* shader = pkgAddType(th, system, "GlShader");
	Def* program = pkgAddType(th, system, "GlProgram");
	Def* texture = pkgAddType(th, system, "GlTexture");
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

	pkgAddConstInt(th, system, "GL_COLOR_BUFFER_BIT", GL_COLOR_BUFFER_BIT, MM.I);
	pkgAddConstInt(th, system, "GL_TRIANGLES", GL_TRIANGLES, MM.I);
	pkgAddConstInt(th, system, "GL_LINE_LOOP", GL_LINE_LOOP, MM.I);
	pkgAddConstInt(th, system, "GL_SRC_ALPHA", GL_SRC_ALPHA, MM.I);
	pkgAddConstInt(th, system, "GL_ONE_MINUS_SRC_ALPHA", GL_ONE_MINUS_SRC_ALPHA, MM.I);
	pkgAddConstInt(th, system, "GL_DONT_CARE", GL_DONT_CARE, MM.I);
	pkgAddConstInt(th, system, "GL_BLEND", GL_BLEND, MM.I);
	pkgAddConstInt(th, system, "GL_TEXTURE_2D", GL_TEXTURE_2D, MM.I);
	pkgAddConstInt(th, system, "GL_TEXTURE_MAG_FILTER", GL_TEXTURE_MAG_FILTER, MM.I);
	pkgAddConstInt(th, system, "GL_LINEAR", GL_LINEAR, MM.I);
	pkgAddConstInt(th, system, "GL_TEXTURE_MIN_FILTER", GL_TEXTURE_MIN_FILTER, MM.I);
	pkgAddConstInt(th, system, "GL_NEAREST", GL_NEAREST, MM.I);
	pkgAddConstInt(th, system, "GL_FRONT_AND_BACK", GL_FRONT_AND_BACK, MM.I);
	pkgAddConstInt(th, system, "GL_POINTS", GL_POINTS, MM.I);
	pkgAddConstInt(th, system, "GL_LINES", GL_LINES, MM.I);
	pkgAddConstInt(th, system, "GL_LINE_STRIP", GL_LINE_STRIP, MM.I);
	pkgAddConstInt(th, system, "GL_TRIANGLE_STRIP", GL_TRIANGLE_STRIP, MM.I);
	pkgAddConstInt(th, system, "GL_TRIANGLE_FAN", GL_TRIANGLE_FAN, MM.I);
	pkgAddConstInt(th, system, "GL_FRONT", GL_FRONT, MM.I);
	pkgAddConstInt(th, system, "GL_BACK", GL_BACK, MM.I);
	pkgAddConstInt(th, system, "GL_RGB", GL_RGB, MM.I);
	pkgAddConstInt(th, system, "GL_RGBA", GL_RGBA, MM.I);
	pkgAddConstInt(th, system, "GL_LUMINANCE", GL_LUMINANCE, MM.I);
	pkgAddConstInt(th, system, "GL_CULL_FACE", GL_CULL_FACE, MM.I);
	pkgAddConstInt(th, system, "GL_DEPTH_TEST", GL_DEPTH_TEST, MM.I);
	pkgAddConstInt(th, system, "GL_LESS", GL_LESS, MM.I);
	pkgAddConstInt(th, system, "GL_GREATER", GL_GREATER, MM.I);
	pkgAddConstInt(th, system, "GL_LEQUAL", GL_LEQUAL, MM.I);
	pkgAddConstInt(th, system, "GL_GEQUAL", GL_GEQUAL, MM.I);
	pkgAddConstInt(th, system, "GL_NOTEQUAL", GL_NOTEQUAL, MM.I);
	pkgAddConstInt(th, system, "GL_EQUAL", GL_EQUAL, MM.I);
	pkgAddConstInt(th, system, "GL_TEXTURE_WRAP_S", GL_TEXTURE_WRAP_S, MM.I);
	pkgAddConstInt(th, system, "GL_TEXTURE_WRAP_T", GL_TEXTURE_WRAP_T, MM.I);
	pkgAddConstInt(th, system, "GL_REPEAT", GL_REPEAT, MM.I);
	pkgAddConstInt(th, system, "GL_CLAMP_TO_EDGE", GL_CLAMP_TO_EDGE, MM.I);
	pkgAddConstInt(th, system, "GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, MM.I);
	pkgAddConstInt(th, system, "GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, MM.I);
	pkgAddConstInt(th, system, "GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, MM.I);
	pkgAddConstInt(th, system, "GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, MM.I);
	pkgAddConstInt(th, system, "GL_REPLACE", GL_REPLACE, MM.I);
	pkgAddConstInt(th, system, "GL_ALWAYS", GL_ALWAYS, MM.I);
	pkgAddConstInt(th, system, "GL_DEPTH_BUFFER_BIT", GL_DEPTH_BUFFER_BIT, MM.I);
	pkgAddConstInt(th, system, "GL_NICEST", GL_NICEST, MM.I);
	pkgAddConstInt(th, system, "GL_DITHER", GL_DITHER, MM.I);
	pkgAddConstInt(th, system, "GL_FASTEST", GL_FASTEST, MM.I);
	pkgAddConstInt(th, system, "GL_ONE", GL_ONE, MM.I);
	pkgAddConstInt(th, system, "GL_ZERO", GL_ZERO, MM.I);
	pkgAddConstInt(th, system, "GL_DST_COLOR", GL_DST_COLOR, MM.I);
	pkgAddConstInt(th, system, "GL_ONE_MINUS_DST_COLOR", GL_ONE_MINUS_DST_COLOR, MM.I);
	pkgAddConstInt(th, system, "GL_DST_ALPHA", GL_DST_ALPHA, MM.I);
	pkgAddConstInt(th, system, "GL_ONE_MINUS_DST_ALPHA", GL_ONE_MINUS_DST_ALPHA, MM.I);
	pkgAddConstInt(th, system, "GL_SRC_ALPHA_SATURATE", GL_SRC_ALPHA_SATURATE, MM.I);
	pkgAddConstInt(th, system, "GL_SRC_COLOR", GL_SRC_COLOR, MM.I);
	pkgAddConstInt(th, system, "GL_ONE_MINUS_SRC_COLOR", GL_ONE_MINUS_SRC_COLOR, MM.I);
	pkgAddConstInt(th, system, "GL_SCISSOR_TEST", GL_SCISSOR_TEST, MM.I);
	pkgAddConstInt(th, system, "GL_TRUE", GL_TRUE, MM.I);
	pkgAddConstInt(th, system, "GL_FALSE", GL_FALSE, MM.I);
	pkgAddConstInt(th, system, "GL_VERSION", GL_VERSION, MM.I);
	pkgAddConstInt(th, system, "GL_FRAGMENT_SHADER", GL_FRAGMENT_SHADER, MM.I);
	pkgAddConstInt(th, system, "GL_VERTEX_SHADER", GL_VERTEX_SHADER, MM.I);
	pkgAddConstInt(th, system, "GL_TEXTURE0", GL_TEXTURE0, MM.I);
	pkgAddConstInt(th, system, "GL_ARRAY_BUFFER", GL_ARRAY_BUFFER, MM.I);
	pkgAddConstInt(th, system, "GL_STATIC_DRAW", GL_STATIC_DRAW, MM.I);
	pkgAddConstInt(th, system, "GL_FRAMEBUFFER", GL_FRAMEBUFFER, MM.I);
	pkgAddConstInt(th, system, "GL_VENDOR", GL_VENDOR, MM.I);
	pkgAddConstInt(th, system, "GL_RENDERER", GL_RENDERER, MM.I);
	pkgAddConstInt(th, system, "GL_SHADING_LANGUAGE_VERSION", GL_SHADING_LANGUAGE_VERSION, MM.I);
	pkgAddConstInt(th, system, "GL_EXTENSIONS", GL_EXTENSIONS, MM.I);
	pkgAddConstInt(th, system, "GL_CW", GL_CW, MM.I);
	pkgAddConstInt(th, system, "GL_CCW", GL_CCW, MM.I);

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
	pkgAddFun(th, system, "_glRefreshContext", fun_glRefreshContext, GLNONE);

	pkgAddFun(th, system, "glCreateShader", fun_glCreateShader, typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.I, shader->type));
	pkgAddFun(th, system, "glCreateProgram", fun_glCreateProgram, typeAlloc(th, TYPECODE_FUN, NULL, 1, program->type));
	pkgAddFun(th, system, "glCreateTexture", fun_glCreateTexture, typeAlloc(th, TYPECODE_FUN, NULL, 1, texture->type));

	pkgAddFun(th, system, "glAttachShader", fun_glAttachShader, typeAlloc(th, TYPECODE_FUN, NULL, 3, program->type, shader->type,MM.I));
	pkgAddFun(th, system, "glShaderSource", fun_glShaderSource, typeAlloc(th, TYPECODE_FUN, NULL, 3, shader->type, MM.S, MM.I));
	pkgAddFun(th, system, "glCompileShader", fun_glCompileShader, typeAlloc(th, TYPECODE_FUN, NULL, 2, shader->type, MM.Boolean));

	pkgAddFun(th, system, "glVertexAttribPointer", fun_glVertexAttribPointer, typeAlloc(th, TYPECODE_FUN, NULL, 7, MM.I, MM.I, MM.I, MM.I, floats->type, MM.I, MM.I));

	pkgAddFun(th, system, "glBindTexture", fun_glBindTexture, typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.I, texture->type, MM.I));
	pkgAddFun(th, system, "glTexImage2D", fun_glTexImage2D, typeAlloc(th, TYPECODE_FUN, NULL, 7, texture->type, MM.I, MM.I, MM.I, MM.I, MM.Bitmap, MM.I));
	pkgAddFun(th, system, "glTexSubImage2D", fun_glTexSubImage2D, typeAlloc(th, TYPECODE_FUN, NULL, 9, texture->type, MM.I, MM.I, MM.I, MM.I, MM.I, MM.I, MM.Bitmap, MM.I));
	pkgAddFun(th, system, "glTexImage2DUpdate", fun_glTexImage2DUpdate, typeAlloc(th, TYPECODE_FUN, NULL, 10, texture->type, MM.Bytes, MM.I, MM.I, MM.I, MM.I, MM.I, MM.I, MM.Bitmap, MM.I));

	pkgAddFun(th, system, "floatsFromArray", fun_floatsFromArray, typeAlloc(th, TYPECODE_FUN, NULL, 2, typeAlloc(th, TYPECODE_ARRAY, NULL, 1, MM.F), floats->type));
	pkgAddFun(th, system, "floatsLength", fun_floatsLength, typeAlloc(th, TYPECODE_FUN, NULL, 2, floats->type, MM.I));
	pkgAddFun(th, system, "floatsGet", fun_floatsGet, GLFloatsI);
	return 0;
}
#endif
