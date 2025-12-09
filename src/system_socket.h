// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _SOCKET_
#define _SOCKET_

#ifdef USE_SOCKET_UNIX
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/time.h>
#include<sys/stat.h>
#include<netinet/in.h>
#include<netinet/tcp.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<errno.h>
#include<sys/ioctl.h>
#include<unistd.h>
#include<fcntl.h>
#ifdef USE_ETH_UNIX
#include<net/ethernet.h>
#include<net/if.h>
#include<ifaddrs.h>
#include<linux/if_packet.h>
#endif

#define SOCKET int
#define INVALID_SOCKET (-1)
#define SOCKETWOULDBLOCK ((errno==EINPROGRESS)||(errno==EAGAIN))
#define SOCKADDR_IN struct sockaddr_in
#define ioctlsocket ioctl
#define closesocket close
#define USE_SOCKET
#endif

#ifdef USE_SOCKET_WIN
#include<winsock.h>
#include<io.h>
#include<conio.h>
#define SOCKETWOULDBLOCK (WSAGetLastError()==WSAEWOULDBLOCK)
#define ssize_t int
#define socklen_t int
#define USE_SOCKET
#endif

#ifdef USE_SOCKET_STUB
#define SOCKET int
#define INVALID_SOCKET (-1)
#define ssize_t int
#define socklen_t int
#define closesocket(x)
#endif

#define PIPE_READ 0
#define PIPE_WRITE 1

typedef struct Socket Socket;
typedef int (*SELECT_READABLE)(Socket* socket);
typedef int (*SELECT_WRITABLE)(Socket* socket);

struct Socket
{
	LB header;
	FORGET forget;
	MARK mark;

	SOCKET fd;
	SELECT_READABLE checkReadable;	// for manual select
	SELECT_WRITABLE checkWritable;	// for manual select
	int selectRead;
	int selectWrite;
	int readable;
	int writable;
	LINT watchAddress;
	LINT watchMask;
	LINT watchValue;
};

void lambdaPipe(SOCKET fds[2]);
void lambdaPipeClose(SOCKET fds[2]);
Socket* _socketCreate(SOCKET sock);
void internalSend(char* src, int len);

void _socketClose(Socket* s);
int fun_socketRead(Thread* th);
int fun_socketWrite(Thread* th);
int fun_socketClose(Thread* th);

int socketNextFd(void);

int sysSocketInit(Pkg* system);
void sysSocketClose(void);

int hwPsw(void);
#endif
