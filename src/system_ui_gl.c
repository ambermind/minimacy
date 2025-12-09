// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
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
	LINT type = STACK_INT(th, 0);
	lglShader* d = (lglShader*)memoryAllocNative(sizeof(lglShader), DBG_BIN, NULL, NULL); if (!d) return EXEC_OM;
	if (!GLready) FUN_RETURN_NIL;
	d->shader= glCreateShader((GLenum)type);
	d->instance = GLinstance;
	FUN_RETURN_PNT((LB*)d);
}

int fun_glShaderSource(Thread* th)
{
	char* p;

	LB* src = STACK_PNT(th, 0);
	lglShader* s = (lglShader*)STACK_PNT(th, 1);
	if ((!src) || (!s)) FUN_RETURN_NIL;
	p = STR_START(src);
	if (GLready) glShaderSource(s->shader, 1, (const GLchar**)&p, NULL);
	FUN_RETURN_INT(0);
}
int fun_glCompileShader(Thread* th)
{
	GLint compiled;
	GLint infoLen = 0;

	lglShader* s = (lglShader*)STACK_PNT(th, 0);
	if ((!s)||(!GLready)) FUN_RETURN_NIL;
	glCompileShader(s->shader);
	glGetShaderiv(s->shader, GL_COMPILE_STATUS, &compiled);

	if (compiled) FUN_RETURN_PNT(MM._true);

	glGetShaderiv(s->shader, GL_INFO_LOG_LENGTH, &infoLen);
	if (infoLen > 1)
	{
		char* infoLog = (char*)malloc(sizeof(char) * infoLen);
		glGetShaderInfoLog(s->shader, infoLen, NULL, infoLog);
		PRINTF(LOG_SYS, "> Error: compiling shader:\n%s\n", infoLog);
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
	lglProgram* d = (lglProgram*)memoryAllocNative(sizeof(lglProgram), DBG_BIN, _programForget, NULL); if (!d) return EXEC_OM;
	if (!GLready) FUN_RETURN_NIL;
	d->program = glCreateProgram();
	d->instance = GLinstance;
	FUN_RETURN_PNT((LB*)(d));
}

int fun_glAttachShader(Thread* th)
{
	lglShader* s = (lglShader*)STACK_PNT(th, 0);
	lglProgram* p = (lglProgram*)STACK_PNT(th, 1);
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
	lglTexture* d = (lglTexture*)memoryAllocNative(sizeof(lglTexture), DBG_BIN, _textureForget, NULL); if (!d) return EXEC_OM;
	d->data0 = NULL;
	glGenTextures(1, &d->Texture);
	d->instance = GLinstance;
	FUN_RETURN_PNT((LB*)(d));
}

int fun_glBindTexture(Thread* th)
{
	lglTexture* d = (lglTexture*)STACK_PNT(th, 0);
	LINT v0 = STACK_INT(th, 1);
	glBindTexture((int)v0, d ? (d->Texture) : 0);
	FUN_RETURN_INT(0);
}
int fun_glTexImage2D(Thread* th)
{
	LBitmap* bmp = (LBitmap*)STACK_PNT(th, 0);
	int vborder = (int)STACK_INT(th, 1);
	int vformat = (int)STACK_INT(th, 2);
	int vlevel = (int)STACK_INT(th, 3);
	int vtarget = (int)STACK_INT(th, 4);
	lglTexture* d = (lglTexture*)STACK_PNT(th, 5);
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
	LBitmap* bmp = (LBitmap*)STACK_PNT(th, 0);
	int vh = (int)STACK_INT(th, 1);
	int vw = (int)STACK_INT(th, 2);
	int vy = (int)STACK_INT(th, 3);
	int vx = (int)STACK_INT(th, 4);
	int vlevel = (int)STACK_INT(th, 5);
	int vtarget = (int)STACK_INT(th, 6);
	lglTexture* d = (lglTexture*)STACK_PNT(th, 7);
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

	LBitmap* bmp = (LBitmap*)STACK_PNT(th, 0);
	int vh = (int)STACK_INT(th, 1);
	int vw = (int)STACK_INT(th, 2);
	int vy = (int)STACK_INT(th, 3);
	int vx = (int)STACK_INT(th, 4);
	int vlevel = (int)STACK_INT(th, 5);
	int vtarget = (int)STACK_INT(th, 6);
	LB* buffer = STACK_PNT(th, 7);
	lglTexture* d = (lglTexture*)STACK_PNT(th, 8);
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
	if (STR_LENGTH(buffer) < vw * vh * 4) FUN_RETURN_INT(1);

	p = (int*)STR_START(buffer);
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
	_glTexSubImage2D(d, vtarget, vlevel, vx, vy, vw, vh, (lchar*)STR_START(buffer));

	FUN_RETURN_INT(0);
}


//---------------- floats

int fun_floatsFromArray(Thread* th)
{
	LINT n,i;
	float* p;
	LB* result;

	LB* src = STACK_PNT(th,0);
	if (!src) FUN_RETURN_NIL;
	n = ARRAY_LENGTH(src);
	result = memoryAllocBin(NULL, sizeof(float) * n, DBG_BIN); if (!result) return EXEC_OM;
	p = (float*)BIN_START(result);
	for (i = 0; i < n; i++) p[i] = (float)ARRAY_FLOAT(src, i);
	FUN_RETURN_PNT(result);
}

int fun_floatsLength(Thread* th)
{
	LB* floats = STACK_PNT(th, 0);
	if (!floats) FUN_RETURN_NIL;
	FUN_RETURN_INT(BIN_LENGTH(floats)/sizeof(float));
}
int fun_floatsGet(Thread* th)
{
	LFLOAT f;
	LINT n;
	float* p;
	LINT i=STACK_PULL_INT(th);
	LB* floats = STACK_PNT(th, 0);
	if (!floats) FUN_RETURN_NIL;
	n = BIN_LENGTH(floats) / sizeof(float);
	if ((i < 0) || (i >= n)) FUN_RETURN_NIL;
	p = (float*)BIN_START(floats);
	f=p[i];
	FUN_RETURN_FLOAT(f);
}
int fun_glVertexAttribPointer(Thread* th)
{
	float* p;
	
	int offset = (int)STACK_INT(th, 0);
	LB* floats = STACK_PNT(th, 1);
	int stride = (int)STACK_INT(th, 2);
	int normalized = (int)STACK_INT(th, 3);
	int size = (int)STACK_INT(th, 4);
	int index = (int)STACK_INT(th, 5);
	p = floats ? (float*)BIN_START(floats) : NULL;
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

int systemUiGLInit(Pkg* system)
{
	pkgAddType(system, "Floats");
	pkgAddType(system, "GlShader");
	pkgAddType(system, "GlProgram");
	pkgAddType(system, "GlTexture");

	static const Native nativeDefs[] = {
		{ NATIVE_INT, "GL_COLOR_BUFFER_BIT", (void*)GL_COLOR_BUFFER_BIT, "Int"},
		{ NATIVE_INT, "GL_TRIANGLES", (void*)GL_TRIANGLES, "Int"},
		{ NATIVE_INT, "GL_LINE_LOOP", (void*)GL_LINE_LOOP, "Int"},
		{ NATIVE_INT, "GL_SRC_ALPHA", (void*)GL_SRC_ALPHA, "Int"},
		{ NATIVE_INT, "GL_ONE_MINUS_SRC_ALPHA", (void*)GL_ONE_MINUS_SRC_ALPHA, "Int"},
		{ NATIVE_INT, "GL_DONT_CARE", (void*)GL_DONT_CARE, "Int"},
		{ NATIVE_INT, "GL_BLEND", (void*)GL_BLEND, "Int"},
		{ NATIVE_INT, "GL_TEXTURE_2D", (void*)GL_TEXTURE_2D, "Int"},
		{ NATIVE_INT, "GL_TEXTURE_MAG_FILTER", (void*)GL_TEXTURE_MAG_FILTER, "Int"},
		{ NATIVE_INT, "GL_LINEAR", (void*)GL_LINEAR, "Int"},
		{ NATIVE_INT, "GL_TEXTURE_MIN_FILTER", (void*)GL_TEXTURE_MIN_FILTER, "Int"},
		{ NATIVE_INT, "GL_NEAREST", (void*)GL_NEAREST, "Int"},
		{ NATIVE_INT, "GL_FRONT_AND_BACK", (void*)GL_FRONT_AND_BACK, "Int"},
		{ NATIVE_INT, "GL_POINTS", (void*)GL_POINTS, "Int"},
		{ NATIVE_INT, "GL_LINES", (void*)GL_LINES, "Int"},
		{ NATIVE_INT, "GL_LINE_STRIP", (void*)GL_LINE_STRIP, "Int"},
		{ NATIVE_INT, "GL_TRIANGLE_STRIP", (void*)GL_TRIANGLE_STRIP, "Int"},
		{ NATIVE_INT, "GL_TRIANGLE_FAN", (void*)GL_TRIANGLE_FAN, "Int"},
		{ NATIVE_INT, "GL_FRONT", (void*)GL_FRONT, "Int"},
		{ NATIVE_INT, "GL_BACK", (void*)GL_BACK, "Int"},
		{ NATIVE_INT, "GL_RGB", (void*)GL_RGB, "Int"},
		{ NATIVE_INT, "GL_RGBA", (void*)GL_RGBA, "Int"},
		{ NATIVE_INT, "GL_LUMINANCE", (void*)GL_LUMINANCE, "Int"},
		{ NATIVE_INT, "GL_CULL_FACE", (void*)GL_CULL_FACE, "Int"},
		{ NATIVE_INT, "GL_DEPTH_TEST", (void*)GL_DEPTH_TEST, "Int"},
		{ NATIVE_INT, "GL_LESS", (void*)GL_LESS, "Int"},
		{ NATIVE_INT, "GL_GREATER", (void*)GL_GREATER, "Int"},
		{ NATIVE_INT, "GL_LEQUAL", (void*)GL_LEQUAL, "Int"},
		{ NATIVE_INT, "GL_GEQUAL", (void*)GL_GEQUAL, "Int"},
		{ NATIVE_INT, "GL_NOTEQUAL", (void*)GL_NOTEQUAL, "Int"},
		{ NATIVE_INT, "GL_EQUAL", (void*)GL_EQUAL, "Int"},
		{ NATIVE_INT, "GL_TEXTURE_WRAP_S", (void*)GL_TEXTURE_WRAP_S, "Int"},
		{ NATIVE_INT, "GL_TEXTURE_WRAP_T", (void*)GL_TEXTURE_WRAP_T, "Int"},
		{ NATIVE_INT, "GL_REPEAT", (void*)GL_REPEAT, "Int"},
		{ NATIVE_INT, "GL_CLAMP_TO_EDGE", (void*)GL_CLAMP_TO_EDGE, "Int"},
		{ NATIVE_INT, "GL_NEAREST_MIPMAP_NEAREST", (void*)GL_NEAREST_MIPMAP_NEAREST, "Int"},
		{ NATIVE_INT, "GL_NEAREST_MIPMAP_LINEAR", (void*)GL_NEAREST_MIPMAP_LINEAR, "Int"},
		{ NATIVE_INT, "GL_LINEAR_MIPMAP_NEAREST", (void*)GL_LINEAR_MIPMAP_NEAREST, "Int"},
		{ NATIVE_INT, "GL_LINEAR_MIPMAP_LINEAR", (void*)GL_LINEAR_MIPMAP_LINEAR, "Int"},
		{ NATIVE_INT, "GL_REPLACE", (void*)GL_REPLACE, "Int"},
		{ NATIVE_INT, "GL_ALWAYS", (void*)GL_ALWAYS, "Int"},
		{ NATIVE_INT, "GL_DEPTH_BUFFER_BIT", (void*)GL_DEPTH_BUFFER_BIT, "Int"},
		{ NATIVE_INT, "GL_NICEST", (void*)GL_NICEST, "Int"},
		{ NATIVE_INT, "GL_DITHER", (void*)GL_DITHER, "Int"},
		{ NATIVE_INT, "GL_FASTEST", (void*)GL_FASTEST, "Int"},
		{ NATIVE_INT, "GL_ONE", (void*)GL_ONE, "Int"},
		{ NATIVE_INT, "GL_ZERO", (void*)GL_ZERO, "Int"},
		{ NATIVE_INT, "GL_DST_COLOR", (void*)GL_DST_COLOR, "Int"},
		{ NATIVE_INT, "GL_ONE_MINUS_DST_COLOR", (void*)GL_ONE_MINUS_DST_COLOR, "Int"},
		{ NATIVE_INT, "GL_DST_ALPHA", (void*)GL_DST_ALPHA, "Int"},
		{ NATIVE_INT, "GL_ONE_MINUS_DST_ALPHA", (void*)GL_ONE_MINUS_DST_ALPHA, "Int"},
		{ NATIVE_INT, "GL_SRC_ALPHA_SATURATE", (void*)GL_SRC_ALPHA_SATURATE, "Int"},
		{ NATIVE_INT, "GL_SRC_COLOR", (void*)GL_SRC_COLOR, "Int"},
		{ NATIVE_INT, "GL_ONE_MINUS_SRC_COLOR", (void*)GL_ONE_MINUS_SRC_COLOR, "Int"},
		{ NATIVE_INT, "GL_SCISSOR_TEST", (void*)GL_SCISSOR_TEST, "Int"},
		{ NATIVE_INT, "GL_TRUE", (void*)GL_TRUE, "Int"},
		{ NATIVE_INT, "GL_FALSE", (void*)GL_FALSE, "Int"},
		{ NATIVE_INT, "GL_VERSION", (void*)GL_VERSION, "Int"},
		{ NATIVE_INT, "GL_FRAGMENT_SHADER", (void*)GL_FRAGMENT_SHADER, "Int"},
		{ NATIVE_INT, "GL_VERTEX_SHADER", (void*)GL_VERTEX_SHADER, "Int"},
		{ NATIVE_INT, "GL_TEXTURE0", (void*)GL_TEXTURE0, "Int"},
		{ NATIVE_INT, "GL_ARRAY_BUFFER", (void*)GL_ARRAY_BUFFER, "Int"},
		{ NATIVE_INT, "GL_STATIC_DRAW", (void*)GL_STATIC_DRAW, "Int"},
		{ NATIVE_INT, "GL_FRAMEBUFFER", (void*)GL_FRAMEBUFFER, "Int"},
		{ NATIVE_INT, "GL_VENDOR", (void*)GL_VENDOR, "Int"},
		{ NATIVE_INT, "GL_RENDERER", (void*)GL_RENDERER, "Int"},
		{ NATIVE_INT, "GL_SHADING_LANGUAGE_VERSION", (void*)GL_SHADING_LANGUAGE_VERSION, "Int"},
		{ NATIVE_INT, "GL_EXTENSIONS", (void*)GL_EXTENSIONS, "Int"},
		{ NATIVE_INT, "GL_CW", (void*)GL_CW, "Int"},
		{ NATIVE_INT, "GL_CCW", (void*)GL_CCW, "Int"}, 
		// generic functions
		{ NATIVE_FUN, "glFlush", fun_glFlush, "fun -> Int"},
		{ NATIVE_FUN, "glClear", fun_glClear, "fun Int -> Int"},
		{ NATIVE_FUN, "glEnable", fun_glEnable, "fun Int -> Int"},
		{ NATIVE_FUN, "glDisable", fun_glDisable, "fun Int -> Int"},
		{ NATIVE_FUN, "glCullFace", fun_glCullFace, "fun Int -> Int"},
		{ NATIVE_FUN, "glDepthFunc", fun_glDepthFunc, "fun Int -> Int"},
		{ NATIVE_FUN, "glDepthMask", fun_glDepthMask, "fun Int -> Int"},
		{ NATIVE_FUN, "glBlendFunc", fun_glBlendFunc, "fun Int Int -> Int"},
		{ NATIVE_FUN, "glHint", fun_glHint, "fun Int Int -> Int"},
		{ NATIVE_FUN, "glTexParameteri", fun_glTexParameteri, "fun Int Int Int -> Int"},
		{ NATIVE_FUN, "glDrawArrays", fun_glDrawArrays, "fun Int Int Int -> Int"},
		{ NATIVE_FUN, "glViewport", fun_glViewport, "fun Int Int Int Int -> Int"},
		{ NATIVE_FUN, "glScissor", fun_glScissor, "fun Int Int Int Int -> Int"},
		{ NATIVE_FUN, "glCopyTexImage2D", fun_glCopyTexImage2D, "fun Int Int Int Int Int Int Int Int -> Int"},
		{ NATIVE_FUN, "glCopyTexSubImage2D", fun_glCopyTexSubImage2D, "fun Int Int Int Int Int Int Int Int -> Int"},
		{ NATIVE_FUN, "glLineWidth", fun_glLineWidth, "fun Float -> Int"},
		{ NATIVE_FUN, "glClearColor", fun_glClearColor, "fun Float Float Float Float -> Int"},
		{ NATIVE_FUN, "glTexParameterf", fun_glTexParameterf, "fun Int Int Float -> Int"},
		{ NATIVE_FUN, "glEnableVertexAttribArray", fun_glEnableVertexAttribArray, "fun Int -> Int"},
		{ NATIVE_FUN, "glActiveTexture", fun_glActiveTexture, "fun Int -> Int"},
		{ NATIVE_FUN, "glDisableVertexAttribArray", fun_glDisableVertexAttribArray, "fun Int -> Int"},
		{ NATIVE_FUN, "glFrontFace", fun_glFrontFace, "fun Int -> Int"},
		{ NATIVE_FUN, "glUseProgram", fun_glUseProgram, "fun GlProgram -> Int"},
		{ NATIVE_FUN, "glLinkProgram", fun_glLinkProgram, "fun GlProgram -> Int"},
		{ NATIVE_FUN, "glGetString", fun_glGetString, "fun Int -> Str"},
		{ NATIVE_FUN, "glGetUniformLocation", fun_glGetUniformLocation, "fun GlProgram Str -> Int"},
		{ NATIVE_FUN, "glGetAttribLocation", fun_glGetAttribLocation, "fun GlProgram Str -> Int"},
		{ NATIVE_FUN, "glUniform1fv", fun_glUniform1fv, "fun Int Int Floats -> Int"},
		{ NATIVE_FUN, "glUniform2fv", fun_glUniform2fv, "fun Int Int Floats -> Int"},
		{ NATIVE_FUN, "glUniform3fv", fun_glUniform3fv, "fun Int Int Floats -> Int"},
		{ NATIVE_FUN, "glUniform4fv", fun_glUniform4fv, "fun Int Int Floats -> Int"},
		{ NATIVE_FUN, "glUniformMatrix2fv", fun_glUniformMatrix2fv, "fun Int Int Int Floats -> Int"},
		{ NATIVE_FUN, "glUniformMatrix3fv", fun_glUniformMatrix3fv, "fun Int Int Int Floats -> Int"},
		{ NATIVE_FUN, "glUniformMatrix4fv", fun_glUniformMatrix4fv, "fun Int Int Int Floats -> Int"},
		// non generic functions
		{ NATIVE_FUN, "glES", fun_glES, "fun -> Bool" },
		{ NATIVE_FUN, "glSwapBuffers", fun_glSwapBuffers, "fun -> Int" },
		{ NATIVE_FUN, "_glMakeContext", fun_glMakeContext, "fun -> Int" },
		{ NATIVE_FUN, "_glRefreshContext", fun_glRefreshContext, "fun -> Int" },
		{ NATIVE_FUN, "glCreateShader", fun_glCreateShader, "fun Int -> GlShader" },
		{ NATIVE_FUN, "glCreateProgram", fun_glCreateProgram, "fun -> GlProgram" },
		{ NATIVE_FUN, "glCreateTexture", fun_glCreateTexture, "fun -> GlTexture" },
		{ NATIVE_FUN, "glAttachShader", fun_glAttachShader, "fun GlProgram GlShader -> Int" },
		{ NATIVE_FUN, "glShaderSource", fun_glShaderSource, "fun GlShader Str -> Int" },
		{ NATIVE_FUN, "glCompileShader", fun_glCompileShader, "fun GlShader -> Bool" },
		{ NATIVE_FUN, "glVertexAttribPointer", fun_glVertexAttribPointer, "fun Int Int Int Int Floats Int -> Int" },
		{ NATIVE_FUN, "glBindTexture", fun_glBindTexture, "fun Int GlTexture -> Int" },
		{ NATIVE_FUN, "glTexImage2D", fun_glTexImage2D, "fun GlTexture Int Int Int Int Bitmap -> Int" },
		{ NATIVE_FUN, "glTexSubImage2D", fun_glTexSubImage2D, "fun GlTexture Int Int Int Int Int Int Bitmap -> Int" },
		{ NATIVE_FUN, "glTexImage2DUpdate", fun_glTexImage2DUpdate, "fun GlTexture Bytes Int Int Int Int Int Int Bitmap -> Int" },
		{ NATIVE_FUN, "floatsFromArray", fun_floatsFromArray, "fun array Float -> Floats" },
		{ NATIVE_FUN, "floatsLength", fun_floatsLength, "fun Floats -> Int" },
		{ NATIVE_FUN, "floatsGet", fun_floatsGet, "fun Floats Int -> Float" },
	};
	NATIVE_DEF(nativeDefs);

	return 0;
}
#else
int fun_glMakeContext(Thread* th) FUN_RETURN_NIL
int fun_glRefreshContext(Thread* th) FUN_RETURN_NIL

int systemUiGLInit(Pkg* system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "_glMakeContext", fun_glMakeContext, "fun -> Int" },
		{ NATIVE_FUN, "_glRefreshContext", fun_glRefreshContext, "fun -> Int" },
	};
	NATIVE_DEF(nativeDefs);

	return 0;
}
#endif
