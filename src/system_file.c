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

typedef struct {
	Ref* READ_ONLY;
	Ref* REWRITE;
	Ref* READ_WRITE;
	Ref* APPEND;
}FileModes;
FileModes FM;

int _fileForget(LB* p)
{
	File* f = (File*)p;
	if (f->file) fileClose(f->file);
	f->file=NULL;
	return 0;
}

File* _fileCreate(Thread* th, void* file)
{
	File* f = (File*)workerAllocExt(th, sizeof(File), DBG_BIN, _fileForget, NULL); if (!f) return NULL;
	f->file = file;
	return f;
}

MTHREAD_START _fileOpen(Thread* th)
{
	LB* result=NULL;
	char* mode;
	void* file;
	LB* pmode = VALTOPNT(STACKGET(th, 0));
	LB* name = VALTOPNT(STACKGET(th, 1));

	mode = "rb";
	if (pmode == (LB*)FM.REWRITE) mode = "wb+";
	else if (pmode == (LB*)FM.READ_WRITE) mode = "rb+";
	else if (pmode == (LB*)FM.APPEND) mode = "ab+";

	file = fileOpen(STRSTART(name), mode);
	if ((!file) && ((!strcmp(mode, "rb+")) || (!strcmp(mode, "ab+"))))
	{
		mode = "wb+";
		file = fileOpen(STRSTART(name), mode);
	}
	if ((!file) && (strcmp(mode, "rb")))
	{
		if (!fileMakedirs(STRSTART(name))) file = fileOpen(STRSTART(name), mode);
	}
	if (file) result = (LB*)_fileCreate(th, file);
	return workerDone(th, PNTTOVAL(result));
}
int fun_fileOpen(Thread* th) { return workerStart(th, 2, _fileOpen); }

MTHREAD_START _fileSize(Thread* th)
{
	LW result = NIL;
	File* f = (File*)VALTOPNT(STACKGET(th, 0));
	if (f) result = INTTOVAL(fileSize(f->file));
	return workerDone(th, result);
}
int fun_fileSize(Thread* th) { return workerStart(th, 1, _fileSize); }

MTHREAD_START _fileRead(Thread* th)
{
	LINT len, seek;
	LW result = NIL;
	LW wlen = STACKGET(th, 0);
	LINT start = VALTOINT(STACKGET(th, 1));
	LB* src = VALTOPNT(STACKGET(th, 2));
	LW wseek = STACKGET(th, 3);
	File* f = (File*)VALTOPNT(STACKGET(th, 4));
	if ((!f) || (!src) || (start < 0)) goto cleanup;
	seek = (wseek == NIL) ? -1 : VALTOINT(wseek);
	len = (wlen == NIL) ? STRLEN(src)-start : VALTOINT(wlen);
	if (seek >= 0) fileSeek(f->file, seek);
	if (len>0) result=INTTOVAL(fileRead(f->file, STRSTART(src)+start, len));
cleanup:
	return workerDone(th, result);
}
int fun_fileRead(Thread* th) { return workerStart(th, 5, _fileRead); }

MTHREAD_START _fileWrite(Thread* th)
{
	LINT len, seek;
	LW result = NIL;
	LW wlen = STACKGET(th, 0);
	LINT start = VALTOINT(STACKGET(th, 1));
	LB* src = VALTOPNT(STACKGET(th, 2));
	LW wseek = STACKGET(th, 3);
	File* f = (File*)VALTOPNT(STACKGET(th, 4));
	if ((!f) || (!src) || (start < 0)) goto cleanup;
	seek = (wseek == NIL) ? -1 : VALTOINT(wseek);
	len = (wlen == NIL) ? STRLEN(src) - start : VALTOINT(wlen);
	if (seek >= 0) fileSeek(f->file, seek);
	if (len > 0) result = INTTOVAL(fileWrite(f->file, STRSTART(src) + start, len));
cleanup:
	return workerDone(th, result);
}
int fun_fileWrite(Thread* th) { return workerStart(th, 5, _fileWrite); }

MTHREAD_START _fileClose(Thread* th)
{
	LW result = NIL;
	File* f = (File*)VALTOPNT(STACKGET(th, 0));
	if (f)
	{
		fileClose(f->file);
		f->file=NULL;
		result = MM.trueRef;
	}
	return workerDone(th, result);
}
int fun_fileClose(Thread* th) { return workerStart(th, 1, _fileClose); }

MTHREAD_START _diskList(Thread* th)
{
	LW result = NIL;

	LB* bytes = VALTOPNT(STACKGET(th, 0));
	LB* path = VALTOPNT(STACKGET(th, 1));
	if ((!path) || (!bytes)) goto cleanup;
	result=INTTOVAL(hwDiskList(STRSTART(path),STRSTART(bytes),STRLEN(bytes)));

cleanup:
	return workerDone(th, result);
}

int fun_diskList(Thread* th) { return workerStart(th, 2, _diskList); }

int sysFileInit(Thread* th, Pkg *system)
{
	Ref* FileMode = pkgAddSum(th, system, "FileMode");
	Ref* File = pkgAddType(th, system, "File");
	Type* fun_File_I = typeAlloc(th, TYPECODE_FUN, NULL, 2, File->type, MM.I);
	Type* fun_File_B = typeAlloc(th, TYPECODE_FUN, NULL, 2, File->type, MM.Boolean);
	Type* fun_S_FM_File = typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.S,FileMode->type, File->type);
	Type* fun_File_I_B_I_I_I = typeAlloc(th, TYPECODE_FUN, NULL, 6, File->type, MM.I, MM.Bytes, MM.I, MM.I, MM.I);
	Type* fun_File_I_S_I_I_I = typeAlloc(th, TYPECODE_FUN, NULL, 6, File->type, MM.I, MM.S, MM.I, MM.I, MM.I);
	Type* fun_S_Bytes_I = typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.S, MM.Bytes, MM.I);

	FM.READ_ONLY = pkgAddCons0(th, system, "FILE_READ_ONLY", FileMode);
	FM.REWRITE = pkgAddCons0(th, system, "FILE_REWRITE", FileMode);
	FM.READ_WRITE = pkgAddCons0(th, system, "FILE_READ_WRITE", FileMode);
	FM.APPEND = pkgAddCons0(th, system, "FILE_APPEND", FileMode);

	pkgAddFun(th, system, "_fileOpen", fun_fileOpen, fun_S_FM_File);
	pkgAddFun(th, system, "_fileSize", fun_fileSize, fun_File_I);
	pkgAddFun(th, system, "_fileRead", fun_fileRead, fun_File_I_B_I_I_I);
	pkgAddFun(th, system, "_fileWrite", fun_fileWrite, fun_File_I_S_I_I_I);
	pkgAddFun(th, system, "_fileClose", fun_fileClose, fun_File_B);

	pkgAddFun(th, system, "_diskList", fun_diskList, fun_S_Bytes_I);

	return 0;
}
