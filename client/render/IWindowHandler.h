/*
 * IWindowHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

VCMI_LIB_NAMESPACE_BEGIN
class Point;
VCMI_LIB_NAMESPACE_END

class IWindowHandler
{
public:
	virtual ~IWindowHandler() = default;

	/// Updates window state after fullscreen state has been changed in settings
	virtual void onFullscreenChanged() = 0;

	/// De-initializes window state
	virtual void close() = 0;

	/// Fills screen with black color, erasing any existing content
	virtual void clearScreen() = 0;

	/// Returns list of resolutions supported by current screen
	virtual std::vector<Point> getSupportedResolutions() const = 0;
};
