/*
 * ConnectionsPlacer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ConnectionsPlacer.h"
#include "../CMapGenerator.h"
#include "../RmgMap.h"
#include "../../TerrainHandler.h"
#include "../../mapObjectConstructors/AObjectTypeHandler.h"
#include "../../mapObjectConstructors/CObjectClassesHandler.h"
#include "../../mapping/CMapEditManager.h"
#include "../RmgObject.h"
#include "ObjectManager.h"
#include "../Functions.h"
#include "RoadPlacer.h"
#include "../TileInfo.h"
#include "WaterAdopter.h"
#include "WaterProxy.h"
#include "TownPlacer.h"
#include <boost/interprocess/sync/scoped_lock.hpp>

VCMI_LIB_NAMESPACE_BEGIN

void ConnectionsPlacer::process()
{
	collectNeighbourZones();

	auto diningPhilosophers = [this](std::function<void(const rmg::ZoneConnection&)> f)
	{
		for (auto& c : dConnections)
		{
			if (c.getZoneA() != zone.getId() && c.getZoneB() != zone.getId())
				continue;

			auto otherZone = map.getZones().at(c.getZoneB());
			auto* cp = otherZone->getModificator<ConnectionsPlacer>();

			while (cp)
			{
				RecursiveLock lock1(externalAccessMutex, boost::try_to_lock_t{});
				RecursiveLock lock2(cp->externalAccessMutex, boost::try_to_lock_t{});
				if (lock1.owns_lock() && lock2.owns_lock())
				{
					if (!vstd::contains(dCompleted, c))
					{
						f(c);
					}
					break;
				}
			}
		}
	};

	diningPhilosophers([this](const rmg::ZoneConnection& c)
	{
		selfSideDirectConnection(c);
	});

	createBorder();

	diningPhilosophers([this](const rmg::ZoneConnection& c)
	{
		selfSideIndirectConnection(c);
	});
}

void ConnectionsPlacer::init()
{
	DEPENDENCY(WaterAdopter);
	DEPENDENCY(TownPlacer);
	POSTFUNCTION(RoadPlacer);
	POSTFUNCTION(ObjectManager);
	
	for(auto c : map.getMapGenOptions().getMapTemplate()->getConnections())
		addConnection(c);
}

void ConnectionsPlacer::addConnection(const rmg::ZoneConnection& connection)
{
	dConnections.push_back(connection);
}

void ConnectionsPlacer::otherSideConnection(const rmg::ZoneConnection & connection)
{
	dCompleted.push_back(connection);
}

void ConnectionsPlacer::selfSideDirectConnection(const rmg::ZoneConnection & connection)
{
	bool success = false;
	auto otherZoneId = (connection.getZoneA() == zone.getId() ? connection.getZoneB() : connection.getZoneA());
	auto & otherZone = map.getZones().at(otherZoneId);
	
	//1. Try to make direct connection
	//Do if it's not prohibited by terrain settings
	const auto * ourTerrain   = VLC->terrainTypeHandler->getById(zone.getTerrainType());
	const auto * otherTerrain = VLC->terrainTypeHandler->getById(otherZone->getTerrainType());

	bool directProhibited = vstd::contains(ourTerrain->prohibitTransitions, otherZone->getTerrainType())
						 || vstd::contains(otherTerrain->prohibitTransitions, zone.getTerrainType());
	auto directConnectionIterator = dNeighbourZones.find(otherZoneId);
	if(!directProhibited && directConnectionIterator != dNeighbourZones.end())
	{
		int3 guardPos(-1, -1, -1);
		int3 borderPos;
		while(!directConnectionIterator->second.empty())
		{
			borderPos = *RandomGeneratorUtil::nextItem(directConnectionIterator->second, zone.getRand());
			guardPos = zone.areaPossible().nearest(borderPos);
			assert(borderPos != guardPos);

			float dist = map.getTileInfo(guardPos).getNearestObjectDistance();
			if (dist >= 3) //Don't place guards at adjacent tiles
			{

				auto safetyGap = rmg::Area({ guardPos });
				safetyGap.unite(safetyGap.getBorderOutside());
				safetyGap.intersect(zone.areaPossible());
				if (!safetyGap.empty())
				{
					safetyGap.intersect(otherZone->areaPossible());
					if (safetyGap.empty())
						break; //successfull position
				}
			}
			
			//failed position
			directConnectionIterator->second.erase(borderPos);
			guardPos = int3(-1, -1, -1);
		}
		
		if(guardPos.valid())
		{
			assert(zone.getModificator<ObjectManager>());
			auto & manager = *zone.getModificator<ObjectManager>();
			auto * monsterType = manager.chooseGuard(connection.getGuardStrength(), true);
			
			rmg::Area border(zone.getArea().getBorder());
			border.unite(otherZone->getArea().getBorder());
			
			auto costFunction = [&border](const int3 & s, const int3 & d)
			{
				return 1.f / (1.f + border.distanceSqr(d));
			};
			
			auto ourArea = zone.areaPossible() + zone.freePaths();
			auto theirArea = otherZone->areaPossible() + otherZone->freePaths();
			theirArea.add(guardPos);
			rmg::Path ourPath(ourArea);
			rmg::Path theirPath(theirArea);
			ourPath.connect(zone.freePaths());
			ourPath = ourPath.search(guardPos, true, costFunction);
			theirPath.connect(otherZone->freePaths());
			theirPath = theirPath.search(guardPos, true, costFunction);
			
			if(ourPath.valid() && theirPath.valid())
			{
				zone.connectPath(ourPath);
				otherZone->connectPath(theirPath);
				
				if(monsterType)
				{
					rmg::Object monster(*monsterType);
					monster.setPosition(guardPos);
					manager.placeObject(monster, false, true);
					//Place objects away from the monster in the other zone, too
					otherZone->getModificator<ObjectManager>()->updateDistances(monster);
				}
				else
				{
					zone.areaPossible().erase(guardPos);
					zone.freePaths().add(guardPos);
					map.setOccupied(guardPos, ETileType::FREE);
				}
				
				assert(zone.getModificator<RoadPlacer>());
				zone.getModificator<RoadPlacer>()->addRoadNode(guardPos);
				
				assert(otherZone->getModificator<RoadPlacer>());
				otherZone->getModificator<RoadPlacer>()->addRoadNode(borderPos);
				
				assert(otherZone->getModificator<ConnectionsPlacer>());
				otherZone->getModificator<ConnectionsPlacer>()->otherSideConnection(connection);
				
				success = true;
			}
		}
	}
	
	//2. connect via water
	bool waterMode = map.getMapGenOptions().getWaterContent() != EWaterContent::NONE;
	if(waterMode && zone.isUnderground() == otherZone->isUnderground())
	{
		if(generator.getZoneWater() && generator.getZoneWater()->getModificator<WaterProxy>())
		{
			if(generator.getZoneWater()->getModificator<WaterProxy>()->waterKeepConnection(connection.getZoneA(), connection.getZoneB()))
			{
				assert(otherZone->getModificator<ConnectionsPlacer>());
				otherZone->getModificator<ConnectionsPlacer>()->otherSideConnection(connection);
				success = true;
			}
		}
	}
	
	if(success)
		dCompleted.push_back(connection);
}

void ConnectionsPlacer::selfSideIndirectConnection(const rmg::ZoneConnection & connection)
{
	bool success = false;
	auto otherZoneId = (connection.getZoneA() == zone.getId() ? connection.getZoneB() : connection.getZoneA());
	auto & otherZone = map.getZones().at(otherZoneId);
	
	//3. place subterrain gates
	if(zone.isUnderground() != otherZone->isUnderground())
	{
		int3 zShift(0, 0, zone.getPos().z - otherZone->getPos().z);
		auto commonArea = zone.areaPossible() * (otherZone->areaPossible() + zShift);
		if(!commonArea.empty())
		{
			assert(zone.getModificator<ObjectManager>());
			auto & manager = *zone.getModificator<ObjectManager>();
			
			assert(otherZone->getModificator<ObjectManager>());
			auto & managerOther = *otherZone->getModificator<ObjectManager>();
			
			auto factory = VLC->objtypeh->getHandlerFor(Obj::SUBTERRANEAN_GATE, 0);
			auto * gate1 = factory->create();
			auto * gate2 = factory->create();
			rmg::Object rmgGate1(*gate1);
			rmg::Object rmgGate2(*gate2);
			rmgGate1.setTemplate(zone.getTerrainType());
			rmgGate2.setTemplate(otherZone->getTerrainType());
			bool guarded1 = manager.addGuard(rmgGate1, connection.getGuardStrength(), true);
			bool guarded2 = managerOther.addGuard(rmgGate2, connection.getGuardStrength(), true);
			int minDist = 3;
			
			std::scoped_lock doubleLock(zone.areaMutex, otherZone->areaMutex);
			rmg::Path path2(otherZone->area());
			rmg::Path path1 = manager.placeAndConnectObject(commonArea, rmgGate1, [this, minDist, &path2, &rmgGate1, &zShift, guarded2, &managerOther, &rmgGate2	](const int3 & tile)
			{
				auto ti = map.getTileInfo(tile);
				auto otherTi = map.getTileInfo(tile - zShift);
				float dist = ti.getNearestObjectDistance();
				float otherDist = otherTi.getNearestObjectDistance();
				if(dist < minDist || otherDist < minDist)
					return -1.f;
				
				rmg::Area toPlace(rmgGate1.getArea() + rmgGate1.getAccessibleArea());
				toPlace.translate(-zShift);
				
				path2 = managerOther.placeAndConnectObject(toPlace, rmgGate2, minDist, guarded2, true, ObjectManager::OptimizeType::NONE);
				
				return path2.valid() ? (dist * otherDist) : -1.f;
			}, guarded1, true, ObjectManager::OptimizeType::DISTANCE);
			
			if(path1.valid() && path2.valid())
			{
				zone.connectPath(path1);
				otherZone->connectPath(path2);
				
				manager.placeObject(rmgGate1, guarded1, true);
				managerOther.placeObject(rmgGate2, guarded2, true);
				
				assert(otherZone->getModificator<ConnectionsPlacer>());
				otherZone->getModificator<ConnectionsPlacer>()->otherSideConnection(connection);
				
				success = true;
			}
		}
	}
	
	//4. place monoliths/portals
	if(!success)
	{
		auto factory = VLC->objtypeh->getHandlerFor(Obj::MONOLITH_TWO_WAY, generator.getNextMonlithIndex());
		auto * teleport1 = factory->create();
		auto * teleport2 = factory->create();

		zone.getModificator<ObjectManager>()->addRequiredObject(teleport1, connection.getGuardStrength());
		otherZone->getModificator<ObjectManager>()->addRequiredObject(teleport2, connection.getGuardStrength());
		
		assert(otherZone->getModificator<ConnectionsPlacer>());
		otherZone->getModificator<ConnectionsPlacer>()->otherSideConnection(connection);
		
		success = true;
	}
	
	if(success)
		dCompleted.push_back(connection);
}

void ConnectionsPlacer::collectNeighbourZones()
{
	auto border = zone.area().getBorderOutside();
	for(const auto & i : border)
	{
		if(!map.isOnMap(i))
			continue;
		
		auto zid = map.getZoneID(i);
		assert(zid != zone.getId());
		dNeighbourZones[zid].insert(i);
	}
}

void ConnectionsPlacer::createBorder()
{
	rmg::Area borderArea(zone.getArea().getBorder());
	rmg::Area borderOutsideArea(zone.getArea().getBorderOutside());
	auto blockBorder = borderArea.getSubarea([this, &borderOutsideArea](const int3 & t)
	{
		auto tile = borderOutsideArea.nearest(t);
		return map.isOnMap(tile) && map.getZones()[map.getZoneID(tile)]->getType() != ETemplateZoneType::WATER;
	});

	Zone::Lock lock(zone.areaMutex); //Protect from erasing same tiles again
	for(const auto & tile : blockBorder.getTilesVector())
	{
		if(map.isPossible(tile))
		{
			map.setOccupied(tile, ETileType::BLOCKED);
			zone.areaPossible().erase(tile);
		}

		map.foreachDirectNeighbour(tile, [this](int3 &nearbyPos)
		{
			if(map.isPossible(nearbyPos) && map.getZoneID(nearbyPos) == zone.getId())
			{
				map.setOccupied(nearbyPos, ETileType::BLOCKED);
				zone.areaPossible().erase(nearbyPos);
			}
		});
	}
}

VCMI_LIB_NAMESPACE_END
