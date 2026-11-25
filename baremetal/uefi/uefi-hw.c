// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"inc/efi.h"
#include"inc/efilib.h"
#include"../../src/minimacy.h"
#include"uefi.h"

EFI_GRAPHICS_OUTPUT_PROTOCOL *EfiGraphic=NULL;
void graphicInit(void)
{
	EFI_STATUS Status = BS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid,NULL,(void**)&EfiGraphic);
}

int fun_consoleSetMode(Thread* th)
{
	int mode=STACK_INT(th,0);
	ST->ConOut->SetMode(ST->ConOut,mode);
}
int fun_consoleOnDisplay(Thread* th)
{
	int useDisplay=STACK_BOOL(th,0);
	consoleOnDisplay(useDisplay);
	return 0;
}
int fun_uefiHasSerial(Thread* th)
{
	FUN_RETURN_BOOL(uefiHasSerial());
}
int fun_uefiEventId(Thread* th)
{
	FUN_RETURN_INT(UefiEvent);
}

int fun_consoleModes(Thread* th)
{
	int i;
	for(i=0;i<ST->ConOut->Mode->MaxMode;i++) {
		UINTN width,height;
		ST->ConOut->QueryMode(ST->ConOut,i,&width,&height);
		PRINTF(LOG_USER,"%d: %d x %d\n",i,width,height);
	}
	FUN_RETURN_BOOL(1);
}
int fun_consoleCls(Thread* th)
{
	ST->ConOut->ClearScreen(ST->ConOut);
	FUN_RETURN_BOOL(1);
}

int fun_graphicModes(Thread* th)
{
	int i;
	if (!EfiGraphic) FUN_RETURN_NIL
	for(i=0;i<EfiGraphic->Mode->MaxMode;i++){
		EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info;
		UINTN size;
		EFI_STATUS Status=EfiGraphic->QueryMode(EfiGraphic,i,&size,&info);
		FUN_PUSH_INT(i);
		FUN_PUSH_INT(info->HorizontalResolution);
		FUN_PUSH_INT(info->VerticalResolution);
		FUN_PUSH_INT(info->PixelFormat);
		FUN_MAKE_ARRAY(4, DBG_TUPLE);
//		PRINTF(LOG_USER,"%d: %d x %d\n",i,info->HorizontalResolution,info->VerticalResolution);
	}
	FUN_PUSH_NIL;
	while (i--) FUN_MAKE_ARRAY(LIST_LENGTH, DBG_LIST);
	return 0;
}
int fun_graphicSetMode(Thread* th)
{
	EFI_STATUS Status;
	int mode=STACK_INT(th,0);
	if (!EfiGraphic) FUN_RETURN_NIL;
	Status=EfiGraphic->SetMode(EfiGraphic,mode);
	if (Status!=EFI_SUCCESS) {
		mode=0;
		Status=EfiGraphic->SetMode(EfiGraphic,mode);
	}
	//PRINTF(LOG_DEV,"graphicSetMode %d\n",mode);
	if (Status!=EFI_SUCCESS) {
		EfiGraphic=NULL;
		FUN_RETURN_NIL
	}
	consoleOnDisplay(0);
	FUN_RETURN_INT(mode)
}

int fun_graphicW(Thread* th)
{
	if (!EfiGraphic) FUN_RETURN_NIL;
	FUN_RETURN_INT(EfiGraphic->Mode->Info->HorizontalResolution);
}
int fun_graphicH(Thread* th)
{
	if (!EfiGraphic) FUN_RETURN_NIL;
	FUN_RETURN_INT(EfiGraphic->Mode->Info->VerticalResolution);
}

int fun_graphicBlit(Thread* th)
{
	EFI_STATUS Status;
	int hIsNil= STACK_IS_NIL(th,0);
	LINT h = STACK_INT(th,0);
	int wIsNil= STACK_IS_NIL(th,1);
	LINT w = STACK_INT(th,1);
	LINT ysrc = STACK_INT(th,2);
	LINT xsrc = STACK_INT(th,3);
	LBitmap* a = (LBitmap*)STACK_PNT(th,4);
	LINT ydst = STACK_INT(th,5);
	LINT xdst = STACK_INT(th,6);
	if (!EfiGraphic) FUN_RETURN_NIL;
	if (!a) FUN_RETURN_NIL;
	if (wIsNil) w= a->w - xsrc;
	if (hIsNil) h= a->h - ysrc;
	
	LINT dx, dy;
	if (_clip1D(0, a->w, &xsrc, &w, &dx)) FUN_RETURN_NIL;
	if (_clip1D(0, a->h, &ysrc, &h, &dy)) FUN_RETURN_NIL;
	xdst += dx; ydst += dy;
	if (_clip1D(0, EfiGraphic->Mode->Info->HorizontalResolution, &xdst, &w, &dx)) FUN_RETURN_NIL;
	if (_clip1D(0, EfiGraphic->Mode->Info->VerticalResolution, &ydst, &h, &dy)) FUN_RETURN_NIL;
	xsrc += dx; ysrc += dy;

#ifdef USE_UEFI_MANUAL_BLIT
	if (sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)==4) {
		long long p=EfiGraphic->Mode->FrameBufferBase + 4*(xdst+ydst*EfiGraphic->Mode->Info->PixelsPerScanLine);
		int* dst=(int*)p;
		int* src=a->start32+ xsrc + ysrc*a->next32;
		while((h--)>0) {
			memcpy(dst,src,w*4);
			dst+=EfiGraphic->Mode->Info->PixelsPerScanLine;
			src+=a->next32;
		}
		FUN_RETURN_BOOL(1);
	}
#else
	Status=EfiGraphic->Blt(EfiGraphic, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)a->start8, EfiBltBufferToVideo, xsrc, ysrc, xdst, ydst, w, h, a->next8);
#endif
	FUN_RETURN_BOOL(Status==EFI_SUCCESS);
}
int fun_uefiPlot(Thread* th)
{
	EFI_STATUS Status;
	LINT color = STACK_INT(th,0);
	LINT y = STACK_INT(th,1);
	LINT x = STACK_INT(th,2);
	if (!EfiGraphic) FUN_RETURN_NIL;
	if (x<0 || x>=EfiGraphic->Mode->Info->HorizontalResolution) FUN_RETURN_NIL;
	if (y<0 || y>=EfiGraphic->Mode->Info->VerticalResolution) FUN_RETURN_NIL;

	long long p=EfiGraphic->Mode->FrameBufferBase + sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)*(x+y*EfiGraphic->Mode->Info->PixelsPerScanLine);
	int* q=(int*)p;
	*q=color;
	FUN_RETURN_BOOL(1);
}

int hostOnlyFunctionsInit(Pkg* system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "uefiHasSerial", fun_uefiHasSerial, "fun -> Bool"},
		{ NATIVE_FUN, "uefiEventId", fun_uefiEventId, "fun -> Int"},
		{ NATIVE_FUN, "consoleOnDisplay", fun_consoleOnDisplay, "fun Bool -> Bool"},
		{ NATIVE_FUN, "consoleModes", fun_consoleModes, "fun -> Bool"},
		{ NATIVE_FUN, "consoleCls", fun_consoleCls, "fun -> Bool"},
		{ NATIVE_FUN, "consoleSetMode", fun_consoleSetMode, "fun Int -> Int"},
		{ NATIVE_FUN, "graphicModes", fun_graphicModes, "fun -> list [Int Int Int Int]"},
		{ NATIVE_FUN, "graphicSetMode", fun_graphicSetMode, "fun Int -> Int"},
		{ NATIVE_FUN, "graphicW", fun_graphicW, "fun -> Int"},
		{ NATIVE_FUN, "graphicH", fun_graphicH, "fun -> Int"},
		{ NATIVE_FUN, "graphicBlit", fun_graphicBlit, "fun Int Int Bitmap Int Int Int Int -> Bool"},
		{ NATIVE_FUN, "uefiPlot", fun_uefiPlot, "fun Int Int Int -> Bool"},
	};
	NATIVE_DEF(nativeDefs);
	return 0;
}
