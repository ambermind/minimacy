// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"inc/efi.h"
#include"inc/efilib.h"
#include "../../src/minimacy.h"
#include "uefi.h"

EFI_SYSTEM_TABLE  *ST;
EFI_BOOT_SERVICES *BS;
EFI_HANDLE        ImageHandle;

EFI_GUID gEfiPciIoProtocolGuid           = EFI_PCI_IO_PROTOCOL_GUID;
EFI_GUID gEfiDevicePathProtocolGuid      = EFI_DEVICE_PATH_PROTOCOL_GUID;
EFI_GUID gEfiBlockIoProtocolGuid         = EFI_BLOCK_IO_PROTOCOL_GUID;
EFI_GUID gEfiDiskIoProtocolGuid          = EFI_DISK_IO_PROTOCOL_GUID;
EFI_GUID gEfiSimpleNetworkProtocolGuid   = EFI_SIMPLE_NETWORK_PROTOCOL_GUID;

EFI_GUID gEfiTimestampProtocolGuid       = EFI_TIMESTAMP_PROTOCOL_GUID;
EFI_GUID gEfiRngProtocolGuid             = EFI_RNG_PROTOCOL_GUID;

EFI_GUID gEfiSerialIoProtocolGuid        = EFI_SERIAL_IO_PROTOCOL_GUID;
EFI_GUID gEfiSimpleTextOutProtocolGuid   = EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID;
EFI_GUID gEfiSimpleTextInProtocolGuid    = EFI_SIMPLE_TEXT_INPUT_PROTOCOL_GUID;
EFI_GUID gEfiSimpleTextInExProtocolGuid  = EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID;

EFI_GUID gEfiGraphicsOutputProtocolGuid  = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
EFI_GUID gEfiSimplePointerProtocolGuid   = EFI_SIMPLE_POINTER_PROTOCOL_GUID;
EFI_GUID gEfiAbsolutePointerProtocolGuid = EFI_ABSOLUTE_POINTER_PROTOCOL_GUID;

LINT lastActive=0;
void selectPollingTemporisation(LINT nbActive, LINT now, LINT maxWait)
{
	acpiPoll();
	consolePoll();
	pointerPoll();
	if (nbActive) lastActive=now;
	else if ((now-lastActive>20)&&(maxWait>=10)) {
		__asm__ volatile("hlt");
	}
}

const char* Argv[]={"minimacy","/baremetal/baremetal.uefi.boot.mcy"};

EFI_STATUS efi_main(EFI_HANDLE _ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	ST = SystemTable;
	ImageHandle=_ImageHandle;
	BS = ST->BootServices;

	BS->SetWatchdogTimer(0,0,0,NULL);
	ST->ConOut->SetMode(ST->ConOut,0);
//	Status=ST->ConOut->SetMode(ST->ConOut,ST->ConOut->Mode->MaxMode-1);
//	ST->ConOut->SetAttribute(ST->ConOut,EFI_LIGHTBLUE);
	ST->ConOut->SetAttribute(ST->ConOut,EFI_BROWN);
	ST->ConOut->EnableCursor(ST->ConOut,1);

	termInit();
	consoleInit();
	pointerInit();
	acpiInit();
	timeInit();
	rngInit();
	storageInit();
	netInit();
	graphicInit();
	memInit();

	BS->Stall(1000000);	// uncomment to have time to watch the boot initial messages on a display

	start(2,Argv);
	memRelease();
	ST->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
	//while(1){
	//	EFI_INPUT_KEY Key;
	//	EFI_STATUS Status;
	//	PRINTF(LOG_SYS,">");
	//	while ((Status = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY) ;
	//	PRINTF(LOG_DEV,"key: %d (%x):%x\n",Key.UnicodeChar,Key.UnicodeChar,Key.ScanCode);
	//}
	return EFI_SUCCESS;
}
