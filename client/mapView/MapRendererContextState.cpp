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

static bool compareObjectBlitOrder(ObjectInstanceID left, ObjectInstanceID right)
{
	//FIXME: remove mh access
	return CMap::compareObjectBlitOrder(GAME->map().getMap()->getObject(left), GAME->map().getMap()->getObject(right));
}

MapRendererContextState::MapRendererContextState()
	: objects(GAME->interface()->cb->getMapSize())
{
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

			if(GAME->interface()->cb->isInTheMap(currTile) && obj->coveringAt(currTile))
			{
				auto & container = objects[currTile];
				auto position = std::upper_bound(container.begin(), container.end(), obj->id, compareObjectBlitOrder);
				container.insert(position, obj->id);

				usedTiles[obj->id].push_back(currTile);
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

			if(GAME->interface()->cb->isInTheMap(currTile))
			{
				auto & container = objects[currTile];
				auto position = std::upper_bound(container.begin(), container.end(), object->id, compareObjectBlitOrder);
				container.insert(position, object->id);

				usedTiles[object->id].push_back(currTile);
			}
		}
	}
}

void MapRendererContextState::removeObject(const CGObjectInstance * object)
{
	for (const auto & usedTile : usedTiles[object->id])
		vstd::erase(objects[usedTile], object->id);

	usedTiles.erase(object->id);
}
