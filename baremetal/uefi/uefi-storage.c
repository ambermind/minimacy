// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"inc/efi.h"
#include"inc/efilib.h"
#include"../../src/minimacy.h"
#include"uefi.h"

#define NB_STORAGE_UEFI_TABLE 16
EFI_BLOCK_IO_PROTOCOL* EfiBlockIO[NB_STORAGE_UEFI_TABLE];
EFI_DISK_IO_PROTOCOL* EfiDiskIO[NB_STORAGE_UEFI_TABLE];
int EfiIoCount=0;

void storageInit()
{
	int i;
	EFI_STATUS Status;
	EFI_HANDLE efiHandle[NB_STORAGE_UEFI_TABLE];
	UINTN efiHandleSize=NB_STORAGE_UEFI_TABLE*sizeof(EFI_HANDLE);

	EfiIoCount=0;
	Status=BS->LocateHandle(ByProtocol,&gEfiBlockIoProtocolGuid,NULL,&efiHandleSize,efiHandle);
	if (Status!=EFI_SUCCESS) return;
	efiHandleSize/=sizeof(EFI_HANDLE);
	for(i=0;i<efiHandleSize;i++) {
		EFI_BLOCK_IO_PROTOCOL* h;
		EFI_DISK_IO_PROTOCOL* hdisk;
		Status=BS->HandleProtocol(efiHandle[i],&gEfiBlockIoProtocolGuid,(void**)&h);
		if (Status!=EFI_SUCCESS) continue;
		Status=BS->HandleProtocol(efiHandle[i],&gEfiDiskIoProtocolGuid,(void**)&hdisk);
		if (Status!=EFI_SUCCESS) continue;
	
		//PRINTF(LOG_DEV,"----%d\n");
		//PRINTF(LOG_DEV,"MediaId: %d\n",h->Media->MediaId);
		//PRINTF(LOG_DEV,"RemovableMedia: %d\n",h->Media->RemovableMedia);
		//PRINTF(LOG_DEV,"MediaPresent: %d\n",h->Media->MediaPresent);
		//PRINTF(LOG_DEV,"LogicalPartition: %d\n",h->Media->LogicalPartition);
		//PRINTF(LOG_DEV,"ReadOnly: %d\n",h->Media->ReadOnly);
		//PRINTF(LOG_DEV,"BlockSize: %d\n",h->Media->BlockSize);
		//PRINTF(LOG_DEV,"LastBlock: %d\n",h->Media->LastBlock);
		//PRINTF(LOG_DEV,"IoAlign: %d\n",h->Media->IoAlign);
		//PRINTF(LOG_DEV,"LowestAlignedLba: %d\n",h->Media->LowestAlignedLba);
		//PRINTF(LOG_DEV,"LogicalBlocksPerPhysicalBlock: %d\n",h->Media->LogicalBlocksPerPhysicalBlock);
		//PRINTF(LOG_DEV,"Capacity: %d\n",h->Media->BlockSize*(1+h->Media->LastBlock));
		if (h->Media->MediaPresent && !h->Media->LogicalPartition && h->Media->BlockSize==512) {
			if (h->Media->BlockSize*h->Media->LastBlock<UEFI_STORAGE_MINIMUM_SIZE) continue;
			PRINTF(LOG_SYS,"> UEFI: storage uefi%d: %s, %d sectors (%d bytes per sector) align %d\n",
				EfiIoCount, h->Media->ReadOnly?"readonly":"writable", h->Media->LastBlock+1,
				h->Media->BlockSize, h->Media->IoAlign);
			EfiBlockIO[EfiIoCount]=h;
			EfiDiskIO[EfiIoCount++]=hdisk;
		}
	}
}	

int storageRead(int index, char* buffer, int start, int nb)
{
	EFI_STATUS Status;
	EFI_BLOCK_IO_MEDIA *Media;
	int i, len;

	if (index<0 || index>=EfiIoCount) return -1;
	Media=EfiBlockIO[index]->Media;
	len=nb*Media->BlockSize;
	for(i=0;i<len;i+=STORAGE_SAFE_BUFFER_SIZE) {
		int toRead=(len-i>STORAGE_SAFE_BUFFER_SIZE)?STORAGE_SAFE_BUFFER_SIZE:len-i;
	//	Status=EfiBlockIO[index]->ReadBlocks(EfiBlockIO[index],Media->MediaId,start,nb*Media->BlockSize,buffer);
		Status=EfiDiskIO[index]->ReadDisk(EfiDiskIO[index],Media->MediaId,start*Media->BlockSize,toRead,StorageSafeBuffer);
		if (Status!=EFI_SUCCESS) return -1;
		memcpy(buffer+i, StorageSafeBuffer, toRead);
	}
	return len;
}
int storageWrite(int index, char* buffer, int start, int len)
{
	EFI_STATUS Status;
	EFI_BLOCK_IO_MEDIA *Media;
	int i;
	int nb=len>>9;
	if (index<0 || index>=EfiIoCount) return -1;
	Media=EfiBlockIO[index]->Media;
	len=nb*Media->BlockSize;
	for(i=0;i<len;i+=STORAGE_SAFE_BUFFER_SIZE) {
		int toWrite=(len-i>STORAGE_SAFE_BUFFER_SIZE)?STORAGE_SAFE_BUFFER_SIZE:len-i;
		memcpy(StorageSafeBuffer, buffer+i, toWrite);
//	Status=EfiBlockIO[index]->WriteBlocks(EfiBlockIO[index],EfiBlockIO[index]->Media->MediaId,start,nb*EfiBlockIO[index]->Media->BlockSize,buffer);
		Status=EfiDiskIO[index]->WriteDisk(EfiDiskIO[index],Media->MediaId,start*Media->BlockSize,toWrite,StorageSafeBuffer);
		if (Status!=EFI_SUCCESS) return -1;
	}
	Status=EfiBlockIO[index]->FlushBlocks(EfiBlockIO[index]);
	return (Status==EFI_SUCCESS)?nb*EfiBlockIO[index]->Media->BlockSize:-1;
}
int storageNbSectors(int index)
{
	if (index<0 || index>=EfiIoCount) return -1;
	return EfiBlockIO[index]->Media->LastBlock+1;
}
int storageSectorSize(int index)
{
	if (index<0 || index>=EfiIoCount) return -1;
	return EfiBlockIO[index]->Media->BlockSize;
}
int storageWritable(int index)
{
	if (index<0 || index>=EfiIoCount) return -1;
	return EfiBlockIO[index]->Media->ReadOnly?0:1;
}
int storageCount()
{
	return EfiIoCount;
}

