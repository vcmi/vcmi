/*
 * revealdirectoryinfiles.mm, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "revealdirectoryinfiles.h"

#import <UIKit/UIKit.h>

namespace iOS_utils
{
void revealDirectoryInFiles(QUrl dirUrl)
{
	auto urlComponents = [NSURLComponents componentsWithURL:dirUrl.toNSURL() resolvingAgainstBaseURL:NO];
	urlComponents.scheme = @"shareddocuments";
	[UIApplication.sharedApplication openURL:urlComponents.URL options:@{} completionHandler:nil];
}
}
