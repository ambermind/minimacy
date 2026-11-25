// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"inc/efi.h"
#include"inc/efilib.h"
#include "../../src/minimacy.h"
#include "uefi.h"

#define PAGE_SIZE 4096ULL

#define EFI_CONVENTIONAL_MEMORY 7		// memory map type for free memory

char *StorageSafeBuffer; // DMA-safe buffer for disk I/O
char  *MemoryHeap;
UINT64 MemoryHeapSize;

// Allocate `size` bytes of pages below 4 GB (for buffers handed to 32bits device drivers)
void *memAllocLow(UINTN size)
{
	EFI_PHYSICAL_ADDRESS a = 0xFFFFFFFF;
	UINTN pages = (size + PAGE_SIZE -1) / PAGE_SIZE;
	if (BS->AllocatePages(AllocateMaxAddress, EfiLoaderData, pages, &a) != EFI_SUCCESS) return NULL;
	return (void*)(UINTN)a;
}

// Reserve the largest possible contiguous block, leaving at least `reserve`
// bytes free for the firmware (its drivers keep allocating while we run).
// Returns the block address and stores its size in *outSize, or NULL on failure.
void *memAllocLargest(UINT64 reserve, UINT64 *outSize)
{
	UINTN mapSize = 0, mapKey, descSize;
	UINT32 descVer;
	EFI_MEMORY_DESCRIPTOR *map = NULL;
	*outSize = 0;

	// 1. Read the memory map (the first call only returns the required size)
	BS->GetMemoryMap(&mapSize, NULL, &mapKey, &descSize, &descVer);
	mapSize += 8 * descSize;				// allocating the map buffer may add entries
	if (BS->AllocatePool(EfiLoaderData, mapSize, (void**)&map) != EFI_SUCCESS) return NULL;
	if (BS->GetMemoryMap(&mapSize, map, &mapKey, &descSize, &descVer) != EFI_SUCCESS) {
		BS->FreePool(map);
		return NULL;
	}

	// 2. Find the largest free region and the total amount of free memory
	UINT64 bestStart = 0, bestPages = 0, freeTotal = 0, total=0;
	for (UINT8 *p = (UINT8*)map; p < (UINT8*)map + mapSize; p += descSize) {	// stride = descSize!
		EFI_MEMORY_DESCRIPTOR *d = (EFI_MEMORY_DESCRIPTOR*)p;
		total+=d->NumberOfPages * PAGE_SIZE;
//		PRINTF(LOG_SYS, "block %llx: %d %s\n", d->PhysicalStart,d->NumberOfPages* PAGE_SIZE/1024, (d->Type == EFI_CONVENTIONAL_MEMORY)?"FREE":"");
		if (d->Type != EFI_CONVENTIONAL_MEMORY) continue;
		//PRINTF(LOG_SYS, "free block %llx: %d\n", d->PhysicalStart,d->NumberOfPages* PAGE_SIZE/1024);
		freeTotal += d->NumberOfPages * PAGE_SIZE;
		if (d->NumberOfPages > bestPages) {
				bestPages = d->NumberOfPages;
				bestStart = d->PhysicalStart;
		}
	}
	BS->FreePool(map);

//	PRINTF(LOG_SYS, "best block %llx: %d / %d\n", bestStart, bestPages* PAGE_SIZE/1024, total/1024);
	// 3. Target size: the largest region, minus what must be left to the firmware
	UINT64 othersFree = freeTotal - bestPages * PAGE_SIZE;   // free memory outside this region
	UINT64 take = bestPages * PAGE_SIZE;
	if (othersFree < reserve) take = (take > reserve - othersFree) ? take - (reserve - othersFree) : 0;
	UINT64 pages = take / PAGE_SIZE;

	// 4. Allocate at the start of the region: the firmware tends to allocate
	//	from the top, so the end of the region remains its margin
	EFI_PHYSICAL_ADDRESS addr = bestStart;
	if (pages && BS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &addr) == EFI_SUCCESS) {
		*outSize = pages * PAGE_SIZE;
		return (void*)(UINTN)addr;
	}

	// 5. Fallback: the map may have changed in the meantime; let the firmware
	//	pick the address, shrinking the request until it succeeds
	while (pages >= 256) {									// not below 1 MB
		if (BS->AllocatePages(AllocateAnyPages, EfiLoaderData, pages, &addr) == EFI_SUCCESS) {
			*outSize = pages * PAGE_SIZE;
			return (void*)(UINTN)addr;
		}
		pages -= pages / 8;	// -12.5% per attempt
	}
	return NULL;
}

EFI_STATUS memInit(void)
{
	StorageSafeBuffer = memAllocLow(STORAGE_SAFE_BUFFER_SIZE);
	if (!StorageSafeBuffer) {
		PRINTF(LOG_SYS, "> UEFI: no memory\n");	// should never happen
		return EFI_OUT_OF_RESOURCES;
	}
	MemoryHeap = memAllocLargest(UEFI_RESERVE_SIZE, &MemoryHeapSize);	// save 32 MB for Uefi internal processes
	if (!MemoryHeap) {
		PRINTF(LOG_SYS, "> UEFI: no memory\n");	// should never happen
		return EFI_OUT_OF_RESOURCES;
	}
	bmmUpdateStart(0,MemoryHeap);
	bmmUpdateSize(0,MemoryHeapSize);
	PRINTF(LOG_SYS, "> UEFI: dynamic memory: %lld MB at %llx\n", (unsigned long long)(MemoryHeapSize >> 20), (unsigned long long)(UINTN)MemoryHeap);
	return EFI_SUCCESS;
}

// Release the memory before returning from efi_main (e.g. back to the UEFI shell)
void memRelease(void)
{
	if (StorageSafeBuffer) BS->FreePages((EFI_PHYSICAL_ADDRESS)(UINTN)StorageSafeBuffer, STORAGE_SAFE_BUFFER_SIZE / PAGE_SIZE);
	StorageSafeBuffer = NULL;

	if (MemoryHeap) BS->FreePages((EFI_PHYSICAL_ADDRESS)(UINTN)MemoryHeap, MemoryHeapSize / PAGE_SIZE);
	MemoryHeap = NULL;
	MemoryHeapSize = 0;
}
