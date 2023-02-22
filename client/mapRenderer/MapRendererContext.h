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

VCMI_LIB_NAMESPACE_BEGIN
struct ObjectPosInfo;
VCMI_LIB_NAMESPACE_END

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

// from VwSymbol.def
enum class EWorldViewIcon
{
	TOWN = 0,
	HERO = 1,
	ARTIFACT = 2,
	TELEPORT = 3,
	GATE = 4,
	MINE_WOOD = 5,
	MINE_MERCURY = 6,
	MINE_STONE = 7,
	MINE_SULFUR = 8,
	MINE_CRYSTAL = 9,
	MINE_GEM = 10,
	MINE_GOLD = 11,
	RES_WOOD = 12,
	RES_MERCURY = 13,
	RES_STONE = 14,
	RES_SULFUR = 15,
	RES_CRYSTAL = 16,
	RES_GEM = 17,
	RES_GOLD = 18,

	ICONS_PER_PLAYER = 19,
	ICONS_TOTAL = 19 * 9 // 8 players + neutral set at the end
};

class MapRendererContext : public IMapRendererContext
{
	friend class MapViewController;

	boost::multi_array<MapObjectsList, 3> objects;

	uint32_t animationTime = 0;

	boost::optional<HeroAnimationState> movementAnimation;
	boost::optional<HeroAnimationState> teleportAnimation;

	boost::optional<FadingAnimationState> fadeOutAnimation;
	boost::optional<FadingAnimationState> fadeInAnimation;

	std::vector<ObjectPosInfo> additionalOverlayIcons;

	bool worldViewModeActive = false;
	bool showAllTerrain = false;
	bool settingsSessionSpectate = false;
	bool settingsAdventureObjectAnimation = true;
	bool settingsAdventureTerrainAnimation = true;
	bool settingsSessionShowGrid = false;
	bool settingsSessionShowVisitable = false;
	bool settingsSessionShowBlockable = false;

	size_t selectOverlayImageForObject(const ObjectPosInfo & objectID) const;

public:
	MapRendererContext();

	void addObject(const CGObjectInstance * object);
	void addMovingObject(const CGObjectInstance * object, const int3 & tileFrom, const int3 & tileDest);
	void removeObject(const CGObjectInstance * object);

	int3 getMapSize() const final;
	bool isInMap(const int3 & coordinates) const final;
	bool isVisible(const int3 & coordinates) const override;
	bool tileAnimated(const int3 & coordinates) const override;

	const TerrainTile & getMapTile(const int3 & coordinates) const override;
	const MapObjectsList & getObjects(const int3 & coordinates) const override;
	const CGObjectInstance * getObject(ObjectInstanceID objectID) const override;
	const CGPath * currentPath() const override;

	size_t objectGroupIndex(ObjectInstanceID objectID) const override;
	Point objectImageOffset(ObjectInstanceID objectID, const int3 & coordinates) const override;
	double objectTransparency(ObjectInstanceID objectID, const int3 &coordinates) const override;
	size_t objectImageIndex(ObjectInstanceID objectID, size_t groupSize) const override;
	size_t terrainImageIndex(size_t groupSize) const override;
	size_t overlayImageIndex(const int3 & coordinates) const override;

	bool showOverlay() const override;
	bool showGrid() const override;
	bool showVisitable() const override;
	bool showBlockable() const override;
};
