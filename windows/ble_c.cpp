// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include "ble_c.h"
#ifdef ENABLE_BLE

#include "ble_cpp.hpp"

static BleDevice* Device = NULL;

int ble_init(void)
{
	winrt::init_apartment();
	return 0;
}

void ble_shutdown(void)
{
	winrt::uninit_apartment();
}

void ble_connect(
	uint64_t address,
	const char* service_uuid,
	const char* tx_uuid,
	const char* rx_uuid,
	ble_notify_cb cb,
	void* user_ctx)
{
	Device = new BleDevice(
		address,
		winrt::to_hstring(service_uuid).c_str(),
		winrt::to_hstring(tx_uuid).c_str(),
		winrt::to_hstring(rx_uuid).c_str(),
		[cb, user_ctx](const uint8_t* d, uint32_t l) {
			cb(d, l, user_ctx);
		}
	);
}
void ble_disconnect()
{
	if (Device) delete Device;
	Device = NULL;
}

int ble_write(
	const uint8_t* data,
	uint32_t len)
{
	if (!Device) return 0;
	return Device->write(
		data,
		len
	);
}

static BleScanner g_scanner;
static ble_scan_cb g_scan_cb = nullptr;
static void* g_scan_ctx = nullptr;

int ble_scan_start_uuid(
	const char* service_uuid,
	ble_scan_cb cb,
	void* user_ctx)
{
	g_scan_cb = cb;
	g_scan_ctx = user_ctx;

	std::wstring uuid = winrt::to_hstring(service_uuid).c_str();

	return g_scanner.start(
		uuid,
		[](uint64_t addr, const std::wstring& name, int rssi, int match)
		{
			if (!g_scan_cb) return;

			char addr_str[32];
			sprintf_s(addr_str, "%012llX", addr);

			g_scan_cb(
				addr,
				winrt::to_string(name).c_str(),
				rssi,
				match,
				g_scan_ctx
			);
		}
	);
}
void ble_scan_stop(void)
{
	g_scanner.stop();
	g_scan_cb = nullptr;
	g_scan_ctx = nullptr;
}

#include <winrt/Windows.Devices.Radios.h>
#include <winrt/Windows.Foundation.Collections.h>
int is_bluetooth_enabled()
{
	using namespace winrt::Windows::Devices::Radios;

	auto radios = Radio::GetRadiosAsync().get();

	for (auto r : radios)
	{
		if (r.Kind() == RadioKind::Bluetooth)
		{
			return (r.State() == RadioState::On)?1:0;
		}
	}
	return 0;
}
#endif