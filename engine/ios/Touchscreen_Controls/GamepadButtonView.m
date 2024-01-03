//
//  GamepadButtonView.m
//  OpenBOR
//
//  Created by Yoshi Sugawara on 12/27/19.
//

#import "GamepadButtonView.h"

@interface GamepadButtonView()
@property(strong,nonatomic) UIImageView *imageView;
@property(strong,nonatomic) UILabel *buttonName;
@end

@implementation GamepadButtonView

-(instancetype)initWithIdentifier:(e_key_id)identifier {
    self = [self initWithFrame:CGRectZero];
    self.identifier = identifier;
    [self setupButtonLabel];
    return self;
}

-(instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    [self commonInit];
    return self;
}

-(instancetype)initWithCoder:(NSCoder *)coder {
    self = [super initWithCoder:coder];
    [self commonInit];
    return self;
}

-(void)commonInit {
    self.translatesAutoresizingMaskIntoConstraints = NO;
//    self.backgroundColor = UIColor.blueColor;
//    self.alpha = 0.7f;
//    self.layer.borderColor = UIColor.blueColor.CGColor;
//    self.layer.borderWidth = 2.0;
    self.clipsToBounds = NO;
    [self setUserInteractionEnabled:YES];
    [[self.widthAnchor constraintEqualToConstant:50.0] setActive:YES];
    [[self.widthAnchor constraintEqualToAnchor:self.heightAnchor] setActive:YES];
    self.imageView = [[UIImageView alloc] initWithFrame:CGRectZero];
    self.imageView.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:self.imageView];
    [self.imageView.leadingAnchor constraintEqualToAnchor:self.leadingAnchor].active = YES;
    [self.imageView.trailingAnchor constraintEqualToAnchor:self.trailingAnchor].active = YES;
    [self.imageView.topAnchor constraintEqualToAnchor:self.topAnchor].active = YES;
    [self.imageView.bottomAnchor constraintEqualToAnchor:self.bottomAnchor].active = YES;
    self.imageView.image = [UIImage imageNamed:@"button"];
    self.buttonName = [[UILabel alloc] initWithFrame:CGRectZero];
    self.buttonName.translatesAutoresizingMaskIntoConstraints = NO;
    self.buttonName.textColor = UIColor.blueColor;
    self.buttonName.font = [UIFont systemFontOfSize:9.0f weight:UIFontWeightBold];
    [self addSubview:self.buttonName];
    [self.buttonName.centerXAnchor constraintEqualToAnchor:self.centerXAnchor].active = YES;
    [self.buttonName.centerYAnchor constraintEqualToAnchor:self.centerYAnchor].active = YES;
}

-(void)setupButtonLabel {
    switch (self.identifier) {
        case SDID_ATTACK:
            self.buttonName.text = @"A1";
            break;
        case SDID_ATTACK2:
            self.buttonName.text = @"A2";
            break;
        case SDID_ATTACK3:
            self.buttonName.text = @"A3";
            break;
        case SDID_ATTACK4:
            self.buttonName.text = @"A4";
            break;
        case SDID_SPECIAL:
            self.buttonName.text = @"SP";
            break;
        case SDID_JUMP:
            self.buttonName.text = @"JMP";
            break;
        case SDID_START:
            self.buttonName.text = @"START";
            break;
        case SDID_ESC:
            self.buttonName.text = @"ESC";
            break;

        default:
            break;
    }
}

-(void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self.delegate gamepadButtonPressed:self];
    self.imageView.image = [UIImage imageNamed:@"button-pressed"];
}

-(void) touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self.delegate gamepadButtonPressed:self];
    self.imageView.image = [UIImage imageNamed:@"button-pressed"];
}

-(void) touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self.delegate gamepadButtonReleased:self];
    self.imageView.image = [UIImage imageNamed:@"button"];
}


@end
