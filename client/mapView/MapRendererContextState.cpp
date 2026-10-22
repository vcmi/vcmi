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
#include "../../lib/mapObjects/MapObjectDrawOrder.h"
#include "../../lib/mapObjects/ObjectTemplate.h"
#include "../../lib/mapping/CMap.h"

#include <tbb/parallel_for.h>

static const CGObjectInstance * getMapObject(ObjectInstanceID id)
{
	//FIXME: remove mh access
	return GAME->map().getMap()->getObject(id);
}

MapRendererContextState::MapRendererContextState()
	: objects(GAME->interface()->cb->getMapSize())
	, orderedObjects(GAME->interface()->cb->getMapSize())
{
	logGlobal->debug("Loading map objects");

	for(const auto & obj : GAME->map().getMap()->getObjects())
		stampObject(obj, false);

	const int3 mapSize = GAME->interface()->cb->getMapSize();

	// order of objects on a tile depends only on objects of that tile, so every tile can be ordered on its own
	tbb::parallel_for(0, mapSize.z * mapSize.x, [&](int column)
	{
		for(int y = 0; y < mapSize.y; ++y)
			orderTile(int3(column % mapSize.x, y, column / mapSize.x));
	});

	logGlobal->debug("Done loading map objects");
}

void MapRendererContextState::orderTile(const int3 & tile)
{
	const CMap & map = *GAME->map().getMap();

	MapObjectsList loadOrder;
	std::swap(loadOrder, orderedObjects[tile]);

	auto & ordered = orderedObjects[tile];
	ordered.reserve(loadOrder.size());

	for(const auto & objectID : loadOrder)
		ordered.insert(MapObjectDrawOrder::findInsertPosition(ordered, map, map.getObject(objectID), tile, [&](ObjectInstanceID id) { return map.getObject(id); }), objectID);

	updateVisibleObjects(tile);
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

		if(object && MapObjectDrawOrder::usesFixedDrawSlot(object))
			visible.push_back(objectID);
	}

	objects[tile].assign(visible.begin(), visible.end());
}

void MapRendererContextState::addObject(const CGObjectInstance * obj)
{
	stampObject(obj, true);
}

void MapRendererContextState::stampObject(const CGObjectInstance * obj, bool orderTiles)
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

			if(MapObjectDrawOrder::usesFixedDrawSlot(obj))
			{
				if(obj->coveringAt(currTile))
				{
					objects[currTile].push_back(obj->id);
					usedTiles[obj->id].push_back(currTile);
				}
				continue;
			}

			// like in H3 every cell of the object takes part in ordering, even if nothing is drawn there
			const CMap & map = *GAME->map().getMap();
			auto & ordered = orderedObjects[currTile];
			usedTiles[obj->id].push_back(currTile);

			if(!orderTiles)
			{
				ordered.push_back(obj->id);
				continue;
			}

			ordered.insert(MapObjectDrawOrder::findInsertPosition(ordered, map, obj, currTile, [&](ObjectInstanceID id) { return map.getObject(id); }), obj->id);

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
