/*
 * GameChatKeyboardHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#import <UIKit/UIKit.h>

#ifdef VCMI_SDL3
#include <SDL3/SDL_events.h>
#else
#include <SDL_events.h>
#endif

NS_ASSUME_NONNULL_BEGIN

@interface GameChatKeyboardHandler : NSObject

+ (void)sendKeyEventWithKeyCode:(SDL_Keycode)keyCode;

@end

NS_ASSUME_NONNULL_END
