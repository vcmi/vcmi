/*
 * CIOSUtils.m, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#import "CIOSUtils.h"

@import Foundation;

static const char *standardPath(NSSearchPathDirectory directory)
{
	return [NSFileManager.defaultManager URLForDirectory:directory inDomain:NSUserDomainMask appropriateForURL:nil create:NO error:NULL].path.UTF8String;
}

const char *ios_documentsPath() { return standardPath(NSDocumentDirectory); }
const char *ios_cachesPath() { return standardPath(NSCachesDirectory); }

const char *ios_bundlePath() { return NSBundle.mainBundle.bundlePath.UTF8String; }
const char *ios_frameworksPath() { return [NSBundle.mainBundle.bundlePath stringByAppendingPathComponent:@"Frameworks"].UTF8String; } // TODO: sharedFrameworksPath?

const char *ios_bundleIdentifier() { return NSBundle.mainBundle.bundleIdentifier.UTF8String; }
