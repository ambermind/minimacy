// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#include "ble_c.h"
#ifdef ENABLE_BLE
#include "ble_cpp.hpp"
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Foundation.Collections.h>


using namespace winrt;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Devices::Bluetooth;
using namespace Windows::Devices::Bluetooth::Advertisement;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace Windows::Storage::Streams;

BleDevice::~BleDevice()
{
	m_device = nullptr;
}

BleDevice::BleDevice(
	uint64_t address,
	const std::wstring& serviceUuid,
	const std::wstring& txUuid,
	const std::wstring& rxUuid,
	std::function<void(const uint8_t*, uint32_t)> cb)
{
	m_connected = false;
	try {
		m_device = BluetoothLEDevice::FromBluetoothAddressAsync(address).get();
	}
	catch (winrt::hresult_error const& ex)
	{
		wprintf(L"> BLE error: %ls (0x%08X)\n",
			ex.message().c_str(),
			ex.code().value);
		return;
	}
	m_device.ConnectionStatusChanged(
		[this, cb](auto const&, auto const&)
		{
			BluetoothConnectionStatus status = m_device.ConnectionStatus();
			//			printf("status=%d\n", status);
			if (status ==
				BluetoothConnectionStatus::Disconnected)
			{
				m_connected = false;
				//				printf("disconnected\n");
				cb(NULL, 0);
			}
		});

	m_RxChar = m_TxChar = nullptr;
	try {
		auto services = m_device.GetGattServicesAsync().get();
		for (auto s : services.Services()) {
			auto uuidStr = winrt::to_hstring(s.Uuid());
			if (uuidStr == serviceUuid) {
				auto chars = s.GetCharacteristicsAsync().get();
				for (auto c : chars.Characteristics()) {
					auto uuidStr2 = winrt::to_hstring(c.Uuid());
					if (uuidStr2 == txUuid)  m_TxChar = c;
					if (uuidStr2 == rxUuid) {
						m_RxChar = c;
						c.ValueChanged([cb](auto&, auto& args) {
							auto reader = DataReader::FromBuffer(args.CharacteristicValue());
							uint32_t size = reader.UnconsumedBufferLength();
							std::vector<uint8_t> buf(size);
							reader.ReadBytes(buf);
							cb(buf.data(), size);
							});

						auto status = c.WriteClientCharacteristicConfigurationDescriptorAsync(
							GattClientCharacteristicConfigurationDescriptorValue::Notify
						).get();
					}
				}
			}
			if (m_RxChar && m_TxChar) {
				//			printf("connected\n");
				m_connected = true;
				cb(NULL, 1);
				return;
			}
		}
	}
	catch (winrt::hresult_error const& ex)
	{
		wprintf(L"> BLE error: %ls (0x%08X)\n",
			ex.message().c_str(),
			ex.code().value);
		cb(NULL, 0);
		return;
	}
}

bool BleDevice::write(
	const uint8_t* data,
	uint32_t len)
{
	if (!m_TxChar) return false;
	DataWriter writer;
	writer.WriteBytes(array_view<const uint8_t>(data, data + len));
	m_TxChar.WriteValueAsync(writer.DetachBuffer(), GattWriteOption::WriteWithoutResponse).get();
	return true;
}


bool BleScanner::start(
	const std::wstring& serviceUuid,
	std::function<void(uint64_t, const std::wstring&, int, int)> cb)
{
	m_filterUuid = guid(serviceUuid);

	m_watcher = BluetoothLEAdvertisementWatcher();
	m_watcher.ScanningMode(BluetoothLEScanningMode::Active);

	m_watcher.Received(
		[this, cb](auto&, BluetoothLEAdvertisementReceivedEventArgs const& args)
		{
			auto name = args.Advertisement().LocalName();
			auto const& uuids = args.Advertisement().ServiceUuids();
			int match = 0;

			uint64_t addr = args.BluetoothAddress();

			for (auto const& u : uuids)
			{
				if (u == m_filterUuid)
				{
					match = 1;
					break;
				}
			}
			cb(
				addr,
				name.empty() ? L"" : name.c_str(),
				args.RawSignalStrengthInDBm(),
				match
			);
		}
	);
	try
	{
		m_watcher.Start();
	}
	catch (winrt::hresult_error const& ex)
	{
		wprintf(L"> BLE error: %ls (0x%08X)\n",
			ex.message().c_str(),
			ex.code().value);
		return false;
	}
	return true;
}

void BleScanner::stop()
{
	if (m_watcher) m_watcher.Stop();
}
#endif