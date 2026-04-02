// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#pragma once
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <string>
#include <functional>

class BleDevice {
public:
	BleDevice(uint64_t address,
		const std::wstring& serviceUuid,
		const std::wstring& txUuid,
		const std::wstring& rxUuid,
		std::function<void(const uint8_t*, uint32_t)> cb
	);
	~BleDevice();

	bool write(
		const uint8_t* data,
		uint32_t len
	);

private:
	bool m_connected;
	winrt::Windows::Devices::Bluetooth::BluetoothLEDevice m_device{ nullptr };
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic m_TxChar{ nullptr };
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic m_RxChar{ nullptr };
};

class BleScanner {
public:
	bool start(
		const std::wstring& serviceUuid,
		std::function<void(uint64_t, const std::wstring&, int, int)> cb
	);
	void stop();

private:
	winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher m_watcher{ nullptr };
	winrt::guid m_filterUuid{};
};