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

#ifdef USE_FS_ROMDISK0
#include USE_FS_ROMDISK0
#else
#define Romdisk0Content NULL
#endif

#define NB_FILE_TABLE 32
typedef struct {
	int native;
	char* data;
	char name[12];
} Romdisk;

Romdisk Romdisks[NB_FILE_TABLE];

int romdiskCheck(char* src, unsigned int len)
{
	unsigned int i,last,dataStart;
	unsigned int* table = (unsigned int*)src;
	if (len < 8) return 0;
	if (len != table[0]) return 0;
	dataStart = table[1];
	if (dataStart == 0) return (len == 8) ? 1 : 0;	// we allow empty romdisk
	if (dataStart & 3) return 0;	// dataStart is necessary a multiple of 4 because the initial table has only 32bits elements
	if (dataStart >= len) return 0;
	if (dataStart < 8) return 0;
	last = (dataStart >> 2) - 1;	// last table entry
	if (table[last]) return 0;		// last table entry must be zero
	for (i = 1; i < last; i++) {
		unsigned int offset0 = table[i];
		if (offset0 >= len) return 0;
		if (i > 1 && offset0 <= table[i - 1]) return 0;	// table entries must be strictly increasing

		if (!src[offset0]) return 0;	// no empty name
		while (offset0 < len && src[offset0]) offset0++;
		if (offset0 >= len) return 0;
		if ((i < last - 1) && (offset0 >= table[i + 1])) return 0;
	}
	return 1;
}

// when called with data=NULL, returns the next available slot
int romdiskAdd(char* data,char* name,int native)
{
	int i;
	if (strlen(name)>11) return -1;
	for(i=0;i<NB_FILE_TABLE;i++) if (!Romdisks[i].data) {
		if (data) {
			strcpy(Romdisks[i].name,name);
			Romdisks[i].data = data;
			Romdisks[i].native = native;
		}
		return i;
	}
	return -1;
}
void romdiskFree(int i)
{
	if (!Romdisks[i].data) return;
	VM_FREE(Romdisks[i].data);
	Romdisks[i].data = NULL;
}

char* romdiskLookup(char* romdisk, char* path, int* size)
{
	int i = 1;
	int* table = (int*)romdisk;
	if (!table) return NULL;
	while (table[i]) {
		if (!strcmp(&romdisk[table[i]], path)) {
			int len = (int)strlen(path) + 1;
			if (size) *size = (table[i + 1] ? table[i + 1]: table[0]) - table[i] - len;
//			if (size) printf("size=%d\n",*size);
			return &romdisk[table[i] + len];
		}
		i++;
	}
	return NULL;
}

void romdiskDump()
{
	int i;
	for(i=0;i<NB_FILE_TABLE;i++) if (Romdisks[i].data) {
		PRINTF(LOG_SYS,"> romdisk %d: %s (%d)\n",i,Romdisks[i].name,Romdisks[i].native);
	
		int j = 1;
		int* table = (int*)Romdisks[i].data;
		if (!table) return;
		while (table[j]) {
			PRINTF(LOG_SYS,"> ---- %s\n",&Romdisks[i].data[table[j]]);
			j++;
		}
	}
}

LB* romdiskReadContent(Thread* th, int romdiskId, char* path, int* size)
{
	int len;
	LB* p;
	char* file;
//	PRINTF(LOG_DEV,"romdiskReadContent #%d %s\n", romdiskId, path);
	if ((romdiskId < 0) || (romdiskId >= NB_FILE_TABLE) || !Romdisks[romdiskId].data) return NULL;
	file= romdiskLookup(Romdisks[romdiskId].data, path, &len);
	if (file == NULL) return NULL;
	if (size) *size = len;
	p = memoryAllocStr(th, file, len); if (!p) return NULL;
	return p;
}

LINT romdiskDirectoryList(Thread* th, int romdiskId, Buffer* out, char* dir) // we expect dir to end with '/'
{
	int i = 1;
	int len;
	char* romdisk;
	int* table;

	if ((romdiskId < 0) || (romdiskId >= NB_FILE_TABLE) || !Romdisks[romdiskId].data) return 0;
	romdisk = Romdisks[romdiskId].data;
	table = (int*)romdisk;
	if (!table) return 0;

//PRINTF(LOG_DEV,"romdiskDirectoryList '%s'\n",dir);
	len = (int)strlen(dir);

	while (table[i]) {
		char* entry = &romdisk[table[i]];
		if (!memcmp(entry, dir, len)) {
			char* start = entry + len;
			char* p = strstr(start, "/");
			if (p) {
				LINT nameLen = p - start;
				_fsAddFileInfo(th, out, start, nameLen, "0 0 d");
			}
			else
			{
				char tmpAttr[32];
				int lenPath = (int)strlen(entry) + 1;
				snprintf(tmpAttr, 32, LSX" "LSX" %c",
					(LINT)(table[i + 1] ? table[i + 1] : table[0]) - table[i] - lenPath,
					(LINT)0,
					'-' // 'd' for directory
				);
//				printf("attr=%s\n", tmpAttr);
				_fsAddFileInfo(th, out, start, -1, tmpAttr);
			}
		}
		i++;
	}
	return 0;
}

int romdiskVolumeList(Thread* th, int* n)
{
	int j;
	for (j = NB_FILE_TABLE-1; j>=0; j--) if (Romdisks[j].data)
	{
		if (_volumeList(th, Romdisks[j].name, MM.romdiskVolume, j, 0)) return EXEC_OM;
		(*n)++;
	}
	return 0;
}

#ifdef WITH_UART
char* bootDiskLoader()
{
	char buffer[4];
	int i=0;
	int len=0;
	unsigned long t0;
	char* bufferBootLoader;   
	for(;uartGet()>=0;);
	uartPut("<BOOTDISK>",10);

	t0=hwTimeMs()+1000;
	while(i<4) {
		int c=uartGet();
		if (c>=0) buffer[i++]=c;
		if (hwTimeMs()>t0) {
			uartPut("<SKIP>\n",7);
			return NULL;
		}
	}
	len=*(int*)buffer;
	bufferBootLoader=VM_MALLOC(len+1);
	for(i=0;i<4;i++) bufferBootLoader[i]=buffer[i];
	uartPutChar('=');
	while(i<len) {
		int c=uartGet();
		if (c>=0) {
			if ((i&((1<<13)-1))==0) uartPutChar('=');
			bufferBootLoader[i++]=c;
		}
	}
	bufferBootLoader[len] = 0;
	if (!romdiskCheck(bufferBootLoader, len)) {
		uartPut("<WRONG_FORMAT_ERROR>\n", 21);
//		_myHexDump(bufferBootLoader, len,0);
		return NULL;
	}
	uartPut("<DONE>\n",7);
	return bufferBootLoader;
}
#endif

void romdiskInit()
{
	int j;
	for (j = 0; j < NB_FILE_TABLE; j++) Romdisks[j].data = NULL;
#ifdef USE_FS_ROMDISK0
	romdiskAdd(Romdisk0Content,"NATIVE",1);
#endif
#ifdef USE_BOOTLOADER
	{
		char* RomdiskBootLoader = bootDiskLoader();
		if (RomdiskBootLoader) romdiskAdd(RomdiskBootLoader,"BOOTDISK",1);
	}
#endif
//	romdiskDump();
}

int romdiskImport(char* src, LINT len)
{
	char* romdisk;
	if (romdiskAdd(NULL,"USER",0)<0) return -1;

	if (!romdiskCheck(src, (unsigned int)len)) {
		PRINTF(LOG_SYS,">Error: romdiskCheck failed\n");
		return -1;
	}
	romdisk = VM_MALLOC(len+1); if (!romdisk) return -1;
	memcpy(romdisk, src, len);
	romdisk[len] = 0;
	return romdiskAdd(romdisk,"USER",0);
}
int romdiskMount(Thread* th, int argc, char** argv, int standalone)
{
#ifndef BOOT_SKIP_FS_ROMDISK0
	int j;
	for (j = 0; j <NB_FILE_TABLE; j++) if (Romdisks[j].data)	// last declared, first order
	{
		_partitionAdd(MM.romdiskVolume,j,"");
	}
#endif
	return 0;
}
void romdiskReleaseUserDisk(void)
{
	int i;
	for (i = 0; i < NB_FILE_TABLE; i++) if (Romdisks[i].data && !Romdisks[i].native) romdiskFree(i);
}
void romdiskRelease(void)
{
	int i;
	for (i = 0; i < NB_FILE_TABLE; i++) if (Romdisks[i].data && Romdisks[i].data!= Romdisk0Content) romdiskFree(i);
}