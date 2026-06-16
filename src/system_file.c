// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
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

int fun_ansiFileOpen(Thread* th)
{
	File* f;
	char* mode;
	void* file;
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
	if (!file) FUN_RETURN_NIL;

	f = (File*)memoryAllocNative(sizeof(File), DBG_BIN, _ansiFileForget, NULL); if (!f) return EXEC_OM;
	f->file = file;
	FUN_RETURN_PNT((LB*)f);
}

int fun_ansiFileSize(Thread* th) 
{
	File* f = (File*)STACK_PNT(th, 0);
	if (!f) FUN_RETURN_NIL;
	FUN_RETURN_INT(ansiFileSize(f->file));
}

WORKER_START _ansiFileRead(Worker* w)
{
	Thread* th = workerThread(w);
	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT start = STACK_INT(th, 1);
	LB* src = STACK_PNT(th, 2);
	int seekIsNil = STACK_IS_NIL(th,3);
	LINT seek = STACK_INT(th, 3);
	File* f = (File*)STACK_PNT(th, 4);
	if (!f) return workerDoneNil(w);
	if ((!seekIsNil)&&(seek >= 0)) ansiFileSeek(f->file, seek);
	WORKER_SUBSTR(src, start, len, lenIsNil, STR_LENGTH(src));
	return workerDoneInt(w, ansiFileRead(f->file, STR_START(src)+start, len));
}
int fun_ansiFileRead(Thread* th) { return workerStart(th, 5, _ansiFileRead); }

WORKER_START _ansiFileWrite(Worker* w)
{
	Thread* th = workerThread(w);
	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT start = (STACK_INT(th, 1));
	LB* dst = (STACK_PNT(th, 2));
	int seekIsNil = STACK_IS_NIL(th,3);
	LINT seek = STACK_INT(th, 3);
	File* f = (File*)STACK_PNT(th, 4);
	if (!f) return workerDoneNil(w);
	if ((!seekIsNil) && (seek >= 0)) ansiFileSeek(f->file, seek);
	WORKER_SUBSTR(dst, start, len, lenIsNil, STR_LENGTH(dst));
	return workerDoneInt(w,ansiFileWrite(f->file, STR_START(dst) + start, len));
}
int fun_ansiFileWrite(Thread* th) { return workerStart(th, 5, _ansiFileWrite); }

int fun_ansiFileClose(Thread* th)
{
	File* f = (File*)STACK_PNT(th, 0);
	if (!f) FUN_RETURN_NIL;
	ansiFileClose(f->file);
	f->file=NULL;
	FUN_RETURN_TRUE;
}

WORKER_START _ansiDiskList(Worker* w)
{
	LINT fixedRootBuffer;
	Thread* th = workerThread(w);
	LINT result;
	LB* path = STACK_PNT(th, 0);
	Buffer* out = (Buffer*)STACK_PNT(th, 1);
	if ((!path) || (!out)) return workerDoneNil(w);
	bufferSetWorker(out, w);
	fixedRootBuffer = fixedRootAlloc((LB*)out);
	result = ansiDirectoryList(fixedRootBuffer, STR_START(path));
	fixedRootRelease(fixedRootBuffer);
	return workerDoneInt(w, result);
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
	if (f) STACK_SET_INT(th, 0, (LINT)ansiFileTell(f->file));
	return 0;
}
int fun_devNbSectors(Thread* th)
{
	LB* path = STACK_PNT(th, 0);
	if (!path) FUN_RETURN_NIL;
	long long nbSectors=devNbSectors(STR_START(path));
	if (nbSectors<0) FUN_RETURN_NIL;
	FUN_RETURN_INT(nbSectors);
}
int fun_devSectorSize(Thread* th)
{
	LB* path = STACK_PNT(th, 0);
	if (!path) FUN_RETURN_NIL;
	int sectorSize=devSectorSize(STR_START(path));
	if (sectorSize<0) FUN_RETURN_NIL;
	FUN_RETURN_INT((LINT)sectorSize);
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
int fun_devNbSectors(Thread* th) FUN_RETURN_NIL
int fun_devSectorSize(Thread* th) FUN_RETURN_NIL
#endif

int fun_romdiskLoad(Thread* th)
{
	LB* content;
	
	LB* path = STACK_PNT(th, 0);
	LINT romdiskId = STACK_INT(th, 1);
	if ((!path) || (romdiskId < 0)) FUN_RETURN_NIL;
	content = romdiskReadContent((int)romdiskId, STR_START(path), NULL);
	FUN_RETURN_PNT(content);
}

int fun_romdiskList(Thread* th)
{
	LB* path = STACK_PNT(th, 0);
	Buffer* out = (Buffer*)STACK_PNT(th, 1);
	LINT romdiskId = STACK_INT(th, 2);
	if ((!path) || (!out)) FUN_RETURN_NIL;
	FUN_RETURN_INT(romdiskDirectoryList((int)romdiskId, out, STR_START(path)));
}
int fun_romdiskImport(Thread* th)
{
	int k;
	LB* bin = STACK_PNT(th, 0);
	if (!bin) FUN_RETURN_NIL;
	k = romdiskImport(bin);
	if (k < 0) FUN_RETURN_NIL;

	FUN_RETURN_INT((LINT)k);
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

int systemFileInit(Pkg *system)
{
	Def* FileMode = pkgAddSum(system, "FileMode");
	Def* VolumeType = pkgAddSum(system, "VolumeType");
	pkgAddType(system, "_AnsiFile");

	MM.ansiVolume = (LB*)pkgAddCons0(system, "ansiVolume", VolumeType);
	MM.romdiskVolume = (LB*)pkgAddCons0(system, "romdiskVolume", VolumeType);

	FM.READ_ONLY = pkgAddCons0(system, "FILE_READ_ONLY", FileMode);
	FM.REWRITE = pkgAddCons0(system, "FILE_REWRITE", FileMode);
	FM.READ_WRITE = pkgAddCons0(system, "FILE_READ_WRITE", FileMode);
	FM.APPEND = pkgAddCons0(system, "FILE_APPEND", FileMode);

	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "_volumes", fun_volumes, "fun -> list [VolumeType Int Bool Int Int]"},
		{ NATIVE_FUN, "_partitions", fun_partitions, "fun -> list [VolumeType Int Str]" },
		{ NATIVE_FUN, "_setPartitions", fun_setPartitions, "fun list [VolumeType Int Str] -> list [VolumeType Int Str]" },
		{ NATIVE_FUN, "_ansiFileOpen", fun_ansiFileOpen, "fun Str FileMode -> _AnsiFile" },
		{ NATIVE_FUN, "_ansiFileSize", fun_ansiFileSize, "fun _AnsiFile -> Int" },
		{ NATIVE_FUN, "_ansiFileTell", fun_ansiFileTell, "fun _AnsiFile -> Int" },
		{ NATIVE_FUN, "_ansiFileRead", fun_ansiFileRead, "fun _AnsiFile Int Bytes Int Int Int -> Int" },
		{ NATIVE_FUN, "_ansiFileWrite", fun_ansiFileWrite, "fun _AnsiFile Int Str Int Int Int -> Int" },
		{ NATIVE_FUN, "_ansiFileClose", fun_ansiFileClose, "fun _AnsiFile -> Bool" },
		{ NATIVE_FUN, "_ansiDiskList", fun_ansiDiskList, "fun Buffer Str Int -> Int" },
		{ NATIVE_FUN, "_ansiFileDelete", fun_ansiFileDelete, "fun Str -> Bool" },
		{ NATIVE_FUN, "_ansiDirDelete", fun_ansiDirDelete, "fun Str -> Bool" },
		{ NATIVE_FUN, "devNbSectors", fun_devNbSectors, "fun Str -> Int" },
		{ NATIVE_FUN, "devSectorSize", fun_devSectorSize, "fun Str -> Int" },
		{ NATIVE_FUN, "_romdiskLoad", fun_romdiskLoad, "fun Int Str -> Str" },
		{ NATIVE_FUN, "_romdiskList", fun_romdiskList, "fun Int Buffer Str -> Int" },
		{ NATIVE_FUN, "_romdiskImport", fun_romdiskImport, "fun Str -> Int" },
	};
	NATIVE_DEF(nativeDefs);

	fsInit();
	pkgAddConstPnt(system, "_currentDir", memoryAllocStr(fsCurrentDir(), -1), MM.Str);
	pkgAddConstPnt(system, "userDir", memoryAllocStr(fsUserDir(), -1), MM.Str);


	return 0;
}
