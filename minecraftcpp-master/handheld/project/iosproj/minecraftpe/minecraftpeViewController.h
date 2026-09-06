//
//  minecraftpeViewController.h
//  minecraftpe
//
//  Created by rhino on 10/17/11.
//  Copyright 2011 Mojang AB. All rights reserved.
//

#import <UIKit/UIKit.h>

#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import "../../../src/App.h"
#import "../../../src/AppPlatform_iOS.h"
#import "../../../src/NinecraftApp.h"
#import "ShowKeyboardView.h"

#import "../../lib_projects/InAppSettingsKit/Controllers/IASKAppSettingsViewController.h"

@class EAGLContext;
//@class App;
//@class AppContext;
//@class AppPlatform_iOS;
@class BaseDialogController;

@interface minecraftpeViewController : UIViewController<IASKSettingsDelegate> {
    EAGLContext *context;
    
    // App and AppPlatform
    App* _app;
    AppContext* _context;
    AppPlatform_iOS* _platform;   
    
    UITouch** _touchMap;
    
    BOOL animating;
    NSInteger animationFrameInterval;
    CADisplayLink *displayLink;
    
    BaseDialogController* _dialog;
    
    int _dialogResultStatus;
    std::vector<std::string> _dialogResultStrings;
    
    ShowKeyboardView* _keyboardView;

    @public
    float viewScale;
}

@property (readonly, nonatomic, getter=isAnimating) BOOL animating;
@property (nonatomic) NSInteger animationFrameInterval;

@property (nonatomic, retain) IASKAppSettingsViewController *appSettingsViewController;


- (void)startAnimation;
- (void)stopAnimation;
- (void)enteredBackground;

- (void)setAudioEnabled:(BOOL)status;

-(int) getUserInputStatus;
-(std::vector<std::string>)getUserInput;

- (void)showDialog_CreateWorld;
- (void)showDialog_MainMenuOptions;
- (void)showDialog_RenameMPWorld;

- (void)showKeyboard;
- (void)hideKeyboard;

- (void) closeDialog;
- (BaseDialogController*) dialog;

+ (void) initialize;

@end
