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

#include "../lib/GameConstants.h"
#include "../lib/int3.h"

VCMI_LIB_NAMESPACE_BEGIN
struct ObjectPosInfo;
VCMI_LIB_NAMESPACE_END

struct MapRendererContextState;

class MapRendererBaseContext : public IMapRendererContext
{
public:
	const MapRendererContextState & viewState;
	bool settingsSessionSpectate = false;

	explicit MapRendererBaseContext(const MapRendererContextState & viewState);

	uint32_t getObjectRotation(ObjectInstanceID objectID) const;

	int3 getMapSize() const override;
	bool isInMap(const int3 & coordinates) const override;
	bool isVisible(const int3 & coordinates) const override;
	bool tileAnimated(const int3 & coordinates) const override;

	bool isActiveHero(const CGObjectInstance* obj) const override;

	const TerrainTile & getMapTile(const int3 & coordinates) const override;
	const MapObjectsList & getObjects(const int3 & coordinates) const override;
	const CGObjectInstance * getObject(ObjectInstanceID objectID) const override;
	const CGPath * currentPath() const override;

	size_t objectGroupIndex(ObjectInstanceID objectID) const override;
	Point objectImageOffset(ObjectInstanceID objectID, const int3 & coordinates) const override;
	double objectTransparency(ObjectInstanceID objectID, const int3 & coordinates) const override;
	size_t objectImageIndex(ObjectInstanceID objectID, size_t groupSize) const override;
	size_t terrainImageIndex(size_t groupSize) const override;
	size_t overlayImageIndex(const int3 & coordinates) const override;

	double viewTransitionProgress() const override;
	bool filterGrayscale() const override;
	bool showRoads() const override;
	bool showRivers() const override;
	bool showBorder() const override;
	bool showOverlay() const override;
	bool showGrid() const override;
	bool showVisitable() const override;
	bool showBlocked() const override;
};

class MapRendererAdventureContext : public MapRendererBaseContext
{
public:
	uint32_t animationTime = 0;
	bool settingShowGrid = false;
	bool settingShowVisitable = false;
	bool settingShowBlocked = false;
	bool settingsAdventureObjectAnimation = true;
	bool settingsAdventureTerrainAnimation = true;

	explicit MapRendererAdventureContext(const MapRendererContextState & viewState);

	const CGPath * currentPath() const override;
	size_t objectImageIndex(ObjectInstanceID objectID, size_t groupSize) const override;
	size_t terrainImageIndex(size_t groupSize) const override;

	bool showBorder() const override;
	bool showGrid() const override;
	bool showVisitable() const override;
	bool showBlocked() const override;
};

class MapRendererAdventureTransitionContext : public MapRendererAdventureContext
{
public:
	double progress = 0;

	explicit MapRendererAdventureTransitionContext(const MapRendererContextState & viewState);

	double viewTransitionProgress() const override;
};

class MapRendererAdventureFadingContext : public MapRendererAdventureContext
{
public:
	ObjectInstanceID target;
	double progress;

	explicit MapRendererAdventureFadingContext(const MapRendererContextState & viewState);

	bool tileAnimated(const int3 & coordinates) const override;
	double objectTransparency(ObjectInstanceID objectID, const int3 & coordinates) const override;
};

class MapRendererAdventureMovingContext : public MapRendererAdventureContext
{
public:
	ObjectInstanceID target;
	int3 tileFrom;
	int3 tileDest;
	double progress;

	explicit MapRendererAdventureMovingContext(const MapRendererContextState & viewState);

	bool tileAnimated(const int3 & coordinates) const override;
	size_t objectGroupIndex(ObjectInstanceID objectID) const override;
	Point objectImageOffset(ObjectInstanceID objectID, const int3 & coordinates) const override;
	size_t objectImageIndex(ObjectInstanceID objectID, size_t groupSize) const override;
};

class MapRendererWorldViewContext : public MapRendererBaseContext
{
protected:
	size_t selectOverlayImageForObject(const ObjectPosInfo & object) const;

public:
	explicit MapRendererWorldViewContext(const MapRendererContextState & viewState);

	size_t overlayImageIndex(const int3 & coordinates) const override;
	bool showOverlay() const override;
};

class MapRendererSpellViewContext : public MapRendererWorldViewContext
{
public:
	std::vector<ObjectPosInfo> additionalOverlayIcons;
	bool showAllTerrain = false;

	explicit MapRendererSpellViewContext(const MapRendererContextState & viewState);

	bool isVisible(const int3 & coordinates) const override;
	double objectTransparency(ObjectInstanceID objectID, const int3 & coordinates) const override;
	size_t overlayImageIndex(const int3 & coordinates) const override;
};

class MapRendererPuzzleMapContext : public MapRendererBaseContext
{
public:
	std::unique_ptr<CGPath> grailPos;

	explicit MapRendererPuzzleMapContext(const MapRendererContextState & viewState);
	~MapRendererPuzzleMapContext();

	const CGPath * currentPath() const override;
	double objectTransparency(ObjectInstanceID objectID, const int3 & coordinates) const override;
	bool isVisible(const int3 & coordinates) const override;
	bool filterGrayscale() const override;
	bool showRoads() const override;
	bool showRivers() const override;
};
