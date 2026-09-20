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
#include "MapRendererContextState.h"

#include "IMapRendererContext.h"
#include "mapHandler.h"

#include "../CPlayerInterface.h"
#include "../GameInstance.h"
#include "../adventureMap/AdventureMapInterface.h"

#include "../../lib/callback/CCallback.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/ObjectTemplate.h"
#include "../../lib/mapping/CMap.h"

static const CGObjectInstance * getMapObject(ObjectInstanceID id)
{
	//FIXME: remove mh access
	return GAME->map().getMap()->getObject(id);
}

/// Whether some tile covered by both objects has a lower layer of the first object than of the second one
static bool isLowerInAnyTile(const CGObjectInstance * object, const CGObjectInstance * other)
{
	const int3 anchor = object->anchorPos();
	const int3 otherAnchor = other->anchorPos();

	const int left = std::max(anchor.x - object->getWidth() + 1, otherAnchor.x - other->getWidth() + 1);
	const int right = std::min(anchor.x, otherAnchor.x);
	const int top = std::max(anchor.y - object->getHeight() + 1, otherAnchor.y - other->getHeight() + 1);
	const int bottom = std::min(anchor.y, otherAnchor.y);

	for(int x = left; x <= right; ++x)
	{
		for(int y = top; y <= bottom; ++y)
		{
			const int3 tile(x, y, anchor.z);

			if(GAME->interface()->cb->isInTheMap(tile) && object->drawLayerAt(tile) < other->drawLayerAt(tile))
				return true;
		}
	}
	return false;
}

/// Whether the object goes below the one that is already on the tile in the same layer
static bool isDrawnBelow(const CGObjectInstance * object, const CGObjectInstance * other)
{
	// objects with print priority are drawn in order of it, the higher first
	const si32 priority = object->appearance->printPriority;
	const si32 otherPriority = other->appearance->printPriority;

	if(priority != otherPriority)
		return priority > otherPriority;

	return isLowerInAnyTile(object, other);
}

/// Puts the object into the list of a tile the way H3 does when it stamps an object onto the map
static void insertByDrawOrder(MapRendererContextState::MapObjectsList & container, const CGObjectInstance * object, const int3 & tile)
{
	const ui8 layer = object->drawLayerAt(tile);
	auto position = container.end();

	while(position != container.begin())
	{
		const auto * previous = getMapObject(*(position - 1));

		if(!previous)
			break;

		const ui8 previousLayer = previous->drawLayerAt(tile);

		if(layer > previousLayer)
			break;

		if(layer == previousLayer && !isDrawnBelow(object, previous))
			break;

		--position;
	}
	container.insert(position, object->id);
}

MapRendererContextState::MapRendererContextState()
	: objects(GAME->interface()->cb->getMapSize())
	, orderedObjects(GAME->interface()->cb->getMapSize())
{
	logGlobal->debug("Loading map objects");
	for(const auto & obj : GAME->map().getMap()->getObjects())
		addObject(obj);
	logGlobal->debug("Done loading map objects");
}

bool MapRendererContextState::usesFixedDrawSlot(const CGObjectInstance * object)
{
	return object->ID == Obj::HERO || object->ID == Obj::BOAT;
}

void MapRendererContextState::updateVisibleObjects(const int3 & tile)
{
	boost::container::small_vector<ObjectInstanceID, 8> visible;

	for(const auto & objectID : orderedObjects[tile])
	{
		const auto * object = getMapObject(objectID);

		if(object && object->coveringAt(tile))
			visible.push_back(objectID);
	}

	for(const auto & objectID : objects[tile])
	{
		const auto * object = getMapObject(objectID);

		if(object && usesFixedDrawSlot(object))
			visible.push_back(objectID);
	}

	objects[tile].assign(visible.begin(), visible.end());
}

void MapRendererContextState::addObject(const CGObjectInstance * obj)
{
	if(!obj)
		return;

	for(int fx = 0; fx < obj->getWidth(); ++fx)
	{
		for(int fy = 0; fy < obj->getHeight(); ++fy)
		{
			int3 currTile(obj->anchorPos().x - fx, obj->anchorPos().y - fy, obj->anchorPos().z);

			if(!GAME->interface()->cb->isInTheMap(currTile))
				continue;

			if(usesFixedDrawSlot(obj))
			{
				if(obj->coveringAt(currTile))
				{
					objects[currTile].push_back(obj->id);
					usedTiles[obj->id].push_back(currTile);
				}
				continue;
			}

			// like in H3 every cell of the object takes part in ordering, even if nothing is drawn there
			insertByDrawOrder(orderedObjects[currTile], obj, currTile);
			usedTiles[obj->id].push_back(currTile);

			if(obj->coveringAt(currTile))
				updateVisibleObjects(currTile);
		}
	}
}

void MapRendererContextState::addMovingObject(const CGObjectInstance * object, const int3 & tileFrom, const int3 & tileDest)
{
	int xFrom = std::min(tileFrom.x, tileDest.x) - object->getWidth();
	int xDest = std::max(tileFrom.x, tileDest.x);
	int yFrom = std::min(tileFrom.y, tileDest.y) - object->getHeight();
	int yDest = std::max(tileFrom.y, tileDest.y);

	for(int x = xFrom; x <= xDest; ++x)
	{
		for(int y = yFrom; y <= yDest; ++y)
		{
			int3 currTile(x, y, object->anchorPos().z);

			if(GAME->interface()->cb->isInTheMap(currTile))
			{
				// only heroes and boats move, and those are not ordered
				objects[currTile].push_back(object->id);
				usedTiles[object->id].push_back(currTile);
			}
		}
	}
}

void MapRendererContextState::removeObject(const CGObjectInstance * object)
{
	for (const auto & usedTile : usedTiles[object->id])
	{
		vstd::erase(orderedObjects[usedTile], object->id);
		vstd::erase(objects[usedTile], object->id);
	}

	usedTiles.erase(object->id);
}
