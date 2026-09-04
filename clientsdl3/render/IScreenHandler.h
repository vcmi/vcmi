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

/// GPU layers composited under the software screen, in draw order. Each is screen sized;
/// a layer is transparent wherever its owner has not drawn.
enum class GpuRenderLayer : uint8_t
{
	MAP,
	BATTLE,

	COUNT
};

/// One region of an offscreen canvas, drawn onto the screen while the frame is composed
struct PresentedRegion
{
	Rect source; ///< within the canvas
	Rect target; ///< in screen pixels
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

	/// True when GPU layers are available, i.e. the driver granted us render targets
	virtual bool isGpuRenderingEnabled() const = 0;

	/// Canvas drawing into one of the GPU layers. Only valid while GPU rendering is active.
	/// Requesting it marks the layer as having content to composite.
	virtual Canvas getLayerCanvas(GpuRenderLayer layer) = 0;

	/// Gives a layer back once its owner is gone, so that its last frame stops showing
	/// through. Safe to call from any thread; acted on at the start of the next frame.
	virtual void releaseLayer(GpuRenderLayer layer) = 0;

	/// Clears any layer released since the last frame. Must run before windows redraw, so
	/// that an owner that is merely covered rather than gone can reclaim its layer.
	virtual void clearReleasedLayers() = 0;

	/// Offscreen canvas of the given logical size. Drawn by the GPU where this backend
	/// supports it, into a plain surface otherwise. The canvas owns whatever backs it.
	virtual Canvas createOffscreenCanvas(const Point & size) const = 0;

	/// Largest render target the GPU driver will create, in pixels along one dimension.
	/// A caller sizing an offscreen canvas from map or window state should stay under this,
	/// rather than rely on the surface fallback createOffscreenCanvas takes on failure.
	virtual int maxOffscreenCanvasSize() const = 0;

	/// Hands everything drawn so far to the GPU instead of leaving it queued. Lets drawing
	/// that a later pass reads back start early, rather than stalling on the first read.
	virtual void flushRenderCommands() = 0;

	/// Draws parts of an offscreen canvas onto the screen while the frame is composed, in place of
	/// the given layer, instead of copying them into that layer during the frame.
	///
	/// Reading a render target in the middle of a frame makes a tiling GPU resolve it right then.
	/// On some Android drivers that reorganises memory and stalls every process on the device for
	/// a fifth of a second; the same read costs nothing once the frame is being handed over.
	///
	/// What is registered here replaces the previous registration and stays until it is replaced,
	/// or clearPresentedCanvas() is called - a layer keeps its content across frames, and a window
	/// opening over the map must not blank it. The layer it stands in for stops being composited.
	virtual void presentFromCanvas(GpuRenderLayer layer, const Canvas & source, const std::vector<PresentedRegion> & regions) = 0;

	/// Forgets what presentFromCanvas() registered. Must be called before the canvas it refers to
	/// is destroyed, since only the caller knows when that happens.
	virtual void clearPresentedCanvas(GpuRenderLayer layer) = 0;
};
