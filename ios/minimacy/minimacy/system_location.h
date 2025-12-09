// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System

#import <CoreLocation/CoreLocation.h>


#import "minimacy.h"
NS_ASSUME_NONNULL_BEGIN

@interface SystemLocation : NSObject<CLLocationManagerDelegate>
{
    @public int eventID;
    int started;
    CLLocationManager *locationManager;
}

- (int) start;
- (void) stop;

@end
int systemLocationInit(Pkg* system);
NS_ASSUME_NONNULL_END
