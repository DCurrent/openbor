//
//  GamepadButtonView.h
//  OpenBOR
//
//  Created by Yoshi Sugawara on 12/27/19.
//

#import <UIKit/UIKit.h>
#import "openbor.h"

NS_ASSUME_NONNULL_BEGIN

@class GamepadButtonView;
@protocol GamepadButtonDelegate <NSObject>
@required
-(void)gamepadButtonPressed:(GamepadButtonView*)button;
-(void)gamepadButtonReleased:(GamepadButtonView*)button;
@end

@interface GamepadButtonView : UIView

@property(weak,nonatomic) id<GamepadButtonDelegate> delegate;
@property(nonatomic,assign) e_key_id identifier;

-(instancetype)initWithIdentifier:(e_key_id)identifier;
-(void)setupButtonLabel;

@end

NS_ASSUME_NONNULL_END
