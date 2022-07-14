/*
 * ConnectionsPlacer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "ConnectionsPlacer.h"
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
#include "TileInfo.h"
#include "WaterAdopter.h"
#include "WaterProxy.h"
#include "TownPlacer.h"

void ConnectionsPlacer::process()
{
	collectNeighbourZones();
	for(auto & c : dConnections)
	{
		if(c.getZoneA() != zone.getId() && c.getZoneB() != zone.getId())
			continue;
		
		auto iter = std::find(dCompleted.begin(), dCompleted.end(), c);
		if(iter != dCompleted.end())
			continue;
		
		selfSideConnection(c);
	}
	
	createBorder(map, zone);
}

void ConnectionsPlacer::init()
{
	dependency(zone.getModificator<WaterAdopter>());
	dependency(zone.getModificator<TownPlacer>());
	postfunction(zone.getModificator<RoadPlacer>());
	
	for(auto & z : map.getZones())
	{
		postfunction(z.second->getModificator<ObjectManager>());
	}
	
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

void ConnectionsPlacer::selfSideConnection(const rmg::ZoneConnection & connection)
{
	bool success = false;
	auto otherZoneId = (connection.getZoneA() == zone.getId() ? connection.getZoneB() : connection.getZoneA());
	auto & otherZone = map.getZones().at(otherZoneId);
	
	//1. Try to make direct connection
	auto directConnectionIterator = dNeighbourZones.find(otherZoneId);
	if(directConnectionIterator != dNeighbourZones.end())
	{
		int3 guardPos(-1, -1, -1);
		int3 borderPos;
		while(!directConnectionIterator->second.empty())
		{
			borderPos = *RandomGeneratorUtil::nextItem(directConnectionIterator->second, generator.rand);
			guardPos = zone.areaPossible().nearest(borderPos);
			assert(borderPos != guardPos);
			
			auto safetyGap = rmg::Area({guardPos});
			safetyGap.unite(safetyGap.getBorderOutside());
			safetyGap.intersect(zone.areaPossible());
			if(!safetyGap.empty())
			{
				safetyGap.intersect(otherZone->areaPossible());
				if(safetyGap.empty())
					break; //successfull position
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
			
			auto ourPath = zone.searchPath(guardPos, true), theirPath = otherZone->searchPath(guardPos, true);
			if(ourPath.valid() && theirPath.valid())
			{
				zone.connectPath(ourPath);
				otherZone->connectPath(theirPath);
				
				if(monsterType)
				{
					rmg::Object monster(*monsterType);
					monster.setPosition(guardPos);
					manager.placeObject(monster, false, true);
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
	if(!success && waterMode && zone.isUnderground() == otherZone->isUnderground())
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
	
	//3. place subterrain gates
	if(!success && zone.isUnderground() != otherZone->isUnderground())
	{
		int3 zShift(0, 0, zone.getPos().z - otherZone->getPos().z);
		auto commonArea = zone.areaPossible() * (otherZone->areaPossible() + zShift);
		if(!commonArea.empty())
		{
			auto otherCommonArea = commonArea - zShift;
			
			assert(zone.getModificator<ObjectManager>());
			auto & manager = *zone.getModificator<ObjectManager>();
			
			assert(otherZone->getModificator<ObjectManager>());
			auto & managerOther = *otherZone->getModificator<ObjectManager>();
			
			auto factory = VLC->objtypeh->getHandlerFor(Obj::SUBTERRANEAN_GATE, 0);
			auto gate1 = factory->create(ObjectTemplate());
			auto gate2 = factory->create(ObjectTemplate());
			rmg::Object rmgGate1(*gate1), rmgGate2(*gate2);
			rmgGate1.setTemplate(zone.getTerrainType());
			rmgGate2.setTemplate(otherZone->getTerrainType());
			bool guarded1 = manager.addGuard(rmgGate1, connection.getGuardStrength(), true);
			bool guarded2 = managerOther.addGuard(rmgGate2, connection.getGuardStrength(), true);
			int minDist = 3;
			int maxDistSq = 3 * 3;
			
			auto path1 = manager.placeAndConnectObject(commonArea, rmgGate1, minDist, guarded1, true, true);
			auto path2 = managerOther.placeAndConnectObject(otherCommonArea, rmgGate2, [this, &rmgGate1, &minDist, &maxDistSq](const int3 & tile)
			{
				auto ti = map.getTile(tile);
				auto dist = ti.getNearestObjectDistance();
				//avoid borders
				if(dist >= minDist)
				{
					float d2 = tile.dist2dSQ(rmgGate1.getPosition());
					if(d2 > maxDistSq)
						return -1.f;
					return 1000000.f - d2 - dist;
				}
				return -1.f;
			}, guarded2, true, true);
			
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
		auto teleport1 = factory->create(ObjectTemplate());
		auto teleport2 = factory->create(ObjectTemplate());
	
		zone.getModificator<ObjectManager>()->addRequiredObject(teleport1, connection.getGuardStrength());
		otherZone->getModificator<ObjectManager>()->addRequiredObject(teleport2, connection.getGuardStrength());
		
		assert(otherZone->getModificator<ConnectionsPlacer>());
		otherZone->getModificator<ConnectionsPlacer>()->otherSideConnection(connection);
		
		success = true;
	}
}

void ConnectionsPlacer::collectNeighbourZones()
{
	auto border = zone.area().getBorderOutside();
	for(auto & i : border)
	{
		if(!map.isOnMap(i))
			continue;
		
		auto zid = map.getZoneID(i);
		assert(zid != zone.getId());
		dNeighbourZones[zid].insert(i);
	}
}
