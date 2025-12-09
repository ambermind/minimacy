// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#import "system_ble.h"
SystemBle* BLE;

@implementation SystemBle
int DEBUG_BLE=0;
//int DEBUG_BLE=1;

#pragma mark - Central Manager delegate methods
- (id) init {
    if (self = [super init]) {
        BLE=self;
    }
    return self;
}
- (int) start:(char*) service Tx:(char*)tx Rx:(char*)rx {
    if (connectAndRun) return eventID;
    if (DEBUG_BLE) NSLog(@"> BLE: start");
    bleServiceUUID=[CBUUID UUIDWithString:[[NSString alloc] initWithUTF8String:service]];
    bleTxUUID=[CBUUID UUIDWithString:[[NSString alloc] initWithUTF8String:tx]];
    bleRxUUID=[CBUUID UUIDWithString:[[NSString alloc] initWithUTF8String:rx]];

    connectAndRun=1;
    connected=0;
    if (!eventID) eventID=eventGetNextID();
    if (!centralManager) {
        centralManager = [[CBCentralManager alloc] initWithDelegate:self queue:nil];
        if (DEBUG_BLE) NSLog(@"> BLE: Start centralManager");
    }
    else if (centralManager.state==CBCentralManagerStatePoweredOn)
    {
        [centralManager scanForPeripheralsWithServices:@[bleServiceUUID] options:nil];
        if (DEBUG_BLE) NSLog(@"> BLE: Start scanning");
    }
    blePeripherals = [NSMutableArray new];
    return eventID;
}
- (void) stop {
    if (!connectAndRun) return;
    connectAndRun=0;
    if (!connected) [centralManager stopScan];
    else [centralManager cancelPeripheralConnection:blePeripheral];
    connected=0;
}
- (int) isConnected {
    return connected;
}
- (NSString*) name {
    if (!connected) return nil;
    return blePeripheral.name;
}
- (int) send:(NSData*)data {
    if (!connected) return 0;
    [blePeripheral writeValue:data forCharacteristic:bleTxChar type:writeType];
    return (int)data.length;
}

- (const char *) centralManagerStateToString: (int)state{
    switch(state) {
        case CBCentralManagerStateUnknown:
            return "State unknown";
        case CBCentralManagerStateResetting:
            return "State resetting";
        case CBCentralManagerStateUnsupported:
            return "State BLE unsupported";
        case CBCentralManagerStateUnauthorized:
            return "State unauthorized";
        case CBCentralManagerStatePoweredOff:
            return "State BLE powered off";
        case CBCentralManagerStatePoweredOn:
            return "State powered up and ready";
    }
    return "State unknown";
}
- (void)centralManagerDidUpdateState:(CBCentralManager *)central {
    if (DEBUG_BLE) NSLog(@"> BLE: Status changed %d (%s)",(int)central.state,[self centralManagerStateToString:(int)central.state]);
    if (central.state==CBCentralManagerStatePoweredOn)
    {
        if (connectAndRun) {
            [centralManager scanForPeripheralsWithServices:@[bleServiceUUID] options:nil];
            if (DEBUG_BLE) NSLog(@"> BLE: Start scanning");
        }
    }
    else
    {
        [blePeripherals removeAllObjects];
    }
}

- (void)centralManager:(CBCentralManager *)central didDiscoverPeripheral:(CBPeripheral *)peripheral advertisementData:(NSDictionary *)advertisementData RSSI:(NSNumber *)RSSI
{
    if (![blePeripherals containsObject:peripheral])
    {
        [blePeripherals addObject:peripheral];
        if (DEBUG_BLE)  NSLog(@"> BLE: discovered %@",peripheral);
        [centralManager connectPeripheral:peripheral options:nil];
    } else {
//        if (DEBUG_BLE) NSLog(@"> BLE: peripheral already in array %@",peripheral);
    }
}

-(void)centralManager:(CBCentralManager *)central didFailToConnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error
{
    if (DEBUG_BLE) NSLog(@"> BLE: Failed to connect peripheral %@ (%@)",peripheral,error);
}

-(void)centralManager:(CBCentralManager *)central didDisconnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error
{
    if (DEBUG_BLE) NSLog(@"> BLE: Disconnected peripheral %@ (%@)",peripheral,error);
    [blePeripherals removeAllObjects];
    connected=0;
    if (connectAndRun) {
        [centralManager scanForPeripheralsWithServices:@[bleServiceUUID] options:nil];
        if (DEBUG_BLE) NSLog(@"> BLE: Start rescanning");
        eventBinary(eventID, "", 0);
    }
    else
        [centralManager stopScan];
}
-(void)centralManager:(CBCentralManager *)central didConnectPeripheral:(CBPeripheral *)peripheral
{
    if (DEBUG_BLE) NSLog(@"> BLE: Connected to peripheral %@",peripheral);
    // Discover the service
    blePeripheral=peripheral;
    peripheral.delegate=self;
    bleRxChar=bleTxChar=nil;
    [peripheral discoverServices:@[bleServiceUUID]];
}

#pragma mark - Peripheral delegate methods
-(void)peripheral:(CBPeripheral *)peripheral didDiscoverServices:(NSError *)error
{
    if (!error)
    {
        for(CBService *service in peripheral.services)
        {
            if (DEBUG_BLE) NSLog(@"> BLE: Discovered service %@",service.UUID);
            if ([service.UUID isEqual:bleServiceUUID])
            {
                if (DEBUG_BLE) NSLog(@"> BLE: Scanning characteristics");
                [peripheral discoverCharacteristics:nil forService:service];
                return;
            }
        }
    }
    if (DEBUG_BLE) NSLog(@"> BLE: didDiscoverServices Cancel peripheral %@ (%@)",peripheral,error);
    [centralManager cancelPeripheralConnection:peripheral];
}
-(void)peripheral:(CBPeripheral *)peripheral didDiscoverCharacteristicsForService:(CBService *)service error:(NSError *)error
{
    if (!error)
    {
        for(CBCharacteristic *characteristic in service.characteristics)
        {
            if (DEBUG_BLE) NSLog(@"> BLE: discover characteristic %@",characteristic);
            if ([characteristic.UUID isEqual:bleRxUUID])
            {
                bleRxChar=characteristic;
                if (DEBUG_BLE) NSLog(@"> BLE: Found Rx %@",characteristic);
                [peripheral setNotifyValue:YES forCharacteristic:characteristic];
            }//properties : 0x1A write only, 0x16 write with response only
            if ([characteristic.UUID isEqual:bleTxUUID])
            {
                bleTxChar=characteristic;
                if (DEBUG_BLE) NSLog(@"> BLE: Found Tx %@",characteristic);
                if (bleTxChar.properties & CBCharacteristicPropertyWrite) {
                    writeType=CBCharacteristicWriteWithResponse;
                    NSLog(@"> BLE: write with response available");
                }
                if (bleTxChar.properties & CBCharacteristicPropertyWriteWithoutResponse) {
                    writeType=CBCharacteristicWriteWithoutResponse;
                    NSLog(@"> BLE: write without response available");
                }
            }
        }
        if ((bleRxChar!=nil)&&(bleTxChar!=nil))
        {
            connected=1;
            eventBinary(eventID, "", 0);
        }
        return;
    }
    if (DEBUG_BLE) NSLog(@"> BLE: didDiscoverCharacteristicsForService Cancel peripheral %@ (%@)",peripheral,error);
    [centralManager cancelPeripheralConnection:peripheral];
}
-(void)peripheral:(CBPeripheral *)peripheral didUpdateNotificationStateForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error
{
    if (error)
    {
        if (DEBUG_BLE) NSLog(@"> BLE: Failed to subscribe to peripheral %@ (%@)",peripheral,error);
        [centralManager cancelPeripheralConnection:peripheral];
        return;
    }
}
-(void)peripheral:(CBPeripheral *)peripheral didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error
{
    if (DEBUG_BLE) NSLog(@"> BLE: val =%@",characteristic.value.description);
    NSData* data=characteristic.value;
    int len=data.length;
    eventBinary(eventID, [data bytes], len);
}
-(void)peripheral:(CBPeripheral *)peripheral didWriteValueForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error
{
}
@end

int fun_bleSerialStart(Thread* th)
{
    LB* tx = STACK_PNT(th, 0);
    LB* rx = STACK_PNT(th, 1);
    LB* service = STACK_PNT(th, 2);
    if (!rx || !tx || !service) FUN_RETURN_NIL;
    if (!BLE) BLE=[[SystemBle alloc] init];
    int event=[BLE start:STR_START(service) Tx:STR_START(tx) Rx:STR_START(rx)];
    FUN_RETURN_INT(event);
}
int fun_bleSerialStop(Thread* th)
{
    [BLE stop];
    FUN_RETURN_BOOL(1);
}
int fun_bleSerialIsConnected(Thread* th)
{
    FUN_RETURN_BOOL([BLE isConnected]);
}
int fun_bleSerialName(Thread* th)
{
    NSString* name=[BLE name];
    if (!name) FUN_RETURN_NIL;
//    NSLog(@"> BLE: name %@",name);
    char* pName=[name cStringUsingEncoding:NSISOLatin1StringEncoding];
//    char* pName=[name cStringUsingEncoding:NSUTF8StringEncoding];
//    if (!pName) NSLog(@"> NULL NAME");
    if (!pName) FUN_RETURN_NIL;
    FUN_RETURN_STR(pName, -1);
}
int fun_bleSerialWrite(Thread* th)
{
    LINT len=0;

    LINT start = STACK_INT(th, 0);
    LB* src = STACK_PNT(th, 1);
    FUN_SUBSTR(src, start, len, 1, STR_LENGTH(src));
    if (len==0) FUN_RETURN_INT(start);
    NSData *data = [[NSMutableData alloc] initWithBytes:STR_START(src) + start length:len];
    len= [BLE send:(NSData*) data];
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
    NATIVE_DEF(nativeDefs);    return 0;
}

