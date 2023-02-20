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
#include "../lib/int3.h"
#include "MapRendererContext.h"

class IImage;
class Canvas;
class MapRenderer;
class MapViewController;

struct HeroAnimationState
{
	ObjectInstanceID target;
	int3 tileFrom;
	int3 tileDest;
	double progress;
};

struct FadingAnimationState
{
	ObjectInstanceID target;
	double progress;
};

class MapRendererContext : public IMapRendererContext
{
	friend class MapViewController;

	boost::multi_array<MapObjectsList, 3> objects;

	//Point tileSize = Point(32, 32);
	uint32_t animationTime = 0;

	boost::optional<HeroAnimationState> movementAnimation;
	boost::optional<HeroAnimationState> teleportAnimation;

	boost::optional<FadingAnimationState> fadeOutAnimation;
	boost::optional<FadingAnimationState> fadeInAnimation;

	bool worldViewModeActive = false;

public:
	MapRendererContext();

	void addObject(const CGObjectInstance * object);
	void addMovingObject(const CGObjectInstance * object, const int3 & tileFrom, const int3 & tileDest);
	void removeObject(const CGObjectInstance * object);

	int3 getMapSize() const final;
	bool isInMap(const int3 & coordinates) const final;
	bool isVisible(const int3 & coordinates) const override;

	const TerrainTile & getMapTile(const int3 & coordinates) const override;
	const MapObjectsList & getObjects(const int3 & coordinates) const override;
	const CGObjectInstance * getObject(ObjectInstanceID objectID) const override;
	const CGPath * currentPath() const override;

	size_t objectGroupIndex(ObjectInstanceID objectID) const override;
	Point objectImageOffset(ObjectInstanceID objectID, const int3 & coordinates) const override;
	double objectTransparency(ObjectInstanceID objectID) const override;
	size_t objectImageIndex(ObjectInstanceID objectID, size_t groupSize) const override;
	size_t terrainImageIndex(size_t groupSize) const override;
//	Point getTileSize() const override;

	bool showOverlay() const override;
	bool showGrid() const override;
	bool showVisitable() const override;
	bool showBlockable() const override;
};

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

/// Class responsible for rendering of entire map view
/// uses rendering parameters provided by owner class
class MapViewCache
{
	std::shared_ptr<MapViewModel> model;

	std::unique_ptr<Canvas> terrain;
	std::unique_ptr<Canvas> terrainTransition;
	std::unique_ptr<Canvas> intermediate;
	std::unique_ptr<MapRenderer> mapRenderer;

	std::unique_ptr<CAnimation> iconsStorage;

	Canvas getTile(const int3 & coordinates);
	void updateTile(const std::shared_ptr<MapRendererContext> & context, const int3 & coordinates);

	std::shared_ptr<IImage> getOverlayImageForTile(const std::shared_ptr<MapRendererContext> & context, const int3 & coordinates);
public:
	explicit MapViewCache(const std::shared_ptr<MapViewModel> & model);
	~MapViewCache();

	/// updates internal terrain cache according to provided time delta
	void update(const std::shared_ptr<MapRendererContext> & context);

	/// renders updated terrain cache onto provided canvas
	void render(const std::shared_ptr<MapRendererContext> &context, Canvas & target);
};

/// Class responsible for updating view state,
/// such as its position and any animations
class MapViewController : public IMapObjectObserver
{
	std::shared_ptr<MapRendererContext> context;
	std::shared_ptr<MapViewModel> model;

private:
	// IMapObjectObserver impl
	bool hasOngoingAnimations() override;
	void onObjectFadeIn(const CGObjectInstance * obj) override;
	void onObjectFadeOut(const CGObjectInstance * obj) override;
	void onObjectInstantAdd(const CGObjectInstance * obj) override;
	void onObjectInstantRemove(const CGObjectInstance * obj) override;
	void onHeroTeleported(const CGHeroInstance * obj, const int3 & from, const int3 & dest) override;
	void onHeroMoved(const CGHeroInstance * obj, const int3 & from, const int3 & dest) override;
	void onHeroRotated(const CGHeroInstance * obj, const int3 & from, const int3 & dest) override;

public:
	MapViewController(std::shared_ptr<MapRendererContext> context, std::shared_ptr<MapViewModel> model);

	void setViewCenter(const int3 & position);
	void setViewCenter(const Point & position, int level);
	void setTileSize(const Point & tileSize);
	void update(uint32_t timeDelta);
};

/// Main map rendering class that mostly acts as container for component classes
class MapView : public CIntObject
{
	std::shared_ptr<MapViewModel> model;
	std::shared_ptr<MapRendererContext> context;
	std::unique_ptr<MapViewCache> tilesCache;
	std::shared_ptr<MapViewController> controller;

	std::shared_ptr<MapViewModel> createModel(const Point & dimensions) const;

public:
	std::shared_ptr<const MapViewModel> getModel() const;
	std::shared_ptr<MapViewController> getController();

	MapView(const Point & offset, const Point & dimensions);

	void show(SDL_Surface * to) override;
	void showAll(SDL_Surface * to) override;
};
