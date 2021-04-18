/*
    SDL_uikit_main.c, placed in the public domain by Sam Lantinga  3/18/2019
*/

/* Include the SDL main definition header */
#include <SDL_main.h>

#include <SDL_events.h>
#include <SDL_render.h>

#include "../Global.h"
#include "CMT.h"
#include "CServerHandler.h"
#include "CFocusableHelper.h"

#import "GameChatKeyboardHanlder.h"

#import <UIKit/UIKit.h>

double ios_screenScale() { return UIScreen.mainScreen.nativeScale; }


@interface SDLViewObserver : NSObject <UIGestureRecognizerDelegate>
@property (nonatomic, strong) GameChatKeyboardHanlder * gameChatHandler;
@end

@implementation SDLViewObserver

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context {
    UIView * view = [object valueForKeyPath:keyPath];

    UITextField * textField;
    for (UIView * v in view.subviews) {
        if ([v isKindOfClass:[UITextField class]]) {
            textField = (UITextField *)v;
            break;
        }
    }

	auto r = textField.frame;
	r.size.height = 40;
	textField.frame = r;
    textField.backgroundColor = UIColor.whiteColor;
    self.gameChatHandler.textFieldSDL = textField;

    auto longPress = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(handleLongPress:)];
    longPress.minimumPressDuration = 0.2;
    [view addGestureRecognizer:longPress];

    auto pinch = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(handlePinch:)];
    [view addGestureRecognizer:pinch];
}

#pragma mark - Gestures

- (void)handleLongPress:(UIGestureRecognizer *)gesture {
    // send RMB click
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

    auto renderer = SDL_GetRenderer(mainWindow);
    float scaleX, scaleY;
    SDL_Rect viewport;
    SDL_RenderGetScale(renderer, &scaleX, &scaleY);
    SDL_RenderGetViewport(renderer, &viewport);

    auto touchedPoint = [gesture locationInView:gesture.view];
    auto screenScale = UIScreen.mainScreen.nativeScale;
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

- (void)handlePinch:(UIGestureRecognizer *)gesture {
    if(gesture.state != UIGestureRecognizerStateBegan || CSH->state != EClientState::GAMEPLAY)
        return;
	[self.gameChatHandler triggerInput];
}

#pragma mark - UIGestureRecognizerDelegate

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldBeRequiredToFailByGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
    return [gestureRecognizer isKindOfClass:[UIPinchGestureRecognizer class]];
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
        auto observer = [SDLViewObserver new];
		observer.gameChatHandler = [GameChatKeyboardHanlder new];
        [NSNotificationCenter.defaultCenter addObserverForName:UIWindowDidBecomeKeyNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
            [UIApplication.sharedApplication.keyWindow.rootViewController addObserver:observer forKeyPath:NSStringFromSelector(@selector(view)) options:NSKeyValueObservingOptionNew context:NULL];
        }];
        [NSNotificationCenter.defaultCenter addObserverForName:UITextFieldTextDidEndEditingNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
            removeFocusFromActiveInput();
        }];
        return SDL_UIKitRunApp(argc, argv, SDL_main);
    }
}
