// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"inc/efi.h"
#include"inc/efilib.h"
#include"../../src/minimacy.h"
#include"uefi.h"

// in this implementation there is only one network interface
EFI_SIMPLE_NETWORK_PROTOCOL *EfiNetwork=NULL;
int EthSocketFd=-1;

// Receiving
#define ETH_RX_BUFFER_NB 32
#define ETH_RX_BUFFER_MASK (ETH_RX_BUFFER_NB-1)
#define ETH_RX_BUFFER_SIZE (1024*10)

char EthRxBuffer[ETH_RX_BUFFER_NB][ETH_RX_BUFFER_SIZE];
UINTN EthRxSize[ETH_RX_BUFFER_NB];
int EthRxWrite=1;	// next buffer for receiving
int EthRxRead=0;	// last buffer read

// Sending
#define ETH_TX_BUFFER_NB 32
#define ETH_TX_BUFFER_SIZE   1536

char EthTxBuffer[ETH_TX_BUFFER_NB][ETH_TX_BUFFER_SIZE];
int  EthTxBusy[ETH_TX_BUFFER_NB];

int ethTxReleaseBuffer(char *buf)
{
	for (int i = 0; i < ETH_TX_BUFFER_NB; i++)
		if (buf == EthTxBuffer[i]) {
			EthTxBusy[i] = 0;
			return 0;
		}
//	PRINTF(LOG_SYS, "unknown %llx / %llx (%llx\n)", buf, EthTxBuffer[0], buf-EthTxBuffer[0]);
	return -1;
}

char *ethTxGetBuffer(void)
{
	for (int i = 0; i < ETH_TX_BUFFER_NB; i++)
		if (!EthTxBusy[i]) {
			EthTxBusy[i] = 1;
			return EthTxBuffer[i];
		}
	return NULL;
}

void ethTxRecycle(void)
{
	for (int i=0;i<ETH_TX_BUFFER_NB;i++) {
		UINT32 irq;
		VOID *txBuf = NULL;
		if (EfiNetwork->GetStatus(EfiNetwork, &irq, &txBuf) != EFI_SUCCESS) break;
		if (!txBuf) break;
		if (ethTxReleaseBuffer(txBuf)) break;
	}
}

int ethTxSend(char* buffer,int len)
{
	if (len > ETH_TX_BUFFER_SIZE) return -1;
	ethTxRecycle();
	char *buf = ethTxGetBuffer();
	if (!buf) return 0;

	memcpy(buf,buffer,len);
	EFI_STATUS Status = EfiNetwork->Transmit(EfiNetwork, 0, len, buf, NULL, NULL, NULL);
	if (Status == EFI_NOT_READY) {	// ring is full
		ethTxRecycle();
		Status = EfiNetwork->Transmit(EfiNetwork, 0, len, buf, NULL, NULL, NULL);
	}
	if (Status != EFI_SUCCESS) ethTxReleaseBuffer(buf); // buffer not used
	if(Status==EFI_NOT_READY) return 0;
	if(Status!=EFI_SUCCESS) return -1;
}

int ethRxFetch(char** buffer)
{
	int len;

	if (EthSocketFd==-1) return 0;
	while(EthRxWrite!=EthRxRead) {
		EthRxSize[EthRxWrite]=ETH_RX_BUFFER_SIZE;
		EFI_STATUS Status=EfiNetwork->Receive(EfiNetwork,NULL,&EthRxSize[EthRxWrite],EthRxBuffer[EthRxWrite],NULL,NULL,NULL);
		if (Status!=EFI_SUCCESS) break;
		EthRxWrite=(EthRxWrite+1)&ETH_RX_BUFFER_MASK;
	}
	if (((EthRxWrite-EthRxRead)&ETH_RX_BUFFER_MASK)==1) return 0;
	if (!buffer) return 1;

	EthRxRead=(EthRxRead+1)&ETH_RX_BUFFER_MASK;
	len=EthRxSize[EthRxRead];
	*buffer=EthRxBuffer[EthRxRead];
	return len;
}

// in this first implementation, there is only one ethernet interface, ifindex is always 0
int ethGetSocketFd(LINT ifindex)
{
	if (!EfiNetwork) return -1;
	if (EthSocketFd>=0) return -1;
	EthSocketFd=0;
	return EthSocketFd;
}

void _netConnect(void)
{
	EFI_HANDLE *h = NULL;
	UINTN n = 0;
	if (BS->LocateHandleBuffer(ByProtocol, &gEfiPciIoProtocolGuid, NULL, &n, &h) != EFI_SUCCESS) return;
	for (UINTN i = 0; i < n; i++) {
		EFI_PCI_IO_PROTOCOL *pci;
		UINT8 cls[3];
		if (BS->HandleProtocol(h[i], &gEfiPciIoProtocolGuid, (void**)&pci) != EFI_SUCCESS) continue;
		pci->Pci.Read(pci, EfiPciIoWidthUint8, 0x09, 3, cls);	// prog-if, sub-class, class
		if (cls[2] == 0x02) {	// network controller
			EFI_STATUS st = BS->ConnectController(h[i], NULL, NULL, TRUE);
			PRINTF(LOG_SYS, "> UEFI: connect network controller #%d = %llx\n", (int)i, (unsigned long long)st);
		}
	}
	BS->FreePool(h);
}
void netInit(void)
{
	EFI_HANDLE *Handles = NULL;
	UINTN Count = 0;

	_netConnect();

	EFI_STATUS Status = BS->LocateHandleBuffer(ByProtocol, &gEfiSimpleNetworkProtocolGuid, NULL, &Count, &Handles);
	if (Status != EFI_SUCCESS) {
		PRINTF(LOG_SYS, "> UEFI: no network (SNP not found)\n");
		return;
	}
	if (Count == 0) {
		PRINTF(LOG_SYS, "> UEFI: no network (Count=0)\n");
		return;
	}
	for (UINTN i = 0; i < ETH_TX_BUFFER_NB; i++) EthTxBusy[i] = 0;

	EfiNetwork = NULL;
	for (UINTN i = 0; i < Count; i++) {
		if (!netIsPhysical(Handles[i])) {
			PRINTF(LOG_DEV, "> UEFI: skip snp #%d (no MAC)\n", (int)i);
			continue;
		}
		Status = BS->OpenProtocol(Handles[i], &gEfiSimpleNetworkProtocolGuid,
			(void**)&EfiNetwork, ImageHandle, NULL, EFI_OPEN_PROTOCOL_EXCLUSIVE);
		if (Status == EFI_SUCCESS) {
			PRINTF(LOG_SYS, "> UEFI: network snp #%d with exclusive mode\n", (int)i);
			break;
		}
		PRINTF(LOG_SYS, "> UEFI: network snp #%d without exclusive mode\n", (int)i);
		EfiNetwork = NULL;
	}
	BS->FreePool(Handles);
	if (!EfiNetwork) {
		Status = BS->LocateProtocol(&gEfiSimpleNetworkProtocolGuid, NULL, (void**)&EfiNetwork);
		if (Status != EFI_SUCCESS) PRINTF(LOG_SYS, "> UEFI: no network\n");
		else PRINTF(LOG_SYS, "> UEFI: network shared (not exclusive)\n");
	}
	if (!EfiNetwork) return;
	PRINTF(LOG_SYS, "> UEFI: network mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
		EfiNetwork->Mode->CurrentAddress.Addr[0], EfiNetwork->Mode->CurrentAddress.Addr[1],
		EfiNetwork->Mode->CurrentAddress.Addr[2], EfiNetwork->Mode->CurrentAddress.Addr[3],
		EfiNetwork->Mode->CurrentAddress.Addr[4], EfiNetwork->Mode->CurrentAddress.Addr[5]);

	// Check state after stopping MNP
	if (EfiNetwork->Mode->State == EfiSimpleNetworkStopped) {
		Status = EfiNetwork->Start(EfiNetwork);
		if (Status != EFI_SUCCESS && Status != EFI_ALREADY_STARTED) {
			PRINTF(LOG_SYS, "> UEFI: network failed to restart\n");
			return;
		}
	}
	if (EfiNetwork->Mode->State == EfiSimpleNetworkStarted) {
		Status = EfiNetwork->Initialize(EfiNetwork, 0, 0);
		if (Status != EFI_SUCCESS) {
			PRINTF(LOG_SYS, "> UEFI: network failed to initialize\n");
			return;
		}
	}
	if (EfiNetwork->Mode->State != EfiSimpleNetworkInitialized) return;

	// Explicit filters
	UINT32 Filters = EFI_SIMPLE_NETWORK_RECEIVE_UNICAST | EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST; //| EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST
	Filters&= EfiNetwork->Mode->ReceiveFilterMask;

	Status = EfiNetwork->ReceiveFilters(EfiNetwork, Filters, EfiNetwork->Mode->ReceiveFilterSetting & ~Filters, FALSE, 0, NULL);
	if (Status != EFI_SUCCESS) return;
	PRINTF(LOG_DEV, "> UEFI: network ready\n");
}

//--------------------- Minimacy functions

int fun_ethList(Thread* th)
{
	int i=0;
	if (EfiNetwork)	{
		FUN_PUSH_INT(0);
		FUN_PUSH_STR("eth0",-1);
		FUN_PUSH_INT(2);	//AF_INET
		FUN_PUSH_STR(EfiNetwork->Mode->CurrentAddress.Addr,6);
		FUN_PUSH_INT(EfiNetwork->Mode->MaxPacketSize);
		FUN_PUSH_NIL;	// IP
		FUN_PUSH_NIL;	// Mask
		FUN_MAKE_ARRAY(7, DBG_TUPLE);
		i++;
//		PRINTF(LOG_DEV,"MaxPacketSize: %d\n",EfiNetwork->Mode->MaxPacketSize);
//		PRINTF(LOG_DEV,"MediaHeaderSize: %d\n",EfiNetwork->Mode->MediaHeaderSize);
	}
	FUN_PUSH_NIL;
	while (i--) FUN_MAKE_ARRAY(LIST_LENGTH, DBG_LIST);
	return 0;
}

int _ethCheckReadable(Socket* s)
{
	return ethRxFetch(NULL);
}
int fun_ethCreate(Thread* th)
{
	Socket* s;
	SOCKET sock = INVALID_SOCKET;

	LINT ifindex = STACK_INT(th, 0);
	if (ifindex) FUN_RETURN_NIL;
	sock=ethGetSocketFd(ifindex);	// we should open only one socket per interface
	if (sock==INVALID_SOCKET) FUN_RETURN_NIL;

	s = _socketCreate(sock); if (!s) return EXEC_OM;
	s->checkReadable= _ethCheckReadable;
	s->checkWritable= NULL;
	FUN_RETURN_PNT((LB*)s);
}

int fun_ethSend(Thread* th)
{
	int len,k;
	char* msg;

	LB* content = STACK_PNT(th, 0);
	LINT ifindex= STACK_INT(th, 1);
	Socket* s = (Socket*)STACK_PNT(th, 2);
	if ((!s) || (s->fd == INVALID_SOCKET) || (!content)|| (STR_LENGTH(content)<6)) FUN_RETURN_NIL;

	len=STR_LENGTH(content);
	msg=STR_START(content);
	k=ethTxSend(msg,len);
	if (k < 0) FUN_RETURN_NIL;
	FUN_RETURN_INT(k);
}

int fun_ethRead(Thread* th)
{
	char* buffer;
	ssize_t len=-1;
	Socket* s = (Socket*)STACK_PNT(th, 0);
	if (s && (s->fd != INVALID_SOCKET)) len=ethRxFetch(&buffer);
	s->readable=0;
	if (len < 0) FUN_RETURN_NIL;
	FUN_RETURN_STR(buffer, len);
}

