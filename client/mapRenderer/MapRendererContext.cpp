/*
 * MapRendererContext.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "MapRendererContext.h"

#include "mapHandler.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../adventureMap/CAdvMapInt.h"
#include "../../CCallback.h"

#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapping/CMap.h"

MapObjectsSorter::MapObjectsSorter(const IMapRendererContext & context)
	: context(context)
{
}

bool MapObjectsSorter::operator()(const ObjectInstanceID & left, const ObjectInstanceID & right) const
{
	return (*this)(context.getObject(left), context.getObject(right));
}

bool MapObjectsSorter::operator()(const CGObjectInstance * left, const CGObjectInstance * right) const
{
	//FIXME: remove mh access
	return CGI->mh->compareObjectBlitOrder(left, right);
}

int3 MapRendererContext::getMapSize() const
{
	return LOCPLINT->cb->getMapSize();
}

bool MapRendererContext::isInMap(const int3 & coordinates) const
{
	return LOCPLINT->cb->isInTheMap(coordinates);
}

const TerrainTile & MapRendererContext::getMapTile(const int3 & coordinates) const
{
	return CGI->mh->getMap()->getTile(coordinates);
}

const CGObjectInstance * MapRendererContext::getObject(ObjectInstanceID objectID) const
{
	return CGI->mh->getMap()->objects.at(objectID.getNum());
}

bool MapRendererContext::isVisible(const int3 & coordinates) const
{
	if (settingsSessionSpectate || showAllTerrain)
		return LOCPLINT->cb->isInTheMap(coordinates);
	return LOCPLINT->cb->isVisible(coordinates);
}

const CGPath * MapRendererContext::currentPath() const
{
	const auto * hero = adventureInt->curHero();

	if(!hero)
		return nullptr;

	if(!LOCPLINT->paths.hasPath(hero))
		return nullptr;

	return &LOCPLINT->paths.getPath(hero);
}

size_t MapRendererContext::objectImageIndex(ObjectInstanceID objectID, size_t groupSize) const
{
	assert(groupSize > 0);
	if(groupSize == 0)
		return 0;

	if (!settingsAdventureObjectAnimation)
		return 0;

	// H3 timing for adventure map objects animation is 180 ms
	// Terrain animations also use identical interval, however those are only present in HotA and/or HD Mod
	size_t baseFrameTime = 180;

	// hero movement animation always plays at ~50ms / frame
	// in-game setting only affect movement across screen
	if(movementAnimation && movementAnimation->target == objectID)
		baseFrameTime = 50;

	size_t frameCounter = animationTime / baseFrameTime;
	size_t frameIndex = frameCounter % groupSize;
	return frameIndex;
}

size_t MapRendererContext::terrainImageIndex(size_t groupSize) const
{
	if (!settingsAdventureTerrainAnimation)
		return 0;

	size_t baseFrameTime = 180;
	size_t frameCounter = animationTime / baseFrameTime;
	size_t frameIndex = frameCounter % groupSize;
	return frameIndex;
}

bool MapRendererContext::tileAnimated(const int3 & coordinates) const
{
	if (!isInMap(coordinates))
		return false;

	if(movementAnimation)
	{
		auto objects = getObjects(coordinates);

		if(vstd::contains(objects, movementAnimation->target))
			return true;
	}

	if(fadeInAnimation)
	{
		auto objects = getObjects(coordinates);

		if(vstd::contains(objects, fadeInAnimation->target))
			return true;
	}

	if(fadeOutAnimation)
	{
		auto objects = getObjects(coordinates);

		if(vstd::contains(objects, fadeOutAnimation->target))
			return true;
	}
	return false;
}

bool MapRendererContext::showOverlay() const
{
	return worldViewModeActive;
}

bool MapRendererContext::showGrid() const
{
<<<<<<< HEAD
	return settings["gameTweaks"]["showGrid"].Bool();
=======
	return settingsSessionShowGrid;
>>>>>>> 9a847b520 (Working version of image caching)
}

bool MapRendererContext::showVisitable() const
{
	return settingsSessionShowVisitable;
}

bool MapRendererContext::showBlockable() const
{
	return settingsSessionShowBlockable;
}

MapRendererContext::MapRendererContext()
{
	auto mapSize = getMapSize();

	objects.resize(boost::extents[mapSize.z][mapSize.x][mapSize.y]);

	for(const auto & obj : CGI->mh->getMap()->objects)
		addObject(obj);
}

void MapRendererContext::addObject(const CGObjectInstance * obj)
{
	if(!obj)
		return;

	for(int fx = 0; fx < obj->getWidth(); ++fx)
	{
		for(int fy = 0; fy < obj->getHeight(); ++fy)
		{
			int3 currTile(obj->pos.x - fx, obj->pos.y - fy, obj->pos.z);

			if(isInMap(currTile) && obj->coveringAt(currTile.x, currTile.y))
			{
				auto & container = objects[currTile.z][currTile.x][currTile.y];

				container.push_back(obj->id);
				boost::range::sort(container, MapObjectsSorter(*this));
			}
		}
	}
}

void MapRendererContext::addMovingObject(const CGObjectInstance * object, const int3 & tileFrom, const int3 & tileDest)
{
	int xFrom = std::min(tileFrom.x, tileDest.x) - object->getWidth();
	int xDest = std::max(tileFrom.x, tileDest.x);
	int yFrom = std::min(tileFrom.y, tileDest.y) - object->getHeight();
	int yDest = std::max(tileFrom.y, tileDest.y);

	for(int x = xFrom; x <= xDest; ++x)
	{
		for(int y = yFrom; y <= yDest; ++y)
		{
			int3 currTile(x, y, object->pos.z);

			if(isInMap(currTile))
			{
				auto & container = objects[currTile.z][currTile.x][currTile.y];

				container.push_back(object->id);
				boost::range::sort(container, MapObjectsSorter(*this));
			}
		}
	}
}

void MapRendererContext::removeObject(const CGObjectInstance * object)
{
	for(int z = 0; z < getMapSize().z; z++)
		for(int x = 0; x < getMapSize().x; x++)
			for(int y = 0; y < getMapSize().y; y++)
				vstd::erase(objects[z][x][y], object->id);
}

const MapRendererContext::MapObjectsList & MapRendererContext::getObjects(const int3 & coordinates) const
{
	assert(isInMap(coordinates));
	return objects[coordinates.z][coordinates.x][coordinates.y];
}

size_t MapRendererContext::objectGroupIndex(ObjectInstanceID objectID) const
{
	const CGObjectInstance * obj = getObject(objectID);
	// TODO
	static const std::vector<size_t> moveGroups = {99, 10, 5, 6, 7, 8, 9, 12, 11};
	static const std::vector<size_t> idleGroups = {99, 13, 0, 1, 2, 3, 4, 15, 14};

	if(obj->ID == Obj::HERO)
	{
		const auto * hero = dynamic_cast<const CGHeroInstance *>(obj);
		if(movementAnimation && movementAnimation->target == objectID)
			return moveGroups[hero->moveDir];
		return idleGroups[hero->moveDir];
	}

	if(obj->ID == Obj::BOAT)
	{
		const auto * boat = dynamic_cast<const CGBoat *>(obj);

		uint8_t direction = boat->hero ? boat->hero->moveDir : boat->direction;

		if(movementAnimation && movementAnimation->target == objectID)
			return moveGroups[direction];
		return idleGroups[direction];
	}
	return 0;
}

Point MapRendererContext::objectImageOffset(ObjectInstanceID objectID, const int3 & coordinates) const
{
	if(movementAnimation && movementAnimation->target == objectID)
	{
		int3 offsetTilesFrom = movementAnimation->tileFrom - coordinates;
		int3 offsetTilesDest = movementAnimation->tileDest - coordinates;

		Point offsetPixelsFrom = Point(offsetTilesFrom) * Point(32, 32);
		Point offsetPixelsDest = Point(offsetTilesDest) * Point(32, 32);

		Point result = vstd::lerp(offsetPixelsFrom, offsetPixelsDest, movementAnimation->progress);

		return result;
	}

	const CGObjectInstance * object = getObject(objectID);
	int3 offsetTiles(object->getPosition() - coordinates);
	return Point(offsetTiles) * Point(32, 32);
}

double MapRendererContext::objectTransparency(ObjectInstanceID objectID, const int3 & coordinates) const
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

	if(showAllTerrain)
	{
		if(object->isVisitable() && !LOCPLINT->cb->isVisible(coordinates))
			return 0;
	}

	if(fadeOutAnimation && objectID == fadeOutAnimation->target)
		return 1.0 - fadeOutAnimation->progress;

	if(fadeInAnimation && objectID == fadeInAnimation->target)
		return fadeInAnimation->progress;

	return 1.0;
}

size_t MapRendererContext::selectOverlayImageForObject(const ObjectPosInfo & object) const
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

size_t MapRendererContext::overlayImageIndex(const int3 & coordinates) const
{
	for(const auto & entry : additionalOverlayIcons)
	{
		if(entry.pos != coordinates)
			continue;

		size_t iconIndex = selectOverlayImageForObject(entry);

		if(iconIndex != std::numeric_limits<size_t>::max())
			return iconIndex;
	}

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
