//
//  DPadView.h
//  OpenBOR
//
//  Created by Yoshi Sugawara on 12/7/19.
//

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, DPadDirection) {
    kUpLeft,
    kUp,
    kUpRight,
    kLeft,
    kNone,
    kRight,
    kDownLeft,
    kDown,
    kDownRight
};

@class DPadView;
@protocol DPadDelegate
@required
-(void)dPad:(DPadView*)dpad didPress:(DPadDirection)direction;
-(void)dPadDidRelease:(DPadView*)dpad;
@end

@interface DPadView : UIView
@property(weak,nonatomic) id<DPadDelegate> delegate;
@end

NS_ASSUME_NONNULL_END
