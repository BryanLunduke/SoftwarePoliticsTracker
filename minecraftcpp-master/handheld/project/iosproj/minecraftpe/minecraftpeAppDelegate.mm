//
//  minecraftpeAppDelegate.m
//  minecraftpe
//
//  Created by rhino on 10/17/11.
//  Copyright 2011 Mojang AB. All rights reserved.
//

#import "minecraftpeAppDelegate.h"

#import "EAGLView.h"

#import "minecraftpeViewController.h"


@implementation minecraftpeAppDelegate

@synthesize window = _window;
@synthesize viewController = _viewController;


- (void) registerDefaultsFromFile:(NSString*)filename {
    NSString* pathToUserDefaultsValues = [[NSBundle mainBundle]
                                      pathForResource:filename 
                                      ofType:@"plist"];
    NSDictionary* userDefaultsValues = [NSDictionary dictionaryWithContentsOfFile:pathToUserDefaultsValues];
    [[NSUserDefaults standardUserDefaults] registerDefaults:userDefaultsValues];
}

NSError* audioSessionError = nil;

- (void) initAudio {
    audioSession = [AVAudioSession sharedInstance];
    [audioSession setActive:YES error:&audioSessionError];

    if (audioSessionError)
        NSLog(@"Warning; Couldn't set audio active\n");

    [audioSession setDelegate:self];

    audioSessionSoundCategory = AVAudioSessionCategoryAmbient;
    audioSessionError = nil;

    [audioSession setCategory:audioSessionSoundCategory error:&audioSessionError];

    if (audioSessionError)
        NSLog(@"Warning; Couldn't init audio\n");
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    // Override point for customization after application launch.
    [self initAudio];

    self.window.rootViewController = self.viewController;
#ifndef ANDROID_PUBLISH
    NSLog(@"ViewController: %p\n", self.viewController);
#endif
    //[self registerDefaultsFromFile:@"userDefaults"];
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    /*
     Sent when the application is about to move from active to inactive
     state. This can occur for certain types of temporary interruptions
     (such as an incoming phone call or SMS message) or when the user
     quits the application and it begins the transition to the background
     state.
       Use this method to pause ongoing tasks, disable timers, and throttle
     down OpenGL ES frame rates. Games should use this method to pause the game.
     */
#ifndef ANDROID_PUBLISH
    NSLog(@"resign-active: %@\n", [NSThread currentThread]);
#endif
    [self.viewController stopAnimation];
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    /*
     Use this method to release shared resources, save user data, invalidate
     timers, and store enough application state information to restore your
     application to its current state in case it is terminated later.
       If your application supports background execution, this method is
     called instead of applicationWillTerminate: when the user quits.
     */
    [self.viewController enteredBackground];
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    /*
     Called as part of the transition from the background to the inactive state;
     here you can undo many of the changes made on entering the background.
     */
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    /*
     Restart any tasks that were paused (or not yet started) while the
     application was inactive. If the application was previously in the
     background, optionally refresh the user interface.
     */
#ifndef ANDROID_PUBLISH
    NSLog(@"become-active: %@\n", [NSThread currentThread]);
#endif
    [self.viewController startAnimation];
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    /*
     Called when the application is about to terminate.
     Save data if appropriate.
     See also applicationDidEnterBackground:.
     */
    [self.viewController stopAnimation];
}

- (void)dealloc
{
    [_window release];
    [_viewController release];
    [super dealloc];
}

+ (void) initialize {
    if ([self class] == [minecraftpeAppDelegate class]) {
    }
}


//
// AudioSesssionDelegate
//
- (void)setAudioEnabled:(BOOL)status {
    //NSLog(@"set-audio-enabled: :%d %@\n", status, [NSThread currentThread]);

    if(status) {
        NSLog(@"INFO - SoundManager: OpenAL Active");
        // Set the AudioSession AudioCategory to what has been defined in soundCategory
		[audioSession setCategory:audioSessionSoundCategory error:&audioSessionError];
        if(audioSessionError) {
            NSLog(@"ERROR - SoundManager: Unable to set the audio session category with error: %@\n", audioSessionError);
            return;
        }
        
        // Set the audio session state to true and report any errors
		[audioSession setActive:YES error:&audioSessionError];
		if (audioSessionError) {
            NSLog(@"ERROR - SoundManager: Unable to set the audio session state to YES with error: %@\n", audioSessionError);
            return;
        }
    } else {
        NSLog(@"INFO - SoundManager: OpenAL Inactive");

        // Set the audio session state to false and report any errors
//		[audioSession setActive:NO error:&audioSessionError];
//		if (audioSessionError) {
//            NSLog(@"ERROR - SoundManager: Unable to set the audio session state to NO with error: %@\n", audioSessionError);
//            return;
//        }
    }

    [_viewController setAudioEnabled:status];
}

- (void)beginInterruption {
    //NSLog(@"beginInterruption\n");
    [self setAudioEnabled:NO];
}
- (void)endInterruption {
    //NSLog(@"endInterruption\n");
    [self setAudioEnabled:YES];
}

@end
