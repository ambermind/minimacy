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

package com.example;

import android.Manifest;
import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Handler;
import android.view.inputmethod.InputMethodManager;
import android.view.KeyEvent;
import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.util.Log;
import android.bluetooth.*;
import android.bluetooth.le.*;
import android.os.ParcelUuid;
import android.os.Looper;

import java.util.Collections;
import java.util.HashSet;
import java.util.UUID;

import android.location.*;

public class Minimacy extends android.app.NativeActivity implements SensorEventListener, KeyEvent.Callback {
	public static String LOG = "mcy";

	public static int PERMISSION_BLE=1;
	public static int PERMISSION_LOCATION=2;

	public static void Log(String str) {
		Log.d(LOG, str);
	}

	private SensorManager mSensorManager = null;
	private Sensor mSensor;
	public float xacc = 0.0f;
	public float yacc = 0.0f;
	public float zacc = 0.0f;

	private BluetoothAdapter bluetoothAdapter;
	private BluetoothLeScanner bluetoothLeScanner;

	public native int notifyChar(int c);

	public native int eventGetNextID();
	public native int eventBinary(int eventID, byte[] msg);
	public native void notifyLocation(int eventID, double x, double y, double z);


	private Context context=this;
	private int bleConnectAndRun = 0;
	private int bleConnected = 0;
	private int bleEventId = 0;

	private HashSet blePeripherals;
	private BluetoothDevice blePeripheral;
	private BluetoothGatt bleGatt;
	private BluetoothGattCharacteristic bleRxChar;
	private BluetoothGattCharacteristic bleTxChar;
	private String bleServiceUUID;
	private String bleTxUUID;
	private String bleRxUUID;

	private int locationEventId = 0;
	private int locationStarted = 0;
	private LocationManager locationManager = null;

	public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
		Minimacy.Log("onRequestPermissionsResult "+requestCode);
		if (grantResults.length>0 && grantResults[0]==PackageManager.PERMISSION_GRANTED) {
			if (requestCode==PERMISSION_BLE) bleStartScan(true);
			else if (requestCode==PERMISSION_LOCATION) locationStart();
		}
	}

	private final LocationListener locationListener = new LocationListener() {
		@Override
		public void onLocationChanged(Location location) {
			Minimacy.Log("location changed "+ location.getLatitude() + ", " + location.getLongitude());
			notifyLocation(locationEventId,location.getLongitude(),location.getLatitude(),location.getAltitude());
		}
	};

	public int locationIsStarted()
	{
		return locationStarted;
	}
	public void locationStop()
	{
		if (locationManager==null) return;
		locationStarted=0;
		locationManager.removeUpdates(locationListener);
	}
	public int locationStart()
	{
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
			Minimacy.Log("============> check permission");
			if (checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
				requestPermissions(new String[]{Manifest.permission.ACCESS_FINE_LOCATION}, PERMISSION_LOCATION);
				return 0;
			}
			if (checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
				requestPermissions(new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, PERMISSION_LOCATION);
				return 0;
			}
		}

		if (locationManager==null) {
			locationManager = (LocationManager) this.getSystemService(Context.LOCATION_SERVICE);
			if (!locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER)) return 0;
		}
		if (locationEventId==0) locationEventId=eventGetNextID();
		locationStarted=1;
		new Handler(Looper.getMainLooper()).post(new Runnable () {
			@SuppressLint("MissingPermission")
			@Override
			public void run () {
				Location location = locationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER);
				if (location!=null) notifyLocation(locationEventId,location.getLongitude(),location.getLatitude(),location.getAltitude());
				locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER,
						5000,          // 10-second interval.
						10,             // 10 meters.
						locationListener);
			}
		});

		Minimacy.Log("=============locationStart ->"+locationEventId);
		return locationEventId;
	}

	@TargetApi(Build.VERSION_CODES.LOLLIPOP)
	private BluetoothGattCallback leGattCallback = new BluetoothGattCallback() {
		@SuppressLint("MissingPermission")
		@Override
		public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
			Minimacy.Log("onConnectionStateChange " + status + "/ " + newState);
			if (newState == BluetoothProfile.STATE_CONNECTED) {
				Minimacy.Log("device connected "+blePeripheral.getName()); // successfully connected to the GATT Server
				gatt.discoverServices();
			} else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
				// disconnected from the GATT Server
				Minimacy.Log("device disconnected");
				if (bleConnected!=0) {
					bleConnected = 0;
					eventBinary(bleEventId, new byte[0]);
					bleStartScan(true);
				}
				else if (bleConnectAndRun!=0) bleStartScan(false);
				else bluetoothLeScanner.stopScan(leScanCallback);
			}
		}
		@SuppressLint("MissingPermission")
		@Override
		public void onServicesDiscovered(final BluetoothGatt gatt, final int status) {
			Minimacy.Log("onServicesDiscovered");
			for (BluetoothGattService gattService : gatt.getServices()) {
				final String uuid = gattService.getUuid().toString();
				Minimacy.Log("srv: " + uuid +" / "+bleServiceUUID);
				if (uuid.equalsIgnoreCase(bleServiceUUID)) {
					Minimacy.Log("getCharacteristics");
					bleTxChar=bleRxChar=null;
					for (BluetoothGattCharacteristic gattCharacteristic : gattService.getCharacteristics()) {
						final String charUuid = gattCharacteristic.getUuid().toString();
						Minimacy.Log("char: " + charUuid);
						int properties=gattCharacteristic.getProperties();
						if ((properties & BluetoothGattCharacteristic.PROPERTY_WRITE)!=0) {
							Minimacy.Log("found TX");
							bleTxChar=gattCharacteristic;
						}
						if ((properties & BluetoothGattCharacteristic.PROPERTY_NOTIFY)!=0) {
							Minimacy.Log("found RX");
							bleRxChar=gattCharacteristic;
						}
					}
					if (bleTxChar!=null && bleRxChar != null) {
						bleGatt=gatt;
						boolean result=bleGatt.setCharacteristicNotification(bleRxChar,true);
						Minimacy.Log("notification enable "+(result?"ok":"nok"));
						UUID CCC_DESCRIPTOR_UUID=UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");
						BluetoothGattDescriptor descriptor = bleRxChar.getDescriptor(CCC_DESCRIPTOR_UUID);
						descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
						result = bleGatt.writeDescriptor(descriptor);
						Minimacy.Log("notification writeDescriptor "+(result?"ok":"nok"));
						bleConnected=1;
						eventBinary(bleEventId, new byte[0]);
						return;
					}
				}
			}
		}
		@Override
		public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
			Minimacy.Log("onCharacteristicChanged");
			if (characteristic!=bleRxChar) return;
			byte[] value=bleRxChar.getValue();
			eventBinary(bleEventId, value);
		}
		@Override
		public void onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
			Minimacy.Log("onCharacteristicWrite "+status);
		}
	};
	@TargetApi(Build.VERSION_CODES.LOLLIPOP)
	private ScanCallback leScanCallback = new ScanCallback() {
		@Override
		public void onScanResult(int callbackType, ScanResult result) {
			Minimacy.Log("onScanResult");
			super.onScanResult(callbackType, result);
			BluetoothDevice device=result.getDevice();
			String address=device.getAddress();
			if (blePeripherals.contains(address)) return;
			try {
				Minimacy.Log("device " + address + " -> '" + device.getName()+"' "+result.getRssi()+" db");
				blePeripherals.add(address);
	//					if (!device.getName().equals("Viecar")) return;
				Minimacy.Log("connecting");
				if (blePeripheral!=null) return;
				blePeripheral = device;
//				bluetoothLeScanner.stopScan(leScanCallback);
				blePeripheral.connectGatt(context, true, leGattCallback,BluetoothDevice.TRANSPORT_LE,-1,null);
			} catch (SecurityException e){
				Minimacy.Log("SecurityException!!!");
			}
		}
	};
	@TargetApi(Build.VERSION_CODES.LOLLIPOP)
	public boolean bleStartScan(boolean clearPeripherals) {
		bluetoothLeScanner = bluetoothAdapter.getBluetoothLeScanner();
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
			Minimacy.Log("============> check permission");
			if (checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
				requestPermissions(new String[]{Manifest.permission.ACCESS_FINE_LOCATION}, PERMISSION_BLE);
				return true;
			}
			if (checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
				requestPermissions(new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, PERMISSION_BLE);
				return true;
			}
			if (checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
				requestPermissions(new String[]{Manifest.permission.BLUETOOTH_SCAN}, PERMISSION_BLE);
				return true;
			}
			if (checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
				requestPermissions(new String[]{Manifest.permission.BLUETOOTH_CONNECT}, PERMISSION_BLE);
				return true;
			}
		}
		Minimacy.Log("============> startScan '"+bleServiceUUID+"'");

		if (clearPeripherals) blePeripherals=new HashSet();

		ScanFilter filter = new ScanFilter.Builder().setServiceUuid(
				ParcelUuid.fromString(bleServiceUUID)
			).build();
		ScanSettings scanSettings = new ScanSettings.Builder()
				.setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
				.build();
		bluetoothLeScanner.startScan(Collections.singletonList(filter), scanSettings, leScanCallback);
		return true;
	}
	private String bleFilterUUID(String uuid)
	{
		if (uuid.length()==4) return "0000"+uuid+"-0000-1000-8000-00805f9b34fb";
		return uuid;
	}
	public int bleSerialStart(String serviceUUID, String rxUUID, String txUUID)
	{
		Minimacy.Log("=============bleSerialStart "+serviceUUID+" / "+rxUUID+" / "+txUUID);
		if (bleConnectAndRun!=0) return bleEventId;
		bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
		if (bluetoothAdapter == null) return 0;

		bleServiceUUID=bleFilterUUID(serviceUUID);
		bleRxUUID=bleFilterUUID(rxUUID);
		bleTxUUID=bleFilterUUID(txUUID);

		bleConnectAndRun=1;
		bleConnected=0;
		if (bleEventId==0) bleEventId=eventGetNextID();
		Minimacy.Log("=============bleSerialStart ->"+bleEventId);
		bleStartScan(true);
		return bleEventId;
	}
	@SuppressLint({"MissingPermission", "NewApi"})
	public int bleSerialStop()
	{
		if (bleConnectAndRun==0) return 0;
		bleConnectAndRun=0;
		if (bleConnected==0) bluetoothLeScanner.stopScan(leScanCallback);
		else bleGatt.disconnect();
		bleConnected=0;
		blePeripheral=null;
		return 0;
	}
	@SuppressLint("MissingPermission")
	public String bleSerialName()
	{
		if (bleConnected!=0) return blePeripheral.getName();
		return null;
	}
	public int bleSerialIsConnected()
	{
		return bleConnected;
	}
	@SuppressLint({"MissingPermission", "NewApi"})
	public int bleSerialWrite(byte[] msg)
	{
//		Log(">>bleSerialWrite "+msg.length);
		if (bleConnected==0) return 0;
//		for(int i=0;i<msg.length;i++) Log("- "+msg[i]);
		bleTxChar.setValue(msg);
		bleGatt.writeCharacteristic(bleTxChar);
		return msg.length;
	}
	@Override
	public boolean onKeyMultiple(int keyCode, int count, KeyEvent event) {
		if (count != 0) return false;
		String input = event.getCharacters();
		char c0 = input.charAt(0);
		Minimacy.Log("onKeyMultiple " + input + "[" + c0 + "]");
		notifyChar((int) c0);
		return true;
	}

	public void showKeyboard() {
		InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
		imm.showSoftInput(this.getWindow().getDecorView(), InputMethodManager.SHOW_FORCED);
	}

	public void hideKeyboard() {
		InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
		imm.hideSoftInputFromWindow(this.getWindow().getDecorView().getWindowToken(), 0);
	}

	public int setAccelerometer(float freq) {
		float x = 1000000 / freq;
		int delay = (int) x;
		if (mSensorManager == null) {
			try {
				mSensorManager = (SensorManager) getSystemService(android.content.Context.SENSOR_SERVICE);
				mSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
				if (mSensor == null) Minimacy.Log("mSensor=null");
				else Minimacy.Log("mSensor not null");
			} catch (Throwable t) {
				Minimacy.Log("cannot open sensormanager");
			}
		} else mSensorManager.unregisterListener(this);
		if (mSensorManager != null) mSensorManager.registerListener(this, mSensor, delay);
		return 0;
	}

	public void onAccuracyChanged(Sensor sensor, int accuracy) {
	}

	public float accGetX() {
		return xacc;
	}

	public float accGetY() {
		return yacc;
	}

	public float accGetZ() {
		return zacc;
	}

	public void onSensorChanged(SensorEvent event) {
		xacc = -event.values[0] / 10;
		yacc = -event.values[1] / 10;
		zacc = event.values[2] / 10;
//		Minimacy.Log("acc "+xacc+" "+yacc+" "+zacc);
	}

	static {
		System.loadLibrary("native-minimacy");
	}
}
