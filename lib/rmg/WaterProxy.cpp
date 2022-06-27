//
//  WaterProxy.cpp
//  vcmi
//
//  Created by nordsoft on 27.06.2022.
//

#include "WaterProxy.h"
#include "CMapGenerator.h"
#include "RmgMap.h"
#include "../mapping/CMap.h"
#include "../mapping/CMapEditManager.h"
#include "../mapObjects/CObjectClassesHandler.h"
#include "CRmgPath.h"
#include "CRmgObject.h"
#include "ObjectManager.h"
#include "Functions.h"
#include "RoadPlacer.h"
#include "TreasurePlacer.h"
#include "TownPlacer.h"
#include "ConnectionsPlacer.h"
#include "TileInfo.h"
#include "WaterAdopter.h"
#include "CRmgArea.h"

WaterProxy::WaterProxy(Zone & zone, RmgMap & map, CMapGenerator & generator) : zone(zone), map(map), generator(generator)
{
}

void WaterProxy::process()
{
	paintZoneTerrain(zone, generator.rand, map, zone.getTerrainType());
	//check terrain type
	for(auto & t : zone.area().getBorder())
	{
		if(map.isOnMap(t) && map.map().getTile(t).terType != zone.getTerrainType())
		{
			zone.areaPossible().erase(t);
			map.setOccupied(t, ETileType::USED);
		}
	}
	for(auto & t : zone.area().getBorderOutside())
	{
		if(map.isOnMap(t) && map.map().getTile(t).terType == zone.getTerrainType())
		{
			map.getZones()[map.getZoneID(t)]->areaPossible().erase(t);
			map.setOccupied(t, ETileType::USED);
		}
	}
	collectLakes();
}

void WaterProxy::init()
{
	for(auto & z : map.getZones())
	{
		dependency(z.second->getModificator<TownPlacer>());
		dependency(z.second->getModificator<WaterAdopter>());
		postfunction(z.second->getModificator<ConnectionsPlacer>());
		postfunction(z.second->getModificator<ObjectManager>());
	}
	postfunction(zone.getModificator<TreasurePlacer>());
}

void WaterProxy::collectLakes()
{
	int lakeId = 0;
	for(auto lake : connectedAreas(zone.getArea()))
	{
		lakes.push_back(Lake{});
		lakes.back().area = lake;
		lakes.back().distanceMap = lake.computeDistanceMap(lakes.back().reverseDistanceMap);
		for(auto & t : lake.getBorderOutside())
			if(map.isOnMap(t))
				lakes.back().neighbourZones[map.getZoneID(t)].add(t);
		for(auto & t : lake.getTiles())
			lakeMap[t] = lakeId;
		++lakeId;
	}
}

void WaterProxy::waterRoute(Zone & dst)
{
	auto * adopter = dst.getModificator<WaterAdopter>();
	if(!adopter)
		return;
	
	if(adopter->getCoastTiles().empty())
		return;
	
	//block zones are not connected by template
	for(auto& lake : lakes)
	{
		if(lake.neighbourZones.count(dst.getId()))
		{
			if(!lake.keepConnections.count(dst.getId()))
			{
				for(auto & ct : lake.neighbourZones[dst.getId()].getTiles())
				{
					if(map.isPossible(ct))
						map.setOccupied(ct, ETileType::BLOCKED);
				}
				dst.areaPossible().subtract(lake.neighbourZones[dst.getId()]);
				continue;
			}
			
			int3 coastTile(-1, -1, -1);
			
			int zoneTowns = 0;
			if(auto * m = dst.getModificator<TownPlacer>())
				zoneTowns = m->getTotalTowns();
			
			if(dst.getType() == ETemplateZoneType::PLAYER_START || dst.getType() == ETemplateZoneType::CPU_START || zoneTowns)
			{
				//coastTile = dst.createShipyard(lake.tiles, gen->getConfig().shipyardGuard);
				if(!coastTile.valid())
				{
					//coastTile = makeBoat(dst.getId(), lake.tiles);
				}
			}
			else
			{
				//coastTile = makeBoat(dst.getId(), lake.tiles);
			}
			
			if(coastTile.valid())
			{
				if(!dst.connectPath(coastTile, true))
					logGlobal->error("Cannot build water route for zone %d", dst.getId());
			}
			else
				logGlobal->error("No entry from water to zone %d", dst.getId());
			
		}
	}
}

bool WaterProxy::waterKeepConnection(TRmgTemplateZoneId zoneA, TRmgTemplateZoneId zoneB)
{
	for(auto & lake : lakes)
	{
		if(lake.neighbourZones.count(zoneA) && lake.neighbourZones.count(zoneB))
		{
			lake.keepConnections.insert(zoneA);
			lake.keepConnections.insert(zoneB);
			return true;
		}
	}
	return false;
}
