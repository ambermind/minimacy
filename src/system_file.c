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

FileModes FM;

#ifdef USE_FS_ANSI
int _ansiFileForget(LB* p)
{
	File* f = (File*)p;
	if (f->file) ansiFileClose(f->file);
	f->file=NULL;
	return 0;
}

File* _ansiFileCreate(Thread* th, void* file)
{
	File* f = (File*)workerAllocExt(th, sizeof(File), DBG_BIN, _ansiFileForget, NULL); if (!f) return NULL;
	f->file = file;
	return f;
}

MTHREAD_START _ansiFileOpen(Thread* th)
{
	char* mode;
	void* file;
	LB* result;
	LB* pmode = STACK_PNT(th, 0);
	LB* name = STACK_PNT(th, 1);

	mode = "rb";
	if (pmode == (LB*)FM.REWRITE) mode = "wb+";
	else if (pmode == (LB*)FM.READ_WRITE) mode = "rb+";
	else if (pmode == (LB*)FM.APPEND) mode = "ab+";

	file = ansiFileOpen(STR_START(name), mode);
	if ((!file) && ((!strcmp(mode, "rb+")) || (!strcmp(mode, "ab+"))))
	{
		mode = "wb+";
		file = ansiFileOpen(STR_START(name), mode);
	}
	if ((!file) && (strcmp(mode, "rb")))
	{
		if (!ansiCreateDirs(STR_START(name))) file = ansiFileOpen(STR_START(name), mode);
	}
	if (!file) return workerDoneNil(th);
	result=(LB*)_ansiFileCreate(th, file);	if (!result) workerDoneNil(th);
	return workerDonePnt(th, result);
}
int fun_ansiFileOpen(Thread* th) { return workerStart(th, 2, _ansiFileOpen); }

MTHREAD_START _ansiFileSize(Thread* th)
{
	File* f = (File*)STACK_PNT(th, 0);
	if (f) return workerDoneInt(th,ansiFileSize(f->file));
	return workerDoneNil(th);
}
int fun_ansiFileSize(Thread* th) { return workerStart(th, 1, _ansiFileSize); }

MTHREAD_START _ansiFileRead(Thread* th)
{
	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT start = STACK_INT(th, 1);
	LB* src = STACK_PNT(th, 2);
	int seekIsNil = STACK_IS_NIL(th,3);
	LINT seek = STACK_INT(th, 3);
	File* f = (File*)STACK_PNT(th, 4);
	if (!f) return workerDoneNil(th);
	if ((!seekIsNil)&&(seek >= 0)) ansiFileSeek(f->file, seek);
	WORKER_SUBSTR(src, start, len, lenIsNil, STR_LENGTH(src));
	return workerDoneInt(th, ansiFileRead(f->file, STR_START(src)+start, len));
}
int fun_ansiFileRead(Thread* th) { return workerStart(th, 5, _ansiFileRead); }

MTHREAD_START _ansiFileWrite(Thread* th)
{
	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT start = (STACK_INT(th, 1));
	LB* dst = (STACK_PNT(th, 2));
	int seekIsNil = STACK_IS_NIL(th,3);
	LINT seek = STACK_INT(th, 3);
	File* f = (File*)STACK_PNT(th, 4);
	if (!f) return workerDoneNil(th);
	if ((!seekIsNil) && (seek >= 0)) ansiFileSeek(f->file, seek);
	WORKER_SUBSTR(dst, start, len, lenIsNil, STR_LENGTH(dst));
	return workerDoneInt(th,ansiFileWrite(f->file, STR_START(dst) + start, len));
}
int fun_ansiFileWrite(Thread* th) { return workerStart(th, 5, _ansiFileWrite); }

MTHREAD_START _ansiFileClose(Thread* th)
{
	File* f = (File*)STACK_PNT(th, 0);
	if (!f) return workerDoneNil(th);
	ansiFileClose(f->file);
	f->file=NULL;
	return workerDonePnt(th, MM._true);
}
int fun_ansiFileClose(Thread* th) { return workerStart(th, 1, _ansiFileClose); }

MTHREAD_START _ansiDiskList(Thread* th)
{
	LB* path = STACK_PNT(th, 0);
	Buffer* out = (Buffer*)STACK_PNT(th, 1);
	if ((!path) || (!out)) return workerDoneNil(th);
	return workerDoneInt(th,ansiDirectoryList(th,out,STR_START(path)));
}
int fun_ansiDiskList(Thread* th) { return workerStart(th, 2, _ansiDiskList); }

int fun_ansiFileDelete(Thread* th)
{
	LB* path = STACK_PNT(th, 0);
	if (!path) FUN_RETURN_NIL;
	FUN_RETURN_BOOL(ansiFileDelete(STR_START(path)));
}
int fun_ansiDirDelete(Thread* th)
{
	LB* path = STACK_PNT(th, 0);
	if (!path) FUN_RETURN_NIL;
	FUN_RETURN_BOOL(ansiDirDelete(STR_START(path)));
}
int fun_ansiFileTell(Thread* th)
{
	File* f = (File*)STACK_PNT(th, 0);
	if (f) STACK_SET_INT(th, 0, ansiFileTell(f->file));
	return 0;
}
#else
int fun_ansiFileOpen(Thread* th) FUN_RETURN_NIL
int fun_ansiFileSize(Thread* th) FUN_RETURN_NIL
int fun_ansiFileTell(Thread* th) FUN_RETURN_NIL
int fun_ansiFileRead(Thread* th) FUN_RETURN_NIL
int fun_ansiFileWrite(Thread* th) FUN_RETURN_NIL
int fun_ansiFileClose(Thread* th) FUN_RETURN_NIL
int fun_ansiFileDelete(Thread* th) FUN_RETURN_NIL
int fun_ansiDirDelete(Thread* th) FUN_RETURN_NIL
int fun_ansiDiskList(Thread* th) FUN_RETURN_NIL
#endif

#ifdef USE_FS_UEFI
int _uefiFileForget(LB* p)
{
	File* f = (File*)p;
	if (f->file) uefiFileClose(f->file);
	f->file=NULL;
	return 0;
}

File* _uefiFileCreate(Thread* th, void* file)
{
	File* f = (File*)memoryAllocExt(th, sizeof(File), DBG_BIN, _uefiFileForget, NULL); if (!f) return NULL;
	f->file = file;
	return f;
}

int fun_uefiFileOpen(Thread* th)
{
	void* file;
	LB* result;
	Def* mode = (Def*)STACK_PNT(th, 0);
	LB* name = STACK_PNT(th, 1);
	LINT index= STACK_INT(th, 2);

	file = uefiFileOpen(index,STR_START(name), mode);
	if (!file) FUN_RETURN_NIL;
	result=(LB*)_uefiFileCreate(th, file);	if (!result) return EXEC_OM;
	FUN_RETURN_PNT(result);
}

int fun_uefiFileRead(Thread* th)
{
	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT start = STACK_INT(th, 1);
	LB* src = STACK_PNT(th, 2);
	int seekIsNil = STACK_IS_NIL(th,3);
	LINT seek = STACK_INT(th, 3);
	File* f = (File*)STACK_PNT(th, 4);
	if (!f) FUN_RETURN_NIL;
	if ((!seekIsNil)&&(seek >= 0)) uefiFileSeek(f->file, seek);
	FUN_SUBSTR(src, start, len, lenIsNil, STR_LENGTH(src));
	FUN_RETURN_INT(uefiFileRead(f->file, STR_START(src)+start, len));
}

int fun_uefiFileWrite(Thread* th)
{
	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT start = (STACK_INT(th, 1));
	LB* dst = (STACK_PNT(th, 2));
	int seekIsNil = STACK_IS_NIL(th,3);
	LINT seek = STACK_INT(th, 3);
	File* f = (File*)STACK_PNT(th, 4);
	if (!f) FUN_RETURN_NIL;
	if ((!seekIsNil) && (seek >= 0)) uefiFileSeek(f->file, seek);
	FUN_SUBSTR(dst, start, len, lenIsNil, STR_LENGTH(dst));
	FUN_RETURN_INT(uefiFileWrite(f->file, STR_START(dst) + start, len));
}

int fun_uefiFileSize(Thread* th) 
{
	File* f = (File*)STACK_PNT(th, 0);
	if (!f) FUN_RETURN_NIL;
	FUN_RETURN_INT(uefiFileSize(f->file));
}

int fun_uefiFileTell(Thread* th) 
{
	File* f = (File*)STACK_PNT(th, 0);
	if (!f) FUN_RETURN_NIL;
	FUN_RETURN_INT(uefiFileTell(f->file));
}

int fun_uefiFileClose(Thread* th) 
{
	File* f = (File*)STACK_PNT(th, 0);
	if (!f) FUN_RETURN_NIL;
	uefiFileClose(f->file);
	f->file=NULL;
	FUN_RETURN_BOOL(1);
}

int fun_uefiDiskList(Thread* th)
{
	LB* path = STACK_PNT(th, 0);
	Buffer* out = (Buffer*)STACK_PNT(th, 1);
	LINT index= STACK_INT(th,2);
	if ((!path) || (!out)) FUN_RETURN_NIL;
	FUN_RETURN_INT(uefiDirectoryList(th,index,out,STR_START(path)));
}

int fun_uefiFileDelete(Thread* th)
{
	LB* path = STACK_PNT(th, 0);
	LINT index= STACK_INT(th,1);
	if (!path) FUN_RETURN_NIL;
	FUN_RETURN_BOOL(uefiFileDelete(index,STR_START(path)));
}
int fun_uefiDirDelete(Thread* th)
{
	LB* path = STACK_PNT(th, 0);
	LINT index= STACK_INT(th,1);
	if (!path) FUN_RETURN_NIL;
	FUN_RETURN_BOOL(uefiDirDelete(index,STR_START(path)));
}

#else
int fun_uefiFileOpen(Thread* th) FUN_RETURN_NIL
int fun_uefiFileRead(Thread* th) FUN_RETURN_NIL
int fun_uefiFileWrite(Thread* th) FUN_RETURN_NIL
int fun_uefiFileSize(Thread* th) FUN_RETURN_NIL
int fun_uefiFileTell(Thread* th) FUN_RETURN_NIL
int fun_uefiFileClose(Thread* th) FUN_RETURN_NIL
int fun_uefiDiskList(Thread* th) FUN_RETURN_NIL
int fun_uefiFileDelete(Thread* th) FUN_RETURN_NIL
int fun_uefiDirDelete(Thread* th) FUN_RETURN_NIL
#endif

int fun_romdiskLoad(Thread* th)
{
	LB* content;
	
	LB* path = STACK_PNT(th, 0);
	LINT romdiskId = STACK_INT(th, 1);
	if ((!path) || (romdiskId < 0)) FUN_RETURN_NIL;
	content = romdiskReadContent(th, (int)romdiskId, STR_START(path), NULL);
	FUN_RETURN_PNT(content);
}

int fun_romdiskList(Thread* th)
{
	LB* path = STACK_PNT(th, 0);
	Buffer* out = (Buffer*)STACK_PNT(th, 1);
	LINT romdiskId = STACK_INT(th, 2);
	if ((!path) || (!out)) FUN_RETURN_NIL;
	FUN_RETURN_INT(romdiskDirectoryList(th, (int)romdiskId, out, STR_START(path)));
}
int fun_romdiskImport(Thread* th)
{
	int k;
	LB* src = STACK_PNT(th, 0);
	if (!src) FUN_RETURN_NIL;
	k = romdiskImport(STR_START(src), STR_LENGTH(src));
	if (k < 0) FUN_RETURN_NIL;

	FUN_RETURN_INT(k);
}

int fun_volumes(Thread* th)
{
	return volumeList(th);
}
int fun_partitions(Thread* th)
{
	FUN_RETURN_PNT(MM.partitionsFS);
}
int fun_setPartitions(Thread* th)
{
	MM.partitionsFS = STACK_PNT(th, 0);
	return 0;
}

int sysFileInit(Thread* th, Pkg *system)
{
	Def* FileMode = pkgAddSum(th, system, "FileMode");
	Def* AnsiFile = pkgAddType(th, system, "_AnsiFile");
	Def* UefiFile = pkgAddType(th, system, "_UefiFile");
	Def* VolumeType = pkgAddSum(th, system, "VolumeType");

	Type* fun_AFile_I = typeAlloc(th, TYPECODE_FUN, NULL, 2, AnsiFile->type, MM.Int);
	Type* fun_AFile_B = typeAlloc(th, TYPECODE_FUN, NULL, 2, AnsiFile->type, MM.Boolean);
	Type* fun_S_FM_AFile = typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.Str,FileMode->type, AnsiFile->type);
	Type* fun_AFile_I_B_I_I_I = typeAlloc(th, TYPECODE_FUN, NULL, 6, AnsiFile->type, MM.Int, MM.Bytes, MM.Int, MM.Int, MM.Int);
	Type* fun_AFile_I_S_I_I_I = typeAlloc(th, TYPECODE_FUN, NULL, 6, AnsiFile->type, MM.Int, MM.Str, MM.Int, MM.Int, MM.Int);

	Type* fun_UFile_I = typeAlloc(th, TYPECODE_FUN, NULL, 2, UefiFile->type, MM.Int);
	Type* fun_UFile_B = typeAlloc(th, TYPECODE_FUN, NULL, 2, UefiFile->type, MM.Boolean);
	Type* fun_I_S_FM_UFile = typeAlloc(th, TYPECODE_FUN, NULL, 4, MM.Int, MM.Str,FileMode->type, UefiFile->type);
	Type* fun_UFile_I_B_I_I_I = typeAlloc(th, TYPECODE_FUN, NULL, 6, UefiFile->type, MM.Int, MM.Bytes, MM.Int, MM.Int, MM.Int);
	Type* fun_UFile_I_S_I_I_I = typeAlloc(th, TYPECODE_FUN, NULL, 6, UefiFile->type, MM.Int, MM.Str, MM.Int, MM.Int, MM.Int);

	Type* fun_S_I = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.Str, MM.Int);
	Type* fun_I_S_Bool = typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.Int, MM.Str, MM.Boolean);
	Type* fun_B_S_I = typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.Buffer, MM.Str, MM.Int);
	Type* fun_I_B_S_I = typeAlloc(th, TYPECODE_FUN, NULL, 4, MM.Int, MM.Buffer, MM.Str, MM.Int);
	Type* fun_I_S_S = typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.Int, MM.Str, MM.Str);
	Type* fun_S_Bool = typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.Str, MM.Boolean);
	Type* list_S_FS_I_Boolean = typeAlloc(th, TYPECODE_LIST, NULL, 1,
		typeAlloc(th, TYPECODE_TUPLE, NULL, 4, MM.Str, VolumeType->type, MM.Int, MM.Boolean));
	Type* list_FS_I_S = typeAlloc(th, TYPECODE_LIST, NULL, 1,
		typeAlloc(th, TYPECODE_TUPLE, NULL, 3, VolumeType->type, MM.Int, MM.Str));

	MM.VolumeType = VolumeType->type;
	MM.ansiVolume = (LB*)pkgAddCons0(th, system, "ansiVolume", VolumeType);
	MM.uefiVolume = (LB*)pkgAddCons0(th, system, "uefiVolume", VolumeType);
	MM.romdiskVolume = (LB*)pkgAddCons0(th, system, "romdiskVolume", VolumeType);

	FM.READ_ONLY = pkgAddCons0(th, system, "FILE_READ_ONLY", FileMode);
	FM.REWRITE = pkgAddCons0(th, system, "FILE_REWRITE", FileMode);
	FM.READ_WRITE = pkgAddCons0(th, system, "FILE_READ_WRITE", FileMode);
	FM.APPEND = pkgAddCons0(th, system, "FILE_APPEND", FileMode);

	pkgAddFun(th, system, "_volumes", fun_volumes, typeAlloc(th, TYPECODE_FUN, NULL, 1, list_S_FS_I_Boolean));
	pkgAddFun(th, system, "_partitions", fun_partitions, typeAlloc(th, TYPECODE_FUN, NULL, 1, list_FS_I_S));
	pkgAddFun(th, system, "_setPartitions", fun_setPartitions, typeAlloc(th, TYPECODE_FUN, NULL, 2, list_FS_I_S, list_FS_I_S));

	pkgAddConstPnt(th, system, "_currentDir", memoryAllocStr(th, fsCurrentDir(), -1), MM.Str);
	pkgAddConstPnt(th, system, "userDir", memoryAllocStr(th, fsUserDir(), -1), MM.Str);

	pkgAddFun(th, system, "_ansiFileOpen", fun_ansiFileOpen, fun_S_FM_AFile);
	pkgAddFun(th, system, "_ansiFileSize", fun_ansiFileSize, fun_AFile_I);
	pkgAddFun(th, system, "_ansiFileTell", fun_ansiFileTell, fun_AFile_I);
	pkgAddFun(th, system, "_ansiFileRead", fun_ansiFileRead, fun_AFile_I_B_I_I_I);
	pkgAddFun(th, system, "_ansiFileWrite", fun_ansiFileWrite, fun_AFile_I_S_I_I_I);
	pkgAddFun(th, system, "_ansiFileClose", fun_ansiFileClose, fun_AFile_B);
	pkgAddFun(th, system, "_ansiDiskList", fun_ansiDiskList, fun_B_S_I);
	pkgAddFun(th, system, "_ansiFileDelete", fun_ansiFileDelete, fun_S_Bool);
	pkgAddFun(th, system, "_ansiDirDelete", fun_ansiDirDelete, fun_S_Bool);

	pkgAddFun(th, system, "_uefiFileOpen", fun_uefiFileOpen, fun_I_S_FM_UFile);
	pkgAddFun(th, system, "_uefiFileSize", fun_uefiFileSize, fun_UFile_I);
	pkgAddFun(th, system, "_uefiFileTell", fun_uefiFileTell, fun_UFile_I);
	pkgAddFun(th, system, "_uefiFileRead", fun_uefiFileRead, fun_UFile_I_B_I_I_I);
	pkgAddFun(th, system, "_uefiFileWrite", fun_uefiFileWrite, fun_UFile_I_S_I_I_I);
	pkgAddFun(th, system, "_uefiFileClose", fun_uefiFileClose, fun_UFile_B);
	pkgAddFun(th, system, "_uefiDiskList", fun_uefiDiskList, fun_I_B_S_I);
	pkgAddFun(th, system, "_uefiFileDelete", fun_uefiFileDelete, fun_I_S_Bool);
	pkgAddFun(th, system, "_uefiDirDelete", fun_uefiDirDelete, fun_I_S_Bool);

	pkgAddFun(th, system, "_romdiskLoad", fun_romdiskLoad, fun_I_S_S);
	pkgAddFun(th, system, "_romdiskList", fun_romdiskList, fun_I_B_S_I);
	pkgAddFun(th, system, "_romdiskImport", fun_romdiskImport, fun_S_I);

	return 0;
}
