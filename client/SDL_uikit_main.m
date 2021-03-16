/*
    SDL_uikit_main.c, placed in the public domain by Sam Lantinga  3/18/2019
*/

/* Include the SDL main definition header */
#include "SDL_main.h"

#include "SDL_events.h"

@import UIKit;


@interface SDLViewObserver : NSObject
@end

@implementation SDLViewObserver

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context {
    __auto_type longPress = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(handleLongPress:)];
    longPress.minimumPressDuration = 0.1;
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

    __auto_type touchedPoint = [gesture locationInView:gesture.view];
    SDL_Event event;
    event.button = (SDL_MouseButtonEvent){
        .type = mouseButtonType,
        .button = SDL_BUTTON_RIGHT,
        .clicks = 1,
        .x = touchedPoint.x,
        .y = touchedPoint.y,
    };
    SDL_PushEvent(&event);
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
        return SDL_UIKitRunApp(argc, argv, SDL_main);
    }
}
