// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"inc/efi.h"
#include"inc/efilib.h"
#include "../../src/minimacy.h"
#include "uefi.h"

//--------------------- Shutdown detection (speed up the VM shutdown process) ACPI stands for "Advanced Configuration and Power Interface"
EFI_GUID Acpi20Guid = ACPI_20_TABLE_GUID;
UINT16 Pm1aEvt;  // state register PM1a (0 when unkown)

inline UINT16 inw(UINT16 p)  { UINT16 v; __asm__ volatile("inw %1,%0" : "=a"(v) : "Nd"(p)); return v; }
inline void outw(UINT16 p, UINT16 v) { __asm__ volatile("outw %0,%1" : : "a"(v), "Nd"(p)); }

void acpiInit(void)
{
	UINT16 Pm1aEn;
	Pm1aEvt=0;
	for (UINTN i = 0; i < ST->NumberOfTableEntries; i++) {
		EFI_CONFIGURATION_TABLE *t = &ST->ConfigurationTable[i];
		if (memcmp(&t->VendorGuid, &Acpi20Guid, sizeof(EFI_GUID))) continue;

		UINT8 *rsdp = t->VendorTable;
		UINT64 xsdtAddr;
		memcpy(&xsdtAddr, rsdp + 24, 8);	// RSDP.XsdtAddress
		UINT8 *xsdt = (UINT8*)(UINTN)xsdtAddr;
		UINT32 len;
		memcpy(&len, xsdt + 4, 4);
		for (UINT32 off = 36; off + 8 <= len; off += 8) {
			UINT64 a;
			memcpy(&a, xsdt + off, 8);
			UINT8 *tbl = (UINT8*)(UINTN)a;
			if (memcmp(tbl, "FACP", 4) == 0) {	// FADT
				UINT32 pm1a;
				memcpy(&pm1a, tbl + 56, 4);	// PM1a_EVT_BLK (port d'E/S)
				UINT8 evtLen = tbl[88]; 
				Pm1aEvt = (UINT16)pm1a;
				Pm1aEn  = (UINT16)(pm1a + evtLen / 2);
			}
		}
	}
	if (Pm1aEvt) {
		outw(Pm1aEvt, 0x0100);	// remove old press
		outw(Pm1aEn, inw(Pm1aEn) | 0x0100);	// PWRBTN_EN
		PRINTF(LOG_SYS, "> UEFI: ACPI ready\n");
	}
	else PRINTF(LOG_SYS, "> UEFI: ACPI not found\n");

}

void acpiPoll(void)
{
	if (Pm1aEvt && (inw(Pm1aEvt) & 0x0100)) {	// PWRBTN_STS
		outw(Pm1aEvt, 0x0100);	// clear
		PRINTF(LOG_SYS, "\n> UEFI: power button: shutdown\n");
		//BS->Stall(5000000);
		ST->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
	}
}

