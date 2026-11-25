// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#define EFI_TIMESTAMP_PROTOCOL_GUID { 0xafbfde41, 0x2e6e, 0x4262,{ 0xba, 0x65, 0x62, 0xb9, 0x23, 0x6e, 0x54, 0x95 }}
extern EFI_GUID gEfiTimestampProtocolGuid;
extern EFI_GUID gEfiRngProtocolGuid;
extern EFI_GUID gEfiSimpleTextInExProtocolGuid;
extern EFI_GUID gEfiSimplePointerProtocolGuid;
extern EFI_GUID gEfiAbsolutePointerProtocolGuid;

extern EFI_HANDLE ImageHandle;
extern char *StorageSafeBuffer;
#define STORAGE_SAFE_BUFFER_SIZE  (256 * 1024)	// must be a multiple of 4096
#define UEFI_RESERVE_SIZE  (32 * 1024 * 1024)

unsigned long long rdtsc(void);

EFI_STATUS memInit(void);
void memRelease(void);

int pathContainsUart(EFI_HANDLE h);
int pathGetSerialUid(EFI_HANDLE h);
const char* keyboardInputType(EFI_HANDLE h);
int netIsPhysical(EFI_HANDLE h);

extern int UefiEvent;
void consoleInit(void);
void consoleOnDisplay(int useDisplay);
int uefiHasSerial(void);
void uartPutChar(unsigned int c);
void uartPut(char *s,int len);
int uartReadable(void);
int uartGet(void);
void consolePoll(void);

void acpiInit(void);
void acpiPoll(void);

void graphicInit(void);
void timeInit(void);
void netInit(void);
void rngInit(void);
void storageInit(void);

void pointerInit(void);
void pointerPoll(void);


