// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"inc/efi.h"
#include"inc/efilib.h"
#include "../../src/minimacy.h"
#include "uefi.h"

#define EISA_PNP0501 0x050141D0
#define EISA_PNP0303 0x030341D0

#define _GET_PATH(error)  EFI_DEVICE_PATH_PROTOCOL *dp; if (BS->HandleProtocol((h), &gEfiDevicePathProtocolGuid, (void**)&dp) != EFI_SUCCESS) return (error);
#define _GET_PATH_INT32(offset) (*(UINT32*)((UINT8*)dp+(offset)))
#define _CHECK_TYPE(type, subtype) (dp->Type == (type) && dp->SubType == (subtype))
#define _END_OF_PATH _CHECK_TYPE(0x7f, 0xff)
#define _NEXT_PATH dp = (EFI_DEVICE_PATH_PROTOCOL*)((UINT8*)dp + (dp->Length[0] | (dp->Length[1] << 8)));

int pathContainsUart(EFI_HANDLE h)
{
	_GET_PATH(0);
	 while (!_END_OF_PATH) {
		if (_CHECK_TYPE(3, 14)) return 1;	// UART node
		_NEXT_PATH
	}
	return 0;
}

int pathGetSerialUid(EFI_HANDLE h)
{
	_GET_PATH(-1);
	while (!_END_OF_PATH) {
		if (_CHECK_TYPE(2, 1) && (_GET_PATH_INT32(4)==EISA_PNP0501)) return _GET_PATH_INT32(8);
		_NEXT_PATH
	}
	return -1;
}

const char* keyboardInputType(EFI_HANDLE h)
{
	 const char* type = "?";
	if (h==ST->ConsoleInHandle) return "aggregated";
	_GET_PATH(type);
	while (!_END_OF_PATH) {
		if (_CHECK_TYPE(3, 5)) type = "usb";
		if (_CHECK_TYPE(3, 15)) type = "usb";
		if (_CHECK_TYPE(2, 1) && (_GET_PATH_INT32(4)== EISA_PNP0303)) return "ps2";
		_NEXT_PATH
	}
	return type;
}

int netIsPhysical(EFI_HANDLE h)
{
	EFI_DEVICE_PATH_PROTOCOL *last = NULL;
	_GET_PATH(0);
	while (!_END_OF_PATH) {
		last = dp;
		_NEXT_PATH
	}
	return last && last->Type == 3 && last->SubType == 11;	// MESSAGING / MAC
}


#define MAX_PROTOCOL_COUNT 512
void listProtocols()
{
	int i;
	EFI_STATUS Status;
	EFI_HANDLE efiHandle[MAX_PROTOCOL_COUNT];
	UINTN efiHandleSize=MAX_PROTOCOL_COUNT*sizeof(EFI_HANDLE);

	Status=BS->LocateHandle(AllHandles,&gEfiBlockIoProtocolGuid,NULL,&efiHandleSize,efiHandle);
	if (Status!=EFI_SUCCESS && Status!=EFI_BUFFER_TOO_SMALL) return;
	efiHandleSize/=sizeof(EFI_HANDLE);
	PRINTF(LOG_DEV,"nb of handles: %d\n",efiHandleSize);
	if (efiHandleSize>MAX_PROTOCOL_COUNT) efiHandleSize=MAX_PROTOCOL_COUNT;
	for(i=0;i<efiHandleSize;i++) {
		EFI_GUID **ProtocolGuidArray;
		UINTN protocolCount=0;
		int j;
		Status = BS->ProtocolsPerHandle(efiHandle[i],&ProtocolGuidArray,&protocolCount);
//		PRINTF(LOG_DEV,"handle %d: %d protocols\n",i,protocolCount);
		for(j=0;j<protocolCount;j++) {
			EFI_GUID* g=ProtocolGuidArray[j];
			PRINTF(LOG_DEV,"{0x%08x,0x%04x,0x%04x,{0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x}}\n",g->Data1,g->Data2,g->Data3,g->Data4[0],g->Data4[1],g->Data4[2],g->Data4[3],g->Data4[4],g->Data4[5],g->Data4[6],g->Data4[7]);
		}
		if (ProtocolGuidArray != NULL) BS->FreePool(ProtocolGuidArray);
	}
}
