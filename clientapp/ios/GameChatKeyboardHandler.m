/*
 * GameChatKeyboardHandler.m, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#import "GameChatKeyboardHandler.h"

#ifdef VCMI_SDL3
static bool watchReturnKey(void * userdata, SDL_Event * event);
#else
static int watchReturnKey(void * userdata, SDL_Event * event);
#endif


@interface GameChatKeyboardHandler ()
@property (nonatomic) BOOL wasChatMessageSent;
@end

@implementation GameChatKeyboardHandler

+ (void)sendKeyEventWithKeyCode:(SDL_Keycode)keyCode
{
	SDL_Event keyEvent;
#ifdef VCMI_SDL3
	keyEvent.key = (SDL_KeyboardEvent){
		.type = SDL_EVENT_KEY_DOWN,
		.down = true,
		.key = keyCode,
	};
#else
	keyEvent.key = (SDL_KeyboardEvent){
		.type = SDL_KEYDOWN,
		.state = SDL_PRESSED,
		.keysym.sym = keyCode,
	};
#endif
	SDL_PushEvent(&keyEvent);
}

- (instancetype)init {
	self = [super init];

	__auto_type notificationCenter = NSNotificationCenter.defaultCenter;
	[notificationCenter addObserver:self selector:@selector(textDidBeginEditing:) name:UITextFieldTextDidBeginEditingNotification object:nil];
	[notificationCenter addObserver:self selector:@selector(textDidEndEditing:) name:UITextFieldTextDidEndEditingNotification object:nil];

	return self;
}

#pragma mark - Notifications

- (void)textDidBeginEditing:(NSNotification *)n {
	self.wasChatMessageSent = NO;

	// watch for pressing Return to ignore sending Escape key after keyboard is closed
	SDL_AddEventWatch(watchReturnKey, (__bridge void *)self);
}

- (void)textDidEndEditing:(NSNotification *)n {
	// discard chat message
	if(!self.wasChatMessageSent)
		[[self class] sendKeyEventWithKeyCode:SDLK_ESCAPE];
}

@end


#ifdef VCMI_SDL3
static bool watchReturnKey(void * userdata, SDL_Event * event)
{
	if(event->type == SDL_EVENT_KEY_DOWN && event->key.scancode == SDL_SCANCODE_RETURN)
	{
		__auto_type self = (__bridge GameChatKeyboardHandler *)userdata;
		self.wasChatMessageSent = YES;
		SDL_RemoveEventWatch(watchReturnKey, userdata);
	}
	return true;
}
#else
static int watchReturnKey(void * userdata, SDL_Event * event)
{
	if(event->type == SDL_KEYDOWN && event->key.keysym.scancode == SDL_SCANCODE_RETURN)
	{
		__auto_type self = (__bridge GameChatKeyboardHandler *)userdata;
		self.wasChatMessageSent = YES;
		SDL_DelEventWatch(watchReturnKey, userdata);
	}
	return 1;
}
#endif
