/*
 * GameChatKeyboardHanlder.m, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#import "GameChatKeyboardHanlder.h"

#include <SDL_events.h>

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

static CGRect keyboardFrame(NSNotification * n, NSString * userInfoKey)
{
	return [n.userInfo[userInfoKey] CGRectValue];
}
static CGRect keyboardFrameBegin(NSNotification * n) { return keyboardFrame(n, UIKeyboardFrameBeginUserInfoKey); }
static CGRect keyboardFrameEnd  (NSNotification * n) { return keyboardFrame(n, UIKeyboardFrameEndUserInfoKey); }


@interface GameChatKeyboardHanlder ()
@property (nonatomic) BOOL wasChatMessageSent;
@end

@implementation GameChatKeyboardHanlder

- (void)triggerInput {
	__auto_type notificationCenter = NSNotificationCenter.defaultCenter;
	[notificationCenter addObserver:self selector:@selector(textDidBeginEditing:) name:UITextFieldTextDidBeginEditingNotification object:nil];
	[notificationCenter addObserver:self selector:@selector(textDidEndEditing:) name:UITextFieldTextDidEndEditingNotification object:nil];
	[notificationCenter addObserver:self selector:@selector(keyboardWillChangeFrame:) name:UIKeyboardWillChangeFrameNotification object:nil];
	[notificationCenter addObserver:self selector:@selector(keyboardDidChangeFrame:) name:UIKeyboardDidChangeFrameNotification object:nil];

	self.wasChatMessageSent = NO;
	sendKeyEvent(SDLK_TAB);
}

- (void)positionTextFieldAboveKeyboardRect:(CGRect)kbFrame {
	__auto_type r = kbFrame;
	r.size.height = CGRectGetHeight(self.textFieldSDL.frame);
	r.origin.y -= r.size.height;
	self.textFieldSDL.frame = r;
}

#pragma mark - Notifications

- (void)textDidBeginEditing:(NSNotification *)n {
	self.textFieldSDL.hidden = NO;
	self.textFieldSDL.text = nil;

	// watch for pressing Return to ignore sending Escape key after keyboard is closed
	SDL_AddEventWatch(watchReturnKey, (__bridge void *)self);
}

- (void)textDidEndEditing:(NSNotification *)n {
	[NSNotificationCenter.defaultCenter removeObserver:self];
	self.textFieldSDL.hidden = YES;

	// discard chat message
	if(!self.wasChatMessageSent)
		sendKeyEvent(SDLK_ESCAPE);
}

- (void)keyboardWillChangeFrame:(NSNotification *)n {
	// animate textfield together with keyboard
	[UIView performWithoutAnimation:^{
		[self positionTextFieldAboveKeyboardRect:keyboardFrameBegin(n)];
	}];

	NSTimeInterval kbAnimationDuration = [n.userInfo[UIKeyboardAnimationDurationUserInfoKey] doubleValue];
	NSUInteger kbAnimationCurve = [n.userInfo[UIKeyboardAnimationCurveUserInfoKey] unsignedIntegerValue];
	[UIView animateWithDuration:kbAnimationDuration delay:0 options:(kbAnimationCurve << 16) animations:^{
		[self positionTextFieldAboveKeyboardRect:keyboardFrameEnd(n)];
	} completion:nil];
}

- (void)keyboardDidChangeFrame:(NSNotification *)n {
	[self positionTextFieldAboveKeyboardRect:keyboardFrameEnd(n)];
}

@end


static int watchReturnKey(void * userdata, SDL_Event * event)
{
	if(event->type == SDL_KEYDOWN && event->key.keysym.scancode == SDL_SCANCODE_RETURN)
	{
		__auto_type self = (__bridge GameChatKeyboardHanlder *)userdata;
		self.wasChatMessageSent = YES;
		SDL_DelEventWatch(watchReturnKey, userdata);
	}
	return 1;
}
