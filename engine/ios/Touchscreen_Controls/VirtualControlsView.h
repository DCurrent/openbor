//
//  VirtualControlsView.h
//  OpenBOR
//
//  Created by Yoshi Sugawara on 12/7/19.
//

#import <UIKit/UIKit.h>
#import "DPadView.h"
#import "GamepadButtonView.h"

NS_ASSUME_NONNULL_BEGIN

@interface VirtualControlsView : UIView
@property(nonatomic,readonly) DPadView *dPadView;
@property(nonatomic,readonly) GamepadButtonView *buttonAttack1;
@property(nonatomic,readonly) GamepadButtonView *buttonJump;
@property(nonatomic,readonly) GamepadButtonView *buttonSpecial;
@property(nonatomic,readonly) GamepadButtonView *buttonAttack2;
@property(nonatomic,readonly) GamepadButtonView *buttonAttack3;
@property(nonatomic,readonly) GamepadButtonView *buttonAttack4;
@property(nonatomic,readonly) GamepadButtonView *buttonStart;
@property(nonatomic,readonly) GamepadButtonView *buttonEsc;
@property(nonatomic,readonly) GamepadButtonView *buttonScreenshot;
@end

NS_ASSUME_NONNULL_END
