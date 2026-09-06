//
//  CreateNewWorldViewController.h
//  minecraftpe
//
//  Created by rhino on 10/20/11.
//  Copyright 2011 Mojang AB. All rights reserved.
//

#import "BaseDialogController.h"

@interface CreateNewWorldViewController : BaseDialogController<UITextFieldDelegate>
{
    IBOutlet UILabel* _labelName;
    IBOutlet UILabel* _labelSeed;
    IBOutlet UILabel* _labelSeedHint;
    
    IBOutlet UILabel* _labelGameMode;
    IBOutlet UILabel* _labelGameModeDesc;

    IBOutlet UITextField* _textName;
    IBOutlet UITextField* _textSeed;
    
    IBOutlet UIButton* _btnGameMode;
    
    int _currentGameModeId;
}

- (void) UpdateGameModeDesc;

- (IBAction)DismissKeyboard;

- (IBAction)Create;
- (IBAction)Cancel;
- (IBAction)ToggleGameMode; 

@end
