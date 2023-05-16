/*
 * MapViewModel.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/Rect.h"

class MapViewModel
{
	Point tileSize;
	Point viewCenter;
	Point viewDimensions;

	int mapLevel = 0;

public:
	void setTileSize(const Point & newValue);
	void setViewCenter(const Point & newValue);
	void setViewDimensions(const Point & newValue);
	void setLevel(int newLevel);

	/// returns maximal possible size for a single tile
	Point getSingleTileSizeUpperLimit() const;

	/// returns minimal possible size for a single tile
	Point getSingleTileSizeLowerLimit() const;

	/// returns current size of map tile in pixels
	Point getSingleTileSize() const;

	/// returns center point of map view, in Map coordinates
	Point getMapViewCenter() const;

	/// returns total number of visible tiles
	Point getTilesVisibleDimensions() const;

	/// returns rect encompassing all visible tiles
	Rect getTilesTotalRect() const;

	/// returns required area in pixels of cache canvas
	Point getCacheDimensionsPixels() const;

	/// returns actual player-visible area
	Point getPixelsVisibleDimensions() const;

	/// returns area covered by specified tile in map cache
	Rect getCacheTileArea(const int3 & coordinates) const;

	/// returns area covered by specified tile in target view
	Rect getTargetTileArea(const int3 & coordinates) const;

	/// returns tile under specified position in target view
	int3 getTileAtPoint(const Point & position) const;

	/// returns currently visible map level
	int getLevel() const;
};
