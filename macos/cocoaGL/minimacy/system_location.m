// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#import "system_location.h"
SystemLocation* Location;

@implementation SystemLocation
int DEBUG_LOCATION=0;
//int DEBUG_LOCATION=1;

- (id) init {
    if (self = [super init]) {
        Location=self;
    }
    return self;
}
- (int) start {
    if (DEBUG_LOCATION) NSLog(@"> Location: start");
    
    if (locationManager == nil) locationManager = [[CLLocationManager alloc] init];
    if (locationManager == nil) {
        if (DEBUG_LOCATION) NSLog(@"> Location: locationManager is nil");
        return 0;
    }
    locationManager.delegate = self;
    locationManager.desiredAccuracy = kCLLocationAccuracyBest;
    
    BOOL permitted = [locationManager locationServicesEnabled];
    if (!permitted) {
        if (DEBUG_LOCATION) NSLog(@"> Location: not permitted");
        return 0;
    }

    if (DEBUG_LOCATION) NSLog(@"> Location: start location");

    [locationManager requestAlwaysAuthorization];

    locationManager.pausesLocationUpdatesAutomatically=NO;
//    locationManager.allowsBackgroundLocationUpdates=true;

    [locationManager startUpdatingLocation];
    started=1;
    if (!eventID) eventID=eventGetNextID();
    return eventID;
}
- (void) stop {
    if (locationManager == nil) {
        if (DEBUG_LOCATION) NSLog(@"> Location: locationManager is nil");
        return;
    }
    if (DEBUG_LOCATION) NSLog(@"> Location: stop location");
    started=0;
    [locationManager stopUpdatingLocation];
}
- (int) isStarted {
    return started;
}
- (void)locationManagerDidPauseLocationUpdates:(CLLocationManager *)manager {
    if (DEBUG_LOCATION) NSLog(@"> Location: locationManagerDidPauseLocationUpdates");
}
- (void)locationManagerDidResumeLocationUpdates:(CLLocationManager *)manager {
    if (DEBUG_LOCATION) NSLog(@"> Location: locationManagerDidResumeLocationUpdates");
}
- (void)locationManager:(CLLocationManager *)manager
     didUpdateLocations:(NSArray *)locations {
    CLLocation* location = [locations lastObject];
    double coords[3];
    coords[0]=location.coordinate.longitude;
    coords[1]=location.coordinate.latitude;
    coords[2]=location.altitude;
    if (DEBUG_LOCATION) NSLog(@"> Location: didUpdateLocations %f %f %f",coords[0],coords[1],coords[2]);
    eventBinary(eventID, (char*)&coords[0], sizeof(coords));
}
- (void)locationManager:(CLLocationManager *)manager didFailWithError:(NSError *)error
{
    if (DEBUG_LOCATION) NSLog(@"> Location: didFailWithError: %@", error);
}
@end

int fun_locationStart(Thread* th)
{
    if (!Location) Location=[[SystemLocation alloc] init];

    [Location performSelectorOnMainThread:@selector(start) withObject:nil waitUntilDone:true];
    FUN_RETURN_INT(Location->eventID);
}
int fun_locationStop(Thread* th)
{
    [Location stop];
    FUN_RETURN_BOOL(1);
}
int fun_locationIsStarted(Thread* th)
{
    FUN_RETURN_BOOL([Location isStarted]);
}

int systemLocationInit(Pkg* system) {
    static const Native nativeDefs[] = {
        { NATIVE_FUN, "locationStart", fun_locationStart, "fun -> Int" },
        { NATIVE_FUN, "locationStop", fun_locationStop, "fun -> Bool" },
        { NATIVE_FUN, "locationIsStarted", fun_locationIsStarted, "fun -> Bool" },
    };
    NATIVE_DEF(nativeDefs);
    return 0;
}

