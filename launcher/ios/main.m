/*
 * main.m, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#import <UIKit/UIKit.h>

void launchGame(int argc, char * argv[]) {
	@autoreleasepool {
		__auto_type app = UIApplication.sharedApplication;
		__auto_type qtNativeWindowIndex = [app.windows indexOfObjectPassingTest:^BOOL(__kindof UIWindow * _Nonnull window, NSUInteger idx, BOOL * _Nonnull stop) {
			return [NSStringFromClass([window class]) isEqualToString:@"QUIWindow"];
		}];
		NSCAssert(qtNativeWindowIndex != NSNotFound, @"Qt native window doesn't exist");

		__auto_type qtNativeWindow = app.windows[qtNativeWindowIndex];
		qtNativeWindow.hidden = YES;
		[qtNativeWindow.rootViewController.view removeFromSuperview];
		qtNativeWindow.rootViewController = nil;
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_13_0
		if (@available(iOS 13.0, *))
			qtNativeWindow.windowScene = nil;
#endif
	}

	__auto_type args = [NSMutableArray arrayWithCapacity:argc];
	for (int i = 0; i < argc; ++i)
		[arr addObject:@(argv[i])];
	[NSNotificationCenter.defaultCenter postNotificationName:@"StartGame" object:nil userInfo:@{@"args": args}];
}