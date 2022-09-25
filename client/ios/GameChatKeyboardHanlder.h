/*
 * GameChatKeyboardHanlder.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface GameChatKeyboardHanlder : NSObject

@property (nonatomic, weak) UITextField * textFieldSDL;

- (void)triggerInput;

@end

NS_ASSUME_NONNULL_END
