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
UIInterfaceOrientationMask swizzled_supportedInterfaceOrientations(id __unused self, SEL __unused _cmd)
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
	id __block observer = [NSNotificationCenter.defaultCenter addObserverForName:UIWindowDidBecomeKeyNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull notification) {
		[NSNotificationCenter.defaultCenter removeObserver:observer];

		UIViewController * rootVc = [notification.object rootViewController];
		auto sel = @selector(supportedInterfaceOrientations);
		auto rootVcClass = object_getClass(rootVc);
		auto methodDesc = method_getDescription(class_getInstanceMethod(rootVcClass, sel));
		class_replaceMethod(rootVcClass, sel, (IMP)swizzled_supportedInterfaceOrientations, methodDesc->types);

		if(@available(iOS 16.0, *))
			[rootVc setNeedsUpdateOfSupportedInterfaceOrientations];
	}];
}
}
