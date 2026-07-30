/*
 * DirtyRegionTracker.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/Rect.h"

#include <list>

struct SDL_Surface;

/// Tracks which parts of the screen framebuffer were written during a frame, so that
/// only those parts have to be uploaded to the GPU.
///
/// The client renders into a software surface which is then copied into a streaming
/// texture once per frame. That upload used to be unconditional and full-surface:
/// at 1280x720 with 2x upscaling it moved 14.7 MB every frame - roughly 4.5 ms, the
/// single largest fixed cost in the frame - no matter how little actually changed.
///
/// Dirty areas are accumulated on a coarse grid rather than as a list of rectangles.
/// A single frame issues hundreds of draw calls onto the screen surface (the adventure
/// map alone blits ~770 tiles), so pairwise merging of that many rectangles would cost
/// more than it saves. Marking grid cells is O(area) with a tiny constant, and the
/// merge step afterwards works on a bounded number of cells instead.
///
/// Coordinates are always *surface* (physical) pixels, not logical ones.
///
/// Not thread safe: rendering to the screen happens on the GUI thread only.
class DirtyRegionTracker
{
public:
	/// Grid granularity in surface pixels. Dirty areas are rounded outwards to cell
	/// boundaries, so a smaller value uploads less but yields more rectangles.
	static constexpr int cellSize = 32;

	/// Upper bound on the number of SDL_UpdateTexture calls per frame. Each call has
	/// driver-side overhead, so past a certain point fewer-but-larger wins.
	static constexpr size_t maxRegions = 12;

	/// If the dirty area exceeds this share of the surface, one full upload is cheaper
	/// than several partial ones that nearly cover it anyway.
	static constexpr double fullUpdateAreaThreshold = 0.6;

	/// Resizes the grid to match a (re)created surface and requests a full upload,
	/// since a fresh texture has undefined contents.
	void setSurfaceSize(const Point & size);

	/// Requests a full upload for this frame.
	void markFull();

	/// Records that `area` (surface coordinates) was written to.
	void markDirty(const Rect & area);

	/// Discards accumulated state. Called after the upload for the frame is done.
	void clear();

	/// True if the whole surface should be uploaded.
	bool isFullUpdate() const;

	/// True if nothing at all was written this frame.
	bool isEmpty() const;

	/// Merged rectangles to upload, in surface coordinates. Only meaningful when
	/// isFullUpdate() is false. Computed on first call and cached until clear().
	const std::vector<Rect> & getRegions();

	Point getSurfaceSize() const;

private:
	Point surfaceSize;
	int gridWidth = 0;
	int gridHeight = 0;

	/// One byte per grid cell; 1 means "written to this frame"
	std::vector<uint8_t> cells;

	bool fullUpdate = true;
	bool anyDirty = false;

	std::vector<Rect> regions;
	bool regionsValid = false;

	void rebuildRegions();
	/// Merges the pair of regions whose union wastes the least area, repeatedly,
	/// until at most maxRegions remain.
	void reduceRegionCount();
};

/// Connects the screen framebuffer to its tracker so that Canvas can report writes
/// without depending on ScreenHandler. Canvas draws onto many surfaces; only the one
/// registered here is tracked, everything else is ignored at negligible cost.
namespace ScreenDirtyRegions
{
	void setTarget(const SDL_Surface * surface, DirtyRegionTracker * tracker);

	/// No-op unless `surface` is the registered screen framebuffer.
	void markDirty(const SDL_Surface * surface, const Rect & area);
}
