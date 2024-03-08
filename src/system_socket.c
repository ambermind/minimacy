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

void _socketClose(Socket* s)
{
	if (s->fd == INVALID_SOCKET) return;
//	PRINTF(LOG_DEV,"close socket %lld\n", s->fd);
	closesocket(s->fd);
	s->fd = INVALID_SOCKET;
}
int _socketForget(LB* p)
{
	_socketClose((Socket*)p);
	return 0;
}
Socket* _socketCreate(Thread* th, SOCKET sock)
{
	Socket* s = (Socket*)memoryAllocExt(th, sizeof(Socket), DBG_SOCKET, _socketForget, NULL);
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
	Socket* s = _socketCreate(th,INVALID_SOCKET); if (!s) return EXEC_OM;
	FUN_RETURN_PNT((LB*)s);
}

int fun_sockSetSelectRead(Thread* th)
{
	LB* state = STACKPULLPNT(th);
	Socket* s = (Socket*)STACKPNT(th, 0);
	if (s) s->selectRead = (state == MM._true) ? 1 : 0;
	return 0;
}
int fun_sockSetSelectWrite(Thread* th)
{
	LB* state = STACKPULLPNT(th);
	Socket* s = (Socket*)STACKPNT(th, 0);
	if (s) s->selectWrite= (state == MM._true) ? 1 : 0;
	return 0;
}

int fun_sockSelectRead(Thread* th)
{
	Socket* s = (Socket*)STACKPNT(th, 0);
	if (!s) FUN_RETURN_NIL;
	FUN_RETURN_BOOL(s->selectRead);
}
int fun_sockSelectWrite(Thread* th)
{
	Socket* s = (Socket*)STACKPNT(th, 0);
	if (!s) FUN_RETURN_NIL;
	FUN_RETURN_BOOL(s->selectWrite);
}
int fun_sockReadable(Thread* th)
{
	Socket* s = (Socket*) STACKPNT(th, 0);
	if (!s) return 0;
	STACKSETBOOL(th, 0, s->readable);
	return 0;
}
int fun_sockWritable(Thread* th)
{
	Socket* s = (Socket*)STACKPNT(th, 0);
	if (!s) return 0;
	STACKSETBOOL(th, 0, s->writable);
	return 0;
}
int fun_sockSetReadable(Thread* th)
{
	LB* state = STACKPULLPNT(th);
	Socket* s = (Socket*)STACKPNT(th, 0);
	if (s) s->readable = (state == MM._true) ? 1 : 0;
	return 0;
}
int fun_sockSetWritable(Thread* th)
{
	LB* state = STACKPULLPNT(th);
	Socket* s = (Socket*)STACKPNT(th, 0);
	if (s) s->writable = (state == MM._true) ? 1 : 0;
	return 0;
}

int fun_sockClose(Thread* th)
{
	Socket* s = (Socket*)STACKPNT(th, 0);
	if (!s) return 0;
	_socketClose(s);
	FUN_RETURN_NIL;
}


#ifdef USE_SOCKET
#define SOCKET_BUFFER_LEN (1024*32)
char SocketBuffer[SOCKET_BUFFER_LEN];

#define INTERNAL_READ 0
#define INTERNAL_WRITE 1
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
#ifdef USE_SOCKET_UNIX
	while (k < len) k += write(InternalPipe[INTERNAL_WRITE], src + k, len - k);
#endif
#ifdef USE_SOCKET_WIN
	while (k < len) k += send(InternalPipe[INTERNAL_WRITE], src + k, len - k, 0);
#endif
}

int fun_hostName(Thread* th)
{
	char buf[256];
	if (gethostname(buf, 256)) FUN_RETURN_STR("localhost", -1);
	FUN_RETURN_STR(buf, -1);
}

MTHREAD_START _ipByName(Thread* th)
{
	struct hostent* hp;
	LINT n = 0;

	LB* name = STACKPNT(th, 0);
	Buffer* out = (Buffer*)STACKPNT(th, 1);
	if ((!name) || (!out)) return workerDoneNil(th);
	hp = gethostbyname(STRSTART(name));
	if (hp)
	{
		u_long** p = (u_long**)hp->h_addr_list;
		while (p[n])
		{
			char* ip;
			struct in_addr Addr;
			Addr.s_addr = *(p[n]);	// warning on Macos should cast with (in_addr_t) not known on windows
			ip = (char*)inet_ntoa(Addr);
			if (n) bufferAddChar(th, out, 0);
			bufferAddBin(th, out, ip, strlen(ip));
			n++;
		}
	}
	return workerDoneInt(th,n);
}
int fun_ipByName(Thread* th) { return workerStart(th, 2, _ipByName); }

MTHREAD_START _nameByIp(Thread* th)
{
	long addr;
	struct hostent* hp;

	LB* ip = STACKPNT(th, 0);
	Buffer* out = (Buffer*)STACKPNT(th, 1);
	if ((!ip) || (!out)) return workerDoneNil(th);
	addr = inet_addr(STRSTART(ip));
	if ((hp = gethostbyaddr((char*)&addr, sizeof(addr), AF_INET)))
	{
		bufferAddBin(th, out, hp->h_name, strlen(hp->h_name));
		return workerDoneInt(th,1);
	}
	return workerDoneNil(th);
}
int fun_nameByIp(Thread* th) { return workerStart(th, 2, _nameByIp); }


int _sockWrite(Thread* th)
{
	LINT sent,len=0;

	LINT start = STACKINT(th, 0);
	LB* src = STACKPNT(th, 1);
	Socket* s = (Socket*)STACKPNT(th, 2);
	if ((!s)|| (s->fd == INVALID_SOCKET)) FUN_RETURN_NIL;
	FUN_SUBSTR(src, start, len, 1, STRLEN(src));

	if (len==0) FUN_RETURN_INT(start);

	sent = send(s->fd, STRSTART(src) + start, (int)len, 0);
	if (SOCKETWOULDBLOCK) sent=0;
	if (sent >= 0) FUN_RETURN_INT(start + sent);
	FUN_RETURN_NIL;
}

// returns :
// - non empty string : received data
// - empty string : socket ok but no received data
// - nil : closed or failed socket
int _sockRead(Thread* th)
{
	ssize_t len = -1;
	Socket* s = (Socket*)STACKPNT(th, 0);
	if (s && (s->fd != INVALID_SOCKET))
	{
#ifdef USE_SOCKET_UNIX
		len = read(s->fd, SocketBuffer, SOCKET_BUFFER_LEN);
#endif
#ifdef USE_SOCKET_WIN
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
		s->readable = 0;
	}
	if (len < 0) FUN_RETURN_NIL;
	FUN_RETURN_STR(SocketBuffer, len);
}

#ifdef USE_CONSOLE_IN_ANSI
int fun_keyboardOpen(Thread* th)
{
	Socket* s = _socketCreate(th,0); if (!s) return EXEC_OM;
	FUN_RETURN_PNT((LB*)s);
}
#endif

#ifdef USE_CONSOLE_IN_WIN
SOCKET KeyboardPipe[2];
MTHREAD KeyboardThread = NULL;
int KeyboardAlive = 1;

MTHREAD_START keyboardThread(void* param)
{
	int k = 0;
//	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
//	DWORD mode = 0;
//	GetConsoleMode(hStdin, &mode);
//	SetConsoleMode(hStdin, mode | (ENABLE_ECHO_INPUT));
//	SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));

//	SetConsoleCtrlHandler(NULL, TRUE);	// this disables Ctr-C default behavior and other like this
	while (KeyboardAlive && (k >= 0))
	{
		char buf[1];
		LINT val;
		while (!kbhit()) Sleep(100);
		val = getch();
		if (val == 13) val = 10;
		buf[0] = (char)val;
		k = -1;
		if (buf[0] == 0x04) closesocket(KeyboardPipe[INTERNAL_WRITE]);	//Ctr-D, aka EOT ascii code
		else {
			putchar((char)val);	// echo
			k = send(KeyboardPipe[INTERNAL_WRITE], buf, 1, 0);
		}
		//		PRINTF(LOG_DEV,"keyboardwrite %d: %d\n", k, val);
	}
//	PRINTF(LOG_DEV,"keyboard close\n");
	return MTHREAD_RETURN;
}

int fun_keyboardOpen(Thread* th)
{
	Socket* s = _socketCreate(th,KeyboardPipe[INTERNAL_READ]); if (!s) return EXEC_OM;
	if (!KeyboardThread)
	{
		long argp = 1;
		KeyboardAlive = 1;
		ioctlsocket(KeyboardPipe[INTERNAL_READ], FIONBIO, &argp);

		KeyboardThread = hwThreadCreate(keyboardThread, NULL);
	}
	FUN_RETURN_PNT((LB*)s);
}
#endif

int fun_keyboardRead(Thread* th) {
	return _sockRead(th);
}

int fun_internalOpen(Thread* th)
{
	Socket* s = _socketCreate(th,InternalPipe[INTERNAL_READ]); if (!s) return EXEC_OM;
	FUN_RETURN_PNT((LB*)s);
}
int fun_internalRead(Thread* th) {
	return _sockRead(th);
}

int fun_udpRead(Thread* th)
{
	char ip_str[32];
	int ip, port;
	ssize_t len = -1;
	Socket* s = (Socket*)STACKPNT(th, 0);
	if (s && (s->fd != INVALID_SOCKET))
	{
		SOCKADDR_IN add;
		socklen_t l = sizeof(add);
		len = recvfrom(s->fd, SocketBuffer, SOCKET_BUFFER_LEN, 0, (struct sockaddr*)&add, &l);
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

	STACKMAKETABLE_ERR(th, 3, DBG_TUPLE, EXEC_OM);
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

	LINT port = STACKINT(th, 0);
	LB* ip_str = STACKPNT(th, 1);

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) FUN_RETURN_NIL;
	ioctlsocket(sock, FIONBIO, &argp);

	if (port>0)
	{
		memset(&ina, 0, sizeof(ina));
		ina.sin_family = PF_INET;
		ina.sin_port = htons((unsigned short)port);
		ina.sin_addr.s_addr = ip_str ? inet_addr(STRSTART(ip_str)) : INADDR_ANY;
		if ((k = bind(sock, (struct sockaddr*)&ina, sizeof(ina))) != 0)
		{
			PRINTF(LOG_SYS, "Sockets: Udp port %s:%d bind error\n", ip_str ? STRSTART(ip_str) : "*", port);
			closesocket(sock);
			FUN_RETURN_NIL;
		}
	}
	lin.l_onoff = 0;
	lin.l_linger = 0;
	setsockopt(sock, SOL_SOCKET, SO_LINGER, (const char*)&lin, sizeof(int));
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(opt));

	s = _socketCreate(th,sock); if (!s) return EXEC_OM;
	FUN_RETURN_PNT((LB*)s);
}

// [content port ip localport localaddr udp]
int fun_udpSend(Thread* th)
{
	int ip;
	LINT sent,len=0;
	SOCKADDR_IN ina;

	LB* src = STACKPNT(th, 0);
	LINT distantPort = STACKINT(th, 1);
	LB* distantIp = STACKPNT(th, 2);
	LINT localPort = STACKINT(th, 3);
	LB* localIp = STACKPNT(th, 4);
	Socket* s = (Socket*)STACKPNT(th, 5);
	if ((!s) || (s->fd == INVALID_SOCKET) || (!src) || (!distantPort) || (!distantIp)) FUN_RETURN_NIL;

	len=STRLEN(src);
	if (len==0) FUN_RETURN_INT(0);

	ip = inet_addr(STRSTART(distantIp));
	ina.sin_family = PF_INET;
	ina.sin_port = htons((unsigned short)distantPort);
	ina.sin_addr.s_addr = ip;
	sent = sendto(s->fd, STRSTART(src), (int)len, 0, (struct sockaddr*)&ina, sizeof(ina));
	if (SOCKETWOULDBLOCK) sent=0;
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

	LINT port = STACKINT(th, 0);
	LB* ip_str = STACKPNT(th, 1);
	if ((!port) || (!ip_str)) FUN_RETURN_NIL;

	ip = inet_addr(STRSTART(ip_str));

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) FUN_RETURN_NIL;
	ioctlsocket(sock, FIONBIO, &argp);
	//	setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));

	ina.sin_family = PF_INET;
	ina.sin_port = htons((unsigned short)port);
	ina.sin_addr.s_addr = ip;
	if ((connect(sock, (struct sockaddr*)&ina, sizeof(ina)) != 0)
		&& (!SOCKETWOULDBLOCK)) {
			closesocket(sock);
			FUN_RETURN_NIL;
		}
	s = _socketCreate(th,sock); if (!s) return EXEC_OM;
	//	PRINTF(LOG_SYS, "create socket %d to %s %d\n", s->fd, STRSTART(ip_str),port);
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

	LINT port = STACKINT(th, 0);
	LB* ip_str = STACKPNT(th, 1);
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
	ina.sin_addr.s_addr = ip_str ? inet_addr(STRSTART(ip_str)) : INADDR_ANY;
	if ((k = bind(sock, (struct sockaddr*)&ina, sizeof(ina))) != 0)
	{
		PRINTF(LOG_SYS, "Sockets: Tcp port %s:%d bind error\n", ip_str ? STRSTART(ip_str) : "*", port);
		closesocket(sock);
		FUN_RETURN_NIL;
	}
	if (listen(sock, 256) != 0)
	{
		PRINTF(LOG_SYS, "Sockets: Tcp port %d listen error\n", port);
		closesocket(sock);
		FUN_RETURN_NIL;
	}
	s = _socketCreate(th,sock); if (!s) return EXEC_OM;
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

	Socket* srv = (Socket*)STACKPNT(th, 0);
	if (!srv) return 0;
	if (srv->fd == INVALID_SOCKET) FUN_RETURN_NIL;

	srv->readable = 0;

	remoteSize = sizeof(remote);
	sock = accept(srv->fd, (struct sockaddr*)&remote, &remoteSize);
	if (sock == INVALID_SOCKET) FUN_RETURN_NIL;
	ioctlsocket(sock, FIONBIO, &argp);

	s = _socketCreate(th,sock); if (!s) return EXEC_OM;

	ip = remote.sin_addr.s_addr;
	port = ntohs(remote.sin_port);
	snprintf(ip_str, 32, "%d.%d.%d.%d", ip & 255, (ip >> 8) & 255, (ip >> 16) & 255, (ip >> 24) & 255);

	FUN_PUSH_PNT((LB*)s);
	FUN_PUSH_STR(ip_str, -1);
	FUN_PUSH_INT(port);

	STACKMAKETABLE_ERR(th, 3, DBG_TUPLE, EXEC_OM);
	return 0;
}
int fun_tcpNoDelay(Thread* th)
{
	int nodelay = (STACKPULLPNT(th) == MM._true) ? 1 : 0;
	Socket* s = (Socket*)STACKPNT(th, 0);
	if (s && (s->fd != INVALID_SOCKET))
		setsockopt(s->fd, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(int));
	return 0;
}
int fun_tcpRead(Thread* th) {
	return _sockRead(th);
}
int fun_tcpWrite(Thread* th) {
	return _sockWrite(th);
}
#else
int  fun_tcpOpen(Thread* th) FUN_RETURN_NIL
int  fun_tcpListen(Thread* th) FUN_RETURN_NIL
int  fun_tcpAccept(Thread* th) FUN_RETURN_NIL
int  fun_tcpRead(Thread* th) FUN_RETURN_NIL
int  fun_tcpWrite(Thread* th) FUN_RETURN_NIL
int  fun_tcpNoDelay(Thread* th) FUN_RETURN_NIL
int  fun_udpCreate(Thread* th) FUN_RETURN_NIL
int  fun_udpSend(Thread* th) FUN_RETURN_NIL
int  fun_udpRead(Thread* th) FUN_RETURN_NIL
int  fun_ipByName(Thread* th) FUN_RETURN_NIL
int  fun_nameByIp(Thread* th) FUN_RETURN_NIL
int  fun_hostName(Thread* th) FUN_RETURN_NIL
int  fun_internalOpen(Thread* th) FUN_RETURN_NIL
int  fun_internalRead(Thread* th) FUN_RETURN_NIL
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

	ethSocket=socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
	if(ethSocket==INVALID_SOCKET) {
		PRINTF(LOG_SYS, "Could not create AF_PACKET socket. User has insufficient permission\n");
		FUN_RETURN_NIL;
	}
	address = addresses;
	while(address)
	{
		struct ifreq if_idx;
		struct ifreq if_mac;
		int family = address->ifa_addr->sa_family;
		if (family == AF_INET)// || family == AF_INET6)
		{
			char ap[100];
			char* ifName=address->ifa_name;
			const int family_size= (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
			
			memset(&if_idx, 0, sizeof(struct ifreq));
			strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
			FUN_PUSH_INT((ioctl(ethSocket, SIOCGIFINDEX, &if_idx) < 0)?-1:if_idx.ifr_ifindex);

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
			FUN_PUSH_INT(0);	// MAX_PACKET_SIZE
			getnameinfo(address->ifa_addr,family_size, ap, sizeof(ap), 0, 0, NI_NUMERICHOST);
			FUN_PUSH_STR(ap,-1);

			STACKMAKETABLE_ERR(th, 6, DBG_TUPLE, EXEC_OM);
			count++;
		}
		address = address->ifa_next;
	}
	closesocket(ethSocket);
	FUN_PUSH_NIL;
	while(count--) STACKMAKETABLE_ERR(th, LIST_LENGTH, DBG_LIST,EXEC_OM);

	freeifaddrs(addresses);
	return 0;
}
int fun_ethCreate(Thread* th)
{
	Socket* s;
	struct sockaddr_ll addr = {0};
	SOCKET sock = INVALID_SOCKET;
	long argp = 1;

	LINT ifindex = STACKINT(th, 0);
	sock=socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));	//only for root on Linux
	if (sock == INVALID_SOCKET) FUN_RETURN_NIL;
	ioctlsocket(sock, FIONBIO, &argp);
	addr.sll_family = AF_PACKET;
	addr.sll_ifindex = ifindex;
	addr.sll_protocol = htons(ETH_P_ALL);
	if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		closesocket(sock);
		FUN_RETURN_NIL;
	}
	s = _socketCreate(th,sock); if (!s) return EXEC_OM;
//	printf("socket ok %d on %d\n",sock,ifindex);
	FUN_RETURN_PNT((LB*)s);
}

// [msg socket]
int fun_ethSend(Thread* th)
{
	struct sockaddr_ll socket_address;
	int sent,len;
	char* msg;

	LB* content = STACKPNT(th, 0);
	LINT ifindex= STACKINT(th, 1);
	Socket* s = (Socket*)STACKPNT(th, 2);
	if ((!s) || (s->fd == INVALID_SOCKET) || (!content)|| (STRLEN(content)<6)) FUN_RETURN_NIL;

	len=STRLEN(content);
	msg=STRSTART(content);
	socket_address.sll_ifindex = ifindex;
	socket_address.sll_halen = ETH_ALEN;
	memcpy(socket_address.sll_addr,msg,6);
	sent=sendto(s->fd, msg, len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
//	printf("write %d on %d result=%d\n",len,ifindex, k);
	if (SOCKETWOULDBLOCK) sent=0;
	if (sent >= 0) FUN_RETURN_INT(sent);
	FUN_RETURN_NIL;
}
int fun_ethRead(Thread* th) {
	return _sockRead(th);
}
#endif
#ifdef USE_SOCKET_UEFI
int fun_ethList(Thread* th);
int fun_ethCreate(Thread* th);
int fun_ethSend(Thread* th);
int fun_ethRead(Thread* th);
#endif

#ifdef USE_ETH_RPI4
int fun_ethList(Thread* th);
int fun_ethCreate(Thread* th) FUN_RETURN_NIL
int fun_ethSend(Thread* th) FUN_RETURN_NIL
int fun_ethRead(Thread* th) FUN_RETURN_NIL
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

	int timeoutIsNil = STACKISNIL(th,0);
	LINT timeout = STACKPULLINT(th);
	LB* sockets = STACKPNT(th, 0);

	FD_ZERO(r);
	FD_ZERO(w);

	p = sockets;
	while (p)
	{
		Socket* s = (Socket*)TABPNT(p, LIST_VAL);
		if (s) {
//			printf("select prepare %llx -> %d\n",(LINT)s,s->readable);
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
		p = (TABPNT(p, LIST_NXT));
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
			Socket* s = (Socket*)TABPNT(p, LIST_VAL);
			if (s && (s->fd != INVALID_SOCKET))
			{
				if (FD_ISSET(s->fd, w)) s->writable = 1;
				if (FD_ISSET(s->fd, r)) s->readable = 1;
			}
			p = TABPNT(p, LIST_NXT);
		}
	}
	FUN_RETURN_INT(k);
}
#else
int fun_select(Thread* th)
{
	LINT k=0;
	int timeoutIsNil = STACKISNIL(th,0);
	LINT timeout = STACKPULLINT(th);
	LB* sockets = STACKPNT(th, 0);

	if (!timeoutIsNil) timeout=hwTimeMs()+timeout;
	while(k==0)
	{
		LB* p = sockets;
		while(p)
		{
			Socket* s = (Socket*)TABPNT(p, LIST_VAL);
			if (s && s->fd != INVALID_SOCKET)
			{
				//if (s->selectRead)  
				s->readable = (s->checkReadable && s->checkReadable(s->fd))?1:0;
				if (s->selectWrite) s->writable = (s->checkWritable && s->checkWritable(s->fd))?1:0;
				if ((s->selectRead&&s->readable)||(s->selectWrite&&s->writable)) k++;
			}
			p = TABPNT(p, LIST_NXT);
		}
		if ((!timeoutIsNil)&&(timeout-hwTimeMs()<0)) break;
	}
	FUN_RETURN_INT(k);
}
#endif

#ifdef USE_CONSOLE_IN_UART
int fun_keyboardRead(Thread* th) {
    ssize_t len=-1;
	char buffer[2];
	Socket* s = (Socket*)STACKPNT(th, 0);
	if (s && (s->fd != INVALID_SOCKET))
	{
		int c=uartGet();
		if (c>=0) {
			buffer[0]=(char)c;
			buffer[1]=0;
			len=1;
		}
	}
	if (len < 0) FUN_RETURN_NIL;
	FUN_RETURN_STR(buffer, len);
}

int _keyboardCheckReadable(SOCKET s)
{
	return uartReadable();
}
int _keyboardCheckWritable(SOCKET s)
{
	return uartWritable();
}

int fun_keyboardOpen(Thread* th)
{
	Socket* s = _socketCreate(th,0); if (!s) return EXEC_OM;
	s->checkReadable= _keyboardCheckReadable;
	s->checkWritable= _keyboardCheckWritable;
	FUN_RETURN_PNT((LB*)s);
}
#endif

#ifdef USE_CONSOLE_IN_STUB
int fun_keyboardRead(Thread* th) FUN_RETURN_NIL
int fun_keyboardOpen(Thread* th) FUN_RETURN_NIL
#endif

int fun_ipChecksum(Thread* th)
{
	LINT i, len;
	unsigned short* q;
	LINT val = STACKINT(th, 0);
	LB* p = STACKPNT(th, 1);
	if (!p) FUN_RETURN_INT(val);
	q = (unsigned short*)STRSTART(p);
	len = STRLEN(p);
	for (i = 0; i < len; i += 2) {	// for odd-length strings we know that there is a final null char
		val += LSBW(*q);
		q++;
	}
	while (val > 0xffff) val = (val >> 16) + (val & 0xffff);
	FUN_RETURN_INT(val);
}
int fun_ipChecksumFinal(Thread* th)
{
	LINT val = STACKINT(th,0);
	unsigned short checkSum = (unsigned short)~val;
	checkSum = LSBW(checkSum);
	FUN_RETURN_STR((char*)&checkSum, 2);
}


int sysSocketInit(Thread* th, Pkg* system)
{
	Def* socket = pkgAddType(th, system, "Socket");
	Type* list_Sock = typeAlloc(th,TYPECODE_LIST, NULL, 1, socket->type);
	Type* list_Eth = typeAlloc(th,TYPECODE_LIST, NULL, 1, typeAlloc(th,TYPECODE_TUPLE, NULL, 6, MM.I, MM.S, MM.I, MM.S, MM.I, MM.S));

	pkgAddFun(th, system, "socketEmpty", fun_socketEmpty, typeAlloc(th, TYPECODE_FUN, NULL, 1, socket->type));
	
	pkgAddFun(th, system, "_sockSetSelectRead", fun_sockSetSelectRead, typeAlloc(th,TYPECODE_FUN, NULL, 3, socket->type, MM.Boolean, socket->type));
	pkgAddFun(th, system, "_sockSelectRead", fun_sockSelectRead, typeAlloc(th,TYPECODE_FUN, NULL, 2, socket->type, MM.Boolean));
	pkgAddFun(th, system, "sockSetReadable", fun_sockSetReadable, typeAlloc(th,TYPECODE_FUN, NULL, 3, socket->type, MM.Boolean, socket->type));
	pkgAddFun(th, system, "_sockReadable", fun_sockReadable, typeAlloc(th,TYPECODE_FUN, NULL, 2, socket->type, MM.Boolean));

	pkgAddFun(th, system, "_sockSetSelectWrite", fun_sockSetSelectWrite, typeAlloc(th,TYPECODE_FUN, NULL, 3, socket->type, MM.Boolean, socket->type));
	pkgAddFun(th, system, "_sockSelectWrite", fun_sockSelectWrite, typeAlloc(th,TYPECODE_FUN, NULL, 2, socket->type, MM.Boolean));
	pkgAddFun(th, system, "sockSetWritable", fun_sockSetWritable, typeAlloc(th,TYPECODE_FUN, NULL, 3, socket->type, MM.Boolean, socket->type));
	pkgAddFun(th, system, "_sockWritable", fun_sockWritable, typeAlloc(th,TYPECODE_FUN, NULL, 2, socket->type, MM.Boolean));

	pkgAddFun(th, system, "_select", fun_select, typeAlloc(th, TYPECODE_FUN, NULL, 3, list_Sock, MM.I, MM.I));

	pkgAddFun(th, system, "_keyboardOpen", fun_keyboardOpen, typeAlloc(th, TYPECODE_FUN, NULL, 1, socket->type));
	pkgAddFun(th, system, "_keyboardRead", fun_keyboardRead, typeAlloc(th, TYPECODE_FUN, NULL, 2, socket->type, MM.S));
	
	pkgAddFun(th, system, "_internalOpen", fun_internalOpen, typeAlloc(th, TYPECODE_FUN, NULL, 1, socket->type));
	pkgAddFun(th, system, "_internalRead", fun_internalRead, typeAlloc(th, TYPECODE_FUN, NULL, 2, socket->type, MM.S));

	pkgAddFun(th, system, "_sockClose", fun_sockClose, typeAlloc(th,TYPECODE_FUN, NULL, 2, socket->type, MM.I));

	pkgAddFun(th, system, "_tcpOpen", fun_tcpOpen, typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.S, MM.I, socket->type));
	pkgAddFun(th, system, "_tcpListen", fun_tcpListen, typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.S, MM.I, socket->type));
	pkgAddFun(th, system, "_tcpAccept", fun_tcpAccept, typeAlloc(th, TYPECODE_FUN, NULL, 2, socket->type, typeAlloc(th, TYPECODE_TUPLE, NULL, 3, socket->type, MM.S, MM.I)));
	pkgAddFun(th, system, "_tcpNoDelay", fun_tcpNoDelay, typeAlloc(th, TYPECODE_FUN, NULL, 3, socket->type, MM.Boolean, socket->type));
	pkgAddFun(th, system, "_tcpRead", fun_tcpRead, typeAlloc(th, TYPECODE_FUN, NULL, 2, socket->type, MM.S));
	pkgAddFun(th, system, "_tcpWrite", fun_tcpWrite, typeAlloc(th, TYPECODE_FUN, NULL, 4, socket->type, MM.S, MM.I, MM.I));
	
	pkgAddFun(th, system, "_udpCreate", fun_udpCreate, typeAlloc(th,TYPECODE_FUN, NULL, 3, MM.S, MM.I, socket->type));
	pkgAddFun(th, system, "_udpSend", fun_udpSend, typeAlloc(th,TYPECODE_FUN, NULL, 7, socket->type, MM.S, MM.I, MM.S, MM.I, MM.S, MM.I));
	pkgAddFun(th, system, "_udpRead", fun_udpRead, typeAlloc(th, TYPECODE_FUN, NULL, 2, socket->type, typeAlloc(th, TYPECODE_TUPLE, NULL, 3, MM.S, MM.S, MM.I)));
	
	pkgAddFun(th, system, "ethList", fun_ethList, typeAlloc(th,TYPECODE_FUN, NULL, 1, list_Eth));
	pkgAddFun(th, system, "_ethCreate", fun_ethCreate, typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.I, socket->type));
	pkgAddFun(th, system, "_ethSend", fun_ethSend, typeAlloc(th,TYPECODE_FUN, NULL, 4, socket->type, MM.I, MM.S, MM.I));
	pkgAddFun(th, system, "_ethRead", fun_ethRead, typeAlloc(th, TYPECODE_FUN, NULL, 2, socket->type, MM.S));

	pkgAddFun(th, system, "_ipByName", fun_ipByName, typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.Buffer, MM.S, MM.I));
	pkgAddFun(th, system, "_nameByIp", fun_nameByIp, typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.Buffer, MM.S, MM.I));
	pkgAddFun(th, system, "hostName", fun_hostName, typeAlloc(th, TYPECODE_FUN, NULL, 1, MM.S));

	pkgAddFun(th, system, "ipChecksum", fun_ipChecksum, typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.S, MM.I, MM.I));
	pkgAddFun(th, system, "ipChecksumFinal", fun_ipChecksumFinal, typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.I, MM.S));

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
#ifdef USE_SOCKET
	lambdaPipeClose(InternalPipe);
#endif
}
