/*
 * WaterProxy.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "WaterProxy.h"
#include "../CMapGenerator.h"
#include "../RmgMap.h"
#include "../../TerrainHandler.h"
#include "../../mapObjectConstructors/AObjectTypeHandler.h"
#include "../../mapObjectConstructors/CObjectClassesHandler.h"
#include "../../mapObjects/MiscObjects.h"
#include "../../mapping/CMap.h"
#include "../../mapping/CMapEditManager.h"
#include "../RmgPath.h"
#include "../RmgObject.h"
#include "ObjectManager.h"
#include "../Functions.h"
#include "RoadPlacer.h"
#include "TreasurePlacer.h"
#include "TownPlacer.h"
#include "ConnectionsPlacer.h"
#include "../TileInfo.h"
#include "WaterAdopter.h"
#include "../RmgArea.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

void WaterProxy::process()
{
	auto area = zone.area();

	for(const auto & t : area->getTilesVector())
	{
		map.setZoneID(t, zone.getId());
		map.setOccupied(t, ETileType::POSSIBLE);
	}
	
	auto v = area->getTilesVector();
	mapProxy->drawTerrain(zone.getRand(), v, zone.getTerrainType());
	
	//check terrain type
	for([[maybe_unused]] const auto & t : area->getTilesVector())
	{
		assert(map.isOnMap(t));
		assert(map.getTile(t).getTerrainID() == zone.getTerrainType());
	}

	// FIXME: Possible deadlock for 2 zones

	auto areaPossible = zone.areaPossible();
	for(const auto & z : map.getZones())
	{
		if(z.second->getId() == zone.getId())
			continue;

		auto secondArea = z.second->area();
		auto secondAreaPossible = z.second->areaPossible();
		for(const auto & t : secondArea->getTilesVector())
		{
			if(map.getTile(t).getTerrainID() == zone.getTerrainType())
			{
				secondArea->erase(t);
				secondAreaPossible->erase(t);
				area->add(t);
				areaPossible->add(t);
				map.setZoneID(t, zone.getId());
				map.setOccupied(t, ETileType::POSSIBLE);
			}
		}
	}
	
	if(!area->contains(zone.getPos()))
	{
		zone.setPos(area->getTilesVector().front());
	}
	
	zone.initFreeTiles();
	
	collectLakes();
}

void WaterProxy::init()
{
	for(auto & z : map.getZones())
	{
		if (!zone.isUnderground())
		{
			dependency(z.second->getModificator<TownPlacer>());
			dependency(z.second->getModificator<WaterAdopter>());
		}
		postfunction(z.second->getModificator<ConnectionsPlacer>());
		postfunction(z.second->getModificator<ObjectManager>());
	}
	POSTFUNCTION(TreasurePlacer);
}

const std::vector<WaterProxy::Lake> & WaterProxy::getLakes() const
{
	RecursiveLock lock(externalAccessMutex);
	return lakes;
}

void WaterProxy::collectLakes()
{
	RecursiveLock lock(externalAccessMutex);
	int lakeId = 0;
	for(const auto & lake : connectedAreas(zone.area().get(), true))
	{
		lakes.push_back(Lake{});
		lakes.back().area = lake;
		lakes.back().distanceMap = lake.computeDistanceMap(lakes.back().reverseDistanceMap);
		for(const auto & t : lake.getBorderOutside())
			if(map.isOnMap(t))
				lakes.back().neighbourZones[map.getZoneID(t)].add(t);
		for(const auto & t : lake.getTilesVector())
			lakeMap[t] = lakeId;
		
		//each lake must have at least one free tile
		if(!lake.overlap(zone.freePaths().get()))
			zone.freePaths()->add(*lakes.back().reverseDistanceMap[lakes.back().reverseDistanceMap.size() - 1].begin());
		
		++lakeId;
	}
}

RouteInfo WaterProxy::waterRoute(Zone & dst)
{
	RouteInfo result;
	
	auto * adopter = dst.getModificator<WaterAdopter>();
	if(!adopter)
		return result;
	
	if(adopter->getCoastTiles().empty())
		return result;

	bool createRoad = false;
	
	//block zones are not connected by template
	for(auto& lake : lakes)
	{
		if(lake.neighbourZones.count(dst.getId()))
		{
			if(!lake.keepConnections.count(dst.getId()))
			{
				for(const auto & ct : lake.neighbourZones[dst.getId()].getTilesVector())
				{
					if(map.isPossible(ct))
						map.setOccupied(ct, ETileType::BLOCKED);
				}

				Zone::Lock lock(dst.areaMutex);
				dst.areaPossible()->subtract(lake.neighbourZones[dst.getId()]);
				continue;
			}

			//Don't place shipyard or boats on the very small lake
			if (lake.area.getTilesVector().size() < 25)
			{
				logGlobal->info("Skipping very small lake at zone %d", dst.getId());
				continue;
			}
						
			int zoneTowns = 0;
			if(auto * m = dst.getModificator<TownPlacer>())
				zoneTowns = m->getTotalTowns();

			if (vstd::contains(lake.keepRoads, dst.getId()))
			{
				createRoad = true;
			}
			
			//FIXME: Why are Shipyards not allowed in zones with no towns?
			if(dst.getType() == ETemplateZoneType::PLAYER_START || dst.getType() == ETemplateZoneType::CPU_START || zoneTowns)
			{
				if(placeShipyard(dst, lake, generator.getConfig().shipyardGuard, createRoad, result))
				{
					logGlobal->info("Shipyard successfully placed at zone %d", dst.getId());
				}
				else
				{
					logGlobal->warn("Shipyard placement failed, trying boat at zone %d", dst.getId());
					if(placeBoat(dst, lake, createRoad, result))
					{
						logGlobal->warn("Boat successfully placed at zone %d", dst.getId());
					}
					else
					{
						logGlobal->error("Boat placement failed at zone %d", dst.getId());
					}
				}
			}
			else
			{
				if(placeBoat(dst, lake,  createRoad, result))
				{
					logGlobal->info("Boat successfully placed at zone %d", dst.getId());
				}
				else
				{
					logGlobal->error("Boat placement failed at zone %d", dst.getId());
				}
			}
		}
	}
	
	return result;
}

bool WaterProxy::waterKeepConnection(const rmg::ZoneConnection & connection, bool createRoad)
{
	const auto & zoneA = connection.getZoneA();
	const auto & zoneB = connection.getZoneB();

	RecursiveLock lock(externalAccessMutex);
	for(auto & lake : lakes)
	{
		if(lake.neighbourZones.count(zoneA) && lake.neighbourZones.count(zoneB))
		{
			lake.keepConnections.insert(zoneA);
			lake.keepConnections.insert(zoneB);
			if (createRoad)
			{
				lake.keepRoads.insert(zoneA);
				lake.keepRoads.insert(zoneB);
			}
			return true;
		}
	}
	return false;
}

bool WaterProxy::placeBoat(Zone & land, const Lake & lake, bool createRoad, RouteInfo & info)
{
	auto * manager = zone.getModificator<ObjectManager>();
	if(!manager)
		return false;

	auto subObjects = LIBRARY->objtypeh->knownSubObjects(Obj::BOAT);
	std::set<si32> sailingBoatTypes; //RMG shall place only sailing boats on water
	for(auto subObj : subObjects)
	{
		//making a temporary object
		auto obj(LIBRARY->objtypeh->getHandlerFor(Obj::BOAT, subObj)->create(map.mapInstance->cb, nullptr));
		if(auto testBoat = dynamic_cast<CGBoat *>(obj.get()))
		{
			if(testBoat->layer == EPathfindingLayer::SAIL)
				sailingBoatTypes.insert(subObj);
		}
	}
			
	if(sailingBoatTypes.empty())
		return false;
	
	auto boat = std::dynamic_pointer_cast<CGBoat>(LIBRARY->objtypeh->getHandlerFor(Obj::BOAT, *RandomGeneratorUtil::nextItem(sailingBoatTypes, zone.getRand()))->create(map.mapInstance->cb, nullptr));

	rmg::Object rmgObject(boat);
	rmgObject.setTemplate(zone.getTerrainType(), zone.getRand());

	auto waterAvailable = zone.areaForRoads();
	rmg::Area coast = lake.neighbourZones.at(land.getId()); //having land tiles
	coast.intersect(land.areaForRoads()); //having only available land tiles
	auto boardingPositions = coast.getSubarea([&waterAvailable, this](const int3 & tile) //tiles where boarding is possible
		{
			//We don't want place boat right to any land object, especiallly the zone guard
			if (map.getTileInfo(tile).getNearestObjectDistance() <= 3)
				return false;

			rmg::Area a({tile});
			a = a.getBorderOutside();
			a.intersect(waterAvailable);
			return !a.empty();
		});

	while(!boardingPositions.empty())
	{
		auto boardingPosition = *boardingPositions.getTilesVector().begin();
		rmg::Area shipPositions({boardingPosition});
		auto boutside = shipPositions.getBorderOutside();
		shipPositions.assign(boutside);
		shipPositions.intersect(waterAvailable);
		if(shipPositions.empty())
		{
			boardingPositions.erase(boardingPosition);
			continue;
		}

		//try to place boat at water, create paths on water and land
		auto path = manager->placeAndConnectObject(shipPositions, rmgObject, 4, false, true, ObjectManager::OptimizeType::NONE);
		auto landPath = land.searchPath(boardingPosition, false);
		if(!path.valid() || !landPath.valid())
		{
			boardingPositions.erase(boardingPosition);
			continue;
		}

		info.blocked = rmgObject.getArea();
		info.visitable = rmgObject.getVisitablePosition();
		info.boarding = boardingPosition;
		info.water = shipPositions;

		zone.connectPath(path);
		land.connectPath(landPath);
		manager->placeObject(rmgObject, false, true, createRoad);
		land.getModificator<ObjectManager>()->updateDistances(rmgObject); //Keep land objects away from the boat
		break;
	}

	return !boardingPositions.empty();
}

bool WaterProxy::placeShipyard(Zone & land, const Lake & lake, si32 guard, bool createRoad, RouteInfo & info)
{
	auto * manager = land.getModificator<ObjectManager>();
	if(!manager)
		return false;
	
	int subtype = chooseRandomAppearance(zone.getRand(), Obj::SHIPYARD, land.getTerrainType());
	auto shipyard = std::dynamic_pointer_cast<CGShipyard>(LIBRARY->objtypeh->getHandlerFor(Obj::SHIPYARD, subtype)->create(map.mapInstance->cb, nullptr));
	shipyard->tempOwner = PlayerColor::NEUTRAL;
	
	rmg::Object rmgObject(shipyard);
	rmgObject.setTemplate(land.getTerrainType(), zone.getRand());
	bool guarded = manager->addGuard(rmgObject, guard);
	
	auto waterAvailable = zone.areaForRoads();
	waterAvailable.intersect(lake.area);
	rmg::Area coast = lake.neighbourZones.at(land.getId()); //having land tiles
	coast.intersect(land.areaForRoads()); //having only available land tiles
	auto boardingPositions = coast.getSubarea([&waterAvailable](const int3 & tile) //tiles where boarding is possible
	{
		rmg::Area a({tile});
		a = a.getBorderOutside();
		a.intersect(waterAvailable);
		return !a.empty();
	});
	
	while(!boardingPositions.empty())
	{
		auto boardingPosition = *boardingPositions.getTilesVector().begin();
		rmg::Area shipPositions({boardingPosition});
		auto boutside = shipPositions.getBorderOutside();
		shipPositions.assign(boutside);
		shipPositions.intersect(waterAvailable);
		if(shipPositions.empty())
		{
			boardingPositions.erase(boardingPosition);
			continue;
		}
		
		//try to place shipyard close to boarding position and appropriate water access
		auto path = manager->placeAndConnectObject(land.areaPossible().get(), rmgObject, [&rmgObject, &shipPositions, &boardingPosition](const int3 & tile)
		{
			//Must only check the border of shipyard and not the added guard
			rmg::Area shipyardOut = rmgObject.instances().front()->getBlockedArea().getBorderOutside();

			if(!shipyardOut.contains(boardingPosition) || (shipyardOut * shipPositions).empty())
				return -1.f;
			
			return 1.0f;
		}, guarded, true, ObjectManager::OptimizeType::NONE);
		
		//search path to boarding position
		auto searchArea = land.areaPossible().get() - rmgObject.getArea();
		rmg::Path pathToBoarding(searchArea);
		pathToBoarding.connect(land.freePaths().get());
		pathToBoarding.connect(path);
		pathToBoarding = pathToBoarding.search(boardingPosition, false);
		
		//make sure shipyard places ship at position we defined
		rmg::Area shipyardOutToBlock(rmgObject.getArea().getBorderOutside());
		shipyardOutToBlock.intersect(waterAvailable);
		shipyardOutToBlock.subtract(shipPositions);
		shipPositions.subtract(shipyardOutToBlock);
		auto pathToBoat = zone.searchPath(shipPositions, true);
		
		if(!path.valid() || !pathToBoarding.valid() || !pathToBoat.valid())
		{
			boardingPositions.erase(boardingPosition);
			continue;
		}
		
		land.connectPath(path);
		land.connectPath(pathToBoarding);
		zone.connectPath(pathToBoat);
		
		info.blocked = rmgObject.getArea();
		info.visitable = rmgObject.getVisitablePosition();
		info.boarding = boardingPosition;
		info.water = shipPositions;
		
		manager->placeObject(rmgObject, guarded, true, createRoad);
		
		zone.areaPossible()->subtract(shipyardOutToBlock);
		for(const auto & i : shipyardOutToBlock.getTilesVector())
			if(map.isOnMap(i) && map.isPossible(i))
				map.setOccupied(i, ETileType::BLOCKED);
		
		break;
	}
	
	return !boardingPositions.empty();
}

char WaterProxy::dump(const int3 & t)
{
	auto lakeIter = lakeMap.find(t);
	if(lakeIter == lakeMap.end())
		return '?';
	
	Lake & lake = lakes[lakeMap.at(t)];
	for(const auto & i : lake.neighbourZones)
	{
		if(i.second.contains(t))
			return lake.keepConnections.count(i.first) ? std::to_string(i.first)[0] : '=';
	}
	
	return '~';
}

VCMI_LIB_NAMESPACE_END
