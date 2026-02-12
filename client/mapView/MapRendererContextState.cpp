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
#include "../../lib/mapping/CMap.h"

namespace
{
CPlayerInterface * getInterface()
{
	return GAME ? GAME->interface() : nullptr;
}

std::shared_ptr<CCallback> getCallback()
{
	auto * interface = getInterface();
	if(!interface)
		return {};

	return interface->cb;
}

int3 getMapSizeSafe()
{
	auto callback = getCallback();
	if(callback)
		return callback->getMapSize();

	const auto * map = GAME->map().getMap();
	return int3(map->width, map->height, map->mapLevels);
}

bool isInMapSafe(const int3 & tile)
{
	auto callback = getCallback();
	if(callback)
		return callback->isInTheMap(tile);

	return GAME->map().getMap()->isInTheMap(tile);
}
}

static bool compareObjectBlitOrder(ObjectInstanceID left, ObjectInstanceID right)
{
	//FIXME: remove mh access
	return CMap::compareObjectBlitOrder(GAME->map().getMap()->getObject(left), GAME->map().getMap()->getObject(right));
}

MapRendererContextState::MapRendererContextState()
{
	auto mapSize = getMapSizeSafe();

	objects.resize(boost::extents[mapSize.z][mapSize.x][mapSize.y]);

	logGlobal->debug("Loading map objects");
	for(const auto & obj : GAME->map().getMap()->getObjects())
		addObject(obj);
	logGlobal->debug("Done loading map objects");
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

			if(isInMapSafe(currTile) && obj->coveringAt(currTile))
			{
				auto & container = objects[currTile.z][currTile.x][currTile.y];
				auto position = std::upper_bound(container.begin(), container.end(), obj->id, compareObjectBlitOrder);
				container.insert(position, obj->id);
			}
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

			if(isInMapSafe(currTile))
			{
				auto & container = objects[currTile.z][currTile.x][currTile.y];

				container.push_back(object->id);
				boost::range::sort(container, compareObjectBlitOrder);
			}
		}
	}
}

void MapRendererContextState::removeObject(const CGObjectInstance * object)
{
	auto mapSize = getMapSizeSafe();

	for(int z = 0; z < mapSize.z; z++)
		for(int x = 0; x < mapSize.x; x++)
			for(int y = 0; y < mapSize.y; y++)
				vstd::erase(objects[z][x][y], object->id);
}
