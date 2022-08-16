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
}
