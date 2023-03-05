/*
 * MapRendererContextState.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "MapRendererContext.h"

#include "MapRendererContextState.h"
#include "mapHandler.h"

#include "../../CCallback.h"
#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../adventureMap/CAdvMapInt.h"

#include "../../lib/CPathfinder.h"
#include "../../lib/Point.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapping/CMap.h"

MapRendererBaseContext::MapRendererBaseContext(const MapRendererContextState & viewState)
	: viewState(viewState)
{
}

uint32_t MapRendererBaseContext::getObjectRotation(ObjectInstanceID objectID) const
{
	const CGObjectInstance * obj = getObject(objectID);

	if(obj->ID == Obj::HERO)
	{
		const auto * hero = dynamic_cast<const CGHeroInstance *>(obj);
		return hero->moveDir;
	}

	if(obj->ID == Obj::BOAT)
	{
		const auto * boat = dynamic_cast<const CGBoat *>(obj);

		if(boat->hero)
			return boat->hero->moveDir;
		return boat->direction;
	}
	return 0;
}

int3 MapRendererBaseContext::getMapSize() const
{
	return LOCPLINT->cb->getMapSize();
}

bool MapRendererBaseContext::isInMap(const int3 & coordinates) const
{
	return LOCPLINT->cb->isInTheMap(coordinates);
}

bool MapRendererBaseContext::isVisible(const int3 & coordinates) const
{
	if(settingsSessionSpectate)
		return LOCPLINT->cb->isInTheMap(coordinates);
	else
		return LOCPLINT->cb->isVisible(coordinates);
}

bool MapRendererBaseContext::tileAnimated(const int3 & coordinates) const
{
	return false;
}

const TerrainTile & MapRendererBaseContext::getMapTile(const int3 & coordinates) const
{
	return CGI->mh->getMap()->getTile(coordinates);
}

const MapRendererBaseContext::MapObjectsList & MapRendererBaseContext::getObjects(const int3 & coordinates) const
{
	assert(isInMap(coordinates));
	return viewState.objects[coordinates.z][coordinates.x][coordinates.y];
}

const CGObjectInstance * MapRendererBaseContext::getObject(ObjectInstanceID objectID) const
{
	return CGI->mh->getMap()->objects.at(objectID.getNum());
}

const CGPath * MapRendererBaseContext::currentPath() const
{
	return nullptr;
}

size_t MapRendererBaseContext::objectGroupIndex(ObjectInstanceID objectID) const
{
	static const std::vector<size_t> idleGroups = {0, 13, 0, 1, 2, 3, 4, 15, 14};
	return idleGroups[getObjectRotation(objectID)];
}

Point MapRendererBaseContext::objectImageOffset(ObjectInstanceID objectID, const int3 & coordinates) const
{
	const CGObjectInstance * object = getObject(objectID);
	int3 offsetTiles(object->getPosition() - coordinates);
	return Point(offsetTiles) * Point(32, 32);
}

double MapRendererBaseContext::objectTransparency(ObjectInstanceID objectID, const int3 & coordinates) const
{
	const CGObjectInstance * object = getObject(objectID);

	if(object->ID == Obj::HERO)
	{
		const auto * hero = dynamic_cast<const CGHeroInstance *>(object);

		if(hero->inTownGarrison)
			return 0;

		if(hero->boat)
			return 0;
	}
	return 1;
}

size_t MapRendererBaseContext::objectImageIndex(ObjectInstanceID objectID, size_t groupSize) const
{
	return 0;
}

size_t MapRendererBaseContext::terrainImageIndex(size_t groupSize) const
{
	return 0;
}

size_t MapRendererBaseContext::overlayImageIndex(const int3 & coordinates) const
{
	return std::numeric_limits<size_t>::max();
}

double MapRendererBaseContext::viewTransitionProgress() const
{
	return 0;
}

bool MapRendererBaseContext::filterGrayscale() const
{
	return false;
}

bool MapRendererBaseContext::showRoads() const
{
	return true;
}

bool MapRendererBaseContext::showRivers() const
{
	return true;
}

bool MapRendererBaseContext::showBorder() const
{
	return false;
}

bool MapRendererBaseContext::showOverlay() const
{
	return false;
}

bool MapRendererBaseContext::showGrid() const
{
	return false;
}

bool MapRendererBaseContext::showVisitable() const
{
	return false;
}

bool MapRendererBaseContext::showBlockable() const
{
	return false;
}

MapRendererAdventureContext::MapRendererAdventureContext(const MapRendererContextState & viewState)
	: MapRendererBaseContext(viewState)
{
}

const CGPath * MapRendererAdventureContext::currentPath() const
{
	const auto * hero = adventureInt->curHero();

	if(!hero)
		return nullptr;

	if(!LOCPLINT->paths.hasPath(hero))
		return nullptr;

	return &LOCPLINT->paths.getPath(hero);
}

size_t MapRendererAdventureContext::objectImageIndex(ObjectInstanceID objectID, size_t groupSize) const
{
	assert(groupSize > 0);

	if(!settingsAdventureObjectAnimation)
		return 0;

	if(groupSize == 0)
		return 0;

	// usign objectID for frameCounter to add pseudo-random element per-object.
	// Without it, animation of multiple visible objects of the same type will always be in sync
	size_t baseFrameTime = 180;
	size_t frameCounter = animationTime / baseFrameTime + objectID.getNum();
	size_t frameIndex = frameCounter % groupSize;
	return frameIndex;
}

size_t MapRendererAdventureContext::terrainImageIndex(size_t groupSize) const
{
	if(!settingsAdventureTerrainAnimation)
		return 0;

	size_t baseFrameTime = 180;
	size_t frameCounter = animationTime / baseFrameTime;
	size_t frameIndex = frameCounter % groupSize;
	return frameIndex;
}

bool MapRendererAdventureContext::showBorder() const
{
	return true;
}

bool MapRendererAdventureContext::showGrid() const
{
	return settingShowGrid;
}

bool MapRendererAdventureContext::showVisitable() const
{
	return settingShowVisitable;
}

bool MapRendererAdventureContext::showBlockable() const
{
	return settingShowBlockable;
}

MapRendererAdventureTransitionContext::MapRendererAdventureTransitionContext(const MapRendererContextState & viewState)
	: MapRendererAdventureContext(viewState)
{
}

double MapRendererAdventureTransitionContext::viewTransitionProgress() const
{
	return progress;
}

MapRendererAdventureFadingContext::MapRendererAdventureFadingContext(const MapRendererContextState & viewState)
	: MapRendererAdventureContext(viewState)
{
}

bool MapRendererAdventureFadingContext::tileAnimated(const int3 & coordinates) const
{
	if(!isInMap(coordinates))
		return false;

	auto objects = getObjects(coordinates);
	if(vstd::contains(objects, target))
		return true;

	return false;
}

double MapRendererAdventureFadingContext::objectTransparency(ObjectInstanceID objectID, const int3 & coordinates) const
{
	if(objectID == target)
		return progress;

	return MapRendererAdventureContext::objectTransparency(objectID, coordinates);
}

MapRendererAdventureMovingContext::MapRendererAdventureMovingContext(const MapRendererContextState & viewState)
	: MapRendererAdventureContext(viewState)
{
}

size_t MapRendererAdventureMovingContext::objectGroupIndex(ObjectInstanceID objectID) const
{
	if(target == objectID)
	{
		static const std::vector<size_t> moveGroups = {0, 10, 5, 6, 7, 8, 9, 12, 11};
		return moveGroups[getObjectRotation(objectID)];
	}
	return MapRendererAdventureContext::objectGroupIndex(objectID);
}

bool MapRendererAdventureMovingContext::tileAnimated(const int3 & coordinates) const
{
	if(!isInMap(coordinates))
		return false;

	auto objects = getObjects(coordinates);
	if(vstd::contains(objects, target))
		return true;

	return false;
}

Point MapRendererAdventureMovingContext::objectImageOffset(ObjectInstanceID objectID, const int3 & coordinates) const
{
	if(target == objectID)
	{
		int3 offsetTilesFrom = tileFrom - coordinates;
		int3 offsetTilesDest = tileDest - coordinates;

		Point offsetPixelsFrom = Point(offsetTilesFrom) * Point(32, 32);
		Point offsetPixelsDest = Point(offsetTilesDest) * Point(32, 32);

		Point result = vstd::lerp(offsetPixelsFrom, offsetPixelsDest, progress);

		return result;
	}

	return MapRendererAdventureContext::objectImageOffset(objectID, coordinates);
}

size_t MapRendererAdventureMovingContext::objectImageIndex(ObjectInstanceID objectID, size_t groupSize) const
{
	if(target != objectID)
		return MapRendererAdventureContext::objectImageIndex(objectID, groupSize);

	int32_t baseFrameTime = 50;
	size_t frameCounter = animationTime / baseFrameTime;
	size_t frameIndex = frameCounter % groupSize;
	return frameIndex;
}

size_t MapRendererWorldViewContext::selectOverlayImageForObject(const ObjectPosInfo & object) const
{
	size_t ownerIndex = PlayerColor::PLAYER_LIMIT.getNum() * static_cast<size_t>(EWorldViewIcon::ICONS_PER_PLAYER);

	if(object.owner.isValidPlayer())
		ownerIndex = object.owner.getNum() * static_cast<size_t>(EWorldViewIcon::ICONS_PER_PLAYER);

	switch(object.id)
	{
		case Obj::MONOLITH_ONE_WAY_ENTRANCE:
		case Obj::MONOLITH_ONE_WAY_EXIT:
		case Obj::MONOLITH_TWO_WAY:
			return ownerIndex + static_cast<size_t>(EWorldViewIcon::TELEPORT);
		case Obj::SUBTERRANEAN_GATE:
			return ownerIndex + static_cast<size_t>(EWorldViewIcon::GATE);
		case Obj::ARTIFACT:
			return ownerIndex + static_cast<size_t>(EWorldViewIcon::ARTIFACT);
		case Obj::TOWN:
			return ownerIndex + static_cast<size_t>(EWorldViewIcon::TOWN);
		case Obj::HERO:
			return ownerIndex + static_cast<size_t>(EWorldViewIcon::HERO);
		case Obj::MINE:
			return ownerIndex + static_cast<size_t>(EWorldViewIcon::MINE_WOOD) + object.subId;
		case Obj::RESOURCE:
			return ownerIndex + static_cast<size_t>(EWorldViewIcon::RES_WOOD) + object.subId;
	}
	return std::numeric_limits<size_t>::max();
}

MapRendererWorldViewContext::MapRendererWorldViewContext(const MapRendererContextState & viewState)
	: MapRendererBaseContext(viewState)
{
}

bool MapRendererWorldViewContext::showOverlay() const
{
	return true;
}

size_t MapRendererWorldViewContext::overlayImageIndex(const int3 & coordinates) const
{
	if(!isVisible(coordinates))
		return std::numeric_limits<size_t>::max();

	for(const auto & objectID : getObjects(coordinates))
	{
		const auto * object = getObject(objectID);

		if(!object->visitableAt(coordinates.x, coordinates.y))
			continue;

		ObjectPosInfo info;
		info.pos = coordinates;
		info.id = object->ID;
		info.subId = object->subID;
		info.owner = object->tempOwner;

		size_t iconIndex = selectOverlayImageForObject(info);

		if(iconIndex != std::numeric_limits<size_t>::max())
			return iconIndex;
	}

	return std::numeric_limits<size_t>::max();
}

MapRendererSpellViewContext::MapRendererSpellViewContext(const MapRendererContextState & viewState)
	: MapRendererWorldViewContext(viewState)
{
}

double MapRendererSpellViewContext::objectTransparency(ObjectInstanceID objectID, const int3 & coordinates) const
{
	if(showAllTerrain)
	{
		if(getObject(objectID)->isVisitable() && !MapRendererWorldViewContext::isVisible(coordinates))
			return 0;
	}

	return MapRendererWorldViewContext::objectTransparency(objectID, coordinates);
}

bool MapRendererSpellViewContext::isVisible(const int3 & coordinates) const
{
	if(showAllTerrain)
		return isInMap(coordinates);
	return MapRendererBaseContext::isVisible(coordinates);
}

size_t MapRendererSpellViewContext::overlayImageIndex(const int3 & coordinates) const
{
	for(const auto & entry : additionalOverlayIcons)
	{
		if(entry.pos != coordinates)
			continue;

		size_t iconIndex = selectOverlayImageForObject(entry);

		if(iconIndex != std::numeric_limits<size_t>::max())
			return iconIndex;
	}

	return MapRendererWorldViewContext::overlayImageIndex(coordinates);
}

MapRendererPuzzleMapContext::MapRendererPuzzleMapContext(const MapRendererContextState & viewState)
	: MapRendererBaseContext(viewState)
{
}

MapRendererPuzzleMapContext::~MapRendererPuzzleMapContext() = default;

const CGPath * MapRendererPuzzleMapContext::currentPath() const
{
	return grailPos.get();
}

double MapRendererPuzzleMapContext::objectTransparency(ObjectInstanceID objectID, const int3 & coordinates) const
{
	const auto * object = getObject(objectID);

	if(!object)
		return 0;

	if(object->isVisitable())
		return 0;

	if(object->ID == Obj::HOLE)
		return 0;

	return MapRendererBaseContext::objectTransparency(objectID, coordinates);
}

bool MapRendererPuzzleMapContext::isVisible(const int3 & coordinates) const
{
	return LOCPLINT->cb->isInTheMap(coordinates);
}

bool MapRendererPuzzleMapContext::filterGrayscale() const
{
	return true;
}

bool MapRendererPuzzleMapContext::showRoads() const
{
	return false;
}

bool MapRendererPuzzleMapContext::showRivers() const
{
	return false;
}
