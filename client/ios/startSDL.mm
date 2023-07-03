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

@end

int startSDL(int argc, char * argv[], BOOL startManually)
{
	@autoreleasepool {
		auto observer = [SDLViewObserver new];
		observer.gameChatHandler = [GameChatKeyboardHandler new];

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
