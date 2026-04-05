//
//  minecraftpeViewController.m
//  minecraftpe
//
//  Created by rhino on 10/17/11.
//  Copyright 2011 Mojang AB. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>

#import "minecraftpeViewController.h"
#import "EAGLView.h"

#import "../../../src/platform/input/Multitouch.h"
#import "../../../src/platform/input/Mouse.h"
#import "../../../src/client/OptionStrings.h"

#import "../../../src/App.h"
#import "../../../src/AppPlatform_iOS.h"
#import "../../../src/NinecraftApp.h"

//#import "dialogs/DialogViewController.h"
#import "dialogs/CreateNewWorldViewController.h"
#import "dialogs/RenameMPWorldViewController.h"
#import "../../lib_projects/InAppSettingsKit/Models/IASKSettingsReader.h"

#import "../../lib_projects/InAppSettingsKit/Views/IASKSwitch.h"
#import "../../lib_projects/InAppSettingsKit/Views/IASKPSToggleSwitchSpecifierViewCell.h"

@interface minecraftpeViewController ()
@property (nonatomic, retain) EAGLContext *context;
@property (nonatomic, assign) CADisplayLink *displayLink;
- (BOOL) initView;
- (BOOL) releaseView;

- (void) _resetAllPointers;
@end

static NSThread* lastThread = nil;

@implementation minecraftpeViewController

@synthesize animating;
@synthesize context;
@synthesize displayLink;

@synthesize appSettingsViewController;

- (IASKAppSettingsViewController*)appSettingsViewController {
	if (!appSettingsViewController) {
		appSettingsViewController = [[IASKAppSettingsViewController alloc] initWithNibName:@"IASKAppSettingsView" bundle:nil];
		appSettingsViewController.delegate = self;

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(settingToggled:) name:kIASKAppSettingChanged object:nil];
	}
	return appSettingsViewController;
}

- (void)awakeFromNib
{
    EAGLContext *aContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
    
    if (!aContext)
        NSLog(@"Failed to create ES context");
    else if (![EAGLContext setCurrentContext:aContext])
        NSLog(@"Failed to set ES context current");
    
	self.context = aContext;
	[aContext release];
	
    [(EAGLView *)self.view setContext:context];
    [(EAGLView *)self.view setFramebuffer];
    
    animating = FALSE;
    animationFrameInterval = 1;

    //animationFrameInterval = 2;

    //NSString* renderer = [[NSString alloc] initWithUTF8String: (const char*)glGetString(GL_RENDERER)];
    //if (renderer != nil && [renderer rangeOfString:@"SGX"].location != NSNotFound && [renderer rangeOfString:@"543"].location != NSNotFound)
    //{
        //NSLog(@"WE HAVE A FAST GRAPHICS CARD\n");
        //NSLog(@"WE HAVE A FAST GRAPHICS CARD\n");
        //NSLog(@"WE HAVE A FAST GRAPHICS CARD\n");
        //animationFrameInterval = 1;
    //}
    //[renderer release];

    self.displayLink = nil;
    
    _platform = new AppPlatform_iOS(self);
    _context = new AppContext();
    _context->platform = _platform;
    _context->doRender = false;
    
    viewScale = 1;

    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];

    NSArray *tmpPaths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    NSString *cachesDirectory    = [tmpPaths objectAtIndex:0];

    _app = new NinecraftApp();
    ((Minecraft*)_app)->externalStoragePath = [documentsDirectory UTF8String];
    ((Minecraft*)_app)->externalCacheStoragePath = [cachesDirectory UTF8String];

    [self initView];
    //_app->init(*_context);

    _touchMap = new UITouch*[Multitouch::MAX_POINTERS];
    [self _resetAllPointers];

    _dialog = nil;
}

- (void)dealloc
{
    // Tear down context.
    if ([EAGLContext currentContext] == context)
        [EAGLContext setCurrentContext:nil];
    
    delete _app;
    delete _platform;
    delete[] _touchMap;
    
    [context release];
    
    [super dealloc];
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc. that aren't in use.
}

- (void)viewWillAppear:(BOOL)animated
{
    [self startAnimation];
    
    [super viewWillAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [self stopAnimation];
    
    [super viewWillDisappear:animated];
}

- (void)viewDidLoad
{
    _keyboardView = [[ShowKeyboardView alloc] init];
	[super viewDidLoad];
}

- (void)viewDidUnload
{
	[super viewDidUnload];
    [_keyboardView release];
    _keyboardView = nil;
    // Tear down context.
    if ([EAGLContext currentContext] == context)
        [EAGLContext setCurrentContext:nil];
	self.context = nil;	
}

- (NSInteger)animationFrameInterval
{
    return animationFrameInterval;
}

- (void)setAnimationFrameInterval:(NSInteger)frameInterval
{
    if (frameInterval >= 1) {
        animationFrameInterval = frameInterval;
        
        if (animating) {
            [self stopAnimation];
            [self startAnimation];
        }
    }
}

- (void)startAnimation
{
    if (!animating) {
        CADisplayLink *aDisplayLink = [[UIScreen mainScreen] displayLinkWithTarget:self selector:@selector(drawFrame)];
        [aDisplayLink setFrameInterval:animationFrameInterval];
        [aDisplayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        self.displayLink = aDisplayLink;

        //[self initView];
        //NSLog(@"start-animation: %@\n", [NSThread currentThread]);

        animating = TRUE;

        //((Minecraft*)_app)->resetServerSocket(true);
    }
}

- (void)stopAnimation
{
    if (animating) {
        [self.displayLink invalidate];
        self.displayLink = nil;
        animating = FALSE;
        
        //[self releaseView];
        //NSLog(@"stop-animation: %@\n", [NSThread currentThread]);
    }
}

- (void)enteredBackground
{
#ifdef APPLE_DEMO_PROMOTION
    if (_app)
        ((Minecraft*)_app)->leaveGame();
#endif
}

- (void)setAudioEnabled:(BOOL)status {
    if (status) _app->audioEngineOn();
    else        _app->audioEngineOff();
}

- (void)drawFrame
{
    NSThread* currentThread = [NSThread currentThread];
    
    if (lastThread == nil) {
        //NSLog(@"draw-frame: %@\n", currentThread);
        lastThread = currentThread;
    }
    
    if (currentThread != lastThread) {
        NSLog(@"Warning! draw-frame thread changed (%@ -> %@)\n", lastThread, currentThread);
        lastThread = currentThread;
    }
    
    [(EAGLView *)self.view setFramebuffer];

    _app->update();

    const GLenum discards[]  = {GL_DEPTH_ATTACHMENT};
//    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glDiscardFramebufferEXT(GL_FRAMEBUFFER, 1, discards);    

    [(EAGLView *)self.view presentFramebuffer];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return UIInterfaceOrientationIsLandscape(interfaceOrientation);
}

- (BOOL) initView
{
    viewScale = ((EAGLView*)self.view)->viewScale;
    
    if (!_app->isInited())
        _app->init(*_context);
    else
        _app->onGraphicsReset(*_context);
    
    CGRect screen = [[UIScreen mainScreen] bounds];
    CGFloat width  = MAX(screen.size.width, screen.size.height) * viewScale;
    CGFloat height = MIN(screen.size.width, screen.size.height) * viewScale;
    
//    CGFloat width  = ((EAGLView*)self.view)->framebufferWidth;
//    CGFloat height = ((EAGLView*)self.view)->framebufferHeight;
    
    //NSLog(@"initView. Setting size: %f, %f\n", width, height);
    _app->setSize((int)width, (int)height);
    
    return true;
}

- (BOOL) releaseView
{
    return true;
}

-(int) _getIndexForTouch:(UITouch*) touch {
    for (int i = 0; i < Multitouch::MAX_POINTERS; ++i)
        if (_touchMap[i] == touch) return i;
    // @todo: implement a fail-safe! something like this;
    // 1) If we've come here (pointer isn't found), increment missValue
    // 2) If missValue >= N, call _resetAllPointers
    // X) If we found a pointer, reset the missValue to 0
    return -1;
}

-(void) _resetAllPointers {
    for (int i = 0; i < Multitouch::MAX_POINTERS; ++i)
        _touchMap[i] = 0;
}

-(int) _startTrackingTouch:(UITouch*) touch {
    for (int i = 0; i < Multitouch::MAX_POINTERS; ++i)
        if (_touchMap[i] == 0) {
            _touchMap[i] = touch;
            return i;
        }
    return -1;
}

-(int) _stopTrackingTouch:(UITouch*) touch {
    for (int i = 0; i < Multitouch::MAX_POINTERS; ++i)
        if (_touchMap[i] == touch) {
            _touchMap[i] = 0;
            return i;
        }
    return -1;
}

// Handles the start of a touch
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    //NSLog(@"touch began count: %d\n",[touches count]);

	for( UITouch *touch in touches ) {
        int pointerId = [self _startTrackingTouch:touch];
//        NSLog(@"touch-began: %d %p\n", pointerId, touch);
        if (pointerId >= 0) {
            CGPoint touchLocation = [touch locationInView:[self view]];
            touchLocation.x *= viewScale;
            touchLocation.y *= viewScale;
            //LOGI("d: %f, %f\n", touchLocation.x, touchLocation.y);
            Mouse::feed(1, 1, touchLocation.x, touchLocation.y);
            Multitouch::feed(1, 1, touchLocation.x, touchLocation.y, pointerId);
        }
	}
}

// Handles the continuation of a touch.
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    //NSLog(@"touch moved: %d\n",[touches count]);

	for( UITouch *touch in touches ) {
        int pointerId = [self _getIndexForTouch:touch];
        if (pointerId >= 0) {
            //NSLog(@"Zilly-bop: %d %p\n", pointerId, touch);
            CGPoint touchLocation = [touch locationInView:[self view]];
            touchLocation.x *= viewScale;
            touchLocation.y *= viewScale;
            //LOGI("t: %f, %f\n", touchLocation.x, touchLocation.y);
            Mouse::feed(0, 0, touchLocation.x, touchLocation.y);
            Multitouch::feed(0, 0, touchLocation.x, touchLocation.y, pointerId);
        }
	}	
}

// Handles the end of a touch event.
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	for( UITouch *touch in touches ) {
        int pointerId = [self _stopTrackingTouch:touch];
        if (pointerId >= 0) {
            CGPoint touchLocation = [touch locationInView:[self view]];
            touchLocation.x *= viewScale;
            touchLocation.y *= viewScale;
            Mouse::feed(1, 0, touchLocation.x, touchLocation.y);
            Multitouch::feed(1, 0, touchLocation.x, touchLocation.y, pointerId);
        }
	}
}

// Handles the cancellation of a touch event.
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self touchesEnded: touches withEvent:event];
}


//
// Interfaces for AppPlatform
//

-(BaseDialogController*) dialog
{
    return _dialog;
}

- (void) initDialog {
    _dialogResultStatus = -1;
    _dialogResultStrings.clear();
    [_dialog setListener:self];
}

- (void) closeDialog {
    if (_dialog != nil) {
        _dialogResultStatus  = [_dialog getUserInputStatus];
        _dialogResultStrings = [_dialog getUserInput];
    }
    //NSLog(@"Close@vc: %p %d\n", _dialog, _dialogResultStatus);
    [_dialog dismissModalViewControllerAnimated:YES];

    [_dialog release];
     _dialog = nil;
}

- (void)showDialog_CreateWorld
{
    if (!_dialog) {
        BOOL isIpad = (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad);
        
        CGRect screen = [[UIScreen mainScreen] bounds];
        CGFloat width  = MAX(screen.size.width, screen.size.height);
        BOOL isIphone5 = (width == 568);
        NSString* xib = isIpad ? @"CreateNewWorld_ipad" : (isIphone5? @"CreateNewWorld_iphone5" : @"CreateNewWorld_iphone");
        _dialog = [[CreateNewWorldViewController alloc] initWithNibName:xib bundle:[NSBundle mainBundle]];
        [self presentModalViewController:_dialog animated:YES];
        [self initDialog];
        //NSLog(@"--- %p %p\n", _dialog, [self view]);
        //[_dialog addToView:[self view]];
    }
}

- (void)showDialog_MainMenuOptions
{
    if (!_dialog) {
        UINavigationController *aNavController = [[UINavigationController alloc] initWithRootViewController:self.appSettingsViewController];
        [self.appSettingsViewController setShowCreditsFooter:NO];
        self.appSettingsViewController.showDoneButton = YES;
        [self presentModalViewController:aNavController animated:YES];
#ifdef APPLE_DEMO_PROMOTION
        [appSettingsViewController setEnabled:NO forKey:[NSString stringWithUTF8String:OptionStrings::Multiplayer_ServerVisible]];
        [appSettingsViewController setEnabled:NO forKey:[NSString stringWithUTF8String:OptionStrings::Multiplayer_Username]];
#endif
        [self initDialog];
        [aNavController release];
    }
}

- (void)showDialog_RenameMPWorld
{
    if (!_dialog) {
        BOOL isIpad = (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad);
        CGRect screen = [[UIScreen mainScreen] bounds];
        CGFloat width  = MAX(screen.size.width, screen.size.height);
        BOOL isIphone5 = (width == 568);
        NSString* xib = isIpad ? @"RenameMPWorld_ipad" : (isIphone5? @"RenameMPWorld_iphone5" : @"RenameMPWorld_iphone");
        _dialog = [[RenameMPWorldViewController alloc] initWithNibName:xib bundle:[NSBundle mainBundle]];
        [self presentModalViewController:_dialog animated:YES];
        [self initDialog];
        //NSLog(@"--- %p %p\n", _dialog, [self view]);
        //[_dialog addToView:[self view]];
    }
}

- (int) getUserInputStatus {
    //NSLog(@"getUserI: %d\n", _dialogResultStatus);
    return _dialogResultStatus;
}

- (std::vector<std::string>)getUserInput {
    //LOGI("-----\n");
    //for (int i = 0; i < _dialogResultStrings.size(); i += 2) {
    //    LOGI("%d: %s, %s\n", i/2, _dialogResultStrings[i].c_str(), _dialogResultStrings[i+1].c_str()); 
    //}
    return _dialogResultStrings;
}

//
// IASK Delegate interface
//

// @note: See StringIfEmpty option (added by me) in IASK as well, that one's triggered
//        earlier. This one is more of a fail-safe to prevent an empty name
NSString* DefaultUsername = @"Stevie";

- (void)settingsViewControllerDidEnd:(IASKAppSettingsViewController*)sender {
    [self dismissModalViewControllerAnimated:YES];

    NSString* Key = [[NSString alloc] initWithUTF8String:OptionStrings::Multiplayer_Username];
    NSString* username = [[NSUserDefaults standardUserDefaults] objectForKey:Key];
    if ([username isEqualToString:@""]) {
        [[NSUserDefaults standardUserDefaults] setObject:DefaultUsername forKey:Key];
    }

    [Key release];

    _dialogResultStatus = 1;
}

- (void)settingToggled:(NSNotification *) notification {
    NSString* name = [notification object];
    NSString* KeyLowQuality = [[NSString alloc] initWithUTF8String:OptionStrings::Graphics_LowQuality];
    NSString* KeyFancy      = [[NSString alloc] initWithUTF8String:OptionStrings::Graphics_Fancy];

    if ([name isEqualToString:KeyLowQuality]) {
         //NSLog(@"Notification: %d %@ %@\n", enabled, [[notification userInfo] description], [[[notification userInfo] objectForKey:name] class]);

        BOOL enabled = ! [[[notification userInfo] objectForKey:name] boolValue];
        if (!enabled) {
            [appSettingsViewController getSwitch:KeyFancy].on = NO;
        }
        //[appSettingsViewController setEnabled:enabled forKey:KeyFancy];
    }
    if ([name isEqualToString:KeyFancy]) {
        BOOL enabled = ! [[[notification userInfo] objectForKey:name] boolValue];
        if (!enabled) {
            [appSettingsViewController getSwitch:KeyLowQuality].on = NO;
        }
    }
    
    [KeyLowQuality release];
    [KeyFancy release];
}

+ (void) initialize {
    if ([self class] == [minecraftpeViewController class]) {
        NSDictionary* defaults = [NSDictionary dictionaryWithObjectsAndKeys:
            DefaultUsername,                [NSString stringWithUTF8String:OptionStrings::Multiplayer_Username],
            [NSNumber numberWithBool:YES],  [NSString stringWithUTF8String:OptionStrings::Multiplayer_ServerVisible],
            [NSNumber numberWithBool:NO],   [NSString stringWithUTF8String:OptionStrings::Graphics_Fancy],
            [NSNumber numberWithBool:NO],   [NSString stringWithUTF8String:OptionStrings::Graphics_LowQuality],
            [NSNumber numberWithFloat:0.5f],[NSString stringWithUTF8String:OptionStrings::Controls_Sensitivity],
            [NSNumber numberWithBool:NO],   [NSString stringWithUTF8String:OptionStrings::Controls_InvertMouse],
            [NSNumber numberWithBool:NO],   [NSString stringWithUTF8String:OptionStrings::Controls_IsLefthanded],
            [NSNumber numberWithBool:NO],   [NSString stringWithUTF8String:OptionStrings::Controls_UseTouchJoypad],
            @"2",   [NSString stringWithUTF8String:OptionStrings::Game_DifficultyLevel],
            nil];
        [[NSUserDefaults standardUserDefaults] registerDefaults:defaults];
    }
}

- (void)showKeyboard {
    for(UIView* view in self.view.subviews) {
        if([view isKindOfClass:[ShowKeyboardView class]]) {
            ShowKeyboardView* kview = (ShowKeyboardView*) view;
            [kview showKeyboard];
            return;
        }
    }
    [self.view insertSubview:_keyboardView atIndex:0];
    [_keyboardView showKeyboard];
}
- (void)hideKeyboard {
    for(UIView* view in self.view.subviews) {
        if([view isKindOfClass:[ShowKeyboardView class]]) {
            ShowKeyboardView* kview = (ShowKeyboardView*) view;
            [kview hideKeyboard];
            [kview removeFromSuperview];
        }
    }
}

@end
