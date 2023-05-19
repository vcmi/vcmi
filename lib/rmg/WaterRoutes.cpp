/*
 * WaterRoutes.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "WaterRoutes.h"
#include "WaterProxy.h"
#include "CMapGenerator.h"
#include "RmgMap.h"
#include "../mapping/CMap.h"
#include "../mapping/CMapEditManager.h"
#include "../mapObjects/CObjectClassesHandler.h"
#include "RmgPath.h"
#include "RmgObject.h"
#include "ObjectManager.h"
#include "Functions.h"
#include "RoadPlacer.h"
#include "TreasurePlacer.h"
#include "TownPlacer.h"
#include "ConnectionsPlacer.h"
#include "TileInfo.h"
#include "WaterAdopter.h"
#include "RmgArea.h"

VCMI_LIB_NAMESPACE_BEGIN

void WaterRoutes::process()
{
	auto * wproxy = zone.getModificator<WaterProxy>();
	if(!wproxy)
		return;
	
	if(auto * manager = zone.getModificator<ObjectManager>())
		manager->createDistancesPriorityQueue();
	
	for(auto & z : map.getZones())
	{
		if(z.first != zone.getId())
			result.push_back(wproxy->waterRoute(*z.second));
	}

	Zone::Lock lock(zone.areaMutex);

	//prohibit to place objects on sealed off lakes
	for(const auto & lake : wproxy->getLakes())
	{
		if((lake.area * zone.freePaths()).getTilesVector().size() == 1)
		{
			zone.freePaths().subtract(lake.area);
			zone.areaPossible().subtract(lake.area);
		}
	}
	
	//prohibit to place objects on the borders
	for(const auto & t : zone.area().getBorder())
	{
		if(zone.areaPossible().contains(t))
		{
			std::vector<int3> landTiles;
			map.foreachDirectNeighbour(t, [this, &landTiles, &t](const int3 & c)
			{
				if(map.isOnMap(c) && map.getZoneID(c) != zone.getId())
				{
					landTiles.push_back(c - t);
				}
			});
			
			if(landTiles.size() == 2)
			{
				int3 o = landTiles[0] + landTiles[1];
				if(o.x * o.x * o.y * o.y == 1) 
				{
					zone.areaPossible().erase(t);
					zone.areaUsed().add(t);
				}
			}
		}
	}
}

void WaterRoutes::init()
{
	for(auto & z : map.getZones())
	{
		dependency(z.second->getModificator<ConnectionsPlacer>());
		postfunction(z.second->getModificator<ObjectManager>());
		postfunction(z.second->getModificator<TreasurePlacer>());
	}
	dependency(zone.getModificator<WaterProxy>());
	postfunction(zone.getModificator<TreasurePlacer>());
}

char WaterRoutes::dump(const int3 & t)
{
	for(auto & i : result)
	{
		if(t == i.boarding)
			return 'B';
		if(t == i.visitable)
			return '@';
		if(i.blocked.contains(t))
			return '#';
		if(i.water.contains(t))
		{
			if(zone.freePaths().contains(t))
				return '+';
			else
				return '-';
		}
	}
	if(zone.freePaths().contains(t))
		return '.';
	if(zone.area().contains(t))
		return '~';
	return ' ';
}


VCMI_LIB_NAMESPACE_END
