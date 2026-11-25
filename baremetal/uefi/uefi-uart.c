// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"inc/efi.h"
#include"inc/efilib.h"
#include "../../src/minimacy.h"
#include "uefi.h"

#define PRINT_BUFFER 128
#define MAX_TEXT_IN 4
int ConsoleOnDisplay=1;
int UefiEvent;

EFI_SERIAL_IO_PROTOCOL       *EfiSerialIO=NULL;
EFI_SIMPLE_TEXT_OUT_PROTOCOL *EfiTextOut=NULL;
EFI_SIMPLE_TEXT_IN_PROTOCOL  *EfiTextIn[MAX_TEXT_IN];
static int TextInCount;

void textOutInit(void)
{
	EFI_HANDLE *h = NULL;
	UINTN n = 0;
	EfiTextOut=NULL;

	if (BS->LocateHandleBuffer(ByProtocol, &gEfiSimpleTextOutProtocolGuid, NULL, &n, &h) != EFI_SUCCESS) return;
	for (UINTN i = 0; i < n; i++) {
		EFI_SIMPLE_TEXT_OUT_PROTOCOL *protocol;
		if (h[i] == ST->ConsoleOutHandle || h[i] == ST->StandardErrorHandle) continue;  // aggregated consoles
		if (BS->HandleProtocol(h[i], &gEfiSimpleTextOutProtocolGuid, (void**)&protocol) != EFI_SUCCESS) continue;
		if (!pathContainsUart(h[i])) {
			EfiTextOut = protocol;
			break;
		}
	}
	BS->FreePool(h);
}

void serialIoInit(int comNumber)
{
	EFI_HANDLE *h = NULL;
	UINTN candidate=-1;
	UINTN n = 0;
	EfiSerialIO = NULL;

	if (BS->LocateHandleBuffer(ByProtocol, &gEfiSerialIoProtocolGuid, NULL, &n, &h) != EFI_SUCCESS) {
		PRINTF(LOG_SYS, "> UEFI: no serial port\n");
		return;
	}
	for (UINTN i = 0; i < n; i++) {
		int com=pathGetSerialUid(h[i]) + 1;
		PRINTF(LOG_SYS, "> UEFI: serial #%d (COM%d)\n", i, com);
		candidate=i;
		if (com == comNumber) break;
	 }
	 if (candidate>=0) {
		EFI_SERIAL_IO_PROTOCOL *protocol;
		if (BS->OpenProtocol(h[candidate], &gEfiSerialIoProtocolGuid, (void**)&protocol,ImageHandle, NULL, EFI_OPEN_PROTOCOL_EXCLUSIVE) == EFI_SUCCESS) 
		{
			UINT32 Control=0;
			EfiSerialIO=protocol;
			EfiSerialIO->GetControl(EfiSerialIO,&Control);
			EfiSerialIO->SetAttributes(EfiSerialIO, 115200,64,0,0,8,1);
			EfiSerialIO->SetControl(EfiSerialIO, EFI_SERIAL_DATA_TERMINAL_READY | EFI_SERIAL_REQUEST_TO_SEND) ;
			PRINTF(LOG_SYS, "> UEFI: serial #%d (COM%d) baud=%d fifo=%d timeout=%d control=%x\n",
				candidate,
				pathGetSerialUid(h[candidate]) + 1,
				(int)EfiSerialIO->Mode->BaudRate,
				(int)EfiSerialIO->Mode->ReceiveFifoDepth,
				(int)EfiSerialIO->Mode->Timeout,
				(int)Control
			);
		}
	}
	BS->FreePool(h);
}

void textInInit(void)
{
	EFI_HANDLE *h = NULL;
	UINTN n = 0;
	TextInCount=0;
	//if (BS->LocateHandleBuffer(ByProtocol, &gEfiSimpleTextInExProtocolGuid, NULL, &n, &h) != EFI_SUCCESS) return;
	if (BS->LocateHandleBuffer(ByProtocol, &gEfiSimpleTextInProtocolGuid, NULL, &n, &h) != EFI_SUCCESS) return;
	for (UINTN i = 0; i < n; i++) {
		//EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *protocol;
		EFI_SIMPLE_TEXT_IN_PROTOCOL *protocol;
		if (EfiSerialIO && (h[i] == ST->ConsoleInHandle)) continue;	// skip aggregated console when serial is detected
		if (pathContainsUart(h[i])) continue;
		//if (BS->HandleProtocol(h[i], &gEfiSimpleTextInExProtocolGuid, (void**)&protocol) != EFI_SUCCESS) continue;
		if (BS->HandleProtocol(h[i], &gEfiSimpleTextInProtocolGuid, (void**)&protocol) != EFI_SUCCESS) continue;
		PRINTF(LOG_SYS, "> UEFI: input device #%d type=%s\n", i, keyboardInputType(h[i]));
		if (TextInCount<MAX_TEXT_IN) EfiTextIn[TextInCount++]=protocol;
	}
	BS->FreePool(h);
}

void consoleInit()
{
	ConsoleOnDisplay=1;

	textOutInit();
	if (EfiTextOut) ST->ConOut->OutputString(ST->ConOut,L"> UEFI: display found\r\n");
	else ST->ConOut->OutputString(ST->ConOut,L"> UEFI: no display found\r\n");

	serialIoInit(1);
	if (EfiSerialIO) ST->ConOut->OutputString(ST->ConOut,L"> UEFI: serialConsole started\r\n");
	else ST->ConOut->OutputString(ST->ConOut,L"> UEFI: serialConsole not found\r\n");

	textInInit();
	if (TextInCount) ST->ConOut->OutputString(ST->ConOut,L"> UEFI: keyboard found\r\n");
	else ST->ConOut->OutputString(ST->ConOut,L"> UEFI: no keyboard found\r\n");

	UefiEvent=eventGetNextID();
}
int uefiHasSerial()
{
	return EfiSerialIO?1:0;
}

void keyboardMoveLeft()
{
	UINTN x=EfiTextOut->Mode->CursorColumn;
	UINTN y=EfiTextOut->Mode->CursorRow;
	if (x==0) {
		UINTN Columns;
		UINTN Rows;
		if (y==0) return;
		EfiTextOut->QueryMode(EfiTextOut,EfiTextOut->Mode->Mode,&Columns,&Rows);
		x=Columns-1;
		y--;
	}
	else x--;
	EfiTextOut->SetCursorPosition(EfiTextOut,x,y);
}
void uartPutChar(unsigned int c) {
	if (EfiSerialIO) {
		UINTN count=1;
		if (c==10) {
			unsigned int cc=13;
			EfiSerialIO->Write(EfiSerialIO,&count,(char*)&cc);
			count=1;
		}
		EfiSerialIO->Write(EfiSerialIO,&count,(char*)&c);
	}
	if (ConsoleOnDisplay && EfiTextOut) {
		CHAR16 buffer[2];
		if(c==10) {
			EfiTextOut->OutputString(EfiTextOut,L"\r\n");
			return;
		}
		if(c==8) {
			keyboardMoveLeft();
			return;
		}
		buffer[0]=c;
		buffer[1]=0;
		EfiTextOut->OutputString(EfiTextOut,buffer);
	}
}

void uartPut(char *s,int len) {
	if (len<0) len=strlen(s);
	if (EfiSerialIO) {
		UINTN count;
		int i;
		int i0=0;
		for(i=0;i<len;i++) {
			if (s[i]==10) {
				unsigned int cc=13;
				count=i-i0;
				if (count) EfiSerialIO->Write(EfiSerialIO,&count,s+i0);
				count=1;
				EfiSerialIO->Write(EfiSerialIO,&count,(char*)&cc);
				i0=i;
			}
		}
		count=i-i0;
		if (count) EfiSerialIO->Write(EfiSerialIO,&count,s+i0);
	}
	if (ConsoleOnDisplay && EfiTextOut) {	// print by converting char into short
		CHAR16 buffer[PRINT_BUFFER+8];
		int i=0;
		int j=0;
		for(i=0;i<len;i++) {
			short c=(short)s[i];
			if (c==10) buffer[j++]=13;
			if(c==8) {
				buffer[j]=0;
				EfiTextOut->OutputString(EfiTextOut,buffer);
				j=0;
				keyboardMoveLeft();
				continue;
			}
			buffer[j++]=255&c;
			if (j>=PRINT_BUFFER) {
				buffer[j]=0;
				EfiTextOut->OutputString(EfiTextOut,buffer);
				j=0;
			}
		}
		if (j) {
			buffer[j]=0;
			EfiTextOut->OutputString(EfiTextOut,buffer);
		}
	}
}

int uartWritable() {
	return 1;
}

#define UART_FIFO_LEN 128
unsigned char inBuffer[UART_FIFO_LEN];
int inWrite=0;
int inRead=0;

int uartFifoIn(unsigned char ch)
{
	int inWriteNext=(inWrite+1)&(UART_FIFO_LEN-1);
	if (inWriteNext!=inRead) {
		inBuffer[inWrite]=ch;
		inWrite=inWriteNext;
	}
	return 0;
}
int uartFifoInStr(unsigned char *str)
{
	while((*str)) uartFifoIn(*(str++));
	return 0;
}

// check whether there is something to read from serial or keyboard
void consolePoll() {
	EFI_STATUS Status;
	if (EfiSerialIO) {
		unsigned char c;
		UINTN count=1;
		while(1) {
			Status = EfiSerialIO->Read(EfiSerialIO,&count,(char*)&c);
			if (Status != EFI_SUCCESS || count==0) break;
			uartFifoIn(c);
		}
	}
	int i;
	for(i=0;i<TextInCount;i++) while(1) {
		EFI_INPUT_KEY Key;
		Status = EfiTextIn[i]->ReadKeyStroke(EfiTextIn[i], &Key);
		if (Status == EFI_NOT_READY) break;
		if (ConsoleOnDisplay) {
			switch(Key.UnicodeChar) {
				case 13: uartFifoIn(10); break;
				case 8:  uartFifoIn(0x7f); break;
				case 0:
					switch(Key.ScanCode) {
						//1: up, 2:down, 3:right, 4:left, 5:home, 6:end
						case 1:  uartFifoInStr("\e[A"); break;
						case 2:  uartFifoInStr("\e[B"); break;
						case 3:  uartFifoInStr("\e[C"); break;
						case 4:  uartFifoInStr("\e[D"); break;
						case 5:  uartFifoInStr("\e[H"); break;
						case 6:  uartFifoInStr("\e[F"); break;
						case 8:  uartFifoInStr("\e[3~"); break;
						case 9:  uartFifoInStr("\e[5~"); break;
						case 10: uartFifoInStr("\e[6~"); break;
						case 23: uartFifoIn(27);
					}
					break;
				default:
					uartFifoIn(Key.UnicodeChar);
			}
		}
		else if (Key.UnicodeChar || Key.ScanCode) {
			short event[3];
			event[0]=0;
			event[1]=Key.UnicodeChar;
			event[2]=Key.ScanCode;
			eventBinary(UefiEvent, (char*)event, 3*2);
		}
	}
}

int uartReadable() {
	if (inWrite!=inRead) return 1;
	return 0;
}
int uartGet() {
	consolePoll();
	if (inWrite==inRead) return -1;
	unsigned char ch=inBuffer[inRead];
	inRead=(inRead+1)&(UART_FIFO_LEN-1);
	return ch;
}

void consoleOnDisplay(int useDisplay)
{
	ConsoleOnDisplay=useDisplay?1:0;
}

void consoleWrite(int user, char* src, int len)
{
	uartPut(src,len);
}
void consoleVPrint(int user, char* format, va_list arglist)
{
	vsnprintf(NULL,0, format, arglist);
}
