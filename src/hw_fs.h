// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#ifndef _HW_FS_H_
#define _HW_FS_H_

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

#ifdef USE_FS_ANSI_UNIX
#ifdef USE_MACOS
#include <sys/disk.h>
#else
#include <sys/mount.h>
#endif
#endif

void systemCleanDir(char* p);
void systemRemoveLast(char* p);
void systemMainDir(char* path, int len, const char* argv0);
void systemUserDir(char* userDir, int len);
void systemCurrentDir(char* curDir, int len);

void ansiFileClose(void* file);
void* ansiFileOpen(char* name, char* mode);
int ansiFileSize(void* file);
int ansiFileTell(void* file);
LINT ansiFileSeek(void* file, LINT offset);
LINT ansiFileWrite(void* file, char* start, LINT len);
LINT ansiFileRead(void* file, char* start, LINT len);
int ansiCreateDirs(char* filename);
int ansiFileDelete(char* path);
int ansiDirDelete(char* path);
long long devNbSectors(char* path);
int devSectorSize(char* path);

LB* ansiReadContent(char* path, int* size);
LINT ansiDirectoryList(volatile Buffer** pout, char* dir);
int ansiVolumeList(Thread* th, int* n);
void ansiHelpBiosFinder(void);
int ansiFsMount(const char* argv0, int standalone);
void ansiFsInit(void);
void ansiFsRelease(void);

void romdiskMark(LB* user);
int romdiskVolumeList(Thread* th, int* n);
LB* romdiskReadContent(int romdiskId, char* path, int* size);
LINT romdiskDirectoryList(int romdiskId, Buffer* out, char* dir);
int romdiskImport(LB* bin);
int romdiskMount(int standalone);
void romdiskReleaseUserDisk(void);
void romdiskInit(void);
void romdiskRelease(void);

void _fsAddFileInfo(Buffer* out, char* path, LINT len, char* tmpAttr);
LB* fsReadPackage(char* pkg, int* size, int verbose);

int _partitionAdd(LB* type, LINT index, char* physicalPath);
int _volumeList(Thread* th, LB* type, LINT index, int writable, LINT nbSectors, LINT sectorSize);
int volumeList(Thread* th);

char* fsCurrentDir(void);
char* fsUserDir(void);
int fsMount(const char* argv0, int standalone);
void fsInit(void);
void fsRelease(void);


#endif //_FS_H_
