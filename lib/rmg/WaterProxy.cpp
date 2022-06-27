/*
 * WaterProxy.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

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
				if(placeShipyard(dst, lake, generator.getConfig().shipyardGuard))
				{
					logGlobal->warn("Shipyard successfully placed at zone %d", dst.getId());
				}
				else
				{
					logGlobal->warn("Shipyard placement failed, trying boat at zone %d", dst.getId());
					if(placeBoat(dst, lake))
					{
						logGlobal->warn("Boat successfully placed at zone %d", dst.getId());
					}
					else
					{
						logGlobal->error("Boat palcement faile at zone %d", dst.getId());
					}
				}
			}
			else
			{
				if(placeBoat(dst, lake))
				{
					logGlobal->warn("Boat successfully placed at zone %d", dst.getId());
				}
				else
				{
					logGlobal->error("Boat palcement faile at zone %d", dst.getId());
				}
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

bool WaterProxy::placeBoat(Zone & land, const Lake & lake)
{
	auto * manager = zone.getModificator<ObjectManager>();
	if(!manager)
		return false;
	
	auto subObjects = VLC->objtypeh->knownSubObjects(Obj::BOAT);
	auto* boat = (CGBoat*)VLC->objtypeh->getHandlerFor(Obj::BOAT, *RandomGeneratorUtil::nextItem(subObjects, generator.rand))->create(ObjectTemplate());
	
	rmg::Object rmgObject(*boat);
	rmgObject.setTemplate(zone.getTerrainType());
	
	auto waterAvailable = zone.areaPossible() + zone.freePaths();
	rmg::Area coast = lake.neighbourZones.at(land.getId()); //having land tiles
	coast.intersect(land.areaPossible() + land.freePaths()); //having only available land tiles
	auto boardingPositions = coast.getSubarea([&waterAvailable](const int3 & tile) //tiles where boarding is possible
											  {
		rmg::Area a({tile});
		a = a.getBorderOutside();
		a.intersect(waterAvailable);
		return !a.empty();
	});
	
	while(!boardingPositions.empty())
	{
		auto boardingPosition = *boardingPositions.getTiles().begin();
		rmg::Area shipPositions({boardingPositions});
		shipPositions.assign(shipPositions.getBorderOutside());
		shipPositions.intersect(waterAvailable);
		if(shipPositions.empty())
		{
			boardingPositions.erase(boardingPosition);
			continue;
		}
		
		//try to place boat at water and create path on water
		bool result = manager->placeAndConnectObject(shipPositions, rmgObject, 2, false, true);
		if(!result)
		{
			boardingPositions.erase(boardingPosition);
			continue;
		}
		
		//connect boat boarding position
		if(!land.connectPath(boardingPosition, false))
		{
			boardingPositions.erase(boardingPosition);
			continue;
		}
		
		manager->placeObject(rmgObject, false, true);
		break;
	}
	
	return !boardingPositions.empty();
}

bool WaterProxy::placeShipyard(Zone & land, const Lake & lake, si32 guard)
{
	auto * manager = land.getModificator<ObjectManager>();
	if(!manager)
		return false;
	
	int subtype = chooseRandomAppearance(generator.rand, Obj::SHIPYARD, land.getTerrainType());
	auto shipyard = (CGShipyard*) VLC->objtypeh->getHandlerFor(Obj::SHIPYARD, subtype)->create(ObjectTemplate());
	shipyard->tempOwner = PlayerColor::NEUTRAL;
	
	rmg::Object rmgObject(*shipyard);
	rmgObject.setTemplate(land.getTerrainType());
	bool guarded = manager->addGuard(rmgObject, guard);
	
	auto waterAvailable = zone.areaPossible() + zone.freePaths();
	rmg::Area coast = lake.neighbourZones.at(land.getId()); //having land tiles
	coast.intersect(land.areaPossible() + land.freePaths()); //having only available land tiles
	auto boardingPositions = coast.getSubarea([&waterAvailable](const int3 & tile) //tiles where boarding is possible
	{
		rmg::Area a({tile});
		a = a.getBorderOutside();
		a.intersect(waterAvailable);
		return !a.empty();
	});
	
	while(!boardingPositions.empty())
	{
		auto boardingPosition = *boardingPositions.getTiles().begin();
		rmg::Area shipPositions({boardingPositions});
		shipPositions.assign(shipPositions.getBorderOutside());
		shipPositions.intersect(waterAvailable);
		if(shipPositions.empty())
		{
			boardingPositions.erase(boardingPosition);
			continue;
		}
		
		//try to place shipyard close to boarding position and appropriate water access
		bool result = manager->placeAndConnectObject(land.areaPossible(), rmgObject, [&rmgObject, &shipPositions, &boardingPosition](const int3 & tile)
		{
			rmg::Area shipyardOut(rmgObject.getArea().getBorderOutside());
			if(!shipyardOut.contains(boardingPosition) || (shipyardOut * shipPositions).empty())
				return -1.f;
			
			return 1.0f;
		}, guarded, true);
		
		
		if(!result)
		{
			boardingPositions.erase(boardingPosition);
			continue;
		}
		
		//ensure access to boat boarding position
		if(!land.connectPath(boardingPosition, false))
		{
			boardingPositions.erase(boardingPosition);
			continue;
		}
		
		//ensure access to boat on water
		if(!zone.connectPath(shipPositions, false))
		{
			boardingPositions.erase(boardingPosition);
			continue;
		}
		
		manager->placeObject(rmgObject, guarded, true);
		break;
	}
	
	return !boardingPositions.empty();
}
