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

#include "../../CCallback.h"
#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../adventureMap/CAdventureMapInterface.h"

#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapping/CMap.h"

static bool compareObjectBlitOrder(ObjectInstanceID left, ObjectInstanceID right)
{
	//FIXME: remove mh access
	return CGI->mh->compareObjectBlitOrder(CGI->mh->getMap()->objects[left.getNum()], CGI->mh->getMap()->objects[right.getNum()]);
}

MapRendererContextState::MapRendererContextState()
{
	auto mapSize = LOCPLINT->cb->getMapSize();

	objects.resize(boost::extents[mapSize.z][mapSize.x][mapSize.y]);

	for(const auto & obj : CGI->mh->getMap()->objects)
		addObject(obj);
}

void MapRendererContextState::addObject(const CGObjectInstance * obj)
{
	if(!obj)
		return;

	for(int fx = 0; fx < obj->getWidth(); ++fx)
	{
		for(int fy = 0; fy < obj->getHeight(); ++fy)
		{
			int3 currTile(obj->pos.x - fx, obj->pos.y - fy, obj->pos.z);

			if(LOCPLINT->cb->isInTheMap(currTile) && obj->coveringAt(currTile.x, currTile.y))
			{
				auto & container = objects[currTile.z][currTile.x][currTile.y];

				container.push_back(obj->id);
				boost::range::sort(container, compareObjectBlitOrder);
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
			int3 currTile(x, y, object->pos.z);

			if(LOCPLINT->cb->isInTheMap(currTile))
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
	for(int z = 0; z < LOCPLINT->cb->getMapSize().z; z++)
		for(int x = 0; x < LOCPLINT->cb->getMapSize().x; x++)
			for(int y = 0; y < LOCPLINT->cb->getMapSize().y; y++)
				vstd::erase(objects[z][x][y], object->id);
}
