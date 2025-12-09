// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#include"minimacy.h"

#ifdef USE_FS_ANSI_UNIX
#include<unistd.h>
#include<sys/types.h>
#include<fcntl.h>
#include<termios.h>
#include<pwd.h>
#include<dirent.h>
#include<sys/stat.h>
#endif
#ifdef USE_FS_ANSI_WIN
#include<Shlobj.h>
#include<conio.h>
#include<io.h>
#include<direct.h>
#endif

#ifdef USE_FS_SYSTEMDIR_STUB
void systemMainDir(char* path, int len, const char* argv0) { path[0] = 0; }
void systemCurrentDir(char* path, int len) { path[0] = 0; }
void systemUserDir(char* path, int len) { path[0] = 0; }
#endif
#ifdef USE_FS_SYSTEMDIR_WINDOWS
void systemMainDir(char* path, int len, const char* argv0)
{
	GetModuleFileNameA(NULL, path, len);
	systemCleanDir(path);
	systemRemoveLast(path);
	systemRemoveLast(path);
}
void systemCurrentDir(char* curDir, int len)
{
	GetCurrentDirectoryA(len, curDir);
	systemCleanDir(curDir);
}
void systemUserDir(char* userDir, int len)
{
	if (S_OK == (SHGetFolderPathA(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, 0, userDir)))
	{
		systemCleanDir(userDir);
		return;
	}
	userDir[0] = 0;
}
#endif
#ifdef USE_FS_SYSTEMDIR_UNIX
int systemCheckFile(char* execpath, int len, const char* argv0)
{
	char realp[1024];
	char* p;
	struct stat info;
	int currentLen=strlen(execpath);
	if (currentLen+1+strlen(argv0)>len) return -1;
	if((currentLen>0)&&(execpath[currentLen-1]!='/')) strcat(execpath,"/");
	strcat(execpath,argv0);
//	printf("check %s\n",execpath);
	if (lstat(execpath, &info)) return -1;
	if (S_ISDIR(info.st_mode)) return -1;
	if (S_ISREG(info.st_mode)) return 0;
	if (!S_ISLNK(info.st_mode)) return -1;
	p=realpath(execpath,NULL);
	if (!p) return -1;
//	printf("Real path %s -> %s\n",execpath,p);
	if (strlen(p)>len) return -1;
	strcpy(execpath,p);
	free(p);
	return 0;
}

int _systemExecPath(char* execpath, int len, const char* argv0)
{
	char* envpath;
	int i,j;
	if (argv0[0]=='/') {	// argv0 contains an absolute path
		struct stat info;
		if (strlen(argv0)>len) return -1;
		strcpy(execpath,argv0);
		return stat(execpath, &info)?-1:0;
	}
	if (getcwd(execpath, len)) {	// test argv0 as a path relative to the current working directory
		if (!systemCheckFile(execpath, len, argv0)) return 0;
	}
	envpath=getenv("PATH");
	j=0;
	for(i=0;i<=strlen(envpath);i++) {
		if ((envpath[i]==':')||(envpath[i]==0)) {
			execpath[j]=0;
			if (!systemCheckFile(execpath, len, argv0)) return 0;
			j=0;
		}
		else {
			if (j<len) execpath[j++]=envpath[i];
		}
	}
	return -1;
}
void systemMainDir(char* path, int len, const char* argv0)
{
	// https://stackoverflow.com/questions/933850/how-do-i-find-the-location-of-the-executable-in-c
	ssize_t result = readlink("/proc/self/exe", path, len); // linux
	if (result < 0) result = readlink("/proc/curproc/file", path, len); // freeBsd
	if (result < 0) result = readlink("/proc/self/path/a.out", path, len); // solaris
	path[(result < 0) ? 0 : result] = 0;
// on Mac we need to search further, from the current directory and from the PATH environment value
	if (result <0 && argv0) result=_systemExecPath(path,len,argv0);
	if (result<0) {
		path[0]=0;
		return;
	}
	systemCleanDir(path);
	systemRemoveLast(path);
	systemRemoveLast(path);
//	printf("<%s>\n",path);
}
void systemCurrentDir(char* curDir, int len)
{
	if (!getcwd(curDir, len)) return;
	systemCleanDir(curDir);
}
void systemUserDir(char* userDir, int len)
{
	struct passwd* pwd;
	char* p = getenv("HOME");
	if (p && (strlen(p) < len - 2))
	{
		strcpy(userDir, p);
		systemCleanDir(userDir);
		return;
	}
	pwd = getpwuid(getuid());
	if (pwd && (strlen(pwd->pw_dir) < len - 2))
	{
		strcpy(userDir, pwd->pw_dir);
		systemCleanDir(userDir);
		return;
	}
	userDir[0] = 0;
}
#endif
#ifdef USE_FS_ANSI
int ansiFileSize(void* file)
{
	int size;
	if (file == NULL) return 0;

	fseek((FILE*)file, 0, SEEK_END);
	size = (int)ftell((FILE*)file);
	fseek((FILE*)file, 0, SEEK_SET);
	return size;
}
int ansiFileTell(void* file)
{
	if (file == NULL) return 0;
	return (int)ftell((FILE*)file);
}

void ansiFileClose(void* file)
{
	if (file) fclose((FILE*)file);
}

void* ansiFileOpen(char* path, char* mode)
{
//		PRINTF(LOG_DEV,"ansiFileOpen '%s' %s\n", path, mode);
#ifdef USE_FS_SYSTEMDIR_UNIX
	{
		struct stat statbuf;
		stat(path, &statbuf);
		if (S_ISDIR(statbuf.st_mode)) return NULL;
}
#endif
	return (void*)fopen(path, mode);
}
LINT ansiFileSeek(void* file, LINT offset)
{
	if (!file) return 0;
	return fseek((FILE*)file, (long)offset, SEEK_SET);
}

LINT ansiFileWrite(void* file, char* start, LINT len)
{
	LINT res;
	if (!file) return 0;
	res = fwrite(start, 1, len, (FILE*)file);
	fflush(file);
	return res;
}

LINT ansiFileRead(void* file, char* start, LINT len)
{
	LINT res;
	if (!file) return 0;
	res = fread(start, 1, len, (FILE*)file);
	return res;
}

LB* ansiReadContent(char* path, int* size)
{
	LB* p;
	char* data;
	int sz;
	FILE* file = fopen(path, "rb");
	//	PRINTF(LOG_DEV,"try open %s\n", path);
	if (file == NULL) return NULL;
	sz = ansiFileSize(file);
	p = memoryAllocStr(NULL, sz); if (!p) return NULL;
	data = STR_START(p);
	sz=(int)fread((void*)data, 1, sz, file);
	data[sz] = 0;
	fclose(file);
	if (size) *size = sz;
	return p;
}

void _ansiAddFileInfo(volatile Buffer** pout, char* path, LINT len, char* tmpAttr)
{
	LINT lAttr;
	LINT lName;
	char* name = path;

	// format is : fileName\$00size\$20time\$20mode\$00
	// it will be easy to add more attributes in the second member to be splitted by space
	// win : dir => (fileinfo.attrib & 16)<>0
	// unix: dir => (info.st_mode&S_IFMT) == S_IFDIR
	if (len < 0) len = strlen(path);
	lName = (path + len) - name;
	lAttr = strlen(tmpAttr);
	bufferAddBinWorker(pout, name, lName);
	bufferAddCharWorker(pout, 0);
	bufferAddBinWorker(pout, tmpAttr, lAttr + 1);
}

#ifdef USE_FS_ANSI_WIN
LINT ansiDirectoryList(volatile Buffer** pout, char* dir)	// we expect dir to end with '/'
{
	char search[MAX_PATH + 2];
	char tmpAttr[32];
	struct _finddata_t fileinfo;
	intptr_t h, res;

//	PRINTF(LOG_DEV, "ansiDirectoryList: %s\n", dir);
	snprintf(search, MAX_PATH, "%s*.*", dir);
	h = _findfirst(search, &fileinfo);
	res = h;
	while (res != -1)
	{
//		PRINTF(LOG_DEV, "filename: %s\n", fileinfo.name);
		snprintf(tmpAttr, 32, LSX" "LSX" %c", (LINT)fileinfo.size, (LINT)fileinfo.time_write, (fileinfo.attrib & _A_SUBDIR) ? 'd' : '-');
		_ansiAddFileInfo(pout, fileinfo.name, -1, tmpAttr);

		res = _findnext(h, &fileinfo);
	}
	if (h != -1) _findclose(h);
	//	PRINTF(LOG_DEV,"_fsize_t: %lld\n", sizeof(_fsize_t));
	//	PRINTF(LOG_DEV,"time_t: %lld\n", sizeof(time_t));
	return 0;
}
int ansiCreateDir(char* dir)
{
	if ((mkdir(dir)) && (errno == ENOENT)) return -1;
	return 0;
}
int ansiFileDelete(char* path)
{
	if (DeleteFile(path)) return 1;	// 1 means ok
	return 0;
}
int ansiDirDelete(char* path)
{
	if (RemoveDirectory(path)) return 1;	// 1 means ok
	return 0;
}
#endif
#ifdef USE_FS_ANSI_UNIX
LINT ansiDirectoryList(volatile Buffer** pout, char* dir)	// we expect dir to end with '/'
{
	char search[MAX_PATH + 2];
	char tmpAttr[32];
	DIR* d;
	struct dirent* dp;

//	PRINTF(LOG_DEV, "ansiDirectoryList: %s\n", dir);
	snprintf(search, MAX_PATH, "%s.", dir);
	d = opendir(search); if (!d) return 0;
	while ((dp = readdir(d)) != NULL)
	{
		struct stat info;
		char candidate[MAX_PATH + 2];
//		PRINTF(LOG_DEV, "filename: %s\n", dp->d_name);
		snprintf(candidate, MAX_PATH, "%s%s", dir, dp->d_name);
		stat(candidate, &info);
		snprintf(tmpAttr, 32, LSX" "LSX" %c", (LINT)info.st_size, (LINT)info.st_mtime, S_ISDIR(info.st_mode) ? 'd' : '-');
		_ansiAddFileInfo(pout, dp->d_name, -1, tmpAttr);
	}
	closedir(d);
	return 0;
}
int ansiCreateDir(char* dir)
{
	if (mkdir(dir, -1)) return -1;
	return 0;
}
int ansiFileDelete(char* path)
{
	if (unlink(path)) return 0;	// 0 means NOK
	return 1;
}
int ansiDirDelete(char* path)
{
	if (rmdir(path)) return 0;	// 1 means ok
	return 1;
}
#endif

#ifdef USE_DEVICE_UNIX
long long devNbSectors(char* path)
{
	long long nbSectors=-1;
	FILE* file;
	if (memcmp(path,"/dev/",5)) return -1;
	file=fopen(path,"rb");
	if (file) {
		int fd=fileno((FILE*)file);
#ifdef USE_MACOS
		ioctl(fd, DKIOCGETBLOCKCOUNT, &nbSectors);
#else
		ioctl(fd, BLKGETSIZE, &nbSectors);
#endif
		fclose(file);
	}
	return nbSectors;
}
int devSectorSize(char* path)
{
	int sectorSize=-1;
	FILE* file;
	if (memcmp(path,"/dev/",5)) return -1;
	file=fopen(path,"rb");
	if (file) {
		int fd=fileno((FILE*)file);
#ifdef USE_MACOS
		ioctl(fd, DKIOCGETBLOCKSIZE, &sectorSize);
#else
		ioctl(fd, BLKSSZGET, &sectorSize);
#endif
		fclose(file);
	}
	return sectorSize;
}
#else
long long devNbSectors(char* path) { return -1;}
int devSectorSize(char* path) { return -1; }
#endif
int ansiCreateDirs(char* filename)
{
	int i;
	i = (int)strlen(filename) - 1;
	//	PRINTF(LOG_DEV,"makedirs %s\n", filename);
	while (i >= 0)
	{
		if ((filename[i] == '/') || (filename[i] == '\\'))
		{
			char dir[MAX_PATH + 2];
			strncpy(dir, filename, i);
			dir[i] = 0;
			//			PRINTF(LOG_DEV,"try mkdir %s\n", dir);
			if (ansiCreateDir(dir))
			{
				ansiCreateDirs(dir);
				if (ansiCreateDir(dir)) return -1;
			}
			return 0;
		}
		i--;
	}
	return -1;
}

int ansiVolumeList(Thread* th, int* n)
{
	if (_volumeList(th, MM.ansiVolume, 0, 1, -1, -1)) return EXEC_OM;
	(*n)++;
	return 0;
}

void ansiHelpBiosFinder(void)
{
	PRINTF(LOG_USER, "\nOn startup Minimacy needs to open the file bios.mcy\n");
	PRINTF(LOG_USER, "This file is expected to be close to the executable\n");
	PRINTF(LOG_USER, "On a standard computer, the Minimacy folder structure is:\n");
	PRINTF(LOG_USER, "- ./bin/[executable]\n");
	PRINTF(LOG_USER, "- ./rom/bios/bios.mcy\n");
	PRINTF(LOG_USER, "On some Unix system such as Macos commandline target, Minimacy may encounter trouble to get its actual path. Then you may launch the Minimacy executable providing its absolute path.\n");
}

int ansiFsMount(const char* argv0, int standalone)
{
#ifndef BOOT_SKIP_FS_ANSI
	char MinimacyDir[MAX_PATH + 2];

	strcpy(MinimacyDir, "");
	systemMainDir(MinimacyDir, MAX_PATH, argv0);
	PRINTF(LOG_SYS, "> Minimacy directory       : %s\n", MinimacyDir);
	snprintf(MinimacyDir+strlen(MinimacyDir), MAX_PATH-strlen(MinimacyDir), "rom/");
	_partitionAdd(MM.ansiVolume, 0, MinimacyDir);
	PRINTF(LOG_SYS, "> Rom directory            : %s\n", MinimacyDir);
#endif
	return 0;
}
void ansiFsInit(void)
{
}
void ansiFsRelease(void)
{
}
#endif

