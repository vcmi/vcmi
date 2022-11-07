/*
 * startSDL.mm, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#import "startSDL.h"
#import "GameChatKeyboardHandler.h"

#include "../Global.h"
#include "CMT.h"
#include "CServerHandler.h"
#include "CFocusableHelper.h"

#include <SDL_main.h>
#include <SDL_events.h>
#include <SDL_render.h>
#include <SDL_system.h>

#import <UIKit/UIKit.h>

@interface SDLViewObserver : NSObject <UIGestureRecognizerDelegate>
@property (nonatomic, strong) GameChatKeyboardHandler * gameChatHandler;
@end

@implementation SDLViewObserver

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context {
	[object removeObserver:self forKeyPath:keyPath];

    UIView * view = [object valueForKeyPath:keyPath];

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


int startSDL(int argc, char * argv[], BOOL startManually)
{
	@autoreleasepool {
		auto observer = [SDLViewObserver new];
		observer.gameChatHandler = [GameChatKeyboardHandler new];

		id __block sdlWindowCreationObserver = [NSNotificationCenter.defaultCenter addObserverForName:UIWindowDidBecomeKeyNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
			[NSNotificationCenter.defaultCenter removeObserver:sdlWindowCreationObserver];
			sdlWindowCreationObserver = nil;

			UIWindow * sdlWindow = note.object;
			[sdlWindow.rootViewController addObserver:observer forKeyPath:NSStringFromSelector(@selector(view)) options:NSKeyValueObservingOptionNew context:NULL];
		}];
		id textFieldObserver = [NSNotificationCenter.defaultCenter addObserverForName:UITextFieldTextDidEndEditingNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
			removeFocusFromActiveInput();
		}];

		int result;
		if (startManually)
		{
			// copied from -[SDLUIKitDelegate postFinishLaunch]
			SDL_SetMainReady();
			SDL_iOSSetEventPump(SDL_TRUE);
			result = SDL_main(argc, argv);
			SDL_iOSSetEventPump(SDL_FALSE);
		}
		else
			result = SDL_UIKitRunApp(argc, argv, SDL_main);

		[NSNotificationCenter.defaultCenter removeObserver:textFieldObserver];
		return result;
	}
}
