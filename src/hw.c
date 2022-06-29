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
#ifdef ON_UNIX
#include<unistd.h>
#include<sys/types.h>
#include<fcntl.h>
#include<termios.h>
#include<pwd.h>
#include<dirent.h>
#include<sys/stat.h>
#endif
#ifdef ON_WINDOWS
#include<Shlobj.h>
#include<conio.h>
#include<io.h>
#include<direct.h>
#endif

#ifdef USE_IOS
void systemIosDir(char* path);
#endif

char RomDir[MAX_PATH + 32];
char SystemDir[MAX_PATH + 2];
char CurrentDir[MAX_PATH + 32];
char UserDir[MAX_PATH + 2];

int macDetect()
{
	if (sizeof(MSEM)==4) return 1;
	return 0;
}
void systemCleanPath(char* p)
{
	char* q;
	while ((q = strstr(p, "/../")))
	{
		int i, j;
		int i0 = (int)(q - p);
		int iLast = i0;
		for (i = 0; i < i0; i++) if (p[i] == '/') iLast = i;

		for (j = i0 + 3; j <= strlen(p); j++) p[iLast + j - i0 - 3] = p[j];
	}
	while ((q = strstr(p, "/./")))
	{
		int j;
		int i0 = (int)(q - p);
		for (j = i0 + 2; j <= strlen(p); j++) p[j - 2] = p[j];
	}
	while ((q = strstr(p, "//")))
	{
		int j;
		int i0 = (int)(q - p);
		for (j = i0 + 1; j <= strlen(p); j++) p[j - 1] = p[j];
	}
}
void systemCleanDir(char* p)
{
	int i;
	if (!p[0]) return;
	for (i = 0; i < (int)strlen(p); i++) if (p[i] == '\\') p[i] = '/';
	if (p[strlen(p) - 1] != '/') strcpy(p + strlen(p), "/");
	systemCleanPath(p);
}
void systemRemoveLast(char* p)
{
	int i;
	for (i = (int)strlen(p) - 2; i >= 0; i--) if (p[i] == '/')
	{
		p[i+1] = 0;
		return;
	}
	p[0] = 0;
}


void systemExecDir(char* execDir, int len)
{
#ifdef ON_UNIX
	execDir[0] = 0;
#endif
#ifdef USE_IOS
    systemIosDir(execDir);
#endif
#ifdef ON_WINDOWS
	GetModuleFileNameA(NULL, execDir, len);
	systemCleanDir(execDir);
	systemRemoveLast(execDir);
	systemRemoveLast(execDir);
#endif
}
void systemEnvDir(char* envDir, int len)
{
#ifdef ON_UNIX
	char* p = getenv("MINIMACY");
	if (p && (strlen(p) < len - 2))
	{
		strcpy(envDir, p);
		systemCleanDir(envDir);
		return;
	}
	envDir[0] = 0;
#endif
#ifdef ON_WINDOWS
	envDir[0] = 0;
#endif
}

void systemCurrentDir(char* curDir, int len)
{
	GetCurrentDirectoryA(len, curDir);
	systemCleanDir(curDir);
}

void systemUserDir(char* userDir, int len)
{
#ifdef ON_UNIX
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
#endif
#ifdef ON_WINDOWS
	if (S_OK == (SHGetFolderPathA(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, 0, userDir)))
	{
		systemCleanDir(userDir);
		return;
	}
	userDir[0] = 0;
#endif
}

int hwSystemCheckBios(Thread* th,char* MinimacyDir)
{
	char tmpDir[MAX_PATH + 2];
	if (!strlen(MinimacyDir)) return 0;
	if (strlen(MinimacyDir) + strlen(BOOT_FILE)+14 > MAX_PATH) return 0;
	sprintf(tmpDir, "%srom/bios/" BOOT_FILE SUFFIX_CODE, MinimacyDir);
	PRINTF(th, LOG_ERR, "Trying: %s\n",tmpDir);
	if (!fileExist(tmpDir)) return 0;
	PRINTF(th, LOG_ERR, "Bios found!\n");
	return 1;
}
int hwGetSystemDir(Thread* th, char* MinimacyDir, int argc, char** argv)
{
	int i;
	strcpy(MinimacyDir, "");
	systemUserDir(UserDir, MAX_PATH);

	for (i = 0; i < argc; i++)
	{
		if ((i < argc - 1) && (!strcmp(argv[i], "-dir")) && (strlen(argv[i]) < MAX_PATH))
		{
			strcpy(MinimacyDir, argv[i+1]);
			systemCleanDir(MinimacyDir);
			if (hwSystemCheckBios(th, MinimacyDir)) return 0;
		}
	}
	systemEnvDir(MinimacyDir, MAX_PATH);
	if (hwSystemCheckBios(th, MinimacyDir)) return 0;

	systemExecDir(MinimacyDir, MAX_PATH);
	if (hwSystemCheckBios(th, MinimacyDir)) return 0;

	if (strlen(UserDir))
	{
		sprintf(MinimacyDir, "%sminimacy/", UserDir);
		if (hwSystemCheckBios(th, MinimacyDir)) return 0;
	}
	systemCurrentDir(MinimacyDir, MAX_PATH);
	if (hwSystemCheckBios(th, MinimacyDir)) return 0;
	return -1;
}
char* hwRomDir() {
	return RomDir;
}
char* hwSystemDir() {
	return SystemDir;
}
char* hwCurrentDir() {
	return CurrentDir;
}
int hwSetSystemDir(char* dir) {
	if (strlen(dir) > MAX_PATH) return -1;
	strcpy(SystemDir, dir);
	systemCleanDir(SystemDir);
	return 0;
}
char* hwUserDir() {
	return UserDir;
}

void consoleWrite(int user, char* src, int len)
{
	FILE* out = user ? stdout : stderr;
	fwrite(src, 1, len, out);
	fflush(out);
}
void consoleVPrint(int user, char* format, va_list arglist)
{
	FILE* out = user ? stdout : stderr;
	vfprintf(out,format, arglist);
	fflush(out);
}

int fileSize(void* file)
{
	int size;
	if (file == NULL) return 0;

	fseek((FILE*)file, 0, SEEK_END);
	size = (int)ftell((FILE*)file);
	fseek((FILE*)file, 0, SEEK_SET);
	return size;
}

LB* fileReadContent(Thread* th,char* path, int* size)
{
	LB* p;
	char* data;
	int sz;
	FILE* file = fopen(path, "rb");
//	printf("try open %s\n", path);
	if (file == NULL) return NULL;
	sz = fileSize(file);
	p = memoryAllocStr(th, NULL, sz); if (!p) return NULL;
	data = STRSTART(p);
	fread((void*)data, 1, sz, file);
	data[sz] = 0;
	fclose(file);
	if (size) *size = sz;
	return p;
}

LB* _fileReadPackage(Thread* th, char* dir,char* pkg, char* suffix, int* size)
{
	int i,j;
	LB* result;
	char path[MAX_PATH+2];
	int prefix_length = (int)strlen(dir);

	if (!suffix) suffix = "";
	if (prefix_length+2*strlen(pkg)+ strlen(suffix) > MAX_PATH) return NULL;
	sprintf(path, "%s%s%s",dir,pkg,suffix);

	if ((result = fileReadContent(th, path, size))) return result;
	for (i = 0; i <= strlen(pkg); i++)
	{
		if ((pkg[i] == 0)||(pkg[i] == '.'))
		{
			strcpy(path+prefix_length, pkg);
			for(j= 0;j<=i;j++) if ((path[prefix_length+j]=='.')|| (path[prefix_length + j] == 0)) path[prefix_length+j]='/';
			sprintf(path+ prefix_length+i+1, "%s%s", pkg, suffix);
			if ((result = fileReadContent(th, path, size))) return result;
		}
	}
	return NULL;
}

LB* fileReadPackage(Thread* th, char* pkg, char* suffix, int* size)
{
	LB* result = NULL;
	if (SystemDir[0]) result = _fileReadPackage(th, SystemDir, pkg, suffix, size);
	if (!result) result = _fileReadPackage(th, RomDir, pkg, suffix, size);
	return result;
}

void fileClose(void* file)
{
	if (file) fclose((FILE*)file);
}

void* fileOpen(char* path, char* mode)
{
//	printf("fileOpen %s\n", path);
	return (void*)fopen(path, mode);
}
int fileExist(char* path)
{
	FILE* f;
	f= fopen(path, "rb");
	if (!f) return 0;
	fclose(f);
	return 1;
}
LINT fileSeek(void* file, LINT offset)
{
	if (!file) return 0;
	return fseek((FILE*)file, (long)offset, SEEK_SET);
}

LINT fileWrite(void* file, char* start, LINT len)
{
	LINT res;
	if (!file) return 0;
	res= fwrite(start, 1, len, (FILE*)file);
	fflush(file);
	return res;
}

LINT fileRead(void* file, char* start, LINT len)
{
	LINT res;
	if (!file) return 0;
	res = fread(start, 1, len, (FILE*)file);
	return res;
}

LB* fileReadAlloc(Thread* th, void* file, int maxSize)
{
	LINT size;
	LB* p;
	LB* q;
	if (!file) return 0;
	p = memoryAllocStr(th, NULL, maxSize); if (!p) return NULL;
	size = fread(STRSTART(p), 1, maxSize, file);
	if (size <= 0) return NULL;
	if (size == maxSize) return p;
	q = memoryAllocStr(th, NULL, size); if (!q) return NULL;
	memcpy(STRSTART(q), STRSTART(p), size);
	return q;
}

LINT _hwAddFileInfo(char* path, char* tmpAttr, char* buffer, LINT bufferLen, LINT iOut)
{
	LINT lAttr;
	LINT lName;
	char* name=path;
	int i;

#ifdef ON_UNIX
	{
		struct stat info;
		stat(path, &info);
		sprintf(tmpAttr, LSX" "LSX" %c", (LINT)info.st_size, (LINT)info.st_mtime, S_ISDIR(info.st_mode)?'d':'-');
	}
#endif
#ifdef ON_WINDOWS
// on windows, the buffer tmpAttr is already filled by the caller
#endif
	// format is : fileName\$00size\$20time\$20mode\$00
	// it will be easy to add more attributes in the second member to be splitted by space
	// win : dir => (fileinfo.attrib & 16)<>0
	// unix: dir => (info.st_mode&S_IFMT) == S_IFDIR

	for(i=0;i<strlen(path);i++) if (path[i]=='/') name=path+i+1;
	lName = strlen(name);
	lAttr = strlen(tmpAttr);
	if (iOut + lName + lAttr + 2 <= bufferLen)
	{
		strcpy(buffer + iOut, name);
		strcpy(buffer + iOut + lName + 1, tmpAttr);
	}
	iOut += lName + lAttr + 2;
	return iOut;
}
LINT hwDiskList(char* dir, char* buffer, LINT bufferLen)
{
	char search[MAX_PATH + 2];
	char tmpAttr[32];
	LINT iOut=0;
#ifdef ON_UNIX
	{
		DIR* d;
		struct dirent* dp;
		
		if (dir[strlen(dir)-1]=='/')
		{
			sprintf(search, "%s.", dir);
			if ((d = opendir(search)))
			{
				while ((dp = readdir(d)) != NULL)
				{
					char candidate[MAX_PATH + 2];
					sprintf(candidate, "%s%s", dir, dp->d_name);
					iOut=_hwAddFileInfo(candidate,tmpAttr,buffer,bufferLen,iOut);
				}
			}
		}
		else iOut=_hwAddFileInfo(dir,tmpAttr,buffer,bufferLen,iOut);
	}
#endif
#ifdef ON_WINDOWS
	{
		struct _finddata_t fileinfo;
		LINT h, res;
		if (dir[strlen(dir)-1]=='/') sprintf(search, "%s*.*", dir);
		else strcpy(search,dir);
		h = _findfirst(search, &fileinfo);
		res = h;
		while (res != -1)
		{
			sprintf(tmpAttr, LSX" "LSX" %c", (LINT)fileinfo.size, (LINT)fileinfo.time_write, (fileinfo.attrib & _A_SUBDIR)?'d':'-');
			iOut=_hwAddFileInfo(fileinfo.name,tmpAttr,buffer,bufferLen,iOut);

			res = _findnext(h, &fileinfo);
		}
		if (h != -1) _findclose(h);
	}
#endif
//	printf("_fsize_t: %lld\n", sizeof(_fsize_t));
//	printf("time_t: %lld\n", sizeof(time_t));
	return iOut;
}

int fileMakedirs(char* filename)
{
	int i;
	i = (int)strlen(filename) - 1;
//	printf("makedirs %s\n", filename);
	while (i >= 0)
	{
		if ((filename[i] == '/') || (filename[i] == '\\'))
		{
			char dir[MAX_PATH + 2];
			strncpy(dir, filename, i);
			dir[i] = 0;
//			printf("try mkdir %s\n", dir);
#ifdef ON_UNIX
			if (mkdir(dir, -1))
#endif
#ifdef ON_WINDOWS
			if ((mkdir(dir)) && (errno == ENOENT))
#endif
			{
				fileMakedirs(dir);
#ifdef ON_UNIX
				if (mkdir(dir, -1))
#endif
#ifdef ON_WINDOWS		
				if (mkdir(dir))
#endif
				{
					return -1;
				}
			}
			return 0;
		}
		i--;
	}
	return -1;
}

//-----------------------------

unsigned long long LS_Seed = 0;

void _lsRand(char* dst, LINT len)
{
    LINT x=0;
    LINT i;
	for (i = 0; i < len; i++)
	{
		if (!(i & 3)) x = lsRand32(&LS_Seed);
		*(dst++) = (char)x; x >>= 8;
	}
}
#ifdef ON_UNIX
int hCryptTry = 0;
FILE* fCrypt = 0;
void lsRand(char* dst, LINT len)
{
	if (!hCryptTry)
	{
		fCrypt = fopen("/dev/urandom", "r");
		hCryptTry = 1;
		//		printf("System random = %s\n",fCrypt?"ok":"nok");
	}
	if (fCrypt) fread(dst, len, 1, fCrypt);
	else _lsRand(dst, len);
}
#endif
#ifdef ON_WINDOWS
#include<Wincrypt.h>
int hCryptTry = 0;
HCRYPTPROV hCryptProv = (HCRYPTPROV)NULL;
void lsRand(char* dst, LINT len)
{
	//	_lsRand(dst,len); return;
	if (!hCryptTry)
	{
		if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, 0)) hCryptProv = (HCRYPTPROV)NULL;
		hCryptTry = 1;
//		printf("System random = %s\n", hCryptProv ? "ok" : "nok");
	}
	if ((!hCryptProv) || (!CryptGenRandom(hCryptProv, (DWORD)len, (BYTE*)dst))) _lsRand(dst, len);
}
#endif

//-----------------------------

LINT hwTime()
{
	time_t x;
	time(&x);
	return (LINT)x;
}

#ifdef ON_WINDOWS
LINT DeltaTimeMs = 0;
LARGE_INTEGER CurrentTimer;
LARGE_INTEGER FreqTimer;

LINT hwTimeMs()
{
	if (!FreqTimer.QuadPart) return DeltaTimeMs + LocalTickCount();
	QueryPerformanceCounter(&CurrentTimer);
	return DeltaTimeMs + CurrentTimer.QuadPart * 1000 / FreqTimer.QuadPart;
}
void hwTimeInit()
{
	if (!QueryPerformanceFrequency(&FreqTimer)) FreqTimer.QuadPart = 0;
	if (FreqTimer.QuadPart)
	{
		QueryPerformanceCounter(&CurrentTimer);
		DeltaTimeMs = hwTime() * 1000 - CurrentTimer.QuadPart * 1000 / FreqTimer.QuadPart;
	}
	else DeltaTimeMs = hwTime() * 1000 - LocalTickCount();
}
#endif

#ifdef ON_UNIX
LINT hwTimeMs()
{
	LINT t;
	struct timeval tm;

	gettimeofday(&tm, NULL);
	t = (tm.tv_sec * 1000) + (tm.tv_usec / 1000);
	return t;
}
void hwTimeInit()
{
}
#endif

void hwSleepMs(LINT ms)
{
#ifdef ON_UNIX
	struct timeval tm;
	tm.tv_sec = ms / 1000;
	tm.tv_usec =(int)( (ms - (1000 * tm.tv_sec)) * 1000);
	select(FD_SETSIZE, NULL, NULL, NULL, &tm);
#endif
#ifdef ON_WINDOWS
	Sleep((DWORD)ms);
#endif
}
void hwHelpBiosFinder(Thread* th)
{
	PRINTF(th, LOG_USER, "The minimacy folder is expected to contain [minimacy folder]/rom/bios/bios.mcy\n");
	PRINTF(th, LOG_USER, "At startup the searching order for this minimacy folder is:\n");
	PRINTF(th, LOG_USER, "1- command line argument: -dir path\n");
	PRINTF(th, LOG_USER, "2- environment variable MINIMACY\n");
	PRINTF(th, LOG_USER, "3- grand parent of minimacy.exe (windows only)\n");
	PRINTF(th, LOG_USER, "4- ~/minimacy (on Unix) or [user]/Documents/minimacy (on Windows)\n");
	PRINTF(th, LOG_USER, "5- Working directory\n\n");
}
int hwInit(Thread* th, int argc, char** argv, int standalone)
{
	char MinimacyDir[MAX_PATH + 2];

	if (hwGetSystemDir(th, MinimacyDir,argc, argv))
	{
		PRINTF(th, LOG_USER, "Error - cannot locate the system folder\n\n");
		hwHelpBiosFinder(th);
		return -1;
	}
	sprintf(RomDir, "%srom/", MinimacyDir);
	strcpy(SystemDir, "");

	systemCurrentDir(CurrentDir, MAX_PATH);
	if (standalone) sprintf(CurrentDir, "%ssystem/", MinimacyDir);


	PRINTF(th, LOG_ERR, "Rom directory            : %s\n",RomDir);
	PRINTF(th, LOG_ERR, "Current working directory: %s\n",CurrentDir);
	PRINTF(th, LOG_ERR, "Default user directory   : %s\n",UserDir);

	hwTimeInit();
	return 0;
}
