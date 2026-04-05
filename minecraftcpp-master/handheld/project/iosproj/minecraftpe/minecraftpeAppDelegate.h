//
//  minecraftpeAppDelegate.h
//  minecraftpe
//
//  Created by rhino on 10/17/11.
//  Copyright 2011 Mojang AB. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>

@class minecraftpeViewController;

@interface minecraftpeAppDelegate : NSObject <UIApplicationDelegate, AVAudioSessionDelegate> {
    AVAudioSession* audioSession;
    NSString*       audioSessionSoundCategory;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;

@property (nonatomic, retain) IBOutlet minecraftpeViewController *viewController;

// AVAudioSessionDelegate
- (void)beginInterruption;
- (void)endInterruption;

+ (void) initialize;

@end
