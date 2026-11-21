/*
 * iOS_utils.mm, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "iOS_utils.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <MobileCoreServices/MobileCoreServices.h>
#import <objc/message.h>
#import <sys/utsname.h>

namespace
{
NSString *standardPathNative(NSSearchPathDirectory directory)
{
	return [NSFileManager.defaultManager URLForDirectory:directory inDomain:NSUserDomainMask appropriateForURL:nil create:NO error:NULL].path;
}
const char *standardPath(NSSearchPathDirectory directory) { return standardPathNative(directory).fileSystemRepresentation; }
}


namespace iOS_utils
{
UIViewController *rootViewController()
{
	for (UIScene *scene in UIApplication.sharedApplication.connectedScenes)
	{
		if (![scene isKindOfClass:UIWindowScene.class])
			continue;

		for (UIWindow *window in ((UIWindowScene *)scene).windows)
			if (window.isKeyWindow)
				return window.rootViewController;
	}

	for (UIScene *scene in UIApplication.sharedApplication.connectedScenes)
	{
		if (![scene isKindOfClass:UIWindowScene.class])
			continue;

		UIWindowScene *windowScene = (UIWindowScene *)scene;
		if (windowScene.activationState != UISceneActivationStateForegroundActive)
			continue;

		for (UIWindow *window in windowScene.windows)
			return window.rootViewController;
	}

	// legacy app lifecycle (SDL2, Qt) has no connected scenes
	return UIApplication.sharedApplication.keyWindow.rootViewController;
}

const char *documentsPath() { return standardPath(NSDocumentDirectory); }
const char *cachesPath() { return standardPath(NSCachesDirectory); }

#if TARGET_OS_SIMULATOR
const char *hostApplicationSupportPath()
{
	static NSString *applicationSupportPath;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		auto cachesPath = standardPathNative(NSCachesDirectory);
		auto afterMacOsHomeDirPos = [cachesPath rangeOfString:@"Library/Developer"].location;
		NSCAssert(afterMacOsHomeDirPos != NSNotFound, @"simulator directory location is not under user's home directory: %@", cachesPath);
		applicationSupportPath = [[cachesPath substringToIndex:afterMacOsHomeDirPos] stringByAppendingPathComponent:@"Library/Application Support/vcmi"].stringByResolvingSymlinksInPath;
	});
	return applicationSupportPath.fileSystemRepresentation;
}
#endif

const char *bundlePath() { return NSBundle.mainBundle.bundlePath.fileSystemRepresentation; }
const char *frameworksPath() { return NSBundle.mainBundle.privateFrameworksPath.fileSystemRepresentation; }

const char *bundleIdentifier() { return NSBundle.mainBundle.bundleIdentifier.UTF8String; }

bool isOsVersionAtLeast(unsigned int osMajorVersion)
{
	return NSProcessInfo.processInfo.operatingSystemVersion.majorVersion >= osMajorVersion;
}

void keepScreenOn(bool isEnabled)
{
	UIApplication.sharedApplication.idleTimerDisabled = isEnabled ? YES : NO;
}

bool hasRegularWidth()
{
	UIViewController *root = rootViewController();
	if (root != nil)
		return root.traitCollection.horizontalSizeClass == UIUserInterfaceSizeClassRegular;

	for (UIScene *scene in UIApplication.sharedApplication.connectedScenes)
	{
		if ([scene isKindOfClass:UIWindowScene.class])
			return ((UIWindowScene *)scene).traitCollection.horizontalSizeClass == UIUserInterfaceSizeClassRegular;
	}

	// legacy app lifecycle (Qt launcher) has no connected scenes
	return UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad;
}

bool isTestFlightBuild()
{
	// side-loaded builds may report the same receipt path without the file
	NSURL * receiptURL = NSBundle.mainBundle.appStoreReceiptURL;
	return [receiptURL.lastPathComponent isEqualToString:@"sandboxReceipt"] && [NSFileManager.defaultManager fileExistsAtPath:receiptURL.path];
}

void shareFile(const std::string & filePath)
{
	NSString *nsPath = [NSString stringWithUTF8String:filePath.c_str()];
	if (nsPath == nil)
		return;

	NSURL *url = [NSURL fileURLWithPath:nsPath];
	if (!url) return;

	NSArray *items = @[url];
	UIActivityViewController *controller = [[UIActivityViewController alloc] initWithActivityItems:items applicationActivities:nil];

	UIViewController *root = rootViewController();
	if (root == nil)
		return;

	if (root.presentedViewController != nil)
		[root.presentedViewController presentViewController:controller animated:YES completion:nil];
	else
		[root presentViewController:controller animated:YES completion:nil];
}

std::string iphoneHardwareId()
{
    struct utsname systemInfo;
    uname(&systemInfo);
    return std::string(systemInfo.machine);
}
}
