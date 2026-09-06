//
//  BaseDialogController.m
//  minecraftpe
//
//  Created by rhino on 10/20/11.
//  Copyright 2011 Mojang AB. All rights reserved.
//

#import "BaseDialogController.h"
#import "../minecraftpeViewController.h"

@implementation BaseDialogController

- (id)init
{
    self = [super init];
    if (self) {
        // Initialization code here.
        _status = -1;
        _listener = nil;
    }
    
    return self;
}

//
// Interface
//
- (void)addToView:(UIView*)parentView {
	
	// Add this view as a subview of EAGLView
	[parentView addSubview:self.view];
    
	// ...then fade it in using core animation
	[UIView beginAnimations:nil context:NULL];
	//self.view.alpha = 1.0f;
	[UIView commitAnimations];
}

- (int) getUserInputStatus { return _status; }

-(std::vector<std::string>)getUserInput
{
    return _strings;
}

-(void)setListener:(minecraftpeViewController*)listener
{
    _listener = listener;
}

//
// Helpers
//
- (void) setOk {
    _status = 1;
}

- (void) setCancel {
    _status = 0;
}

- (void) closeOk {
    [self setOk];
    NSLog(@"Close dialog %p\n", _listener);
    [_listener closeDialog];
}

- (void) closeCancel {
    [self setCancel];
    [_listener closeDialog];
}

- (void) addString: (std::string) s {
    _strings.push_back(s);
}

@end
