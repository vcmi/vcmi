/*
 * utils.mm, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "utils.h"
#include "iOS_utils.h"

#import <UIKit/UIKit.h>

namespace
{
UIActivityIndicatorView *indicator;
}

namespace iOS_utils
{
double screenScale()
{
	UIViewController *root = rootViewController();
	if (root != nil)
		return root.traitCollection.displayScale;

	for (UIScene *scene in UIApplication.sharedApplication.connectedScenes)
	{
		if ([scene isKindOfClass:UIWindowScene.class])
			return ((UIWindowScene *)scene).screen.nativeScale;
	}

	// legacy app lifecycle (SDL2) has no connected scenes
	return UIScreen.mainScreen.nativeScale;
}

void showLoadingIndicator()
{
	NSCAssert(!indicator, @"activity indicator must be hidden before attempting to show it again");
	indicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge];
	indicator.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleBottomMargin;
	[indicator startAnimating];

	UIView *mainView = rootViewController().view;
	NSCAssert(mainView != nil, @"root view controller is not available");
	mainView.userInteractionEnabled = NO;
	[mainView addSubview:indicator];
	indicator.center = {CGRectGetMidX(mainView.bounds), CGRectGetMidY(mainView.bounds)};
}

void hideLoadingIndicator()
{
	indicator.superview.userInteractionEnabled = YES;
	[indicator stopAnimating];
	[indicator removeFromSuperview];
	indicator = nil;
}

void hapticFeedback()
{
    auto hapticGen = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleLight];
    [hapticGen impactOccurred];
}
}
