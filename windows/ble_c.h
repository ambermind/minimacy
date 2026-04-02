// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#pragma once
#include <stdint.h>
//#define ENABLE_BLE	// uncomment also #define WITH_SERIAL_BLE in src/minimacy.h

#ifdef __cplusplus
extern "C" {
#endif

	typedef void* ble_device_t;

	typedef void (*ble_notify_cb)(
		const uint8_t* data,
		uint32_t length,
		void* user_ctx
		);

	/* Init / shutdown */
	int ble_init(void);
	void ble_shutdown(void);


	/* Connexion */
	void ble_connect(
		uint64_t address,
		const char* service_uuid,
		const char* tx_uuid,
		const char* rx_uuid,
		ble_notify_cb cb,
		void* user_ctx
	);
	//ble_device_t ble_connect(const char* device_id);
	void ble_disconnect(void);

	/* GATT */
	int ble_write(
		const uint8_t* data,
		uint32_t len
	);

	/* Scan BLE */
	typedef void (*ble_scan_cb)(
		uint64_t address,
		const char* name,
		int rssi,
		int match,
		void* user_ctx
		);

	/* Scan BLE filtr� par UUID de service */
	int ble_scan_start_uuid(
		const char* service_uuid,
		ble_scan_cb cb,
		void* user_ctx
	);
	void ble_scan_stop(void);
	int is_bluetooth_enabled(void);

#ifdef __cplusplus
}
#endif
