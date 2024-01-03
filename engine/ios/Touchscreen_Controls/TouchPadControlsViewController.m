//
//  TouchPadControlsViewController.m
//  OpenBOR
//
//  Created by Yoshi Sugawara on 12/7/19.
//

#import "TouchPadControlsViewController.h"
#import "VirtualControlsView.h"
#import <GameController/GameController.h>

#include "ios-glue.h"

@interface TouchPadControlsViewController ()<DPadDelegate, GamepadButtonDelegate>
@property(nonatomic,strong) VirtualControlsView *virtualControlsView;
@end

@implementation TouchPadControlsViewController

-(VirtualControlsView*)virtualControlsView {
    if (_virtualControlsView == nil) {
        _virtualControlsView = [[VirtualControlsView alloc] init];
    }
    return _virtualControlsView;
}

-(void)loadView {
    self.view = [self virtualControlsView];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    [self virtualControlsView].dPadView.delegate = self;
    self.virtualControlsView.buttonAttack1.delegate = self;
    self.virtualControlsView.buttonAttack2.delegate = self;
    self.virtualControlsView.buttonAttack3.delegate = self;
    self.virtualControlsView.buttonAttack4.delegate = self;
    self.virtualControlsView.buttonJump.delegate = self;
    self.virtualControlsView.buttonSpecial.delegate = self;
    self.virtualControlsView.buttonEsc.delegate = self;
    self.virtualControlsView.buttonStart.delegate = self;
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateTouchControls:) name:GCControllerDidConnectNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateTouchControls:) name:GCControllerDidDisconnectNotification object:nil];
    
    self.view.alpha = 0.7f;
}

-(void)updateTouchControls:(NSNotification*)notification {
    self.virtualControlsView.hidden = [GCController controllers].count > 0;
}

-(void)updateTouchControlsVisibility:(BOOL)doHide {
    self.virtualControlsView.hidden = doHide;
}

#pragma mark - DPadDelegate

- (void)dPad:(nonnull DPadView *)dpad didPress:(DPadDirection)direction {
    switch (direction) {
        case kUp:
            ios_touchstates[SDID_MOVEUP] = 1;
            break;
        case kUpLeft:
            ios_touchstates[SDID_MOVEUP] = 1;
            ios_touchstates[SDID_MOVELEFT] = 1;
            break;
        case kUpRight:
            ios_touchstates[SDID_MOVEUP] = 1;
            ios_touchstates[SDID_MOVERIGHT] = 1;
            break;
        case kLeft:
            ios_touchstates[SDID_MOVELEFT] = 1;
            break;
        case kNone:
            ios_touchstates[SDID_MOVEUP] = 0;
            ios_touchstates[SDID_MOVELEFT] = 0;
            ios_touchstates[SDID_MOVERIGHT] = 0;
            ios_touchstates[SDID_MOVEDOWN] = 0;
            break;
        case kRight:
            ios_touchstates[SDID_MOVERIGHT] = 1;
            break;
        case kDownLeft:
            ios_touchstates[SDID_MOVELEFT] = 1;
            ios_touchstates[SDID_MOVEDOWN] = 1;
            break;
        case kDown:
            ios_touchstates[SDID_MOVEDOWN] = 1;
            break;
        case kDownRight:
            ios_touchstates[SDID_MOVEDOWN] = 1;
            ios_touchstates[SDID_MOVERIGHT] = 1;
            break;
        default:
            ios_touchstates[SDID_MOVEUP] = 0;
            ios_touchstates[SDID_MOVELEFT] = 0;
            ios_touchstates[SDID_MOVERIGHT] = 0;
            ios_touchstates[SDID_MOVEDOWN] = 0;
            
            break;
    }
}

- (void)dPadDidRelease:(nonnull DPadView *)dpad {
    ios_touchstates[SDID_MOVEUP] = 0;
    ios_touchstates[SDID_MOVELEFT] = 0;
    ios_touchstates[SDID_MOVERIGHT] = 0;
    ios_touchstates[SDID_MOVEDOWN] = 0;
}

#pragma mark - GamepadButtonDelegate

- (void)gamepadButtonPressed:(GamepadButtonView *)button {
    switch (button.identifier) {
        case SDID_ATTACK:
            ios_touchstates[SDID_ATTACK] = 1;
            break;
        case SDID_ATTACK2:
            ios_touchstates[SDID_ATTACK2] = 1;
            break;
        case SDID_ATTACK3:
            ios_touchstates[SDID_ATTACK3] = 1;
            break;
        case SDID_ATTACK4:
            ios_touchstates[SDID_ATTACK4] = 1;
            break;
        case SDID_SPECIAL:
            ios_touchstates[SDID_SPECIAL] = 1;
            break;
        case SDID_JUMP:
            ios_touchstates[SDID_JUMP] = 1;
            break;
        case SDID_START:
            ios_touchstates[SDID_START] = 1;
            break;
        case SDID_ESC:
            ios_touchstates[SDID_ESC] = 1;
            break;
            
        default:
            break;
    }
}

-(void) gamepadButtonReleased:(GamepadButtonView *)button {
    switch (button.identifier) {
        case SDID_ATTACK:
            ios_touchstates[SDID_ATTACK] = 0;
            break;
        case SDID_ATTACK2:
            ios_touchstates[SDID_ATTACK2] = 0;
            break;
        case SDID_ATTACK3:
            ios_touchstates[SDID_ATTACK3] = 0;
            break;
        case SDID_ATTACK4:
            ios_touchstates[SDID_ATTACK4] = 0;
            break;
        case SDID_SPECIAL:
            ios_touchstates[SDID_SPECIAL] = 0;
            break;
        case SDID_JUMP:
            ios_touchstates[SDID_JUMP] = 0;
            break;
        case SDID_START:
            ios_touchstates[SDID_START] = 0;
            break;
        case SDID_ESC:
            ios_touchstates[SDID_ESC] = 0;
            break;
            
        default:
            break;
    }
}

@end
