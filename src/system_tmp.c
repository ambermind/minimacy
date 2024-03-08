/* Copyright (c) 2022, Sylvain Huet, Ambermind
   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License, version 2.0, as
   published by the Free Software Foundation.
   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License,
   version 2.0, for more details.
   You should have received a copy of the GNU General Public License along
   with this program. If not, see <https://www.gnu.org/licenses/>. */
#include"minimacy.h"




#ifdef WITH_DECODE_MP3
#define MINIMP3_IMPLEMENTATION
#include"../tmpSrc/minimp3.h"

MTHREAD_START _mp3Decode(Thread* th)
{
	unsigned char* input_buf;
	int buf_size = -1;
	static mp3dec_t mp3d;
	mp3dec_frame_info_t info;
	int samples;
	short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];

	LB* src = STACKPNT(th, 0);
	Buffer* out = (Buffer*)STACKPNT(th, 1);
	if ((!src) || (!out)) return workerDonePnt(th, MM._false);
	input_buf = STRSTART(src);
	buf_size = (int)STRLEN(src);
	mp3dec_init(&mp3d);
	do {
		samples = mp3dec_decode_frame(&mp3d, input_buf, buf_size, pcm, &info);
//		printf("frame: samples=%d frame_bytes=%d\n", samples, info.frame_bytes);
		input_buf += info.frame_bytes;
		buf_size -= info.frame_bytes;
		if (samples) {
			if (bufferAddBin(th, out, (char*)pcm, 2 * info.channels * samples)) return workerDoneNil(th);
		}
	} while (samples || info.frame_bytes);

	return workerDonePnt(th, MM._true);
}

int fun_mp3Decode(Thread* th) { return workerStart(th, 2, _mp3Decode); }
int usrMp3Init(Thread* th, Pkg* system)
{
	pkgAddFun(th, system, "_mp3Decode", fun_mp3Decode, typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.Buffer, MM.S, MM.Boolean));
	return 0;
}
#else
int usrMp3Init(Thread* th, Pkg* system)
{
	return 0;
}
#endif

#ifdef WITH_SERIAL
#ifdef USE_UART_WIN
typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	HANDLE hCom;
}Serial;

int _serialForget(LB* p)
{
	Serial* f = (Serial*)p;
	if (f->hCom) fclose(f->hCom);
	f->hCom = 0;
	return 0;
}

Serial* _serialCreate(Thread* th, void* hCom)
{
	Serial* f = (Serial*)memoryAllocExt(th, sizeof(Serial), DBG_BIN, _serialForget, NULL); if (!f) return NULL;
	f->hCom = hCom;
	return f;
}

int fun_serialOpen(Thread* th)
{
	HANDLE hCom=0;
	DCB dcb;
	COMMTIMEOUTS ct;
	BOOL fSuccess;

	LINT timeout = STACKINT(th, 0);
	LINT stop = STACKINT(th, 1);
	LINT parity = STACKINT(th, 2);
	LINT bits = STACKINT(th, 3);
	LINT bds = STACKINT(th, 4);
	LB* v = STACKPNT(th, 5);
	if (!v) goto cleanup;
	hCom = CreateFile(STRSTART(v), GENERIC_READ | GENERIC_WRITE, 0,
		NULL, OPEN_EXISTING, 0, NULL);
	if (hCom == INVALID_HANDLE_VALUE)
	{
		PRINTF(LOG_SYS, "Cannot open serial %s\n", STRSTART(v));
		goto cleanup;
	}
	fSuccess = GetCommState(hCom, &dcb);
	if (!fSuccess)
	{
		PRINTF(LOG_SYS, "Cannot read serial state\n");
		goto cleanup;
	}

	dcb.BaudRate = (DWORD)bds;     // set the baud rate
	dcb.ByteSize = (BYTE) bits;             // data size, xmit, and rcv
	dcb.Parity = (BYTE)parity;        // no parity bit
	dcb.StopBits = (BYTE)stop;    // stop bit : 0->1, 1->1.5, 2->2
	dcb.fOutX = 0;
	dcb.fInX = 0;
	dcb.fOutxCtsFlow = 0;
	dcb.fOutxDsrFlow = 0;
	dcb.fTXContinueOnXoff = 0;	// 1
//	printf("SetCommState %d %d %d %d\n",dcb.BaudRate,dcb.ByteSize,dcb.Parity,dcb.StopBits);
	fSuccess = SetCommState(hCom, &dcb);
	if (!fSuccess)
	{
		PRINTF(LOG_SYS, "Cannot write rs state\n");
		goto cleanup;
	}
	fSuccess = GetCommTimeouts(hCom, &ct);
	if (!fSuccess)
	{
		PRINTF(LOG_SYS, "Cannot read rs timeouts\n");
		goto cleanup;
	}
	ct.ReadIntervalTimeout = MAXDWORD;
	ct.ReadTotalTimeoutConstant = (DWORD)timeout;
	ct.ReadTotalTimeoutMultiplier = 0;

	ct.WriteTotalTimeoutConstant = 100;
	ct.WriteTotalTimeoutMultiplier = 100;

	fSuccess = SetCommTimeouts(hCom, &ct);
	if (!fSuccess)
	{
		PRINTF(LOG_SYS, "Cannot write rs timeouts\n");
		goto cleanup;
	}
	PRINTF(LOG_SYS, "Serial '%s' ready\n", STRSTART(v));
	FUN_RETURN_PNT((LB*)_serialCreate(th, (void*)hCom));

cleanup:
	if (hCom) CloseHandle(hCom);
	FUN_RETURN_NIL;
}

int fun_serialClose(Thread* th)
{
	LB* p=(LB*)STACKPNT(th, 0);
	if (p) _serialForget(p);
	return 0;
}

//[index content rs]
int fun_serialWrite(Thread* th)
{
	DWORD dwWritten = 0;
	LINT len=0;

	LINT start = STACKINT(th, 0);
	LB* src = STACKPNT(th, 1);
	Serial* s = (Serial*)STACKPNT(th, 2);
	if ((!s)||(!s->hCom)) FUN_RETURN_NIL;
	FUN_SUBSTR(src, start, len, 1, STRLEN(src));

	if (len == 0) FUN_RETURN_INT(start);
	
	if (WriteFile(s->hCom, STRSTART(src) + start, (DWORD)len, &dwWritten, NULL)) FUN_RETURN_INT(start + dwWritten);

	if (GetLastError() == ERROR_COUNTER_TIMEOUT) FUN_RETURN_INT(start);
	FUN_RETURN_NIL;
}

int fun_serialRead(Thread* th)
{
	DWORD dwRead;
	CHAR chBuf[1024];

	Serial* p = (Serial*)STACKPNT(th, 0);
	if (!p || !p->hCom) FUN_RETURN_NIL;
	if (!ReadFile(p->hCom, chBuf, 1024, &dwRead, NULL) || dwRead == 0) FUN_RETURN_NIL;
	FUN_RETURN_STR(chBuf, dwRead);
}
#endif
int usrUartInit(Thread* th, Pkg* system)
{
	Def* Serial = pkgAddType(th, system, "Serial");
	Def* Parity = pkgAddType(th, system, "Parity");
	Def* Stop = pkgAddType(th, system, "Stop");
	Type* fun_Serial_S = typeAlloc(th, TYPECODE_FUN, NULL, 2, Serial->type, MM.S);
	Type* fun_Serial_Serial = typeAlloc(th, TYPECODE_FUN, NULL, 2, Serial->type, Serial->type);
	Type* fun_Serial_S_I_I = typeAlloc(th, TYPECODE_FUN, NULL, 4, Serial->type, MM.S, MM.I, MM.I);
	Type* fun_S_I_I_I_I_I_Serial = typeAlloc(th, TYPECODE_FUN, NULL, 7, MM.S, MM.I, MM.I, Parity->type, Stop->type, MM.I, Serial->type);

	pkgAddFun(th, system, "serialOpen", fun_serialOpen, fun_S_I_I_I_I_I_Serial);
	pkgAddFun(th, system, "serialClose", fun_serialClose, fun_Serial_Serial);
	pkgAddFun(th, system, "serialWrite", fun_serialWrite, fun_Serial_S_I_I);
	pkgAddFun(th, system, "serialRead", fun_serialRead, fun_Serial_S);

	pkgAddConstInt(th, system, "SERIAL_ONESTOPBIT", 0, Stop->type);
	pkgAddConstInt(th, system, "SERIAL_ONE5STOPBITS", 1, Stop->type);
	pkgAddConstInt(th, system, "SERIAL_TWOSTOPBITS", 2, Stop->type);

	pkgAddConstInt(th, system, "SERIAL_NOPARITY", 0, Parity->type);
	pkgAddConstInt(th, system, "SERIAL_ODDPARITY", 1, Parity->type);
	pkgAddConstInt(th, system, "SERIAL_EVENPARITY", 2, Parity->type);
	pkgAddConstInt(th, system, "SERIAL_MARKPARITY", 3, Parity->type);
	pkgAddConstInt(th, system, "SERIAL_SPACEPARITY", 4, Parity->type);
	return 0;
}
#else
int usrUartInit(Thread* th, Pkg* system){ return 0; }
#endif

#ifdef WITH_SECTOR_STORAGE
int fun_storageRead(Thread* th)
{
	LINT len;
	
	LINT nb = STACKINT(th, 0);
	LINT start = STACKINT(th, 1);
	LINT offset = STACKINT(th, 2);
	LB* bytes = STACKPNT(th, 3);
	LINT index = STACKINT(th, 4);	// index of storage device
	if ((!bytes)||(offset<0)||(start < 0)||(nb<=0)) FUN_RETURN_NIL;
	if (offset+512*nb>STRLEN(bytes)) FUN_RETURN_NIL;
	len = storageRead(index, STRSTART(bytes)+offset, (int)start, (int)nb);
	if (len < 0) FUN_RETURN_NIL;
	FUN_RETURN_INT(len);
}

int fun_storageWrite(Thread* th)
{
	LINT len;

	LINT nb = STACKINT(th, 0);
	LINT start = STACKINT(th, 1);
	LINT offset = STACKINT(th, 2);
	LB* bytes = STACKPNT(th, 3);
	LINT index = STACKINT(th, 4);	// index of storage device
	if ((!bytes)||(offset<0)||(start < 0)||(nb<=0)) FUN_RETURN_NIL;
	if (offset+512*nb>STRLEN(bytes)) FUN_RETURN_NIL;
	len = storageWrite(index, STRSTART(bytes)+offset, (int)start, (int)nb);
	if (len < 0) FUN_RETURN_NIL;
	FUN_RETURN_INT(len);
}

int usrSdInit(Thread* th, Pkg* system)
{
	pkgAddFun(th, system, "storageRead", fun_storageRead, typeAlloc(th, TYPECODE_FUN, NULL, 6, MM.I, MM.Bytes, MM.I,  MM.I, MM.I, MM.I));
	pkgAddFun(th, system, "storageWrite", fun_storageWrite, typeAlloc(th, TYPECODE_FUN, NULL, 6, MM.I, MM.Bytes, MM.I, MM.I, MM.I, MM.I));
	return 0;
}
#else
int usrSdInit(Thread* th, Pkg* system){ return 0; }
#endif

#ifdef WITH_ACTIVITY_LED
int fun_activityLed(Thread* th)
{
	LB* val=STACKPNT(th, 0);
	hwActivityLedSet((val==MM._true)?1:0);
	return 0;
}
#else
int fun_activityLed(Thread* th) FUN_RETURN_NIL;
#endif

int usrActivityLedInit(Thread* th, Pkg* system)
{
	pkgAddFun(th, system, "activityLed", fun_activityLed, typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.Boolean, MM.Boolean));
	return 0;
}

int usrUefiInit(Thread* th, Pkg* system);
int usrRpiInit(Thread* th, Pkg* system);

int tmpInit(Thread* th, Pkg* system)
{
	usrUartInit(th, system);
	usrSdInit(th, system);
	usrActivityLedInit(th, system);
	usrMp3Init(th, system);
#ifdef ON_UEFI
	usrUefiInit(th, system);
#endif
#ifdef GROUP_RPI
	usrRpiInit(th, system);
#endif
	return 0;
}

