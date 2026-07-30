/*
 * DirtyRegionTracker.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "DirtyRegionTracker.h"

#include <limits>

void DirtyRegionTracker::setSurfaceSize(const Point & size)
{
	surfaceSize = size;
	gridWidth = (size.x + cellSize - 1) / cellSize;
	gridHeight = (size.y + cellSize - 1) / cellSize;
	cells.assign(static_cast<size_t>(gridWidth) * gridHeight, 0);

	// A newly created texture holds undefined pixels, so the first frame must upload
	// everything regardless of what was drawn.
	markFull();
}

void DirtyRegionTracker::markFull()
{
	fullUpdate = true;
	anyDirty = true;
	regionsValid = false;
}

void DirtyRegionTracker::markDirty(const Rect & area)
{
	if(fullUpdate || cells.empty())
		return;

	Rect clamped = area.intersect(Rect(Point(0, 0), surfaceSize));
	if(clamped.w <= 0 || clamped.h <= 0)
		return;

	const int firstX = clamped.left() / cellSize;
	const int firstY = clamped.top() / cellSize;
	// bottom()/right() are exclusive, so the last touched cell is at (edge - 1)
	const int lastX = std::min((clamped.right() - 1) / cellSize, gridWidth - 1);
	const int lastY = std::min((clamped.bottom() - 1) / cellSize, gridHeight - 1);

	for(int y = firstY; y <= lastY; ++y)
	{
		uint8_t * row = cells.data() + static_cast<size_t>(y) * gridWidth;
		std::fill(row + firstX, row + lastX + 1, uint8_t(1));
	}

	anyDirty = true;
	regionsValid = false;
}

void DirtyRegionTracker::clear()
{
	if(anyDirty && !cells.empty())
		std::fill(cells.begin(), cells.end(), uint8_t(0));

	fullUpdate = false;
	anyDirty = false;
	regions.clear();
	regionsValid = false;
}

bool DirtyRegionTracker::isFullUpdate() const
{
	return fullUpdate;
}

bool DirtyRegionTracker::isEmpty() const
{
	return !anyDirty;
}

Point DirtyRegionTracker::getSurfaceSize() const
{
	return surfaceSize;
}

const std::vector<Rect> & DirtyRegionTracker::getRegions()
{
	if(!regionsValid)
		rebuildRegions();

	return regions;
}

void DirtyRegionTracker::rebuildRegions()
{
	regions.clear();
	regionsValid = true;

	if(fullUpdate || !anyDirty)
		return;

	// Scan the grid row by row, collecting horizontal runs of dirty cells. A run that
	// repeats identically on the next row extends the rectangle that is already open
	// instead of starting a new one, so vertically adjacent bands coalesce.
	struct OpenRect
	{
		int cellX0;
		int cellX1; // exclusive
		int cellY0;
	};

	std::vector<OpenRect> open;
	std::vector<OpenRect> current;

	const auto & closeRect = [this](const OpenRect & entry, int cellY1)
	{
		Rect result(
			entry.cellX0 * cellSize,
			entry.cellY0 * cellSize,
			(entry.cellX1 - entry.cellX0) * cellSize,
			(cellY1 - entry.cellY0) * cellSize);

		regions.push_back(result.intersect(Rect(Point(0, 0), surfaceSize)));
	};

	for(int y = 0; y < gridHeight; ++y)
	{
		const uint8_t * row = cells.data() + static_cast<size_t>(y) * gridWidth;

		current.clear();
		for(int x = 0; x < gridWidth; ++x)
		{
			if(row[x] == 0)
				continue;

			int runStart = x;
			while(x < gridWidth && row[x] != 0)
				++x;

			current.push_back({runStart, x, y});
		}

		// Extend rectangles whose run is unchanged; close the rest.
		size_t openIndex = 0;
		size_t currentIndex = 0;
		std::vector<OpenRect> stillOpen;
		stillOpen.reserve(current.size());

		while(openIndex < open.size() || currentIndex < current.size())
		{
			const bool haveOpen = openIndex < open.size();
			const bool haveCurrent = currentIndex < current.size();

			if(haveOpen && haveCurrent && open[openIndex].cellX0 == current[currentIndex].cellX0 && open[openIndex].cellX1 == current[currentIndex].cellX1)
			{
				stillOpen.push_back(open[openIndex]); // keep original cellY0, extends downwards
				++openIndex;
				++currentIndex;
			}
			else if(haveOpen && (!haveCurrent || open[openIndex].cellX0 < current[currentIndex].cellX0))
			{
				closeRect(open[openIndex], y);
				++openIndex;
			}
			else
			{
				stillOpen.push_back(current[currentIndex]);
				++currentIndex;
			}
		}

		open.swap(stillOpen);
	}

	for(const auto & entry : open)
		closeRect(entry, gridHeight);

	if(regions.size() > maxRegions)
		reduceRegionCount();

	// Several regions that between them cover most of the surface are more expensive
	// than a single upload of everything.
	int64_t dirtyArea = 0;
	for(const auto & region : regions)
		dirtyArea += static_cast<int64_t>(region.w) * region.h;

	const int64_t totalArea = static_cast<int64_t>(surfaceSize.x) * surfaceSize.y;
	if(totalArea > 0 && dirtyArea > totalArea * fullUpdateAreaThreshold)
	{
		fullUpdate = true;
		regions.clear();
	}
}

void DirtyRegionTracker::reduceRegionCount()
{
	// Region count is already small here (bounded by the grid), so a straightforward
	// quadratic search for the cheapest merge is fine.
	while(regions.size() > maxRegions)
	{
		size_t bestFirst = 0;
		size_t bestSecond = 1;
		int64_t bestWaste = std::numeric_limits<int64_t>::max();

		for(size_t i = 0; i < regions.size(); ++i)
		{
			for(size_t j = i + 1; j < regions.size(); ++j)
			{
				const Rect & left = regions[i];
				const Rect & right = regions[j];

				const int x0 = std::min(left.left(), right.left());
				const int y0 = std::min(left.top(), right.top());
				const int x1 = std::max(left.right(), right.right());
				const int y1 = std::max(left.bottom(), right.bottom());

				const int64_t unionArea = static_cast<int64_t>(x1 - x0) * (y1 - y0);
				const int64_t ownArea = static_cast<int64_t>(left.w) * left.h + static_cast<int64_t>(right.w) * right.h;
				const int64_t waste = unionArea - ownArea;

				if(waste < bestWaste)
				{
					bestWaste = waste;
					bestFirst = i;
					bestSecond = j;
				}
			}
		}

		const Rect & left = regions[bestFirst];
		const Rect & right = regions[bestSecond];
		const int x0 = std::min(left.left(), right.left());
		const int y0 = std::min(left.top(), right.top());
		const int x1 = std::max(left.right(), right.right());
		const int y1 = std::max(left.bottom(), right.bottom());

		regions[bestFirst] = Rect(x0, y0, x1 - x0, y1 - y0);
		regions.erase(regions.begin() + bestSecond);
	}
}

namespace ScreenDirtyRegions
{
	namespace
	{
		const SDL_Surface * trackedSurface = nullptr;
		DirtyRegionTracker * activeTracker = nullptr;
	}

	void setTarget(const SDL_Surface * surface, DirtyRegionTracker * tracker)
	{
		trackedSurface = surface;
		activeTracker = tracker;
	}

	void markDirty(const SDL_Surface * surface, const Rect & area)
	{
		if(surface != trackedSurface || activeTracker == nullptr)
			return;

		activeTracker->markDirty(area);
	}
}
