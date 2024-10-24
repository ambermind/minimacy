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

#ifndef _HW_FS_H_
#define _HW_FS_H_

#ifndef MAX_PATH
#define MAX_PATH 1024
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

LB* ansiReadContent(char* path, int* size);
LINT ansiDirectoryList(Buffer* out, char* dir);
int ansiVolumeList(Thread* th, int* n);
void ansiHelpBiosFinder(void);
int ansiFsMount(const char* argv0, int standalone);
void ansiFsInit(void);
void ansiFsRelease(void);

void uefiFileClose(void* file);
void* uefiFileOpen(int index, char* name, Def* mode);
int uefiFileSize(void* file);
int uefiFileTell(void* file);
LINT uefiFileSeek(void* file, LINT offset);
LINT uefiFileWrite(void* file, char* start, LINT len);
LINT uefiFileRead(void* file, char* start, LINT len);
int uefiFileDelete(int index, char* path);
int uefiDirDelete(int index, char* path);

LB* uefiReadContent(int index, char* path, int* size);
LINT uefiDirectoryList(int index, Buffer* out, char* dir);
int uefiVolumeList(Thread* th, int* n);
int uefiFsMount(int standalone);
void uefiFsInit(void);
void uefiFsRelease(void);

int romdiskVolumeList(Thread* th, int* n);
LB* romdiskReadContent(int romdiskId, char* path, int* size);
LINT romdiskDirectoryList(int romdiskId, Buffer* out, char* dir);
int romdiskImport(char* src, LINT len);
int romdiskMount(int standalone);
void romdiskReleaseUserDisk(void);
void romdiskInit(void);
void romdiskRelease(void);

void _fsAddFileInfo(Buffer* out, char* path, LINT len, char* tmpAttr);
LB* fsReadPackage(char* pkg, int* size, int verbose);

int _partitionAdd(LB* type, LINT index, char* physicalPath);
int _volumeList(Thread* th, LB* type, LINT index, int writable);
int volumeList(Thread* th);

char* fsCurrentDir(void);
char* fsUserDir(void);
int fsMount(const char* argv0, int standalone);
void fsInit(void);
void fsRelease(void);


#endif //_FS_H_
