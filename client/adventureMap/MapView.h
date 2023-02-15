/*
 * MapView.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

#include "MapRendererContext.h"

class Canvas;
class MapRenderer;

class MapRendererContext : public IMapRendererContext
{
	Point tileSize = Point(32,32);
	uint32_t animationTime = 0;

public:
	void advanceAnimations(uint32_t ms);
	void setTileSize(const Point & dimensions);

	int3 getMapSize() const override;
	bool isInMap(const int3 & coordinates) const override;
	const TerrainTile & getMapTile(const int3 & coordinates) const override;

	ObjectsVector getAllObjects() const override;
	const CGObjectInstance * getObject(ObjectInstanceID objectID) const override;

	const CGPath * currentPath() const override;

	bool isVisible(const int3 & coordinates) const override;

	uint32_t getAnimationPeriod() const override;
	uint32_t getAnimationTime() const override;
	Point getTileSize() const override;

	bool showGrid() const override;
};

class MapViewModel
{
	Point tileSize;
	Point viewCenter;
	Point viewDimensions;

	int mapLevel = 0;
public:
	void setTileSize(Point const & newValue);
	void setViewCenter(Point const & newValue);
	void setViewDimensions(Point const & newValue);
	void setLevel(int newLevel);

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

	int getLevel() const;
	int3 getTileCenter() const;

	/// returns tile under specified position in target view
	int3 getTileAtPoint(const Point & position) const;
};

class MapCache
{
	std::unique_ptr<Canvas> terrain;
	std::shared_ptr<MapViewModel> model;

	std::unique_ptr<MapRendererContext> context;
	std::unique_ptr<MapRenderer> mapRenderer;

	Canvas getTile(const int3 & coordinates);
	void updateTile(const int3 & coordinates);

public:
	explicit MapCache(const std::shared_ptr<MapViewModel> & model);
	~MapCache();

	void update(uint32_t timeDelta);
	void render(Canvas & target);
};

class MapView : public CIntObject
{
	std::shared_ptr<MapViewModel> model;
	std::unique_ptr<MapCache> tilesCache;

	std::shared_ptr<MapViewModel> createModel(const Point & dimensions) const;
public:
	std::shared_ptr<const MapViewModel> getModel() const;

	MapView(const Point & offset, const Point & dimensions);

	void setViewCenter(const int3 & position);
	void setViewCenter(const Point & position, int level);
	void setTileSize(const Point & tileSize);

	void show(SDL_Surface * to) override;
	void showAll(SDL_Surface * to) override;
};
