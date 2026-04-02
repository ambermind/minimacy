// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#include "ble_c.h"
#include "../src/minimacy.h"
#ifdef ENABLE_BLE

//"0000"+uuid+"-0000-1000-8000-00805f9b34fb"

int BLE_DEBUG = 0;

int BleInit = 0;
int BleEventID = 0;

char BleServiceUUID[128];
char BleTxUUID[128];
char BleRxUUID[128];

volatile int BleDevReady = 0;
volatile int BleConnectAndRun = 0;
uint64_t BleDevAddr = 0;
char BleDevName[128];

void on_device(
	uint64_t addr,
	const char* name,
	int rssi,
	int match,
	void* ctx)
{
	if (BleDevReady) return;
	if (match == 1) {
		if (BleDevAddr == addr) {
			if (BLE_DEBUG) {
				printf("BLE correspondant:\n");
				printf("  Addr : %llx\n", addr);
				printf("  Nom  : %s\n", BleDevName[0] ? BleDevName : "(sans nom)");
				printf("  RSSI : %d dBm\n", rssi);
				printf("  Match: %d\n\n", match);
			}
			BleDevReady = 1;
			return;
		}
		BleDevAddr = addr;
		BleDevName[0] = 0;
		return;
	}
	if (BleDevAddr == addr) {
		strcpy(BleDevName, name);
	}
}

void on_notify(const uint8_t* data, uint32_t len, void* ctx)
{
	if (data == NULL) {
		if (len) {  // when data is NULL, len is used as ble state (1=connected, 0=disconnected)
			if (BLE_DEBUG) printf("--connected\n");
		}
		else {
			if (BLE_DEBUG) printf("--disconnected\n");
			BleDevAddr = 0;
			BleDevName[0] = 0;
			BleDevReady = 0;
		}
		eventBinary(BleEventID, "", 0);
		return;
	}
	eventBinary(BleEventID, data, len);
	//	printf("> Notified: '");
	//	while (len--) printf("%c", *(data++));
	//	printf("'\n");
}

WORKER_START bleThread(void* param)
{
	while (BleConnectAndRun)
	{
		if (BLE_DEBUG) printf("> Scan BLE...\n");
		BleDevReady = 0;
		ble_scan_start_uuid(
			BleServiceUUID,
			on_device,
			NULL
		);

		while (BleConnectAndRun && !BleDevReady)
		{
			if (BLE_DEBUG) printf(".");
			Sleep(100);
		}
		ble_scan_stop();
		if (!BleConnectAndRun) break;

		if (BLE_DEBUG) printf("> BLE connect\n");
		ble_connect(
			BleDevAddr,
			BleServiceUUID,
			BleTxUUID,
			BleRxUUID,
			on_notify,
			NULL
		);
		while (BleConnectAndRun && BleDevReady) {
			if (BLE_DEBUG) printf(".");
			Sleep(100);
		}
		ble_disconnect();
	}
	if (BLE_DEBUG) printf("> BLE done\n");
	return WORKER_RETURN;
}

void filterUUID(char* uuid, char* out)
{
	if (strlen(uuid) == 4) sprintf(out, "{0000%s-0000-1000-8000-00805f9b34fb}", uuid);
	else if (strlen(uuid) == 36) sprintf(out, "{%s}", uuid);
	else out[0] = 0;
}
int fun_bleSerialStart(Thread* th)
{
	LB* tx = STACK_PNT(th, 0);
	LB* rx = STACK_PNT(th, 1);
	LB* service = STACK_PNT(th, 2);
	if (!rx || !tx || !service) FUN_RETURN_NIL;

	filterUUID(STR_START(tx), BleTxUUID);
	filterUUID(STR_START(rx), BleRxUUID);
	filterUUID(STR_START(service), BleServiceUUID);

	if (!BleInit) ble_init();
	if (!is_bluetooth_enabled()) FUN_RETURN_NIL;

	if (BleConnectAndRun) return BleEventID;
	if (BLE_DEBUG) PRINTF(LOG_DEV, "> BLE: start");
	BleConnectAndRun = 1;
	BleDevReady = 0;
	if (!BleEventID) BleEventID = eventGetNextID();
	hwThreadCreate(bleThread, NULL);
	FUN_RETURN_INT(BleEventID);
}
int fun_bleSerialStop(Thread* th)
{
	BleConnectAndRun = 0;
	FUN_RETURN_BOOL(1);
}
int fun_bleSerialIsConnected(Thread* th)
{
	FUN_RETURN_BOOL(BleDevReady);
}
int fun_bleSerialName(Thread* th)
{
	if (!BleDevReady) FUN_RETURN_NIL;
	FUN_RETURN_STR(BleDevName, -1);
}
int fun_bleSerialWrite(Thread* th)
{
	LINT len = 0;
	LINT start = STACK_INT(th, 0);
	LB* src = STACK_PNT(th, 1);
	FUN_SUBSTR(src, start, len, 1, STR_LENGTH(src));
	if (len == 0) FUN_RETURN_INT(start);
	ble_write(STR_START(src) + start, (uint32_t)len);
	FUN_RETURN_INT(start + len);
}
int systemBleInit(Pkg* system) {
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "bleSerialStart", fun_bleSerialStart, "fun Str Str Str-> Int" },
		{ NATIVE_FUN, "bleSerialStop", fun_bleSerialStop, "fun -> Bool" },
		{ NATIVE_FUN, "bleSerialIsConnected", fun_bleSerialIsConnected, "fun -> Bool" },
		{ NATIVE_FUN, "bleSerialName", fun_bleSerialName, "fun -> Str" },
		{ NATIVE_FUN, "bleSerialWrite", fun_bleSerialWrite, "fun Str Int -> Int" },
	};
	NATIVE_DEF(nativeDefs);
	return 0;
}
#endif