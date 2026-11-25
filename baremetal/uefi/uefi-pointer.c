// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"inc/efi.h"
#include"inc/efilib.h"
#include "../../src/minimacy.h"
#include "uefi.h"

#define MAX_PTR 4
static EFI_SIMPLE_POINTER_PROTOCOL   *EfiRelativePointer[MAX_PTR];
static EFI_ABSOLUTE_POINTER_PROTOCOL *EfiAbsolutePointer[MAX_PTR];
static int RelCount, AbsCount;

// Collect every pointer device, skipping the aggregated console handle
void pointerInit(void)
{
    EFI_HANDLE *h;
    UINTN n, i;

    RelCount = AbsCount = 0;
    if (BS->LocateHandleBuffer(ByProtocol, &gEfiSimplePointerProtocolGuid, NULL, &n, &h) == EFI_SUCCESS) {
        for (i = 0; i < n && RelCount < MAX_PTR; i++) {
            if (h[i] == ST->ConsoleInHandle) continue;
            if (BS->HandleProtocol(h[i], &gEfiSimplePointerProtocolGuid, (void**)&EfiRelativePointer[RelCount]) != EFI_SUCCESS) continue;
            EfiRelativePointer[RelCount]->Reset(EfiRelativePointer[RelCount], FALSE);          // some drivers need it to start reporting
            PRINTF(LOG_SYS, "> UEFI: mouse #%d: relative, res=%d,%d\n", RelCount,
                   (int)EfiRelativePointer[RelCount]->Mode->ResolutionX, (int)EfiRelativePointer[RelCount]->Mode->ResolutionY);
            RelCount++;
        }
        BS->FreePool(h);
    }
    if (BS->LocateHandleBuffer(ByProtocol, &gEfiAbsolutePointerProtocolGuid, NULL, &n, &h) == EFI_SUCCESS) {
        for (i = 0; i < n && AbsCount < MAX_PTR; i++) {
            if (h[i] == ST->ConsoleInHandle) continue;
            if (BS->HandleProtocol(h[i], &gEfiAbsolutePointerProtocolGuid, (void**)&EfiAbsolutePointer[AbsCount]) != EFI_SUCCESS) continue;
            EfiAbsolutePointer[AbsCount]->Reset(EfiAbsolutePointer[AbsCount], FALSE);
            PRINTF(LOG_SYS, "> UEFI: mouse #%d: absolute, x=%d..%d y=%d..%d\n", AbsCount,
                   (int)EfiAbsolutePointer[AbsCount]->Mode->AbsoluteMinX, (int)EfiAbsolutePointer[AbsCount]->Mode->AbsoluteMaxX,
                   (int)EfiAbsolutePointer[AbsCount]->Mode->AbsoluteMinY, (int)EfiAbsolutePointer[AbsCount]->Mode->AbsoluteMaxY);
            AbsCount++;
        }
        BS->FreePool(h);
    }
    if (!RelCount && !AbsCount) PRINTF(LOG_SYS, "> UEFI: no pointer device\n");
}

// Poll all pointer devices; GetState returns EFI_NOT_READY when nothing changed
void pointerPoll(void)
{
    int i;
    for (i = 0; i < RelCount; i++) {
        EFI_SIMPLE_POINTER_STATE s;
        if (EfiRelativePointer[i]->GetState(EfiRelativePointer[i], &s) == EFI_SUCCESS) {
			short event[6];
			event[0]=1;
			event[1]=(short)s.RelativeMovementX;
			event[2]=(short)s.RelativeMovementY;
			event[3]=(short)s.RelativeMovementZ;
			event[4]=(short)s.LeftButton;
			event[5]=(short)s.RightButton;
			eventBinary(UefiEvent, (char*)event, 6*2);
            //PRINTF(LOG_SYS, "rel #%d dx=%d dy=%d dz=%d L=%d R=%d\n", i,
            //       (int)s.RelativeMovementX, (int)s.RelativeMovementY, (int)s.RelativeMovementZ,
            //       (int)s.LeftButton, (int)s.RightButton);
		}
    }
    for (i = 0; i < AbsCount; i++) {
        EFI_ABSOLUTE_POINTER_STATE s;
        if (EfiAbsolutePointer[i]->GetState(EfiAbsolutePointer[i], &s) == EFI_SUCCESS) {
			short event[6];
			EFI_ABSOLUTE_POINTER_MODE* Mode=EfiAbsolutePointer[i]->Mode;
			event[0]=2;
			event[1]=(short)s.CurrentX - Mode->AbsoluteMinX ;
			event[2]=(short)s.CurrentY - Mode->AbsoluteMinY ;
			event[3]=(short)Mode->AbsoluteMaxX - Mode->AbsoluteMinX ;
			event[4]=(short)Mode->AbsoluteMaxY - Mode->AbsoluteMinY ;
			event[5]=(short)s.ActiveButtons;
			eventBinary(UefiEvent, (char*)event, 6*2);
            //PRINTF(LOG_SYS, "abs #%d x=%d y=%d buttons=%x\n", i,
            //       (int)s.CurrentX, (int)s.CurrentY, (unsigned)s.ActiveButtons);
    	}
	}
}

