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
	uint32_t animationTime = 0;

public:
	void advanceAnimations(uint32_t ms);

	int3 getMapSize() const override;
	bool isInMap(const int3 & coordinates) const override;
	const TerrainTile & getMapTile(const int3 & coordinates) const override;

	ObjectsVector getAllObjects() const override;
	const CGObjectInstance * getObject(ObjectInstanceID objectID) const override;

	bool isVisible(const int3 & coordinates) const override;
	VisibilityMap getVisibilityMap() const override;

	uint32_t getAnimationPeriod() const override;
	uint32_t getAnimationTime() const override;
	Point tileSize() const override;

	bool showGrid() const override;
};

class MapCache
{
	std::unique_ptr<Canvas> terrain;

	Point tileSize;
	Point viewCenter;
	Point viewDimensionsTiles;
	Point viewDimensionsPixels;
	Point targetDimensionsPixels;

	int mapLevel;

	std::unique_ptr<MapRendererContext> context;
	std::unique_ptr<MapRenderer> mapRenderer;

	Canvas getTile(const int3 & coordinates);

	void updateTile(const int3 & coordinates);

public:
	explicit MapCache(const Point & tileSize, const Point & dimensions);
	~MapCache();

	void setViewCenter(const Point & center, int level);

	Rect getVisibleAreaTiles() const;
	int3 getTileCenter() const;
	int3 getTileAtPoint(const Point & position) const;
	Point getViewCenter() const;

	void update(uint32_t timeDelta);
	void render(Canvas & target);
};

class MapView : public CIntObject
{
	std::unique_ptr<MapCache> tilesCache;

	Point tileSize;

public:
	MapView(const Point & offset, const Point & dimensions);

	Rect getVisibleAreaTiles() const;
	Point getViewCenter() const;
	int3 getTileCenter() const;
	int3 getTileAtPoint(const Point & position) const;

	void setViewCenter(const int3 & position);
	void setViewCenter(const Point & position, int level);

	void show(SDL_Surface * to) override;
	void showAll(SDL_Surface * to) override;
};
