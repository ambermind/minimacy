// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

int SOCKET_FD=1;

int socketNextFd(void)
{
	return SOCKET_FD++;
}
void _socketClose(Socket* s)
{
//	PRINTF(LOG_DEV,"close socket %lld\n", s->fd);
	if (s->fd == INVALID_SOCKET) return;
	if (s->fd==0) return;	// we don't close stdin
	closesocket(s->fd);
	s->fd = INVALID_SOCKET;
}
int _socketForget(LB* p)
{
	_socketClose((Socket*)p);
	return 0;
}
Socket* _socketCreate(SOCKET sock)
{
	Socket* s = (Socket*)memoryAllocNative(sizeof(Socket), DBG_SOCKET, _socketForget, NULL);
	if (!s) {
		closesocket(sock);
		return NULL;
	}
	s->fd = sock;
	s->selectRead = 0;
	s->selectWrite = 0;
	s->readable = s->writable = 0;
	s->checkReadable= s->checkWritable= NULL;
	return s;
}

int fun_socketEmpty(Thread* th)
{
	Socket* s = _socketCreate(INVALID_SOCKET); if (!s) return EXEC_OM;
	FUN_RETURN_PNT((LB*)s);
}

int fun_socketSetSelectRead(Thread* th)
{
	LB* state = STACK_PULL_PNT(th);
	Socket* s = (Socket*)STACK_PNT(th, 0);
	if (s) s->selectRead = (state == MM._true) ? 1 : 0;
	return 0;
}
int fun_socketSetSelectWrite(Thread* th)
{
	LB* state = STACK_PULL_PNT(th);
	Socket* s = (Socket*)STACK_PNT(th, 0);
	if (s) s->selectWrite= (state == MM._true) ? 1 : 0;
	return 0;
}

int fun_socketSelectRead(Thread* th)
{
	Socket* s = (Socket*)STACK_PNT(th, 0);
	if (!s) FUN_RETURN_NIL;
	FUN_RETURN_BOOL(s->selectRead);
}
int fun_socketSelectWrite(Thread* th)
{
	Socket* s = (Socket*)STACK_PNT(th, 0);
	if (!s) FUN_RETURN_NIL;
	FUN_RETURN_BOOL(s->selectWrite);
}
int fun_socketReadable(Thread* th)
{
	Socket* s = (Socket*) STACK_PNT(th, 0);
	if (!s) return 0;
	STACK_SET_BOOL(th, 0, s->readable);
	return 0;
}
int fun_socketWritable(Thread* th)
{
	Socket* s = (Socket*)STACK_PNT(th, 0);
	if (!s) return 0;
	STACK_SET_BOOL(th, 0, s->writable);
	return 0;
}
int fun_socketSetReadable(Thread* th)
{
	LB* state = STACK_PULL_PNT(th);
	Socket* s = (Socket*)STACK_PNT(th, 0);
	if (s) s->readable = (state == MM._true) ? 1 : 0;
	return 0;
}
int fun_socketSetWritable(Thread* th)
{
	LB* state = STACK_PULL_PNT(th);
	Socket* s = (Socket*)STACK_PNT(th, 0);
	if (s) s->writable = (state == MM._true) ? 1 : 0;
	return 0;
}

int fun_socketClose(Thread* th)
{
	Socket* s = (Socket*)STACK_PNT(th, 0);
	if (!s) return 0;
	_socketClose(s);
	FUN_RETURN_NIL;
}


#ifdef USE_SOCKET
#define SOCKET_BUFFER_LENGTH (1024*32)
char SocketBuffer[SOCKET_BUFFER_LENGTH];

SOCKET InternalPipe[2];

#ifdef USE_SOCKET_UNIX
void lambdaPipe(SOCKET fds[2])
{
	int saved_flags;
	if (pipe(fds)) return;
	saved_flags = fcntl(fds[0], F_GETFL);
	fcntl(fds[0], F_SETFL, saved_flags | O_NONBLOCK);
}
#endif
#ifdef USE_SOCKET_WIN
void lambdaPipe(SOCKET fds[2])
{
	struct sockaddr_in inaddr;
	struct sockaddr addr;
	int yes = 1;
	int len = sizeof(inaddr);
	long argp = 1;

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
	ioctlsocket(fds[0], FIONBIO, &argp);
	ioctlsocket(fds[1], FIONBIO, &argp);

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
#ifdef USE_SOCKET_UNIX
	while (k < len) k += write(InternalPipe[PIPE_WRITE], src + k, len - k);
#endif
#ifdef USE_SOCKET_WIN
	while (k < len) k += send(InternalPipe[PIPE_WRITE], src + k, len - k, 0);
#endif
}

int fun_hostName(Thread* th)
{
	char buf[256];
	if (gethostname(buf, 256)) FUN_RETURN_STR("localhost", -1);
	FUN_RETURN_STR(buf, -1);
}

WORKER_START _ipByName(volatile Thread* th)
{
	struct hostent* hp;
	LINT n = 0;

	LB* name = STACK_PNT(th, 0);
	volatile Buffer* out = (Buffer*)STACK_PNT(th, 1);
	if ((!name) || (!out)) return workerDoneNil(th);
	bufferSetWorkerThread(&out, &th);
	hp = gethostbyname(STR_START(name));
	if (hp)
	{
		u_int** p = (u_int**)hp->h_addr_list;
		while (p[n])
		{
			char* ip;
			struct in_addr Addr;
			Addr.s_addr = *(p[n]);	// warning on Macos should cast with (in_addr_t) not known on windows
			ip = (char*)inet_ntoa(Addr);
			if (n) bufferAddCharWorker(&out, 0);
			bufferAddBinWorker(&out, ip, strlen(ip));
			n++;
		}
	}
	bufferUnsetWorkerThread(&out, &th);
	return workerDoneInt(th,n);
}
int fun_ipByName(Thread* th) { return workerStart(th, 2, _ipByName); }

WORKER_START _nameByIp(volatile Thread* th)
{
	long addr;
	struct hostent* hp;

	LB* ip = STACK_PNT(th, 0);
	volatile Buffer* out = (Buffer*)STACK_PNT(th, 1);
	if ((!ip) || (!out)) return workerDoneNil(th);
	bufferSetWorkerThread(&out, &th);
	addr = inet_addr(STR_START(ip));
	if ((hp = gethostbyaddr((char*)&addr, sizeof(addr), AF_INET)))
	{
		bufferAddBinWorker(&out, hp->h_name, strlen(hp->h_name));
		bufferUnsetWorkerThread(&out, &th);
		return workerDoneInt(th,1);
	}
	bufferUnsetWorkerThread(&out, &th);
	return workerDoneNil(th);
}
int fun_nameByIp(Thread* th) { return workerStart(th, 2, _nameByIp); }


int fun_socketWrite(Thread* th)
{
	LINT sent,len=0;

	LINT start = STACK_INT(th, 0);
	LB* src = STACK_PNT(th, 1);
	Socket* s = (Socket*)STACK_PNT(th, 2);
	if ((!s)|| (s->fd == INVALID_SOCKET)) FUN_RETURN_NIL;
	FUN_SUBSTR(src, start, len, 1, STR_LENGTH(src));

	if (len==0) FUN_RETURN_INT(start);
//	if (len > 16384) len = 16384;
	sent = send(s->fd, STR_START(src) + start, (int)len, 0);
	if (sent<0 && SOCKETWOULDBLOCK) sent=0;
	if (sent >= 0) FUN_RETURN_INT(start + sent);
	FUN_RETURN_NIL;
}

// returns :
// - non empty string : received data
// - empty string : socket ok but no received data
// - nil : closed or failed socket
int fun_socketRead(Thread* th)
{
	ssize_t len = -1;
	Socket* s = (Socket*)STACK_PNT(th, 0);
	if (s && (s->fd != INVALID_SOCKET))
	{
#ifdef USE_SOCKET_UNIX
		len = read(s->fd, SocketBuffer, SOCKET_BUFFER_LENGTH);
#endif
#ifdef USE_SOCKET_WIN
		len = recv(s->fd, SocketBuffer, SOCKET_BUFFER_LENGTH, 0);
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
		s->readable = 0;
	}
	if (len < 0) FUN_RETURN_NIL;
	FUN_RETURN_STR(SocketBuffer, len);
}

#ifdef USE_CONSOLE_IN_ANSI
#include<termios.h>
struct termios Term0;
int fun_keyboardOpen(Thread* th)
{
	struct termios term;
	Socket* s = _socketCreate(0); if (!s) return EXEC_OM;

	if (!tcgetattr(0, &term)) {
		term.c_lflag &= ~(ICANON | ECHO);
		term.c_cc[VMIN] = 0;
		term.c_cc[VTIME] = 0;

		tcsetattr(0, TCSANOW, &term);
	}
	FUN_RETURN_PNT((LB*)s);
}
int fun_keyboardRead(Thread* th) {
	return fun_socketRead(th);
}
#endif

#ifdef USE_CONSOLE_IN_WIN
SOCKET KeyboardPipe[2];
MTHREAD KeyboardThread = NULL;
int KeyboardAlive = 1;

char* EscCodes[128] = {
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, /*0.*/
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
NULL,"\033[5~","\033[6~","\033[F","\033[H","\033[D","\033[A","\033[C",
"\033[B",NULL,NULL,NULL,NULL,"\033[2~","\033[3~",NULL,
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, /*4.*/
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
"\033[OP","\033[OQ","\033[OR","\033[OS","\033[15~","\033[17~","\033[18~","\033[19~",
"\033[20~", "\033[21~", "\033[23~","\033[24~",NULL,NULL,NULL,NULL,
};

WORKER_START keyboardThread(void* param)
{
//	CONSOLE_CURSOR_INFO cinfo;
	DWORD mode = 0;
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	GetConsoleMode(hStdin, &mode);
	SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));

//	cinfo.bVisible = TRUE;
//	cinfo.dwSize = 1;
//	SetConsoleCursorInfo(hStdin, &cinfo);
//	SetConsoleCtrlHandler(NULL, TRUE);	// this disables Ctr-C default behavior and other like this
	while (KeyboardAlive)
	{
		char buf[8];
		INPUT_RECORD records;
		DWORD count;
		LINT val;
		int len = 0;

		PeekConsoleInput(hStdin, &records, 1, &count);
		if (!count) {
			Sleep(100);
			continue;
		}
		ReadConsoleInputW(hStdin, &records, 1, &count);
		if ((records.EventType != KEY_EVENT) || !records.Event.KeyEvent.bKeyDown) continue;
		val = records.Event.KeyEvent.uChar.UnicodeChar&0xfffff;
		if (!val) {
			char* code;
			val = records.Event.KeyEvent.wVirtualKeyCode;
//			printf("[%x].", val);
			if (val < 0 || val >= 128) continue;
			code = EscCodes[val];
			if (!code) continue;
			send(KeyboardPipe[PIPE_WRITE], code, (int)strlen(code), 0);
			continue;
		}
//		printf("%x.", val);
		if (val == 13) val = 10;
		if (val == 8) val = 0x7f;	// backspace
		if (val < 0x80) buf[len++] = (char)val;
		else if (val < 0x7ff) {
			buf[len++] = 0xc0 + (char)(val >> 6);
			buf[len++] = 0x80 + (char)(val & 0x3f);
		}
		else if (val < 0xffff) {
			buf[len++] = 0xe0 + (char)(val >> 12);
			buf[len++] = 0x80 + (char)((val>>6) & 0x3f);
			buf[len++] = 0x80 + (char)(val & 0x3f);
		}
		else if (val < 0x10ffff) {
			buf[len++] = 0xf0 + (char)(val >> 18);
			buf[len++] = 0x80 + (char)((val>>12) & 0x3f);
			buf[len++] = 0x80 + (char)((val>>6) & 0x3f);
			buf[len++] = 0x80 + (char)(val & 0x3f);
		}
		else continue;
//		putchar((char)val);	// echo
		len = send(KeyboardPipe[PIPE_WRITE], buf, len, 0);
		if (len < 0) break;
	}
//	PRINTF(LOG_DEV,"keyboard close\n");
	return WORKER_RETURN;
}

int fun_keyboardOpen(Thread* th)
{
	Socket* s = _socketCreate(KeyboardPipe[PIPE_READ]); if (!s) return EXEC_OM;
	if (!KeyboardThread)
	{
		long argp = 1;
		KeyboardAlive = 1;
		ioctlsocket(KeyboardPipe[PIPE_READ], FIONBIO, &argp);

		KeyboardThread = hwThreadCreate(keyboardThread, NULL);
	}
	FUN_RETURN_PNT((LB*)s);
}
int fun_keyboardRead(Thread* th) {
	return fun_socketRead(th);
}
#endif

int fun_internalOpen(Thread* th)
{
	Socket* s = _socketCreate(InternalPipe[PIPE_READ]); if (!s) return EXEC_OM;
	FUN_RETURN_PNT((LB*)s);
}
int fun_internalRead(Thread* th) {
	return fun_socketRead(th);
}

int fun_udpRead(Thread* th)
{
	char ip_str[32];
	int ip;
	int port=0;
	ssize_t len = -1;
	Socket* s = (Socket*)STACK_PNT(th, 0);
	if (s && (s->fd != INVALID_SOCKET))
	{
		SOCKADDR_IN add;
		socklen_t l = sizeof(add);
		len = recvfrom(s->fd, SocketBuffer, SOCKET_BUFFER_LENGTH, 0, (struct sockaddr*)&add, &l);
		ip = add.sin_addr.s_addr;
		port = ntohs(add.sin_port);
		snprintf(ip_str, 32, "%d.%d.%d.%d", ip & 255, (ip >> 8) & 255, (ip >> 16) & 255, (ip >> 24) & 255);

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
		s->readable = 0;
	}
	if (len < 0) FUN_RETURN_NIL;

	FUN_PUSH_STR(SocketBuffer, len);
	FUN_PUSH_STR(ip_str, -1);
	FUN_PUSH_INT(port);

	STACK_PUSH_FILLED_ARRAY_ERR(th, 3, DBG_TUPLE, EXEC_OM);
	return 0;
}

int fun_udpCreate(Thread* th)
{
	Socket* s;
	SOCKADDR_IN ina;
	SOCKET sock = INVALID_SOCKET;
	long argp = 1;
	int opt = 1;
	struct linger lin;
	int k;

	LINT port = STACK_INT(th, 0);
	LB* ip_str = STACK_PNT(th, 1);

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) FUN_RETURN_NIL;
	ioctlsocket(sock, FIONBIO, &argp);

	if (port>0)
	{
		memset(&ina, 0, sizeof(ina));
		ina.sin_family = PF_INET;
		ina.sin_port = htons((unsigned short)port);
		ina.sin_addr.s_addr = ip_str ? inet_addr(STR_START(ip_str)) : INADDR_ANY;
		if ((k = bind(sock, (struct sockaddr*)&ina, sizeof(ina))) != 0)
		{
			PRINTF(LOG_SYS, "> Error: Udp port %s:%d bind error\n", ip_str ? STR_START(ip_str) : "*", port);
			closesocket(sock);
			FUN_RETURN_NIL;
		}
	}
	lin.l_onoff = 0;
	lin.l_linger = 0;
	setsockopt(sock, SOL_SOCKET, SO_LINGER, (const char*)&lin, sizeof(int));
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(opt));

	s = _socketCreate(sock); if (!s) return EXEC_OM;
	FUN_RETURN_PNT((LB*)s);
}

// [content port ip localport localaddr udp]
int fun_udpSend(Thread* th)
{
	int ip;
	LINT sent,len=0;
	SOCKADDR_IN ina;

	LB* src = STACK_PNT(th, 0);
	LINT distantPort = STACK_INT(th, 1);
	LB* distantIp = STACK_PNT(th, 2);
	Socket* s = (Socket*)STACK_PNT(th, 3);
	if ((!s) || (s->fd == INVALID_SOCKET) || (!src) || (!distantPort) || (!distantIp)) FUN_RETURN_NIL;

	len=STR_LENGTH(src);
	if (len==0) FUN_RETURN_INT(0);

	ip = inet_addr(STR_START(distantIp));
	ina.sin_family = PF_INET;
	ina.sin_port = htons((unsigned short)distantPort);
	ina.sin_addr.s_addr = ip;
	sent = sendto(s->fd, STR_START(src), (int)len, 0, (struct sockaddr*)&ina, sizeof(ina));
	if (sent<0 && SOCKETWOULDBLOCK) sent=0;
	if (sent >= 0) FUN_RETURN_INT(sent);
	FUN_RETURN_NIL
}

int fun_tcpOpen(Thread* th)
{
	Socket* s;
	SOCKADDR_IN ina;
	int ip;
	SOCKET sock = INVALID_SOCKET;
	long argp = 1;
	//	int set = 1;

	LINT port = STACK_INT(th, 0);
	LB* ip_str = STACK_PNT(th, 1);
	if ((!port) || (!ip_str)) FUN_RETURN_NIL;

	ip = inet_addr(STR_START(ip_str));

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) FUN_RETURN_NIL;
	ioctlsocket(sock, FIONBIO, &argp);
	//	setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));

	ina.sin_family = PF_INET;
	ina.sin_port = htons((unsigned short)port);
	ina.sin_addr.s_addr = ip;
	if (connect(sock, (struct sockaddr*)&ina, sizeof(ina)) != 0) {
		if (!SOCKETWOULDBLOCK) {
			closesocket(sock);
			FUN_RETURN_NIL;
		}
	}
	s = _socketCreate(sock); if (!s) return EXEC_OM;
//	PRINTF(LOG_DEV, "create socket %d to %s %d\n", s->fd, STR_START(ip_str),port);
	FUN_RETURN_PNT((LB*)s);
}
int fun_tcpListen(Thread* th)
{
	Socket* s;
	SOCKADDR_IN ina;
	SOCKET sock = INVALID_SOCKET;
	long argp = 1;
	struct linger lin;
	int k;

	LINT port = STACK_INT(th, 0);
	LB* ip_str = STACK_PNT(th, 1);
	if (!port) FUN_RETURN_NIL;

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) FUN_RETURN_NIL;

	lin.l_onoff = 0;
	lin.l_linger = 0;
	setsockopt(sock, SOL_SOCKET, SO_LINGER, (const char*)&lin, sizeof(int));

	ioctlsocket(sock, FIONBIO, &argp);

	memset(&ina, 0, sizeof(ina));
	ina.sin_family = PF_INET;
	ina.sin_port = htons((unsigned short)port);
	ina.sin_addr.s_addr = ip_str ? inet_addr(STR_START(ip_str)) : INADDR_ANY;
	if ((k = bind(sock, (struct sockaddr*)&ina, sizeof(ina))) != 0)
	{
		PRINTF(LOG_SYS, "> Error: Tcp port %s:%d bind error\n", ip_str ? STR_START(ip_str) : "*", port);
		closesocket(sock);
		FUN_RETURN_NIL;
	}
	if (listen(sock, 256) != 0)
	{
		PRINTF(LOG_SYS, "> Error: Tcp port %d listen error\n", port);
		closesocket(sock);
		FUN_RETURN_NIL;
	}
	s = _socketCreate(sock); if (!s) return EXEC_OM;
	FUN_RETURN_PNT((LB*)s);
}
int fun_tcpAccept(Thread* th)
{
	char ip_str[32];
	int ip, port;
	Socket* s;
	SOCKET sock;
	SOCKADDR_IN remote;
	socklen_t remoteSize;
	long argp = 1;

	Socket* srv = (Socket*)STACK_PNT(th, 0);
	if (!srv) return 0;
	if (srv->fd == INVALID_SOCKET) FUN_RETURN_NIL;

	srv->readable = 0;

	remoteSize = sizeof(remote);
	sock = accept(srv->fd, (struct sockaddr*)&remote, &remoteSize);
	if (sock == INVALID_SOCKET) FUN_RETURN_NIL;
	ioctlsocket(sock, FIONBIO, &argp);

	s = _socketCreate(sock); if (!s) return EXEC_OM;

	ip = remote.sin_addr.s_addr;
	port = ntohs(remote.sin_port);
	snprintf(ip_str, 32, "%d.%d.%d.%d", ip & 255, (ip >> 8) & 255, (ip >> 16) & 255, (ip >> 24) & 255);

	FUN_PUSH_PNT((LB*)s);
	FUN_PUSH_STR(ip_str, -1);
	FUN_PUSH_INT(port);

	STACK_PUSH_FILLED_ARRAY_ERR(th, 3, DBG_TUPLE, EXEC_OM);
	return 0;
}
int fun_tcpNoDelay(Thread* th)
{
	int nodelay = (STACK_PULL_PNT(th) == MM._true) ? 1 : 0;
	Socket* s = (Socket*)STACK_PNT(th, 0);
	if (s && (s->fd != INVALID_SOCKET))
		setsockopt(s->fd, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(int));
	return 0;
}
int fun_tcpRead(Thread* th) {
	return fun_socketRead(th);
}
int fun_tcpWrite(Thread* th) {
	return fun_socketWrite(th);
}
#else
int fun_socketRead(Thread* th) FUN_RETURN_NIL
int fun_socketWrite(Thread* th) FUN_RETURN_NIL
int fun_tcpOpen(Thread* th) FUN_RETURN_NIL
int fun_tcpListen(Thread* th) FUN_RETURN_NIL
int fun_tcpAccept(Thread* th) FUN_RETURN_NIL
int fun_tcpRead(Thread* th) FUN_RETURN_NIL
int fun_tcpWrite(Thread* th) FUN_RETURN_NIL
int fun_tcpNoDelay(Thread* th) FUN_RETURN_NIL
int fun_udpCreate(Thread* th) FUN_RETURN_NIL
int fun_udpSend(Thread* th) FUN_RETURN_NIL
int fun_udpRead(Thread* th) FUN_RETURN_NIL
int fun_ipByName(Thread* th) FUN_RETURN_NIL
int fun_nameByIp(Thread* th) FUN_RETURN_NIL
int fun_hostName(Thread* th) FUN_RETURN_NIL
int fun_internalOpen(Thread* th) FUN_RETURN_NIL
int fun_internalRead(Thread* th) FUN_RETURN_NIL
void internalSend(char* src, int len) {}
#endif


#ifdef USE_ETH_UNIX
int fun_ethList(Thread* th)
{
	int count=0;
	struct ifaddrs *addresses = NULL;
	struct ifaddrs *address = NULL;\
	SOCKET ethSocket;

	if (getifaddrs(&addresses) == -1) FUN_RETURN_NIL;

	ethSocket=socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);	//only for root on Linux
	if(ethSocket==INVALID_SOCKET) FUN_RETURN_NIL;

	address = addresses;
	while(address)
	{
		struct ifreq if_idx;
		struct ifreq if_mac;
		int family = address->ifa_addr?address->ifa_addr->sa_family:-1;
		if (family == AF_INET)// || family == AF_INET6)
		{
			char ap[100];
			char* ifName=address->ifa_name;
			const int family_size= (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
			LINT idx=-1;
			
			memset(&if_idx, 0, sizeof(struct ifreq));
			strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
			if (ioctl(ethSocket, SIOCGIFINDEX, &if_idx) < 0) idx=if_idx.ifr_ifindex;
			FUN_PUSH_INT(idx);

			FUN_PUSH_STR(ifName,-1);

			FUN_PUSH_INT(family);

			memset(&if_mac, 0, sizeof(struct ifreq));
			strncpy(if_mac.ifr_name, ifName, IFNAMSIZ-1);
			if (ioctl(ethSocket, SIOCGIFHWADDR, &if_mac) < 0) {
				FUN_PUSH_NIL;
			}
			else {
				FUN_PUSH_STR(if_mac.ifr_hwaddr.sa_data,6);
			}
			FUN_PUSH_NIL;	// MAX_PACKET_SIZE
			getnameinfo(address->ifa_addr,family_size, ap, sizeof(ap), 0, 0, NI_NUMERICHOST);
			FUN_PUSH_STR(ap,-1);
			getnameinfo(address->ifa_netmask,family_size, ap, sizeof(ap), 0, 0, NI_NUMERICHOST);
			FUN_PUSH_STR(ap,-1);

			STACK_PUSH_FILLED_ARRAY_ERR(th, 7, DBG_TUPLE, EXEC_OM);
			count++;
		}
		address = address->ifa_next;
	}
	closesocket(ethSocket);
	FUN_PUSH_NIL;
	while(count--) STACK_PUSH_FILLED_ARRAY_ERR(th, LIST_LENGTH, DBG_LIST,EXEC_OM);

	freeifaddrs(addresses);
	return 0;
}
int fun_ethCreate(Thread* th)
{
	Socket* s;
	struct sockaddr_ll addr = {0};
	SOCKET sock = INVALID_SOCKET;
	long argp = 1;

	LINT ifindex = STACK_INT(th, 0);
	sock=socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));	//only for root on Linux
	if (sock == INVALID_SOCKET) {
		PRINTF(LOG_SYS, "> Error: Could not create AF_PACKET socket. User has insufficient permission\n");
		FUN_RETURN_NIL;
	}
	ioctlsocket(sock, FIONBIO, &argp);
	addr.sll_family = AF_PACKET;
	addr.sll_ifindex = ifindex;
	addr.sll_protocol = htons(ETH_P_ALL);
	if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		closesocket(sock);
		FUN_RETURN_NIL;
	}
	s = _socketCreate(sock); if (!s) return EXEC_OM;
//	printf("socket ok %d on %d\n",sock,ifindex);
	FUN_RETURN_PNT((LB*)s);
}

// [msg socket]
int fun_ethSend(Thread* th)
{
	struct sockaddr_ll socket_address;
	int sent,len;
	char* msg;

	LB* content = STACK_PNT(th, 0);
	LINT ifindex= STACK_INT(th, 1);
	Socket* s = (Socket*)STACK_PNT(th, 2);
	if ((!s) || (s->fd == INVALID_SOCKET) || (!content)|| (STR_LENGTH(content)<6)) FUN_RETURN_NIL;

	len=STR_LENGTH(content);
	msg=STR_START(content);
	socket_address.sll_ifindex = ifindex;
	socket_address.sll_halen = ETH_ALEN;
	memcpy(socket_address.sll_addr,msg,6);
	sent=sendto(s->fd, msg, len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
//	printf("write %d on %d result=%d\n",len,ifindex, k);
	if (sent<0 && SOCKETWOULDBLOCK) sent=0;
	if (sent >= 0) FUN_RETURN_INT(sent);
	FUN_RETURN_NIL;
}
int fun_ethRead(Thread* th) {
	return fun_socketRead(th);
}
#endif
#ifdef USE_SOCKET_UEFI
int fun_ethList(Thread* th);
int fun_ethCreate(Thread* th);
int fun_ethSend(Thread* th);
int fun_ethRead(Thread* th);
#endif

#ifdef USE_ETH_STUB
int fun_ethList(Thread* th) FUN_RETURN_NIL
int fun_ethCreate(Thread* th) FUN_RETURN_NIL
int fun_ethSend(Thread* th) FUN_RETURN_NIL
int fun_ethRead(Thread* th) FUN_RETURN_NIL
#endif

#ifdef USE_SOCKET
int fun_select(Thread* th)
{
	fd_set r[1];
	fd_set w[1];
	LB* p;
	int k;

	int timeoutIsNil = STACK_IS_NIL(th,0);
	LINT timeout = STACK_PULL_INT(th);
	LB* sockets = STACK_PNT(th, 0);

	FD_ZERO(r);
	FD_ZERO(w);

	p = sockets;
	while (p)
	{
		Socket* s = (Socket*)ARRAY_PNT(p, LIST_VAL);
		if (s) {
//			PRINTF(LOG_DEV,"select %d\n",s->fd);
			if (s->fd != INVALID_SOCKET)
			{
				if (s->selectRead) {
					FD_SET(s->fd, r);
					s->readable =0;
				}
				if (s->selectWrite) {
					FD_SET(s->fd, w);
					s->writable = 0;
				}
			}
		}
		p = (ARRAY_PNT(p, LIST_NXT));
	}
#ifdef ON_WINDOWS
	if (!sockets) {
		Sleep((DWORD)timeout);	// looks like Windows select does not work properly without socket ??
	}
	else 
#endif
	if (timeoutIsNil)
	{
		k = select(FD_SETSIZE, r, w, NULL, NULL);
	}
	else
	{
		struct timeval tm;
		int d = (int)timeout;
		tm.tv_sec = d / 1000;
		tm.tv_usec = (int)((d - (1000 * tm.tv_sec)) * 1000);
		k = select(FD_SETSIZE, r, w, NULL, &tm);
	}
	if (k)
	{
		p = sockets;
		while (p)
		{
			Socket* s = (Socket*)ARRAY_PNT(p, LIST_VAL);
			if (s && (s->fd != INVALID_SOCKET))
			{
//				if (FD_ISSET(s->fd, w)) PRINTF(LOG_DEV,"writable %d\n",s->fd);
//				if (FD_ISSET(s->fd, r)) PRINTF(LOG_DEV,"readable %d\n",s->fd);

				if (FD_ISSET(s->fd, w)) s->writable = 1;
				if (FD_ISSET(s->fd, r)) s->readable = 1;
			}
			p = ARRAY_PNT(p, LIST_NXT);
		}
	}
	FUN_RETURN_INT(k);
}
#else
int fun_select(Thread* th)
{
	LINT k=0;
	int timeoutIsNil = STACK_IS_NIL(th,0);
	LINT timeout = STACK_PULL_INT(th);
	LB* sockets = STACK_PNT(th, 0);

	if (!timeoutIsNil) timeout=hwTimeMs()+timeout;
	while(k==0)
	{
		LB* p = sockets;
#ifdef WITH_POWER_SWITCH
		if (hwPsw()) {
			PRINTF(LOG_SYS,"psw\n");
			MM.reboot = 1;
			return EXEC_EXIT;
		}
#endif
		while(p)
		{
			Socket* s = (Socket*)ARRAY_PNT(p, LIST_VAL);
			if (s && s->fd != INVALID_SOCKET)
			{
				//if (s->selectRead)  
				s->readable = (s->checkReadable && s->checkReadable(s))?1:0;
				if (s->selectWrite) s->writable = (s->checkWritable && s->checkWritable(s))?1:0;
				if ((s->selectRead&&s->readable)||(s->selectWrite&&s->writable)) k++;
			}
			p = ARRAY_PNT(p, LIST_NXT);
		}
		if ((!timeoutIsNil)&&(timeout-hwTimeMs()<=0)) break;
		
	}

	FUN_RETURN_INT(k);
}
#endif

#ifdef USE_CONSOLE_IN_UART
int fun_keyboardRead(Thread* th) {
    ssize_t len=-1;
	char buffer[128];
	Socket* s = (Socket*)STACK_PNT(th, 0);
	if (s && (s->fd != INVALID_SOCKET))
	{
		int c;
		len=0;
		while(len<128 && ((c=uartGet())>=0) ) buffer[len++]=c;
		if (len==0) len=-1;
	}
	if (len < 0) FUN_RETURN_NIL;
	FUN_RETURN_STR(buffer, len);
}

int _keyboardCheckReadable(Socket* s)
{
	return uartReadable();
}
int _keyboardCheckWritable(Socket* s)
{
	return uartWritable();
}

int fun_keyboardOpen(Thread* th)
{
	Socket* s = _socketCreate(0); if (!s) return EXEC_OM;
	s->checkReadable= _keyboardCheckReadable;
	s->checkWritable= _keyboardCheckWritable;
	FUN_RETURN_PNT((LB*)s);
}
#endif

#ifdef USE_CONSOLE_IN_STUB
int fun_keyboardRead(Thread* th) FUN_RETURN_NIL
int fun_keyboardOpen(Thread* th) FUN_RETURN_NIL
#endif


int _watcherCheckReadable(Socket* s)
{
	LINT val=0;
	if (!(s->watchMask&~0xff)) val=*(char*)s->watchAddress;
	else if (!(s->watchMask&~0xffff)) val=*(short*)s->watchAddress;
	else if (!(s->watchMask&~0xffffffff)) val=*(int*)s->watchAddress;
	if ((val & s->watchMask) != (s->watchValue& s->watchMask)) {
//		PRINTF(LOG_DEV,"watcher %llx (%llx) %llx %llx\n",val,(LINT)s->watchAddress,s->watchMask,s->watchValue);
		return 1;
	}
	return 0;
}

int fun_watcherOpen(Thread* th)
{
	Socket* s = _socketCreate(0); if (!s) return EXEC_OM;
	s->watchAddress = s->watchMask = s->watchValue = 0;
	s->checkReadable = _watcherCheckReadable;
	FUN_RETURN_PNT((LB*)s);
}
int fun_watcherUpdate(Thread* th)
{
	LINT val = STACK_INT(th, 0);
	LINT mask = STACK_INT(th, 1);
	LINT addr = STACK_INT(th, 2);
	Socket* s = (Socket*)STACK_PNT(th,3);
	if (!s) FUN_RETURN_NIL;
	s->watchAddress = addr;
	s->watchMask = mask;
	s->watchValue = val;
	FUN_RETURN_PNT((LB*)s);
}
int fun_ipChecksum(Thread* th)
{
	LINT i, len;
	unsigned short* q;
	LINT val = STACK_INT(th, 0);
	LB* p = STACK_PNT(th, 1);
	if (!p) FUN_RETURN_INT(val);
	q = (unsigned short*)STR_START(p);
	len = STR_LENGTH(p);
	for (i = 0; i < len; i += 2) {	// for odd-length strings we know that there is a final null char
		val += LSBW(*q);
		q++;
	}
	while (val > 0xffff) val = (val >> 16) + (val & 0xffff);
	FUN_RETURN_INT(val);
}
int fun_ipChecksumFinal(Thread* th)
{
	LINT val = STACK_INT(th,0);
	unsigned short checkSum = (unsigned short)~val;
	checkSum = LSBW(checkSum);
	FUN_RETURN_STR((char*)&checkSum, 2);
}

int sysSocketInit(Pkg* system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "socketEmpty", fun_socketEmpty, "fun -> Socket"},
		{ NATIVE_FUN, "_socketSetSelectRead", fun_socketSetSelectRead, "fun Socket Bool -> Socket"},
		{ NATIVE_FUN, "_socketSelectRead", fun_socketSelectRead, "fun Socket -> Bool"},
		{ NATIVE_FUN, "socketSetReadable", fun_socketSetReadable, "fun Socket Bool -> Socket"},
		{ NATIVE_FUN, "_socketReadable", fun_socketReadable, "fun Socket -> Bool"},
		{ NATIVE_FUN, "_socketSetSelectWrite", fun_socketSetSelectWrite, "fun Socket Bool -> Socket"},
		{ NATIVE_FUN, "_socketSelectWrite", fun_socketSelectWrite, "fun Socket -> Bool"},
		{ NATIVE_FUN, "socketSetWritable", fun_socketSetWritable, "fun Socket Bool -> Socket"},
		{ NATIVE_FUN, "_socketWritable", fun_socketWritable, "fun Socket -> Bool"},
		{ NATIVE_FUN, "_select", fun_select, "fun list Socket Int -> Int"},
		{ NATIVE_FUN, "_keyboardOpen", fun_keyboardOpen, "fun -> Socket"},
		{ NATIVE_FUN, "_keyboardRead", fun_keyboardRead, "fun Socket -> Str"},
		{ NATIVE_FUN, "_internalOpen", fun_internalOpen, "fun -> Socket"},
		{ NATIVE_FUN, "_internalRead", fun_internalRead, "fun Socket -> Str"},
		{ NATIVE_FUN, "_watcherOpen", fun_watcherOpen, "fun -> Socket"},
		{ NATIVE_FUN, "_watcherUpdate", fun_watcherUpdate, "fun Socket Int Int Int -> Socket"},
		{ NATIVE_FUN, "_socketClose", fun_socketClose, "fun Socket -> Int"},
		{ NATIVE_FUN, "_socketRead", fun_socketRead, "fun Socket -> Str"},
		{ NATIVE_FUN, "_socketWrite", fun_socketWrite, "fun Socket Str Int -> Int"},
		{ NATIVE_FUN, "_tcpOpen", fun_tcpOpen, "fun Str Int -> Socket"},
		{ NATIVE_FUN, "_tcpListen", fun_tcpListen, "fun Str Int -> Socket"},
		{ NATIVE_FUN, "_tcpAccept", fun_tcpAccept, "fun Socket -> [Socket Str Int]"},
		{ NATIVE_FUN, "_tcpNoDelay", fun_tcpNoDelay, "fun Socket Bool -> Socket"},
		{ NATIVE_FUN, "_tcpRead", fun_tcpRead, "fun Socket -> Str"},
		{ NATIVE_FUN, "_tcpWrite", fun_tcpWrite, "fun Socket Str Int -> Int"},
		{ NATIVE_FUN, "_udpCreate", fun_udpCreate, "fun Str Int -> Socket"},
		{ NATIVE_FUN, "_udpSend", fun_udpSend, "fun Socket Str Int Str -> Int"},
		{ NATIVE_FUN, "_udpRead", fun_udpRead, "fun Socket -> [Str Str Int]"},
		{ NATIVE_FUN, "_ethList", fun_ethList, "fun -> list [Int Str Int Str Int Str Str]"},
		{ NATIVE_FUN, "_ethCreate", fun_ethCreate, "fun Int -> Socket"},
		{ NATIVE_FUN, "_ethSend", fun_ethSend, "fun Socket Int Str -> Int"},
		{ NATIVE_FUN, "_ethRead", fun_ethRead, "fun Socket -> Str"},
		{ NATIVE_FUN, "_ipByName", fun_ipByName, "fun Buffer Str -> Int"},
		{ NATIVE_FUN, "_nameByIp", fun_nameByIp, "fun Buffer Str -> Int"},
		{ NATIVE_FUN, "hostName", fun_hostName, "fun -> Str"},
		{ NATIVE_FUN, "ipChecksum", fun_ipChecksum, "fun Str Int -> Int"},
		{ NATIVE_FUN, "ipChecksumFinal", fun_ipChecksumFinal, "fun Int -> Str"},
	};
	NATIVE_DEF(nativeDefs);

#ifdef USE_SOCKET
#ifdef USE_SOCKET_WIN
	{
		WORD wVersionRequested;
		WSADATA wsaData;
		//		long argp = 1;

		wVersionRequested = MAKEWORD(1, 1);
		if (WSAStartup(wVersionRequested, &wsaData)) return -1;
	}
#endif
	lambdaPipe(InternalPipe);
#ifdef USE_CONSOLE_IN_WIN
	lambdaPipe(KeyboardPipe);
#endif
#ifdef USE_CONSOLE_IN_ANSI
	tcgetattr(0, &Term0);
#endif
#endif
	return 0;
}

void sysSocketClose(void)
{
#ifdef USE_CONSOLE_IN_WIN
	KeyboardAlive = 0;
	KeyboardThread = NULL;
	Sleep(200);
	lambdaPipeClose(KeyboardPipe);
#endif
#ifdef USE_CONSOLE_IN_ANSI
	tcsetattr(0, TCSANOW, &Term0);
#endif
#ifdef USE_SOCKET
	lambdaPipeClose(InternalPipe);
#endif
}
