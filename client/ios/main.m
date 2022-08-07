/*
 * main.m, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#import "startSDL.h"

#import <UIKit/UIKit.h>

#import <objc/runtime.h>

static void startSDLManually(int argc, char * argv[])
{
	id<UIApplicationDelegate> appDelegate;
	@autoreleasepool {
		__auto_type sdlAppDelegateClass = NSClassFromString(@"SDLUIKitDelegate");
		NSCAssert(sdlAppDelegateClass != nil, @"SDL AppDelegate class doesn't exist");
		NSCAssert(class_conformsToProtocol(sdlAppDelegateClass, @protocol(UIApplicationDelegate)), @"SDL AppDelegate doesn't conform to UIApplicationDelegate");
		appDelegate = [sdlAppDelegateClass new];
	}
	UIApplication.sharedApplication.delegate = appDelegate;

	int result = startSDL(argc, argv, YES);
	exit(result);
}

int qt_main_wrapper(int argc, char * argv[]);

int client_main(int argc, char * argv[])
{
	NSInteger launchType;
	@autoreleasepool {
		launchType = [NSUserDefaults.standardUserDefaults integerForKey:@"LaunchType"];
	}
	if (launchType == 1)
		return startSDL(argc, argv, NO);

	@autoreleasepool {
		id __block startGameObserver = [NSNotificationCenter.defaultCenter addObserverForName:@"StartGame" object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
			[NSNotificationCenter.defaultCenter removeObserver:startGameObserver];
			startGameObserver = nil;
			startSDLManually(argc, argv);
		}];
		return qt_main_wrapper(argc, argv);
	}
}
