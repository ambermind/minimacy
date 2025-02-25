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
}

- (int) start:(char*) service Tx:(char*)tx Rx:(char*)rx;
- (void) stop;
- (int) isConnected;
- (NSString*) name;
- (int) send:(NSData*)data;

@end
int systemBleInit(Pkg* system);
NS_ASSUME_NONNULL_END
