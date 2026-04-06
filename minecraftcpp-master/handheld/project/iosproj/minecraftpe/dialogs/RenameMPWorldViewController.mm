//
//  RenameMPWorldViewController.m
//  minecraftpe
//
//  Created by rhino on 10/20/11.
//  Copyright 2011 Mojang. All rights reserved.
//

#import "RenameMPWorldViewController.h"
#import <QuartzCore/QuartzCore.h>

@implementation RenameMPWorldViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    //NSLog(@"TextField should return\n");
    [textField setUserInteractionEnabled:YES];
    [textField resignFirstResponder];
    if (textField == _textName)
        [self Create];
    return YES;
}

- (void) resizeView:(UIView*)obj width:(int)w height:(int)h {
    if (w < 0) w = obj.frame.size.width;
    if (h < 0) h = obj.frame.size.height;
    obj.frame = CGRectMake(obj.frame.origin.x, obj.frame.origin.y, w, h);
}

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
    if (textField == _textName) {
        NSUInteger newLength = [textField.text length] + [string length] - range.length;
        if (newLength > 18)
            return NO;
    }

    int length = [string length];
    for (int i = 0; i < length; ++i) {
        unichar ch = [string characterAtIndex:i];

        if (ch >= 128)
            return NO;
    }
    return YES;
}

// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad
{
    [super viewDidLoad];
    BOOL isIpad = (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad);

    UIFont* fontLarge = nil;
    UIFont* fontSmall = nil;

    if (isIpad) {
        [self resizeView:_textName width:-1 height:48];
        self.view.backgroundColor = [UIColor colorWithPatternImage: [UIImage imageNamed:@"bg128.png"] ];
        fontLarge = [UIFont fontWithName:@"minecraft" size:24];
        fontSmall = [UIFont fontWithName:@"minecraft" size:18];
    } else {
        [self resizeView:_textName width:-1 height:32];
        self.view.backgroundColor = [UIColor colorWithPatternImage: [UIImage imageNamed:@"bg64.png"] ];
        fontLarge = [UIFont fontWithName:@"minecraft" size:16];
        fontSmall = [UIFont fontWithName:@"minecraft" size: 14];
    }

    UIView *paddingView = [[[UIView alloc] initWithFrame:CGRectMake(0, 0, 4, 20)] autorelease];
    _textName.leftView = paddingView;

    [_labelName     setFont:fontLarge];
    
    [_textName      setFont:fontLarge];
    
    _textName.layer.borderColor = [[UIColor whiteColor] CGColor];
    _textName.layer.borderWidth = 2.0f;       

    _textName.delegate = self;
    [_textName becomeFirstResponder];
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return UIInterfaceOrientationIsLandscape(interfaceOrientation);
}

- (IBAction)Create {
    //NSLog(@"I'm done!");
    // Push the strings
    [self addString: [[_textName text] UTF8String]];
    [self closeOk];
}

- (IBAction)Cancel {
    //NSLog(@"I'm cancelled!");
    [self closeCancel];
}

- (IBAction)DismissKeyboard {
    //NSLog(@"Trying to dismiss keyboard %p %p\n", _textName, _textSeed);
    [_textName resignFirstResponder];
}

@end
