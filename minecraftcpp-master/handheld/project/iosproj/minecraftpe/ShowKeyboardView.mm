//
//  ShowKeyboardView.m
//  minecraftpe
//
//  Created by Johan Bernhardsson on 12/17/12.
//
//

#import "ShowKeyboardView.h"
#import "../../../src/platform/log.h"
#include "../../../src/platform/input/Keyboard.h"
#include <string>

@implementation ShowKeyboardView

- (id)initWithFrame:(CGRect)frame {
    id returnId = [super initWithFrame:frame];
    textField = [[UITextField alloc] init];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(textFieldDidChange:) name:@"UITextFieldTextDidChangeNotification" object:textField];
    [textField setDelegate:self];
    [self addSubview:textField];
    textField.text = lastString = @"AAAAAAAAAAAAAAAAAAAA";
    return returnId;
}

/*- (void)insertText:(NSString *)text {
    const char* cText = [text cStringUsingEncoding:[NSString defaultCStringEncoding]];
    //std::string textString([text cStringUsingEncoding:[NSString defaultCStringEncoding]]);
    int strLength = strlen(cText);
    for(int a = 0; a < strLength; ++a) {
        LOGW("NewCharInput: %c (%d)\n", cText[a], cText[a]);
        if(cText[a] == 0 || cText[a] == '\n') {
            Keyboard::feed((char)Keyboard::KEY_RETURN, 1);
            Keyboard::feed((char)Keyboard::KEY_RETURN, 0);
        } else {
            Keyboard::feedText(cText[a]);
        }
    }
    //delete cText;
    //LOGW("Insert text: %s\n", [text cStringUsingEncoding:[NSString defaultCStringEncoding]]);
}

- (void)deleteBackward {
    //LOGW("deleteBackward\n");
    //Keyboard::feed((char)Keyboard::KEY_BACKSPACE, 1);
    //Keyboard::feed((char)Keyboard::KEY_BACKSPACE, 0);§
}*/

- (BOOL)hasText {
    return YES;
}

- (BOOL)canBecomeFirstResponder {
    return YES;
}

- (void)showKeyboard {
    //[self becomeFirstResponder];
    [textField becomeFirstResponder];
    //[self becomeFirstResponder];
}

- (void)hideKeyboard {
    [textField resignFirstResponder];
    [self resignFirstResponder];
}

- (void)textFieldDidChange :(NSNotification *)notif {
    UITextField* txt = (UITextField*)notif.object;
    if(![txt.text isEqualToString:lastString]) {
        if(lastString.length > txt.text.length) {
            Keyboard::feed((char)Keyboard::KEY_BACKSPACE, 1);
            Keyboard::feed((char)Keyboard::KEY_BACKSPACE, 0);
        } else if([txt.text characterAtIndex:(txt.text.length - 1)] == '\n') {
            Keyboard::feed((char)Keyboard::KEY_RETURN, 1);
            Keyboard::feed((char)Keyboard::KEY_RETURN, 0);
        } else {
            Keyboard::feedText([txt.text characterAtIndex:(txt.text.length - 1)]);
        }
        textField.text = lastString = @"AAAAAAAAAAAAAAAAAAAA";
    }
}
- (BOOL)textFieldShouldReturn:(UITextField *)theTextField {
    Keyboard::feed((char)Keyboard::KEY_RETURN, 1);
    Keyboard::feed((char)Keyboard::KEY_RETURN, 0);
    return NO;
}
@end
