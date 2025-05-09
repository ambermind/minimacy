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



#ifdef USE_SERIAL_UNIX
#include<termios.h>

int fun_serialOpen(Thread* th)
{
	int fd=0;
	struct termios options;
	Socket* socket;

	LINT stop = STACK_INT(th, 0);
	LINT parity = STACK_INT(th, 1);
	LINT bits = STACK_INT(th, 2);
	LINT bds = STACK_INT(th, 3);
	LB* v = STACK_PNT(th, 4);
	if (!v) goto cleanup;
	fd = open(STR_START(v), O_RDWR | O_NOCTTY| O_NDELAY);
	if (fd <0)
	{
		PRINTF(LOG_SYS, "> Error: Cannot open serial %s\n", STR_START(v));
		goto cleanup;
	}
	tcgetattr(fd, &options);
	
	tcflush(fd, TCIOFLUSH);
	
	// printf("sizeof(options)=%d tcflag_t=%d cc_t=%d speed_t=%d NCCS=%d\n",
	// 	sizeof(options),sizeof(tcflag_t),sizeof(cc_t),sizeof(speed_t),NCCS);
	// _myHexDump((char*)&options,sizeof(options),0);

	options.c_cflag =0;
	cfsetspeed(&options, bds);
	// _myHexDump((char*)&options,sizeof(options),0);
	options.c_cflag |= CREAD | CLOCAL;
	if (bits==8) options.c_cflag |= CS8;
	if (bits==7) options.c_cflag |= CS7;

//	if (stop==1) options.c_cflag |= CSTOPB;	// should be 1.5 bits
	if (stop==2) options.c_cflag |= CSTOPB;

	if (parity) {
		options.c_cflag |= PARENB;
		if (parity==1) options.c_cflag |= PARODD;
	}

	options.c_iflag = 0; //IGNPAR | ICRNL;
	options.c_oflag = 0;
	options.c_lflag = 0;;
	options.c_cc[VTIME] = 0;
	options.c_cc[VMIN] = 0;
	// _myHexDump((char*)&options,sizeof(options),0);

	tcsetattr(fd, TCSANOW, &options);
	PRINTF(LOG_SYS, "> Serial '%s' %d ready\n", STR_START(v), fd);
	
	socket=_socketCreate(fd); if (!socket) goto cleanup;
	
	FUN_RETURN_PNT((LB*)socket);

cleanup:
	if (fd>0) close(fd);
	FUN_RETURN_NIL;
}

int fun_serialClose(Thread* th)
{
	return fun_socketClose(th);
}

//[index content rs]
int fun_serialWrite(Thread* th)
{
	LINT sent,len=0;

	LINT start = STACK_INT(th, 0);
	LB* src = STACK_PNT(th, 1);
	Socket* s = (Socket*)STACK_PNT(th, 2);
	if ((!s)|| (s->fd == INVALID_SOCKET)) FUN_RETURN_NIL;
	FUN_SUBSTR(src, start, len, 1, STR_LENGTH(src));

	if (len==0) FUN_RETURN_INT(start);

// here we need 'write' (not 'send' as with sockets)
	sent = write(s->fd, STR_START(src) + start, (int)len);
	if (sent<0 && SOCKETWOULDBLOCK) sent=0;
	if (sent >= 0) FUN_RETURN_INT(start + sent);
	FUN_RETURN_NIL;
}

int fun_serialRead(Thread* th)
{
	return fun_socketRead(th);
}
int fun_serialSocket(Thread* th)
{
	return 0;
}
#else
#define B115200 115200
#define B57600 57600
#define B38400 38400
#define B19200 19200
#define B9600 9600
#endif
#ifdef USE_SERIAL_WIN

#define SERIAL_SLOT_NB 16
#define SERIAL_BUFFER_LEN 2048

typedef struct {
	HANDLE hCom;
	SOCKET pipe[2];
	int running;
	CHAR bufferIn[SERIAL_BUFFER_LEN];
	char bufferOut[SERIAL_BUFFER_LEN];
}SerialSlot;

typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	Socket* socket;
	int slot;
}Serial;

SerialSlot SerialSlots[SERIAL_SLOT_NB];

int _serialForget(LB* p)
{
	Serial* f = (Serial*)p;
//	printf("release _serialForget %llx\n",f);
	if (f->slot >= 0) SerialSlots[f->slot].running = 0;
	f->slot = -1;
	return 0;
}
void _serialMark(LB* user)
{
	Serial* f = (Serial*)user;
	MARK_OR_MOVE(f->socket);
}

WORKER_START _serialThread(void* param)
{
	SerialSlot* f = (SerialSlot*)param;
	HANDLE hCom = f->hCom;
	DWORD inIndex, inLen;
	int outIndex, outLen;

	inIndex = inLen = 0;
	outIndex = outLen = 0;
	while(f->running==1)
	{
		int sent;
		BOOL result;
		if (inLen == 0) {
			result = ReadFile(hCom, f->bufferIn, SERIAL_BUFFER_LEN, &inLen, NULL);
			if (!result) break;
		}
		if (outLen == 0) {
			outLen = recv(f->pipe[PIPE_WRITE], f->bufferOut, SERIAL_BUFFER_LEN, 0);
			if (outLen < 0)
			{
				if (SOCKETWOULDBLOCK) outLen = 0;
				else break;
			}
		}
		if ((inLen == 0)&&(outLen == 0)) {
//			printf(".");
			Sleep(5);
			continue;
		}
//		Sleep(100);
//		printf(".");
		if (inLen) {
//			printf("->%d/%d.", inIndex, inLen);
			sent = send(f->pipe[PIPE_WRITE], f->bufferIn + inIndex, inLen - inIndex, 0);
			if (sent < 0) {
				if (SOCKETWOULDBLOCK) sent = 0;
				else break;
			}
			inIndex += sent;
			if (inIndex >= inLen) inIndex = inLen = 0;
		}
		if (outLen) {
//			printf("<-%d/%d.", outIndex, outLen);

			result = WriteFile(hCom, f->bufferOut + outIndex, outLen - outIndex, &sent, NULL);
			if (!result) {
				if (GetLastError() == ERROR_COUNTER_TIMEOUT) sent = 0;
				else break;
			}
			outIndex += sent;
			if (outIndex >= outLen) outIndex = outLen = 0;

		}
	}
	CloseHandle(hCom);
	lambdaPipeClose(f->pipe);
	f->running = -1;
	PRINTF(LOG_SYS,"> Serial thread closed\n");
	return WORKER_RETURN;
}

Serial* _serialCreate(int slot, HANDLE hCom)
{
	Serial* f;
	f = (Serial*)memoryAllocExt(sizeof(Serial), DBG_BIN, _serialForget, _serialMark); if (!f) return NULL;
	f->slot = slot;
	SerialSlots[slot].running = 1;
	lambdaPipe(SerialSlots[slot].pipe);
	SerialSlots[slot].hCom = hCom;
	f->socket = _socketCreate(SerialSlots[slot].pipe[PIPE_READ]); if (!f->socket) return NULL;
	hwThreadCreate(_serialThread, &SerialSlots[slot]);
	return f;
}

int fun_serialOpen(Thread* th)
{
	HANDLE hCom=0;
	DCB dcb;
	COMMTIMEOUTS ct;
	BOOL fSuccess;
	int slot = 0;

	LINT stop = STACK_INT(th, 0);
	LINT parity = STACK_INT(th, 1);
	LINT bits = STACK_INT(th, 2);
	LINT bds = STACK_INT(th, 3);
	LB* v = STACK_PNT(th, 4);
	if (!v) goto cleanup;
	for (slot = 0; slot < SERIAL_SLOT_NB; slot++) if (SerialSlots[slot].running < 0) break;
	if (slot >= SERIAL_SLOT_NB) FUN_RETURN_NIL;

	hCom = CreateFile(STR_START(v), GENERIC_READ | GENERIC_WRITE, 0,
		NULL, OPEN_EXISTING, 0, NULL);
	if (hCom == INVALID_HANDLE_VALUE)
	{
		PRINTF(LOG_SYS, "> Error: Cannot open serial %s\n", STR_START(v));
		goto cleanup;
	}
	fSuccess = GetCommState(hCom, &dcb);
	if (!fSuccess)
	{
		PRINTF(LOG_SYS, "> Error: Cannot read serial state\n");
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
		PRINTF(LOG_SYS, "> Error: Cannot write rs state\n");
		goto cleanup;
	}
	fSuccess = GetCommTimeouts(hCom, &ct);
	if (!fSuccess)
	{
		PRINTF(LOG_SYS, "> Error: Cannot read rs timeouts\n");
		goto cleanup;
	}
	ct.ReadIntervalTimeout = MAXDWORD;
	ct.ReadTotalTimeoutConstant = (DWORD)0;
	ct.ReadTotalTimeoutMultiplier = 0;

	ct.WriteTotalTimeoutConstant = 0;
	ct.WriteTotalTimeoutMultiplier = 0;

	fSuccess = SetCommTimeouts(hCom, &ct);
	if (!fSuccess)
	{
		PRINTF(LOG_SYS, "> Error: Cannot write rs timeouts\n");
		goto cleanup;
	}
	PRINTF(LOG_SYS, "> Serial '%s' ready\n", STR_START(v));
	FUN_RETURN_PNT((LB*)_serialCreate(slot, hCom));

cleanup:
	if (hCom) CloseHandle(hCom);
	FUN_RETURN_NIL;
}

int fun_serialClose(Thread* th)
{
	LB* p=(LB*)STACK_PNT(th, 0);
	if (p) _serialForget(p);
	return 0;
}

//[index content rs]
int fun_serialWrite(Thread* th)
{
	LINT sent, len = 0;

	LINT start = STACK_INT(th, 0);
	LB* src = STACK_PNT(th, 1);
	Serial* s = (Serial*)STACK_PNT(th, 2);
	if ((!s) || (s->slot < 0) || (SerialSlots[s->slot].running != 1)) FUN_RETURN_NIL;
	FUN_SUBSTR(src, start, len, 1, STR_LENGTH(src));

	if (len == 0) FUN_RETURN_INT(start);
	// we reduce len to get closer to a direct 115kbps uart connection
	// (here we are using an internal socket/pipe to deal with the fact that windows select cannot handle windows Handles).
	// Measured 1024 on MacOS.
	if (len > 16384) len = 16384;
	sent = send(s->socket->fd, STR_START(src) + start, (int)len, 0);
	if (sent<0 && SOCKETWOULDBLOCK) sent = 0;
	if (sent >= 0) FUN_RETURN_INT(start + sent);
	FUN_RETURN_NIL;
}

int fun_serialRead(Thread* th)
{
	Serial* s = (Serial*)STACK_PNT(th, 0);
	if ((!s) || (s->slot < 0) || (SerialSlots[s->slot].running != 1)) FUN_RETURN_NIL;
	STACK_SET_PNT(th, 0, s->socket);
	return fun_socketRead(th);
}
int fun_serialSocket(Thread* th)
{
	Serial* s = (Serial*)STACK_PNT(th, 0);
	if ((!s) || (s->slot < 0) || (SerialSlots[s->slot].running != 1)) FUN_RETURN_NIL;
	FUN_RETURN_PNT((LB*)s->socket);
}
#endif
#ifdef USE_SERIAL_STUB
int fun_serialOpen(Thread* th) FUN_RETURN_NIL
int fun_serialClose(Thread* th) FUN_RETURN_NIL
int fun_serialWrite(Thread* th) FUN_RETURN_NIL
int fun_serialRead(Thread* th) FUN_RETURN_NIL
int fun_serialSocket(Thread* th) FUN_RETURN_NIL
#endif
int sysSerialInit(Pkg* system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "_serialOpen", fun_serialOpen, "fun Str SerialSpeed SerialBits Parity Stop -> Serial"},
		{ NATIVE_FUN, "_serialClose", fun_serialClose, "fun Serial -> Serial" },
		{ NATIVE_FUN, "_serialWrite", fun_serialWrite, "fun Serial Str Int -> Int" },
		{ NATIVE_FUN, "_serialRead", fun_serialRead, "fun Serial -> Str" },
		{ NATIVE_FUN, "_serialSocket", fun_serialSocket, "fun Serial -> Socket" },
		{ NATIVE_INT, "SERIAL_8BITS", (void*)8, "SerialBits" },
		{ NATIVE_INT, "SERIAL_7BITS", (void*)7, "SerialBits" },
		{ NATIVE_INT, "SERIAL_115200", (void*)B115200, "SerialSpeed" },
		{ NATIVE_INT, "SERIAL_57600", (void*)B57600, "SerialSpeed" },
		{ NATIVE_INT, "SERIAL_38400", (void*)B38400, "SerialSpeed" },
		{ NATIVE_INT, "SERIAL_19200", (void*)B19200, "SerialSpeed" },
		{ NATIVE_INT, "SERIAL_9600", (void*)B9600, "SerialSpeed" },
		{ NATIVE_INT, "SERIAL_ONESTOPBIT", (void*)0, "Stop" },
		{ NATIVE_INT, "SERIAL_TWOSTOPBITS", (void*)2, "Stop" },
		{ NATIVE_INT, "SERIAL_NOPARITY", (void*)0, "Parity" },
		{ NATIVE_INT, "SERIAL_ODDPARITY", (void*)1, "Parity" },
		{ NATIVE_INT, "SERIAL_EVENPARITY", (void*)2, "Parity" },
	};
	pkgAddType(system, "Serial");
	pkgAddType(system, "SerialBits");
	pkgAddType(system, "SerialSpeed");
	pkgAddType(system, "Parity");
	pkgAddType(system, "Stop");
	NATIVE_DEF(nativeDefs);

#ifdef USE_SERIAL_WIN
	{
		int slot;
		for (slot = 0; slot < SERIAL_SLOT_NB; slot++) SerialSlots[slot].running = -1;
	}
#endif
	return 0;
}

