// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#include"minimacy.h"

#ifdef USE_FS_ROMDISK0
#include USE_FS_ROMDISK0
#else
#define Romdisk0Content NULL
#endif

#define ROMDISK_MAX_NB 16

#define ROMDISK_USER 0
#define ROMDISK_BOOTLOADER 1
#define ROMDISK_NATIVE 2

typedef struct {
	char* data;
	LINT type;
} Romdisk;

Romdisk Romdisks[ROMDISK_MAX_NB];

void romdiskMark(LB* user)
{
	int i;
	for (i = 0; i < ROMDISK_MAX_NB; i++) if (Romdisks[i].data && Romdisks[i].type != ROMDISK_NATIVE) {
		char* node = Romdisks[i].data - sizeof(LB);
		if (MOVING_BLOCKS) Romdisks[i].data = STR_START(((LB*)node)->listMark);
		else BLOCK_MARK((LB*)node);
	}
}
int romdiskCheck(char* src, unsigned int len)
{
	unsigned int i,last,dataStart;
	unsigned int* table = (unsigned int*)src;
	if (len < 8) return 0;
	if (len < table[0]) return 0;	// we accept the romdisk to be larger than expected, so that we may have a digital signature at the end
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
int romdiskAdd(char* data,int type)
{
	int i;
	for(i=0;i<ROMDISK_MAX_NB;i++) {
		if (i == 0 && type != ROMDISK_BOOTLOADER) continue;
		if (i && Romdisks[i].data) continue;
		if (data) {
			Romdisks[i].data = data;
			Romdisks[i].type = type;
		}
		return i;
	}
	return -1;
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

void romdiskDump(void)
{
	int i;
	for(i=0;i<ROMDISK_MAX_NB;i++) if (Romdisks[i].data) {
		PRINTF(LOG_SYS,"> romdisk %d: (type=%d)\n",i,Romdisks[i].type);
	
		int j = 1;
		int* table = (int*)Romdisks[i].data;
		if (!table) return;
		while (table[j]) {
			PRINTF(LOG_SYS,"> ---- %s\n",&Romdisks[i].data[table[j]]);
			j++;
		}
	}
}

LB* romdiskReadContent(int romdiskId, char* path, int* size)
{
	int len;
	LB* p;
	char* file;
//	PRINTF(LOG_DEV,"romdiskReadContent #%d %s\n", romdiskId, path);
	if ((romdiskId < 0) || (romdiskId >= ROMDISK_MAX_NB) || !Romdisks[romdiskId].data) return NULL;
	file= romdiskLookup(Romdisks[romdiskId].data, path, &len);
	if (file == NULL) return NULL;
	if (size) *size = len;
	p = memoryAllocStr(file, len); if (!p) return NULL;
	return p;
}

LINT romdiskDirectoryList(int romdiskId, Buffer* out, char* dir) // we expect dir to end with '/'
{
	int i = 1;
	int len;
	char* romdisk;
	int* table;

	if ((romdiskId < 0) || (romdiskId >= ROMDISK_MAX_NB) || !Romdisks[romdiskId].data) return 0;
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
				_fsAddFileInfo(out, start, nameLen, "0 0 d");
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
				_fsAddFileInfo(out, start, -1, tmpAttr);
			}
		}
		i++;
	}
	return 0;
}

int romdiskVolumeList(Thread* th, int* n)
{
	int j;
	for (j = ROMDISK_MAX_NB-1; j>=0; j--) if (Romdisks[j].data)
	{
		unsigned int* table = (unsigned int*)Romdisks[j].data;
		if (_volumeList(th, MM.romdiskVolume, j, 0, 1, table[0])) return EXEC_OM;
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
	LINT t0;
	LB* bin;
	char* bufferBootLoader;   
	for(;uartGet()>=0;);
	uartPut("<BOOTDISK>",10);

	t0=hwTimeMs()+1000;
	while(i<4) {
		int c=uartGet();
		if (c>=0) buffer[i++]=c;
		if ((hwTimeMs()-t0)>0) {
			uartPut("<SKIP>\n",7);
			return NULL;
		}
	}
	TimeDelta=*(unsigned int*)buffer;
//	PRINTF(LOG_DEV,"time %x\n",TimeDelta);
	i=0;
	while(i<4) {
		int c=uartGet();
		if (c>=0) buffer[i++]=c;
		if ((hwTimeMs()-t0)>0) {
			uartPut("<DONE>\n",7);
			return NULL;
		}
	}
	len=*(int*)buffer;
	bin = memoryAllocStr(NULL,len + 1);
	bufferBootLoader= STR_START(bin);
	memcpy(bufferBootLoader,buffer,4);
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

void romdiskInit(void)
{
	int j;
	for (j = 0; j < ROMDISK_MAX_NB; j++) Romdisks[j].data = NULL;
#ifdef USE_FS_ROMDISK0
	romdiskAdd((char*)Romdisk0Content, ROMDISK_NATIVE);
#endif
#ifdef USE_BOOTLOADER
	{
		char* RomdiskBootLoader = bootDiskLoader();
		if (RomdiskBootLoader) romdiskAdd(RomdiskBootLoader, ROMDISK_BOOTLOADER);
	}
#endif
}

int romdiskImport(LB* bin)
{
	char* src = STR_START(bin);
	LINT len = STR_LENGTH(bin);

	if (romdiskAdd(NULL, ROMDISK_USER)<0) return -1;

	if (!romdiskCheck(src, (unsigned int)len)) {
		PRINTF(LOG_SYS,">Error: romdiskCheck failed\n");
		return -1;
	}
	return romdiskAdd(src, ROMDISK_USER);
}
int romdiskMount(int standalone)
{
#ifndef BOOT_SKIP_FS_ROMDISK0
	int j;
	for (j = 1; j <ROMDISK_MAX_NB; j++) 	// last declared, first order
	{
		if (Romdisks[j].data) _partitionAdd(MM.romdiskVolume,j,"");
	}
	if (Romdisks[0].data) _partitionAdd(MM.romdiskVolume, 0, "");
#endif
	return 0;
}
void romdiskReleaseUserDisk(void)
{
	int i;
	for (i = 0; i < ROMDISK_MAX_NB; i++) if (Romdisks[i].data && Romdisks[i].type==ROMDISK_USER) Romdisks[i].data = NULL;
}
void romdiskRelease(void)
{
	int i;
	for (i = 0; i < ROMDISK_MAX_NB; i++) if (Romdisks[i].data && Romdisks[i].data!= Romdisk0Content) Romdisks[i].data = NULL;
}
