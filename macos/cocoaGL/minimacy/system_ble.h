// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#import <CoreBluetooth/CoreBluetooth.h>

#import "minimacy.h"
NS_ASSUME_NONNULL_BEGIN

@interface SystemBle : NSObject<CBCentralManagerDelegate, CBPeripheralDelegate>
{
    int connectAndRun;
    int connected;
    int eventID;
    CBCentralManager *centralManager;
    NSMutableArray *blePeripherals;
    CBUUID* bleServiceUUID;
    CBUUID* bleTxUUID;
    CBUUID* bleRxUUID;
    CBPeripheral *blePeripheral;
    CBCharacteristic *bleRxChar;
    CBCharacteristic *bleTxChar;
    CBCharacteristicWriteType writeType;
}

- (int) start:(char*) service Tx:(char*)tx Rx:(char*)rx;
- (void) stop;
- (int) isConnected;
- (NSString*) name;
- (int) send:(NSData*)data;

@end
int systemBleInit(Pkg* system);
NS_ASSUME_NONNULL_END
