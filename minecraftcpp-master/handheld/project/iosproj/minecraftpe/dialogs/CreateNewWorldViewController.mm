//
//  CreateNewWorldViewController.m
//  minecraftpe
//
//  Created by rhino on 10/20/11.
//  Copyright 2011 Mojang. All rights reserved.
//

#import "CreateNewWorldViewController.h"
#import <QuartzCore/QuartzCore.h>

static const int GameMode_Creative = 0;
static const int GameMode_Survival = 1;
static const char* getGameModeName(int mode) {
    if (mode == GameMode_Survival) return "survival";
    return "creative";
}

@implementation CreateNewWorldViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
        _currentGameModeId = GameMode_Creative;
    }
    return self;
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

- (void) UpdateGameModeDesc
{    
    if (_currentGameModeId == GameMode_Creative) {
        [_labelGameModeDesc setText:@"Unlimited resources, flying"];
        
        UIImage *img = [UIImage imageNamed:@"creative_0_4.png"];
        [_btnGameMode setImage:img forState:UIControlStateNormal];
        UIImage *img2 = [UIImage imageNamed:@"creative_1_4.png"];
        [_btnGameMode setImage:img2 forState:UIControlStateHighlighted];
    }
    if (_currentGameModeId == GameMode_Survival) {
        [_labelGameModeDesc setText:@"Mobs, health and gather resources"];

        UIImage *img = [UIImage imageNamed:@"survival_0_4.png"];
        [_btnGameMode setImage:img forState:UIControlStateNormal];
        UIImage *img2 = [UIImage imageNamed:@"survival_1_4.png"];
        [_btnGameMode setImage:img2 forState:UIControlStateHighlighted];
    }
}

#pragma mark - View lifecycle

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    //NSLog(@"TextField should return\n");
    [textField setUserInteractionEnabled:YES];
    [self DismissKeyboard];
//    [textField resignFirstResponder];
//    if (textField == _textName)
//        [_textSeed becomeFirstResponder];
//    else if (textField == _textSeed)
//        [self Create];
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
        [self resizeView:_textSeed width:-1 height:48];
        self.view.backgroundColor = [UIColor colorWithPatternImage: [UIImage imageNamed:@"bg128.png"] ];
        fontLarge = [UIFont fontWithName:@"minecraft" size:28];
        fontSmall = [UIFont fontWithName:@"minecraft" size:24];
    } else {
        [self resizeView:_textName width:-1 height:32];
        [self resizeView:_textSeed width:-1 height:32];
        self.view.backgroundColor = [UIColor colorWithPatternImage: [UIImage imageNamed:@"bg64.png"] ];
        fontLarge = [UIFont fontWithName:@"minecraft" size:16];
        fontSmall = [UIFont fontWithName:@"minecraft" size: 14];
    }

    UIView *paddingView = [[[UIView alloc] initWithFrame:CGRectMake(0, 0, 4, 20)] autorelease];
    _textName.leftView = paddingView;
    _textSeed.leftView = paddingView;

    [_labelName     setFont:fontLarge];
    [_labelSeed     setFont:fontSmall];
    [_labelSeedHint setFont:fontSmall];
    [_labelGameMode setFont:fontSmall];
    [_labelGameModeDesc setFont:fontSmall];
    
    [_textName      setFont:fontLarge];
    [_textSeed      setFont:fontLarge];
    
    _textName.layer.borderColor = [[UIColor whiteColor] CGColor];
    _textName.layer.borderWidth = 2.0f;       

    _textSeed.layer.borderColor = [[UIColor whiteColor] CGColor];
    _textSeed.layer.borderWidth = 2.0f;       

    _textSeed.delegate = self;
    _textName.delegate = self;

    [self UpdateGameModeDesc];
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
    [self addString: [[_textSeed text] UTF8String]];
    [self addString: getGameModeName(_currentGameModeId)];
    [self closeOk];
}

- (IBAction)Cancel {
    //NSLog(@"I'm cancelled!");
    [self closeCancel];
}

- (IBAction)ToggleGameMode {
    const int NumGameModes = 2;
    if (++_currentGameModeId >= NumGameModes)
        _currentGameModeId = 0;
    
    [self UpdateGameModeDesc];
}

- (IBAction)DismissKeyboard {
    //NSLog(@"Trying to dismiss keyboard %p %p\n", _textName, _textSeed);
    [_textName resignFirstResponder];
    [_textSeed resignFirstResponder];
}

@end
