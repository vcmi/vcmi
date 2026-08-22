/*
 * startSDL.mm, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#import "startSDL.h"
#import "GameChatKeyboardHandler.h"

#include "../Global.h"
#include "CMT.h"
#include "CServerHandler.h"
#include "CFocusableHelper.h"

#ifdef VCMI_SDL3
// this file only calls SDL_main(); the entry point is client_main() in main.m
#define SDL_MAIN_NOIMPL
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_system.h>
#else
#include <SDL_main.h>
#include <SDL_events.h>
#include <SDL_render.h>
#include <SDL_system.h>
#endif

#import <UIKit/UIKit.h>

@interface SDLViewObserver : NSObject <UIGestureRecognizerDelegate>
@property (nonatomic, strong) GameChatKeyboardHandler * gameChatHandler;
@end

@implementation SDLViewObserver

@end

void removeQtNotificationObserver(NSString * qtClassName, NSArray<NSNotificationName> * notificationNames)
{
	auto qtClass = NSClassFromString(qtClassName);
	NSCAssert(qtClass, @"%@ class not found", qtClass);
	for (NSNotificationName notificationName in notificationNames)
		[NSNotificationCenter.defaultCenter removeObserver:qtClass name:notificationName object:nil];
}

int startSDL(int argc, char * argv[], BOOL startManually)
{
	@autoreleasepool {
		auto observer = [SDLViewObserver new];
		observer.gameChatHandler = [GameChatKeyboardHandler new];

		id textFieldObserver = [NSNotificationCenter.defaultCenter addObserverForName:UITextFieldTextDidEndEditingNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
			removeFocusFromActiveInput();
		}];
		auto cleanup = ^{
			[NSNotificationCenter.defaultCenter removeObserver:textFieldObserver];
		};

		// TODO: check with Qt 6
		// https://code.qt.io/cgit/qt/qtbase.git/tree/src/plugins/platforms/ios/qiosscreen.mm?h=5.15
		removeQtNotificationObserver(@"QIOSScreenTracker", @[UIScreenDidConnectNotification, UIScreenDidDisconnectNotification]);
		// https://code.qt.io/cgit/qt/qtbase.git/tree/src/plugins/platforms/ios/qioseventdispatcher.mm?h=5.15
		removeQtNotificationObserver(@"QIOSApplicationStateTracker", @[UIApplicationWillTerminateNotification]);

		int result;
		if (startManually)
		{
			// copied from -[SDLUIKitDelegate postFinishLaunch]
			SDL_SetMainReady();
#ifdef VCMI_SDL3
			SDL_SetiOSEventPump(true);
			result = SDL_main(argc, argv);
			SDL_SetiOSEventPump(false);
#else
			SDL_iOSSetEventPump(SDL_TRUE);
			result = SDL_main(argc, argv);
			SDL_iOSSetEventPump(SDL_FALSE);
#endif
		}
		else
		{
			// exiting SDL app will destroy main window making it no longer key window
			[NSNotificationCenter.defaultCenter addObserverForName:UIWindowDidResignKeyNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull notification) {
				cleanup();
				exit(0);
			}];
			// calls UIApplicationMain internally, never returns
#ifdef VCMI_SDL3
			result = SDL_RunApp(argc, argv, SDL_main, nullptr);
#else
			result = SDL_UIKitRunApp(argc, argv, SDL_main);
#endif
		}

		cleanup();
		return result;
	}
}
