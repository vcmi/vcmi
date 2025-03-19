/*
 * IScreenHandler.h, part of VCMI engine
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
class Rect;
VCMI_LIB_NAMESPACE_END

class Canvas;

class IScreenHandler
{
public:
	virtual ~IScreenHandler() = default;

	/// Updates window state after fullscreen state has been changed in settings
	virtual void onScreenResize() = 0;

	/// Fills screen with black color, erasing any existing content
	virtual void clearScreen() = 0;

	/// Returns canvas that can be used to display objects on screen
	virtual Canvas getScreenCanvas() const = 0;

	/// Synchronizes internal screen texture. Screen canvas may not be modified during this call
	virtual void updateScreenTexture() = 0;

	/// Presents screen texture on the screen
	virtual void presentScreenTexture() = 0;

	/// Returns list of resolutions supported by current screen
	virtual std::vector<Point> getSupportedResolutions() const = 0;

	/// Returns <min, max> range of possible values for screen scaling percentage
	virtual std::tuple<int, int> getSupportedScalingRange() const = 0;

	/// Converts provided rect from logical coordinates into coordinates within window, accounting for scaling and viewport
	virtual Rect convertLogicalPointsToWindow(const Rect & input) const = 0;

	/// Dimensions of render output
	virtual Point getRenderResolution() const = 0;

	/// Dimensions of logical output. Can be different if scaling is used
	virtual Point getLogicalResolution() const = 0;

	virtual int getInterfaceScalingPercentage() const = 0;

	virtual int getScalingFactor() const = 0;

	/// Window has focus
	virtual bool hasFocus() = 0;
};
