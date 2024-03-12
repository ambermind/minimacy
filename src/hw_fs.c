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

char CurrentDir[MAX_PATH + 32];
char UserDir[MAX_PATH + 2];

void _fsAddFileInfo(Thread* th, Buffer* out, char* path, LINT len, char* tmpAttr)
{
	LINT lAttr;
	LINT lName;
	char* name = path;

	// format is : fileName\$00size\$20time\$20mode\$00
	// it will be easy to add more attributes in the second member to be splitted by space
	// win : dir => (fileinfo.attrib & 16)<>0
	// unix: dir => (info.st_mode&S_IFMT) == S_IFDIR
	if (len < 0) len = strlen(path);
	lName = (path+len)-name;
	lAttr = strlen(tmpAttr);
	bufferAddBin(th, out, name, lName);
	bufferAddChar(th, out, 0);
	bufferAddBin(th, out, tmpAttr, lAttr + 1);
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

		for (j = i0 + 3; j <= (int)strlen(p); j++) p[iLast + j - i0 - 3] = p[j];
	}
	while ((q = strstr(p, "/./")))
	{
		int j;
		int i0 = (int)(q - p);
		for (j = i0 + 2; j <= (int)strlen(p); j++) p[j - 2] = p[j];
	}
	while ((q = strstr(p, "//")))
	{
		int j;
		int i0 = (int)(q - p);
		for (j = i0 + 1; j <= (int)strlen(p); j++) p[j - 1] = p[j];
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
		p[i + 1] = 0;
		return;
	}
	p[0] = 0;
}

LB* fileReadContent(Thread* th, LB* type, LINT index, char* path, int* size, int verbose)
{
	if (verbose) PRINTF(LOG_USER, "Tried: '%s'\n", path);
#ifdef USE_FS_ANSI
	if (type == MM.ansiVolume) return ansiReadContent(th, path, size);
#endif
#ifdef USE_FS_UEFI
	if (type == MM.uefiVolume) return uefiReadContent(th, index, path, size);
#endif
	if (type == MM.romdiskVolume) return romdiskReadContent(th, (int)index, path, size);
	return NULL;
}
LB* _fsReadPackage(Thread* th, LB* type, LINT index, char* dir, char* pkg, int* size, int verbose)
{
	int i, j;
	LB* result;
	char path[MAX_PATH + 2];
	int prefix_length = (int)strlen(dir);

	if (prefix_length + 2 * strlen(pkg) + strlen(SUFFIX_CODE) > MAX_PATH) return NULL;
	snprintf(path, MAX_PATH, "%s%s%s", dir, pkg, SUFFIX_CODE);
	if ((result = fileReadContent(th, type, index, path, size, verbose))) return result;
	if (verbose) return NULL;
	for (i = 0; i <= (int)strlen(pkg); i++)
	{
		if ((pkg[i] == 0) || (pkg[i] == '.'))
		{
			strcpy(path + prefix_length, pkg);
			for (j = 0; j <= i; j++) if ((path[prefix_length + j] == '.') || (path[prefix_length + j] == 0)) path[prefix_length + j] = '/';
			snprintf(path + prefix_length + i + 1, MAX_PATH - (prefix_length + i + 1), "%s%s", pkg, SUFFIX_CODE);
			if ((result = fileReadContent(th, type, index, path, size, verbose))) return result;
		}
	}
	return NULL;
}

LB* fsReadPackage(Thread* th, char* pkg, int* size, int verbose)
{
	LB* l = MM.partitionsFS;
	LB* result;
	while (l)
	{
		LB* partition = ARRAY_PNT(l, 0);
		if (partition) {
			LB* type = ARRAY_PNT(partition, 0);
			LINT index = ARRAY_INT(partition, 1);
			LB* physicalPath = ARRAY_PNT(partition, 2);
			if (physicalPath) {
				if ((result = _fsReadPackage(th, type, index, STR_START(physicalPath), pkg, size, verbose))) return result;
			}
		}
		l = ARRAY_PNT(l, 1);
	}
	return NULL;
}

int _volumeList(Thread* th, char* name, LB* type, LINT index, int writable)
{
	FUN_PUSH_STR(name, -1);
	FUN_PUSH_PNT(type);
	FUN_PUSH_INT(index);
	FUN_PUSH_BOOL(writable);
	FUN_MAKE_ARRAY(4, DBG_TUPLE);
	return 0;
}
int volumeList(Thread* th)
{
	int n = 0;
#ifdef USE_FS_ANSI
	if (ansiVolumeList(th, &n)) return EXEC_OM;
#endif
#ifdef USE_FS_UEFI
	if (uefiVolumeList(th, &n)) return EXEC_OM;
#endif
	if (romdiskVolumeList(th, &n)) return EXEC_OM;
	FUN_PUSH_NIL;
	while (n--) FUN_MAKE_ARRAY(LIST_LENGTH, DBG_LIST);
	return 0;
}
int _partitionAdd(LB* type, LINT index, char* physicalPath)
{
	STACK_PUSH_PNT_ERR(MM.tmpStack,type,EXEC_OM);
	STACK_PUSH_INT_ERR(MM.tmpStack, index, EXEC_OM);
	STACK_PUSH_STR_ERR(MM.tmpStack, physicalPath, -1, EXEC_OM);
	STACK_PUSH_FILLED_ARRAY_ERR(MM.tmpStack, 3, DBG_TUPLE, EXEC_OM);
	STACK_PUSH_PNT_ERR(MM.tmpStack, MM.partitionsFS, EXEC_OM);
	STACK_PUSH_FILLED_ARRAY_ERR(MM.tmpStack, 2, DBG_TUPLE, EXEC_OM);
	MM.partitionsFS = STACK_PULL_PNT(MM.tmpStack);
	return 0;
}
char* fsCurrentDir(void) {
	return CurrentDir;
}
char* fsUserDir(void) {
	return UserDir;
}

int fsMount(Thread* th, int argc, char** argv, int standalone)
{

	if (romdiskMount(th, argc, argv, standalone)) return -1;
#ifdef USE_FS_ANSI
	if (ansiFsMount(th, argc, argv, standalone)) return -1;
#endif
#ifdef USE_FS_UEFI
	if (uefiFsMount(th, argc, argv, standalone)) return -1;
#endif
	return 0;
}

void fsInit()
{
	romdiskInit();
#ifdef USE_FS_ANSI
	ansiFsInit();
#endif
#ifdef USE_FS_UEFI
	uefiFsInit();
#endif
	strcpy(CurrentDir, "");
	strcpy(UserDir, "");
	systemUserDir(UserDir, MAX_PATH);
	systemCurrentDir(CurrentDir, MAX_PATH);
	PRINTF(LOG_SYS, "> Current working directory: %s\n", CurrentDir);
	PRINTF(LOG_SYS, "> Default user directory   : %s\n", UserDir);

}

void fsRelease()
{
	romdiskRelease();
#ifdef USE_FS_ANSI
	ansiFsRelease();
#endif
#ifdef USE_FS_UEFI
	uefiFsRelease();
#endif
}
