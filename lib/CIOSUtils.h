/*
 * CIOSUtils.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <TargetConditionals.h>

#ifdef __OBJC__
@class NSURL;
#endif

extern const char *ios_documentsPath();
extern const char *ios_cachesPath();

#ifdef __OBJC__
NSURL *sharedContainerURL();
NSURL *sharedGameDataURL();
#endif
extern const char *ios_sharedDataPath();

#if TARGET_OS_SIMULATOR
extern const char *ios_hostApplicationSupportPath();
#endif

extern const char *ios_bundlePath();
extern const char *ios_frameworksPath();

extern const char *ios_bundleIdentifier();

#ifdef __cplusplus
}
#endif
