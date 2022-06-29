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
#include "minimacy.h"

char* gzipDeflate(int mode, char* src, int len, int* outlen, void* fmalloc, void* ffree, void* user);
char* gzipInflate(int mode, char* src, int len, int* outlen, void* fmalloc, void* ffree, void* user);
unsigned int mcrc32(unsigned int crc, unsigned char* buf, int size);
unsigned int madler32(unsigned int adler, unsigned char* buf, int size);

void* mzcalloc(void* opaque, unsigned items, unsigned size)
{
	Thread* th = (Thread*)opaque;
	return workerAllocStr(th, items * size);
}

void mzcfree(void* opaque, void* ptr)
{
	//	printf("MZCFREE %llx %llx\n", opaque, ptr);
	//	free(ptr);
}
MTHREAD_START _deflate(Thread* th)
{
	int len;
	char* q;
	LB* out;
	LW result = NIL;

	LINT mode=VALTOINT(STACKGET(th, 0));
	LB* a = VALTOPNT(STACKGET(th, 1));
	if (!a) goto cleanup;
	q = gzipDeflate((int)mode, STRSTART(a), (int)STRLEN(a), &len, mzcalloc, mzcfree, th);
	out = memoryAllocStr(th, q, len); if (!out) goto cleanup;
	result=PNTTOVAL(out);
cleanup:
	return workerDone(th, result);
}
int fun_deflate(Thread* th) { return workerStart(th, 2, _deflate); }
MTHREAD_START _inflate(Thread* th)
{
	int len;
	char* q;
	LB* out;
	LW result = NIL;

	LINT mode = VALTOINT(STACKGET(th, 0));
	LB* a = VALTOPNT(STACKGET(th, 1));
	if (!a) goto cleanup;
	q = gzipInflate((int)mode, STRSTART(a), (int)STRLEN(a), &len, mzcalloc, mzcfree, th);
	out = memoryAllocStr(th, q, len); if (!out) goto cleanup;
	result = PNTTOVAL(out);
cleanup:
	return workerDone(th, result);
}
int fun_inflate(Thread* th) { return workerStart(th, 2, _inflate); }

int fun_crc32(Thread* th)
{
	
	LINT v;
	int NDROP = 2 - 1;
	LW result = NIL;

	LINT crc = VALTOINT(STACKGET(th, 0));
	LB* a = VALTOPNT(STACKGET(th, 1));
	if (!a) goto cleanup;
	v = (LINT)mcrc32((unsigned int)crc, (unsigned char*)STRSTART(a), (int)STRLEN(a));
	result=INTTOVAL(v & 0xffffffff);
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}

int fun_adler32(Thread* th)
{

	LINT v;
	int NDROP = 2 - 1;
	LW result = NIL;

	LINT crc = VALTOINT(STACKGET(th, 0));
	LB* a = VALTOPNT(STACKGET(th, 1));
	if (!a) goto cleanup;
	v = (LINT)madler32((unsigned int)crc, (unsigned char*)STRSTART(a), (int)STRLEN(a));
	result = INTTOVAL(v & 0xffffffff);
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}

int coreGzipInit(Thread* th, Pkg* system)
{
	Type* fun_S_I_S = typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.S, MM.I, MM.S);
	Type* fun_S_I_I = typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.S, MM.I, MM.I);

	pkgAddFun(th, system, "_deflate", fun_deflate, fun_S_I_S);
	pkgAddFun(th, system, "_inflate", fun_inflate, fun_S_I_S);
	pkgAddFun(th, system, "crc32", fun_crc32, fun_S_I_I);
	pkgAddFun(th, system, "adler32", fun_adler32, fun_S_I_I);
	return 0;
}
