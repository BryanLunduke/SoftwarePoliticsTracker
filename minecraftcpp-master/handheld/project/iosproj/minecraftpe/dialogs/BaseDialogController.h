//
//  BaseDialogController.h
//  minecraftpe
//
//  Created by rhino on 10/20/11.
//  Copyright 2011 Mojang. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "IDialog.h"

@class minecraftpeViewController;

@interface BaseDialogController : UIViewController<IDialog>
{
    int _status;
    std::vector<std::string> _strings;
    
    minecraftpeViewController* _listener;
}

// Interface
-(void)addToView:(UIView*)parentView;
-(int) getUserInputStatus;
-(std::vector<std::string>)getUserInput;
-(void)setListener:(minecraftpeViewController*)listener;

// Helpers
-(void)setOk;
-(void)setCancel;
-(void)closeOk;
-(void)closeCancel;
-(void)addString:(std::string)s;

@end
