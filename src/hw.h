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
#ifndef _HW_
#define _HW_

#ifdef ON_UNIX
#define GetCurrentDirectoryA(a,b) getcwd(b,a)
#define MAX_PATH 1024

#endif
#ifdef ON_WINDOWS
#ifdef ATOMIC_32
#define LocalTickCount GetTickCount
#else
#define LocalTickCount GetTickCount64
#endif
#endif

#define LS_GETCHAR_WOULDBLOCK (-2)

int macDetect(void);

void systemExecDir(char* execDir, int len);
void systemEnvDir(char* envDir, int len);
void systemCurrentDir(char* curDir, int len);
void systemUserDir(char* userDir, int len);

void consoleWrite(int user, char* src, int len);
void consoleVPrint(int user, char* format, va_list arglist);

LB* fileReadPackage(Thread* th, char* pkg, char* suffix, int* size);

int fileExist(char* path);
void fileClose(void* file);
void* fileOpen(char* name, char* mode);
int fileSize(void* file);
LINT fileSeek(void* file, LINT offset);
LINT fileWrite(void* file, char* start, LINT len);
LINT fileRead(void* file, char* start, LINT len);
LB* fileReadAlloc(Thread* th, void* file, int maxSize);
int fileMakedirs(char* filename);

void lsRand(char* dst, LINT len);

LINT hwTime(void);
LINT hwTimeMs(void);
void hwTimeInit(void);

void hwSleepMs(LINT ms);


LINT hwDiskList(char* dir, char* buffer, LINT bufferLen);

char* hwRomDir(void);
char* hwSystemDir(void);
char* hwCurrentDir(void);
int hwSetSystemDir(char* dir);
char* hwUserDir(void);

void hwHelpBiosFinder(Thread* th);
int hwInit(Thread* th, int argc, char** argv, int standalone);

#endif
