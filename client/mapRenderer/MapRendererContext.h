/*
 * MapRendererContext.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "IMapRendererContext.h"

#include "../lib/int3.h"
#include "../lib/GameConstants.h"

class MapObjectsSorter
{
	const IMapRendererContext & context;

public:
	explicit MapObjectsSorter(const IMapRendererContext & context);

	bool operator()(const ObjectInstanceID & left, const ObjectInstanceID & right) const;
	bool operator()(const CGObjectInstance * left, const CGObjectInstance * right) const;
};

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
