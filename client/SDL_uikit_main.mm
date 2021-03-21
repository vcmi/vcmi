/*
    SDL_uikit_main.c, placed in the public domain by Sam Lantinga  3/18/2019
*/

/* Include the SDL main definition header */
#include <SDL_main.h>

#include <SDL_events.h>
#include <SDL_render.h>

#include "../Global.h"
#include "CMT.h"
#include "CFocusableHelper.h"

#import <UIKit/UIKit.h>


@interface SDLViewObserver : NSObject
@end

@implementation SDLViewObserver

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context {
    __auto_type longPress = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(handleLongPress:)];
    longPress.minimumPressDuration = 0.1;
    longPress.delaysTouchesBegan = YES; // prevent normal clicks by SDL
    [(UIView *)[object valueForKey:keyPath] addGestureRecognizer:longPress];
}

- (void)handleLongPress:(UIGestureRecognizer *)gesture {
    SDL_EventType mouseButtonType;
    switch (gesture.state)
    {
        case UIGestureRecognizerStateBegan:
            mouseButtonType = SDL_MOUSEBUTTONDOWN;
            break;
        case UIGestureRecognizerStateEnded:
            mouseButtonType = SDL_MOUSEBUTTONUP;
            break;
        default:
            return;
    }

    __auto_type renderer = SDL_GetRenderer(mainWindow);
    float scaleX, scaleY;
    SDL_Rect viewport;
    SDL_RenderGetScale(renderer, &scaleX, &scaleY);
    SDL_RenderGetViewport(renderer, &viewport);

    __auto_type touchedPoint = [gesture locationInView:gesture.view];
    __auto_type screenScale = UIScreen.mainScreen.nativeScale;
    Sint32 x = (int)touchedPoint.x * screenScale / scaleX - viewport.x;
    Sint32 y = (int)touchedPoint.y * screenScale / scaleY - viewport.y;

    SDL_Event rmbEvent;
    rmbEvent.button = (SDL_MouseButtonEvent){
        .type = mouseButtonType,
        .button = SDL_BUTTON_RIGHT,
        .clicks = 1,
        .x = x,
        .y = y,
    };
    SDL_PushEvent(&rmbEvent);

    // small hack to prevent cursor jumping
    if (mouseButtonType == SDL_MOUSEBUTTONUP)
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.025 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            SDL_Event motionEvent;
            motionEvent.motion = (SDL_MouseMotionEvent){
                .type = SDL_MOUSEMOTION,
                .x = x,
                .y = y,
            };
            SDL_PushEvent(&motionEvent);
        });
}

@end


#ifdef main
#undef main
#endif

int
main(int argc, char *argv[])
{
    @autoreleasepool
    {
        __auto_type observer = [SDLViewObserver new];
        [NSNotificationCenter.defaultCenter addObserverForName:UIWindowDidBecomeKeyNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
            [UIApplication.sharedApplication.keyWindow.rootViewController addObserver:observer forKeyPath:NSStringFromSelector(@selector(view)) options:NSKeyValueObservingOptionNew context:NULL];
        }];
        [NSNotificationCenter.defaultCenter addObserverForName:UIKeyboardDidHideNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
            removeFocusFromActiveInput();
        }];
        return SDL_UIKitRunApp(argc, argv, SDL_main);
    }
}
