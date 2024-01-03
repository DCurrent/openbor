//
//  VirtualControlsView.m
//  OpenBOR
//
//  Created by Yoshi Sugawara on 12/7/19.
//

#import "VirtualControlsView.h"
#import "DPadView.h"

@interface VirtualControlsView()
@property(nonatomic) NSLayoutConstraint *dPadWidthConstraint;
@property(nonatomic) NSLayoutConstraint *dPadHeightConstraint;
@property(nonatomic) NSLayoutConstraint *buttonWidthConstraint;
@end

@implementation VirtualControlsView

-(instancetype) init {
    self = [self initWithFrame:CGRectZero];
    return self;
}

-(instancetype) initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    [self commonInit];
    return self;
}

-(instancetype) initWithCoder:(NSCoder *)coder {
    self = [super initWithCoder:coder];
    [self commonInit];
    return self;
}

-(void)commonInit {
    _dPadView = [[DPadView alloc] initWithFrame:CGRectZero];
    self.translatesAutoresizingMaskIntoConstraints = NO;
    self.backgroundColor = UIColor.clearColor;
    [self addSubview:_dPadView];
    [[_dPadView.leadingAnchor constraintEqualToAnchor:self.leadingAnchor constant:5.0f] setActive:YES];
    [[_dPadView.bottomAnchor constraintEqualToAnchor:self.bottomAnchor constant:-10.0f] setActive:YES];
    _dPadWidthConstraint = [_dPadView.widthAnchor constraintEqualToConstant:0.0];
    _dPadHeightConstraint = [_dPadView.heightAnchor constraintEqualToConstant:0.0];
    [self setupButtons];
}

-(void)setupButtons {
    _buttonAttack1 = [[GamepadButtonView alloc] initWithIdentifier:SDID_ATTACK];
    _buttonJump = [[GamepadButtonView alloc] initWithIdentifier:SDID_JUMP];
    _buttonAttack2 = [[GamepadButtonView alloc] initWithIdentifier:SDID_ATTACK2];
    _buttonAttack3 = [[GamepadButtonView alloc] initWithIdentifier:SDID_ATTACK3];
    _buttonAttack4 = [[GamepadButtonView alloc] initWithIdentifier:SDID_ATTACK4];
    _buttonSpecial = [[GamepadButtonView alloc] initWithIdentifier:SDID_SPECIAL];
    _buttonStart = [[GamepadButtonView alloc] initWithIdentifier:SDID_START];
    _buttonEsc = [[GamepadButtonView alloc] initWithIdentifier:SDID_ESC];
        
    [self addSubview:_buttonAttack1];
    [self addSubview:_buttonJump];
    [self addSubview:_buttonAttack2];
    [self addSubview:_buttonAttack3];
    [self addSubview:_buttonAttack4];
    [self addSubview:_buttonSpecial];
    [self addSubview:_buttonStart];
    [self addSubview:_buttonEsc];
    
    // 2 3 4
    // 1 J S
    [[_buttonSpecial.trailingAnchor constraintEqualToAnchor:self.trailingAnchor constant:-10.0f] setActive:YES];
    [[_buttonSpecial.bottomAnchor constraintEqualToAnchor:self.bottomAnchor constant:-20.0f] setActive:YES];
    [[_buttonJump.centerYAnchor constraintEqualToAnchor:_buttonSpecial.centerYAnchor] setActive:YES];
    [[_buttonJump.trailingAnchor constraintEqualToAnchor:_buttonSpecial.leadingAnchor constant:-10.0f] setActive:YES];
    [[_buttonAttack1.centerYAnchor constraintEqualToAnchor:_buttonSpecial.centerYAnchor] setActive:YES];
    [[_buttonAttack1.trailingAnchor constraintEqualToAnchor:_buttonJump.leadingAnchor constant:-10.0f] setActive:YES];
    
    [[_buttonEsc.leadingAnchor constraintEqualToAnchor:self.leadingAnchor constant:20.0f] setActive:YES];
    [[_buttonEsc.topAnchor constraintEqualToAnchor:self.topAnchor constant:20.0f] setActive:YES];
    [[_buttonStart.trailingAnchor constraintEqualToAnchor:self.trailingAnchor constant:-20.0f] setActive:YES];
    [[_buttonStart.topAnchor constraintEqualToAnchor:self.topAnchor constant:20.0f] setActive:YES];
    
    [[_buttonAttack4.trailingAnchor constraintEqualToAnchor:_buttonSpecial.trailingAnchor] setActive:YES];
    [[_buttonAttack4.bottomAnchor constraintEqualToAnchor:_buttonSpecial.topAnchor constant:-10.0f] setActive:YES];
    [[_buttonAttack3.centerYAnchor constraintEqualToAnchor:_buttonAttack4.centerYAnchor] setActive:YES];
    [[_buttonAttack3.trailingAnchor constraintEqualToAnchor:_buttonAttack4.leadingAnchor constant:-10.0f] setActive:YES];
    [[_buttonAttack2.centerYAnchor constraintEqualToAnchor:_buttonAttack3.centerYAnchor] setActive:YES];
    [[_buttonAttack2.trailingAnchor constraintEqualToAnchor:_buttonAttack3.leadingAnchor constant:-10.0f] setActive:YES];
}

-(void)layoutSubviews {
    [super layoutSubviews];
    CGFloat size = self.bounds.size.width * 0.25f;
    [_dPadWidthConstraint setActive:NO];
    _dPadWidthConstraint.constant = size;
    [_dPadWidthConstraint setActive:YES];
    [_dPadHeightConstraint setActive:NO];
    _dPadHeightConstraint.constant = size;
    [_dPadHeightConstraint setActive:YES];
}

@end
