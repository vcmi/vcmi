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

#include "lib/constants/Enumerations.h"
#include "lib/Rect.h"

class Point;
class Rect;

class Canvas;

/// One region of an offscreen canvas. Unused here; client code is shared between backends.
struct PresentedRegion
{
	Rect source;
	Rect target;
};

/// GPU layers composited under the software screen. This backend has none, but client code
/// is shared between backends and names them.
enum class GpuRenderLayer : uint8_t
{
	MAP,
	BATTLE,

	COUNT
};

class IScreenHandler
{
public:
	virtual ~IScreenHandler() = default;

	/// Updates window state after fullscreen state has been changed in settings
	virtual bool onScreenResize(bool keepWindowResolution) = 0;

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

	virtual void screenShot() const = 0;

	/// Window has focus
	virtual bool hasFocus() = 0;

	virtual void setColorScheme(ColorScheme scheme) = 0;

	/// This backend renders in software only; there are no GPU layers to draw into
	virtual bool isGpuRenderingEnabled() const { return false; }

	/// Unreachable here - callers must gate on isGpuRenderingEnabled(), which is always false
	virtual Canvas getLayerCanvas(GpuRenderLayer layer) = 0;

	void releaseLayer(GpuRenderLayer) {}
	void clearReleasedLayers() {}

	/// Offscreen canvas of the given logical size, always a software surface in this backend
	virtual Canvas createOffscreenCanvas(const Point & size) const = 0;

	/// A software surface has no GPU-imposed size limit
	int maxOffscreenCanvasSize() const { return INT_MAX; }

	/// No GPU drawing happens in this backend, so there is nothing queued to hand over
	void flushRenderCommands() {}

	/// This backend composes in software, so there is no separate present to defer a copy to
	void presentFromCanvas(GpuRenderLayer, const Canvas &, const std::vector<PresentedRegion> &) {}
	void clearPresentedCanvas(GpuRenderLayer) {}
};
