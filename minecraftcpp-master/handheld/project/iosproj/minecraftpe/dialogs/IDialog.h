//
//  IDialog.h
//  minecraftpe
//
//  Created by rhino on 10/20/11.
//  Copyright 2011 Mojang AB. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <vector>
#import <string>

@class minecraftpeViewController;

@protocol IDialog <NSObject>

-(void)addToView:(UIView*)parentView;
-(void)setListener:(minecraftpeViewController*)listener;

-(int)getUserInputStatus;
-(std::vector<std::string>)getUserInput;

@end
