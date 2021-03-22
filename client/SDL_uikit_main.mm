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

#import <UIKit/UIKit.h>


static int watchReturnKey(void * userdata, SDL_Event * event);

static void sendKeyEvent(SDL_KeyCode keyCode)
{
    SDL_Event keyEvent;
    keyEvent.key = (SDL_KeyboardEvent){
        .type = SDL_KEYDOWN,
        .keysym.sym = keyCode,
    };
    SDL_PushEvent(&keyEvent);
}


@interface SDLViewObserver : NSObject <UIGestureRecognizerDelegate>
@property (nonatomic) bool wasChatMessageSent;
@end

@implementation SDLViewObserver

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context {
    UIView * view = [object valueForKey:keyPath];

    auto longPress = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(handleLongPress:)];
    longPress.minimumPressDuration = 0.1;
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

    // Tab triggers chat message input
    self.wasChatMessageSent = false;
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(handleKeyboardDidShowForGameChat:) name:UIKeyboardDidShowNotification object:nil];
    sendKeyEvent(SDLK_TAB);
}

#pragma mark - Notifications

- (void)handleKeyboardDidShowForGameChat:(NSNotification *)n {
    [NSNotificationCenter.defaultCenter removeObserver:self name:n.name object:nil];

    // watch for pressing Return to ignore sending Escape key after keyboard is closed
    SDL_AddEventWatch(watchReturnKey, (__bridge void *)self);
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(handleKeyboardDidHideForGameChat:) name:UIKeyboardDidHideNotification object:nil];
}

- (void)handleKeyboardDidHideForGameChat:(NSNotification *)n {
    [NSNotificationCenter.defaultCenter removeObserver:self name:n.name object:nil];

    // discard chat message
    if(!self.wasChatMessageSent)
        sendKeyEvent(SDLK_ESCAPE);
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
        [NSNotificationCenter.defaultCenter addObserverForName:UIWindowDidBecomeKeyNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
            [UIApplication.sharedApplication.keyWindow.rootViewController addObserver:observer forKeyPath:NSStringFromSelector(@selector(view)) options:NSKeyValueObservingOptionNew context:NULL];
        }];
        [NSNotificationCenter.defaultCenter addObserverForName:UIKeyboardDidHideNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
            removeFocusFromActiveInput();
        }];
        return SDL_UIKitRunApp(argc, argv, SDL_main);
    }
}

static int watchReturnKey(void * userdata, SDL_Event * event)
{
    if(event->type == SDL_KEYDOWN && event->key.keysym.scancode == SDL_SCANCODE_RETURN)
    {
        auto self = (__bridge SDLViewObserver *)userdata;
        self.wasChatMessageSent = true;
        SDL_DelEventWatch(watchReturnKey, userdata);
    }
    return 1;
}
