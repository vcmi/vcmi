/*
 * prepare_ios.mm, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "prepare_p.h"

#import <UIKit/UIKit.h>

#include <objc/runtime.h>

namespace
{
UIInterfaceOrientationMask swizzled_supportedInterfaceOrientationsForWindow
	(id __unused self, SEL __unused _cmd, UIApplication * __unused application, UIWindow * __unused _Nullable window)
{
	if(UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad)
		return UIInterfaceOrientationMaskAll;
	return UIInterfaceOrientationMaskLandscape;
}
}

namespace launcher
{
void prepareIos()
{
	auto sel = @selector(application:supportedInterfaceOrientationsForWindow:);
	auto methodDesc = protocol_getMethodDescription(@protocol(UIApplicationDelegate), sel, NO, YES);
	auto appDelegateClass = object_getClass(UIApplication.sharedApplication.delegate);
	[[maybe_unused]] auto existingImp = class_replaceMethod(
		appDelegateClass, sel, (IMP)swizzled_supportedInterfaceOrientationsForWindow, methodDesc.types);
	// also check implementation in qtbase - src/plugins/platforms/ios/qiosapplicationdelegate.mm
	NSCAssert(existingImp == nullptr, @"original app delegate has this method, don't ignore it");
}
}
