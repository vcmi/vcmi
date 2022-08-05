/*
 * AppDelegate.mm, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#import "AppDelegate.h"

#include "startSDL.h"
#include "../launcher/main.h"

#include <vector>

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary<UIApplicationLaunchOptionsKey, id> *)launchOptions {
	self.window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
	self.window.rootViewController = [UIViewController new];
	[self.window makeKeyAndVisible];

	[NSNotificationCenter.defaultCenter addObserverForName:@"StartGame" object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
		[self startMainFunc:startSDL];
	}];
	return YES;
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		[self startMainFunc:qt_main];
	});
}

#pragma mark - Private

- (void)startMainFunc:(int(*)(int argc, char * argv[]))mainPtr {
	auto args = NSProcessInfo.processInfo.arguments;
	std::vector<char *> argv;
	argv.reserve(args.count);
	for (NSString *arg in args)
		argv.push_back(const_cast<char *>(arg.UTF8String));

	mainPtr(argv.size(), argv.data());
}

@end
