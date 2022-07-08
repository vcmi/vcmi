/*
 * ObstaclePlacer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "../mapObjects/CObjectClassesHandler.h"
#include "ObstaclePlacer.h"
#include "ObjectManager.h"
#include "TreasurePlacer.h"
#include "RockPlacer.h"
#include "WaterRoutes.h"
#include "WaterProxy.h"
#include "RoadPlacer.h"
#include "RmgMap.h"
#include "CMapGenerator.h"
#include "../CRandomGenerator.h"
//#include "../mapping/CMapEditManager.h"
#include "Functions.h"

void ObstaclePlacer::process()
{
	auto * manager = zone.getModificator<ObjectManager>();
	if(!manager)
		return;
	
	typedef std::vector<ObjectTemplate> obstacleVector;
	//obstacleVector possibleObstacles;
	
	std::map<ui8, obstacleVector> obstaclesBySize;
	typedef std::pair<ui8, obstacleVector> obstaclePair;
	std::vector<obstaclePair> possibleObstacles;
	
	//get all possible obstacles for this terrain
	for(auto primaryID : VLC->objtypeh->knownObjects())
	{
		for(auto secondaryID : VLC->objtypeh->knownSubObjects(primaryID))
		{
			auto handler = VLC->objtypeh->getHandlerFor(primaryID, secondaryID);
			if(handler->isStaticObject())
			{
				for(auto temp : handler->getTemplates())
				{
					if(temp.canBePlacedAt(zone.getTerrainType()) && temp.getBlockMapOffset().valid())
						obstaclesBySize[(ui8)temp.getBlockedOffsets().size()].push_back(temp);
				}
			}
		}
	}
	for(auto o : obstaclesBySize)
	{
		possibleObstacles.push_back(std::make_pair(o.first, o.second));
	}
	boost::sort(possibleObstacles, [](const obstaclePair &p1, const obstaclePair &p2) -> bool
	{
		return p1.first > p2.first; //bigger obstacles first
	});
	
	auto blockedArea = zone.area().getSubarea([this](const int3 & t)
	{
		return map.shouldBeBlocked(t);
	});
	blockedArea.subtract(zone.areaUsed());
	
	//reverse order, since obstacles begin in bottom-right corner, while the map coordinates begin in top-left
	auto blockedTiles = blockedArea.getTilesVector();
	for(auto tile : boost::adaptors::reverse(blockedTiles))
	{
		if(!blockedArea.contains(tile ))
			continue;
		
		//start from biggets obstacles
		for(int i = 0; i < possibleObstacles.size(); i++)
		{
			if(!possibleObstacles[i].first)
				continue;
			
			auto temp = *RandomGeneratorUtil::nextItem(possibleObstacles[i].second, generator.rand);
			auto obj = VLC->objtypeh->getHandlerFor(temp.id, temp.subid)->create(temp);
			rmg::Object rmgObject(*obj);
			rmgObject.setPosition(tile);
			if(blockedArea.contains(rmgObject.getArea()))
			{
				manager->placeObject(rmgObject, false, false);
				blockedArea.subtract(rmgObject.getArea());
				break;
			}
			else
			{
				rmgObject.clear();
			}
		}
	}
}

void ObstaclePlacer::init()
{
	dependency(zone.getModificator<ObjectManager>());
	dependency(zone.getModificator<TreasurePlacer>());
	dependency(zone.getModificator<RockPlacer>());
	dependency(zone.getModificator<WaterRoutes>());
	dependency(zone.getModificator<WaterProxy>());
	dependency(zone.getModificator<RoadPlacer>());
}
