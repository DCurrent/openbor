//
//  IOSUtils.m
//  xm8-ios
//
//  Created by Yoshi Sugawara on 12/12/16.
//
//

#import "IOSUtils.h"
#import <UIKit/UIKit.h>
#import "TouchPadControlsViewController.h"
#import <GameController/GameController.h>

#include "ios-glue.h"

unsigned ios_touchstates[MAXTOUCHB];
TouchPadControlsViewController *touchPadController;

UIViewController* GetSDLViewController(SDL_Window *sdlWindow) {
    SDL_SysWMinfo systemWindowInfo;
    SDL_VERSION(&systemWindowInfo.version);
    if ( ! SDL_GetWindowWMInfo(sdlWindow, &systemWindowInfo)) {
        // error handle?
        return nil;
    }
    UIWindow *appWindow = systemWindowInfo.info.uikit.window;
    UIViewController *rootVC = appWindow.rootViewController;
    return rootVC;
}

void ios_after_window_create(SDL_Window *window) {
    UIViewController *rootVC = GetSDLViewController(window);
    TouchPadControlsViewController *padController = [[TouchPadControlsViewController alloc] init];
    touchPadController = padController;
    [rootVC addChildViewController:padController];
    [rootVC.view addSubview:padController.view];
    [padController didMoveToParentViewController:rootVC];
    [rootVC.view.leadingAnchor constraintEqualToAnchor:padController.view.leadingAnchor].active = YES;
    [rootVC.view.trailingAnchor constraintEqualToAnchor:padController.view.trailingAnchor].active = YES;
    [rootVC.view.topAnchor constraintEqualToAnchor:padController.view.topAnchor].active = YES;
    [rootVC.view.bottomAnchor constraintEqualToAnchor:padController.view.bottomAnchor].active = YES;
    [rootVC.view bringSubviewToFront:padController.view];
}

bool ios_controller_connected() {
    return [GCController controllers].count > 0;
}

void update_touch_controls_visibility(bool doHide) {
    if ( touchPadController == nil ) {
        return;
    }
    [touchPadController updateTouchControlsVisibility:doHide];
}

void ios_get_base_path(char *path) {
    NSArray *paths;
    NSString *DocumentsDirPath;
    
    paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    DocumentsDirPath = [paths objectAtIndex:0];
    sprintf(path, "%s/", [DocumentsDirPath UTF8String]);
    
}

void ios_get_screen_width_height(int *width, int *height) {
    CGRect screenRect = [[UIScreen mainScreen] bounds];
    *height = (int) screenRect.size.width;
    *width = (int) screenRect.size.height;
}

@implementation IOSUtils


@end

