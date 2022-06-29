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

#ifdef ON_UNIX
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

#define SOCKET int
#define INVALID_SOCKET (-1)
#define SOCKETWOULDBLOCK ((errno==EINPROGRESS)||(errno==EAGAIN))
#define SOCKADDR_IN struct sockaddr_in
#define ioctlsocket ioctl
#define closesocket close
#endif
#ifdef ON_WINDOWS
#include<winsock.h>
#include<io.h>
#include<conio.h>
#define SOCKETWOULDBLOCK (WSAGetLastError()==WSAEWOULDBLOCK)
#define ssize_t int
#endif

typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	SOCKET fd;
	int getIp;
	int ip;
	int port;
	int selectRead;
	int selectWrite;
	int readable;
	int writable;
}Socket;

#define SOCKET_BUFFER_LEN (1024*32)
char SocketBuffer[SOCKET_BUFFER_LEN];

#define INTERNAL_READ 0
#define INTERNAL_WRITE 1
SOCKET InternalPipe[2];

SOCKET KeyboardPipe[2];

#ifdef ON_WINDOWS
MTHREAD KeyboardThread = NULL;
int KeyboardAlive = 1;
#endif

SOCKET UIsocket=0;
UI_HOOK UIloop=NULL;
void uiSocketRegister(int fd,UI_HOOK uiLoop)
{
	UIsocket=(SOCKET)fd;
	UIloop=uiLoop;
}
int _socketInfo(void* p, int* ip, int* port)
{
	Socket* s = (Socket*)p;
	*ip = s->ip;
	*port = s->port;
	return (int)s->fd;
}
void _socketClose(Socket* s)
{
	if (s->fd == INVALID_SOCKET) return;
//	printf("close socket %lld\n", s->fd);
	closesocket(s->fd);
	s->fd = INVALID_SOCKET;
}
int _socketForget(LB* p)
{
	_socketClose((Socket*)p);
	return 0;
}

int fun_hostName(Thread* th)
{
	char buf[256];
	if (gethostname(buf, 256)) return stackPushStr(th, "localhost",-1);
	return stackPushStr(th, buf, -1);
}

MTHREAD_START _ipByName(Thread* th)
{
	LW result = NIL;
	struct hostent* hp;
	LINT n = 0;
	LB* name = VALTOPNT(STACKGET(th, 0));
	if (!name) goto cleanup;
	hp = gethostbyname(STRSTART(name));
	if (hp)
	{
		u_long** p = (u_long**)hp->h_addr_list;
		while (p[n])
		{
			char* ip;
			char* q;
			struct in_addr Addr;
			Addr.s_addr = *(p[n]);
			ip = (char*)inet_ntoa(Addr);
			q = workerAllocStr(th, strlen(ip)); if (!q) goto cleanup;
			memcpy(q, ip, strlen(ip));
			n++;
		}
	}
	workerMakeList(th, n);
	result = STACKGET(th, 0);
cleanup:
	return workerDone(th, result);
}
int fun_ipByName(Thread* th) { return workerStart(th, 1, _ipByName); }

MTHREAD_START _nameByIp(Thread* th)
{
	LW result = NIL;
	long addr;
	struct hostent* hp;
	LB* ip = VALTOPNT(STACKGET(th, 0));
	if (!ip) goto cleanup;
	addr = inet_addr(STRSTART(ip));
	if ((hp = gethostbyaddr((char*)&addr, sizeof(addr), AF_INET)))
	{
		char* q;
		q = workerAllocStr(th, strlen(hp->h_name)); if (!q) goto cleanup;
		memcpy(q, hp->h_name, strlen(hp->h_name));
		result = STACKGET(th, 0);
//		printf("-->%llx\n", hp->h_aliases[0]);
	}
cleanup:
	return workerDone(th, result);
}
int fun_nameByIp(Thread* th) { return workerStart(th, 1, _nameByIp); }

int fun_tcpOpen(Thread* th)
{
	Socket* s;
	SOCKADDR_IN ina;
	int ip;
	SOCKET sock= INVALID_SOCKET;
	long argp = 1;
//	int set = 1;

	LINT NDROP = 2 - 1;
	LW result = NIL;

	LINT port = VALTOINT(STACKGET(th, 0));
	LB* ip_str = VALTOPNT(STACKGET(th, 1));
	if ((!port) || (!ip_str)) goto cleanup;

	ip = inet_addr(STRSTART(ip_str));

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) goto cleanup;
	ioctlsocket(sock, FIONBIO, &argp);
//	setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));

	ina.sin_family = PF_INET;
	ina.sin_port = htons((unsigned short)port);
	ina.sin_addr.s_addr = ip;
	if ((connect(sock, (struct sockaddr*)&ina, sizeof(ina)) != 0)
		&& (!SOCKETWOULDBLOCK)) goto cleanup;
	s = (Socket*)memoryAllocExt(th, sizeof(Socket), DBG_SOCKET, _socketForget, NULL);
	if (!s) {
		closesocket(sock);
		return EXEC_OM;
	}
	s->fd = sock;
	s->getIp = 0;
	s->ip = ina.sin_addr.s_addr;
	s->port = ntohs(ina.sin_port);
	s->selectRead = 1;
	s->selectWrite = 1;
	s->readable = 0;
	s->writable = 0;

//	PRINTF(th,LOG_ERR, "create socket %d to %s %d\n", s->fd, STRSTART(ip_str),port);
	sock = INVALID_SOCKET;
	result = PNTTOVAL(s);
cleanup:
	if (sock != INVALID_SOCKET) closesocket(sock);
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}
int fun_tcpListen(Thread* th)
{
	Socket* s;
	SOCKADDR_IN ina;
	SOCKET sock = INVALID_SOCKET;
	long argp = 1;
	int opt = 1;
	int k;

	LINT NDROP = 2 - 1;
	LW result = NIL;

	LINT port = VALTOINT(STACKGET(th, 0));
	LB* ip_str = VALTOPNT(STACKGET(th, 1));
	if (!port) goto cleanup;

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) goto cleanup;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
	ioctlsocket(sock, FIONBIO, &argp);

	memset(&ina, 0, sizeof(ina));
	ina.sin_family = PF_INET;
	ina.sin_port = htons((unsigned short)port);
	ina.sin_addr.s_addr = ip_str ? inet_addr(STRSTART(ip_str)) : INADDR_ANY;
	if ((k = bind(sock, (struct sockaddr*)&ina, sizeof(ina))) != 0)
	{
		PRINTF(th,LOG_ERR,"Sockets: Tcp port %s:%d bind error\n", ip_str ? STRSTART(ip_str) : "*" , port);
		goto cleanup;
	}
	if (listen(sock, 256) != 0)
	{
		PRINTF(th,LOG_ERR,"Sockets: Tcp port %d listen error\n", port);
		goto cleanup;
	}
	s = (Socket*)memoryAllocExt(th, sizeof(Socket), DBG_SOCKET, _socketForget, NULL);
	if (!s) {
		closesocket(sock);
		return EXEC_OM;
	}
	s->fd = sock;
	s->getIp = 0;
	s->ip = ina.sin_addr.s_addr;
	s->port = ntohs(ina.sin_port);
	s->selectRead = 1;
	s->selectWrite = 0;
	s->readable = 0;
	s->writable = 0;

	sock = INVALID_SOCKET;
	result = PNTTOVAL(s);
cleanup:
	if (sock != INVALID_SOCKET) closesocket(sock);
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}
int fun_tcpAccept(Thread* th)
{
	Socket* s;
	SOCKET sock;
	SOCKADDR_IN cor;
	unsigned int sizecor = sizeof(SOCKADDR_IN);
	long argp = 1;

	Socket* srv = (Socket*)VALTOPNT(STACKGET(th, 0));
	if (!srv) return 0;
	if (srv->fd == INVALID_SOCKET)
	{
		STACKSET(th, 0, NIL);
		return 0;
	}

	sizecor = sizeof(cor);
	sock = accept(srv->fd, (struct sockaddr*)&cor, &sizecor);
	if (sock == INVALID_SOCKET)
	{
		STACKSET(th, 0, NIL);
		return 0;
	}
	ioctlsocket(sock, FIONBIO, &argp);

	s = (Socket*)memoryAllocExt(th, sizeof(Socket), DBG_SOCKET, _socketForget, NULL);
	if (!s) {
		closesocket(sock);
		return EXEC_OM;
	}
	s->fd = sock;
	s->getIp = 0;
	s->ip = cor.sin_addr.s_addr;
	s->port = ntohs(cor.sin_port);

	s->selectRead = 1;
	s->selectWrite = 0;
	s->readable = 0;
	s->writable = 0;

	STACKSET(th, 0, PNTTOVAL(s));
	return 0;
}

int fun_sockSelectRead(Thread* th)
{
	LW state = STACKPULL(th);
	Socket* s = (Socket*)VALTOPNT(STACKGET(th, 0));
	if (!s) return 0;
	s->selectRead = (state == MM.trueRef) ? 1 : 0;
	return 0;
}
int fun_sockSelectWrite(Thread* th)
{
	LW state = STACKPULL(th);
	Socket* s = (Socket*)VALTOPNT(STACKGET(th, 0));
	if (!s) return 0;
	s->selectWrite = (state == MM.trueRef) ? 1 : 0;
	return 0;
}

int fun_tcpNoDelay(Thread* th)
{
	int nodelay = (STACKPULL(th) == MM.trueRef) ? 1 : 0;
	Socket* s = (Socket*)VALTOPNT(STACKGET(th, 0));
	if (!s) return 0;
	if (s->fd != INVALID_SOCKET)
	{
		setsockopt(s->fd, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(int));
	}
	return 0;
}

int fun_sockSelectable(Thread* th)
{
	Socket* s = (Socket*)VALTOPNT(STACKGET(th, 0));
	if (!s) return 0;
	STACKSET(th, 0, (s->selectWrite || s->selectRead) ? MM.trueRef : MM.falseRef);
	return 0;
}


int fun_sockReadable(Thread* th)
{
	Socket* s = (Socket*) VALTOPNT(STACKGET(th, 0));
	if (!s) return 0;
	STACKSET(th, 0, s->readable ? MM.trueRef: MM.falseRef);
	return 0;
}
int fun_sockWritable(Thread* th)
{
	Socket* s = (Socket*)VALTOPNT(STACKGET(th, 0));
	if (!s) return 0;
	STACKSET(th, 0, s->writable ? MM.trueRef : MM.falseRef);
	return 0;
}
int fun_sockIp(Thread* th)
{
	LB* p;
	struct in_addr Addr;
	char* ip_str;
	Socket* s = (Socket*)VALTOPNT(STACKGET(th, 0));
	if (!s) return 0;
	Addr.s_addr = s->ip;
	ip_str = inet_ntoa(Addr);
	p = memoryAllocStr(th, ip_str, -1); if (!p) return EXEC_OM;
	STACKSET(th, 0, PNTTOVAL(p));
	return 0;
}
int fun_sockPort(Thread* th)
{
	Socket* s = (Socket*)VALTOPNT(STACKGET(th, 0));
	if (!s) return 0;
	STACKSETINT(th, 0, (s->port));
	return 0;
}


int fun_sockClose(Thread* th)
{
	Socket* s = (Socket*)VALTOPNT(STACKGET(th, 0));
	if (!s) return 0;
	_socketClose(s);
	STACKSET(th, 0, NIL);
	return 0;
}


int fun_sockRead(Thread* th)
{
    ssize_t len=-1;
	Socket* s = (Socket*)VALTOPNT(STACKGET(th, 0));
	if (s && (s->fd != INVALID_SOCKET))
	{
		if (s->getIp)
		{
			SOCKADDR_IN add;
            unsigned int l = sizeof(add);
			len = recvfrom(s->fd, SocketBuffer, SOCKET_BUFFER_LEN, 0, (struct sockaddr*)&add, &l);
			s->ip = add.sin_addr.s_addr;
			s->port = ntohs(add.sin_port);
		}
		else
#ifdef ON_UNIX
			len = read(s->fd, SocketBuffer, SOCKET_BUFFER_LEN);
#endif
#ifdef ON_WINDOWS
			len = recv(s->fd, SocketBuffer, SOCKET_BUFFER_LEN, 0);
#endif
		if (len == 0)
		{
			len = -2;
			_socketClose(s);
		}
		else if (len < 0)
		{
			if (SOCKETWOULDBLOCK) len = 0;
			else len = -1;
		}
	}
	if (len < 0) STACKSETNIL(th, 0);
	else {
		LB* p = memoryAllocStr(th, SocketBuffer, len); if (!p) return EXEC_OM;
		STACKSET(th, 0, PNTTOVAL(p));
	}
	return 0;
}

int fun_sockWrite(Thread* th)
{
	ssize_t len,len0;
	LINT NDROP = 3 - 1;
	LW result = NIL;

	int start = (int) VALTOINT(STACKGET(th, 0));
	LB* str = VALTOPNT(STACKGET(th, 1));
	Socket* s = (Socket*)VALTOPNT(STACKGET(th, 2));
	if (!s) goto cleanup;
	s->selectWrite = 0;
	if ((!str) || (start<0) || (s->fd == INVALID_SOCKET)) goto cleanup;
	len0 = (ssize_t)(STRLEN(str) - start);
	len = len0;
	if (len<0) goto cleanup;
	if (len > 0)
	{
        ssize_t len0 = len;
		len = send(s->fd, STRSTART(str)+start, len0, 0);
		s->selectWrite = ((len >= 0) && (len < len0)) ? 1 : 0;
	}
	if (len < 0)
	{
		if (SOCKETWOULDBLOCK) len = start;
		else len = -1;
	}
	else len += start;
	result = INTTOVAL(len);
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}

int fun_udpCreate(Thread* th)
{
	Socket* s;
	SOCKADDR_IN ina;
	SOCKET sock = INVALID_SOCKET;
	long argp = 1;
	int opt = 1;
	int k;

	LINT NDROP = 2 - 1;
	LW result = NIL;

	LINT port = VALTOINT(STACKGET(th, 0));
	LB* ip_str = VALTOPNT(STACKGET(th, 1));

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) goto cleanup;
	ioctlsocket(sock, FIONBIO, &argp);

	if (port)
	{
		memset(&ina, 0, sizeof(ina));
		ina.sin_family = PF_INET;
		ina.sin_port = htons((unsigned short)port);
		ina.sin_addr.s_addr = ip_str ? inet_addr(STRSTART(ip_str)) : INADDR_ANY;
		if ((k = bind(sock, (struct sockaddr*)&ina, sizeof(ina))) != 0)
		{
			PRINTF(th,LOG_ERR, "Sockets: Udp port %s:%d bind error\n", ip_str ? STRSTART(ip_str) : "*", port);
			goto cleanup;
		}
	}
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(opt));

	s = (Socket*)memoryAllocExt(th, sizeof(Socket), DBG_SOCKET, _socketForget, NULL);
	if (!s) {
		closesocket(sock);
		return EXEC_OM;
	}
	s->fd = sock;
	s->getIp = 1;
	s->ip = 0;
	s->port = 0;
	s->selectRead = 1;
	s->selectWrite = 0;
	s->readable = 0;
	s->writable = 0;

	sock = INVALID_SOCKET;
	result = PNTTOVAL(s);
cleanup:
	if (sock != INVALID_SOCKET) closesocket(sock);
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}

// [content port ip udp]
int fun_udpSend(Thread* th)
{
	int ip;
	SOCKADDR_IN ina;
	int NDROP = 4 - 1;
	LW result = NIL;

	LB* content = VALTOPNT(STACKGET(th, 0));
	LINT port = VALTOINT(STACKGET(th, 1));
	LB* ip_str = VALTOPNT(STACKGET(th, 2));
	Socket* s = (Socket*)VALTOPNT(STACKGET(th, 3));
	if ((!s) || (s->fd == INVALID_SOCKET) || (!content)) goto cleanup;
	if ((!port) || (!ip_str)) goto cleanup;

	ip = inet_addr(STRSTART(ip_str));

	ina.sin_family = PF_INET;
	ina.sin_port = htons((unsigned short)port);
	ina.sin_addr.s_addr = ip;
	sendto(s->fd, STRSTART(content), (int)STRLEN(content), 0, (struct sockaddr*)&ina, sizeof(ina));
	//PRINTF(th,th,LOG_DEVCORE,"Socket: send '%s' (%d) on %d\n",STRSTART(content),STRLEN(content),s);
	result = MM.trueRef;
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}

int fun_select(Thread* th)
{
	fd_set r[1];
	fd_set w[1];
	LB* p;
	int k;

	LW wTimeout = STACKPULL(th);
	LB* sockets = VALTOPNT(STACKGET(th, 0));

	FD_ZERO(r);
	FD_ZERO(w);
//	FD_SET(InternalPipe[INTERNAL_READ], r);

	p = sockets;
	while (p)
	{
		Socket* s = (Socket*)VALTOPNT(TABGET(p, LIST_VAL));
		if (s->fd != INVALID_SOCKET)
		{
			if (s->selectRead) FD_SET(s->fd, r);
			if (s->selectWrite) FD_SET(s->fd, w);
			s->readable = s->writable = 0;
		}
		p = VALTOPNT(TABGET(p, LIST_NXT));
	}
	if (UIloop) FD_SET(UIsocket,r);
	if (wTimeout == NIL)
	{
		k = select(FD_SETSIZE, r, w, NULL, NULL);
	}
	else
	{
		struct timeval tm;
		int d = (int)VALTOINT(wTimeout);
		tm.tv_sec = d / 1000;
		tm.tv_usec = (int)((d - (1000 * tm.tv_sec)) * 1000);
		k = select(FD_SETSIZE, r, w, NULL, &tm);
	}
	if (k)
	{
		p = sockets;
		while (p)
		{
			Socket* s = (Socket*)VALTOPNT(TABGET(p, LIST_VAL));
			if (s->fd != INVALID_SOCKET)
			{
				if (FD_ISSET(s->fd, w)) s->writable = 1;
				if (FD_ISSET(s->fd, r)) s->readable = 1;
			}
			p = VALTOPNT(TABGET(p, LIST_NXT));
		}
	}
	if (UIloop) (*UIloop)();
	STACKSETINT(th, 0, (k));
	return 0;
}

#ifdef ON_UNIX
void lambdaPipe(SOCKET fds[2])
{
	int saved_flags;
	pipe(fds);
	saved_flags = fcntl(fds[0], F_GETFL);
	fcntl(fds[0], F_SETFL, saved_flags | O_NONBLOCK);
}
#endif
#ifdef ON_WINDOWS
void lambdaPipe(SOCKET fds[2])
{
	struct sockaddr_in inaddr;
	struct sockaddr addr;
	int yes = 1;
	int len = sizeof(inaddr);

	SOCKET lst = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	memset(&inaddr, 0, sizeof(inaddr));
	memset(&addr, 0, sizeof(addr));
	inaddr.sin_family = AF_INET;
	inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	inaddr.sin_port = 0;
	setsockopt(lst, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes));
	bind(lst, (struct sockaddr*)&inaddr, sizeof(inaddr));
	listen(lst, 1);
	getsockname(lst, &addr, &len);
	fds[1] = socket(AF_INET, SOCK_STREAM, 0);
	connect(fds[1], &addr, len);
	fds[0] = accept(lst, 0, 0);
	closesocket(lst);
}
#endif
void lambdaPipeClose(SOCKET fds[2])
{
	closesocket(fds[0]);
	closesocket(fds[1]);
}
void internalSend(char* src, int len)
{
	int k = 0;
	while (k < len) {
#ifdef ON_UNIX
		k += write(InternalPipe[INTERNAL_WRITE], src + k, len - k);
#endif
#ifdef ON_WINDOWS
		k += send(InternalPipe[INTERNAL_WRITE], src + k, len - k, 0);
#endif
	}
}

#ifdef ON_WINDOWS
MTHREAD_START keyboardThread(void* param)
{
	int k = 0;

	//	SetConsoleCtrlHandler(NULL, TRUE);	// this disables Ctr-C default behavior and other like this
	while (KeyboardAlive && (k >= 0))
	{
		char buf[1];
		LINT val;
		while (!kbhit()) Sleep(100);
		val = getch();
		buf[0] = (char)val;
		k = -1;
		if (buf[0] == 0x04) closesocket(KeyboardPipe[INTERNAL_WRITE]);	//Ctr-D, aka EOT ascii code
		else k = send(KeyboardPipe[INTERNAL_WRITE], buf, 1, 0);
		//		printf("keyboardwrite %d: %d\n", k, val);
	}
//	printf("keyboard close\n");
	return MTHREAD_RETURN;
}
#endif

int fun_kbdOpen(Thread* th)
{
	Socket* s = (Socket*)memoryAllocExt(th, sizeof(Socket), DBG_SOCKET, _socketForget, NULL); if (!s) return EXEC_OM;
	s->fd = KeyboardPipe[INTERNAL_READ];
	s->getIp = 0;
	s->ip = 0;
	s->port = 0;
	s->selectRead = 0;
	s->selectWrite = 0;
	s->readable = 0;
	s->writable = 0;
#ifdef ON_WINDOWS
	if (!KeyboardThread)
	{
		long argp = 1;
		KeyboardAlive = 1;
		ioctlsocket(KeyboardPipe[INTERNAL_READ], FIONBIO, &argp);

		KeyboardThread = hwThreadCreate(keyboardThread, NULL);
	}
#endif
	return STACKPUSH(th, PNTTOVAL(s));
}

int fun_internalOpen(Thread* th)
{
	Socket* s = (Socket*)memoryAllocExt(th, sizeof(Socket), DBG_SOCKET, _socketForget, NULL); if (!s) return EXEC_OM;
	s->fd = InternalPipe[INTERNAL_READ];
	s->getIp = 0;
	s->ip = 0;
	s->port = 0;
	s->selectRead = 0;
	s->selectWrite = 0;
	s->readable = 0;
	s->writable = 0;
	return STACKPUSH(th, PNTTOVAL(s));
}

int sysSocketInit(Thread* th, Pkg* system)
{
	Ref* socket = pkgAddType(th, system, "Socket");
	Type* list_S = typeAlloc(th,TYPECODE_LIST, NULL, 1, MM.S);
	Type* list_Sock = typeAlloc(th,TYPECODE_LIST, NULL, 1, socket->type);

	pkgAddFun(th, system, "_kbdOpen", fun_kbdOpen, typeAlloc(th,TYPECODE_FUN, NULL, 1, socket->type));
	pkgAddFun(th, system, "_internalOpen", fun_internalOpen, typeAlloc(th,TYPECODE_FUN, NULL, 1, socket->type));
	pkgAddFun(th, system, "_tcpOpen", fun_tcpOpen, typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.S, MM.I, socket->type));
	pkgAddFun(th, system, "_tcpListen", fun_tcpListen, typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.S, MM.I, socket->type));
	pkgAddFun(th, system, "_tcpAccept", fun_tcpAccept, typeAlloc(th,TYPECODE_FUN, NULL, 2, socket->type, socket->type));
	pkgAddFun(th, system, "_sockRead", fun_sockRead, typeAlloc(th,TYPECODE_FUN, NULL, 2, socket->type, MM.S));
	pkgAddFun(th, system, "_sockWrite", fun_sockWrite, typeAlloc(th,TYPECODE_FUN, NULL, 4, socket->type, MM.S, MM.I, MM.I));
	pkgAddFun(th, system, "_tcpNoDelay", fun_tcpNoDelay, typeAlloc(th,TYPECODE_FUN, NULL, 3, socket->type, MM.Boolean, socket->type));
	pkgAddFun(th, system, "_sockSelectRead", fun_sockSelectRead, typeAlloc(th,TYPECODE_FUN, NULL, 3, socket->type, MM.Boolean, socket->type));
	pkgAddFun(th, system, "_sockSelectWrite", fun_sockSelectWrite, typeAlloc(th,TYPECODE_FUN, NULL, 3, socket->type, MM.Boolean, socket->type));
	pkgAddFun(th, system, "_sockSelectable", fun_sockSelectable, typeAlloc(th,TYPECODE_FUN, NULL, 2, socket->type, MM.Boolean));
	pkgAddFun(th, system, "_sockReadable", fun_sockReadable, typeAlloc(th,TYPECODE_FUN, NULL, 2, socket->type, MM.Boolean));
	pkgAddFun(th, system, "_sockWritable", fun_sockWritable, typeAlloc(th,TYPECODE_FUN, NULL, 2, socket->type, MM.Boolean));
	pkgAddFun(th, system, "_sockClose", fun_sockClose, typeAlloc(th,TYPECODE_FUN, NULL, 2, socket->type, MM.I));
	pkgAddFun(th, system, "_sockIp", fun_sockIp, typeAlloc(th,TYPECODE_FUN, NULL, 2, socket->type, MM.S));
	pkgAddFun(th, system, "_sockPort", fun_sockPort, typeAlloc(th,TYPECODE_FUN, NULL, 2, socket->type, MM.I));
	pkgAddFun(th, system, "_udpCreate", fun_udpCreate, typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.S, MM.I, socket->type));
	pkgAddFun(th, system, "_udpSend", fun_udpSend, typeAlloc(th,TYPECODE_FUN, NULL, 5, socket->type, MM.S, MM.I, MM.S, MM.Boolean));

	pkgAddFun(th, system, "_select", fun_select, typeAlloc(th,TYPECODE_FUN, NULL, 3, list_Sock, MM.I, MM.I));
	pkgAddFun(th, system, "_ipByName", fun_ipByName, typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.S, list_S));
	pkgAddFun(th, system, "_nameByIp", fun_nameByIp, typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.S, MM.S));
	pkgAddFun(th, system, "hostName", fun_hostName, typeAlloc(th, TYPECODE_FUN, NULL, 1, MM.S));

	{
#ifdef ON_UNIX
		KeyboardPipe[INTERNAL_READ] = 0;
#endif
#ifdef ON_WINDOWS
		WORD wVersionRequested;
		WSADATA wsaData;
//		long argp = 1;

		wVersionRequested = MAKEWORD(1, 1);
		if (WSAStartup(wVersionRequested, &wsaData)) return -1;
		lambdaPipe(KeyboardPipe);
//		ioctlsocket(KeyboardPipe[INTERNAL_READ], FIONBIO, &argp);

#endif
		lambdaPipe(InternalPipe);

	}
	return 0;
}

void sysSocketClose()
{
#ifdef ON_WINDOWS
	KeyboardAlive = 0;
	KeyboardThread = NULL;
	Sleep(200);
	lambdaPipeClose(KeyboardPipe);
#endif
	lambdaPipeClose(InternalPipe);
}
