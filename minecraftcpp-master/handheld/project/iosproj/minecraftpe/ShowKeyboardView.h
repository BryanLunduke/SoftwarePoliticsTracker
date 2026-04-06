//
//  ShowKeyboardView.h
//  minecraftpe
//
//  Created by Johan Bernhardsson on 12/17/12.
//
//

#import <UIKit/UIKit.h>
@interface ShowKeyboardView : UIView</*UIKeyInput, */UITextFieldDelegate> {
    UITextField* textField;
    NSString* lastString;
}
- (id)initWithFrame:(CGRect)frame;
//- (void)insertText:(NSString *)text;
//- (void)deleteBackward;
- (BOOL)hasText;
- (BOOL)canBecomeFirstResponder;
- (void)showKeyboard;
- (void)hideKeyboard;
- (void)textFieldDidChange :(NSNotification *)notif;
- (BOOL)textFieldShouldReturn:(UITextField *)theTextField;
@end
