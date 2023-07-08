/*
 * utils.mm, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "utils.h"

#import <UIKit/UIKit.h>

namespace
{
UIActivityIndicatorView *indicator;
}

namespace iOS_utils
{
double screenScale()
{
	return UIScreen.mainScreen.nativeScale;
}

void showLoadingIndicator()
{
	NSCAssert(!indicator, @"activity indicator must be hidden before attempting to show it again");
	indicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge];
	[indicator startAnimating];

	auto mainView = UIApplication.sharedApplication.keyWindow.rootViewController.view;
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
	UIImpactFeedbackGenerator *hapticGen = [[UIImpactFeedbackGenerator alloc] initWithStyle:(UIImpactFeedbackStyleLight)];
	[hapticGen impactOccurred];
	hapticGen = NULL;
}
}
