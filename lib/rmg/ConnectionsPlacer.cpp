//
//  ConnectionsPlacer.cpp
//  vcmi
//
//  Created by nordsoft on 26.06.2022.
//

#include "ConnectionsPlacer.h"
#include "CMapGenerator.h"
#include "RmgMap.h"
#include "../mapping/CMap.h"
#include "../mapping/CMapEditManager.h"
#include "CRmgPath.h"
#include "CRmgObject.h"
#include "ObjectManager.h"
#include "Functions.h"
#include "RoadPlacer.h"

ConnectionsPlacer::ConnectionsPlacer(Zone & zone, RmgMap & map, CRandomGenerator & generator) : zone(zone), map(map), generator(generator)
{
}

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
			borderPos = *RandomGeneratorUtil::nextItem(directConnectionIterator->second, generator);
			guardPos = zone.areaPossible().nearest(borderPos);
			assert(borderPos != guardPos);
			
			auto safetyGap = Rmg::Area({guardPos});
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
			
			if(zone.connectPath(guardPos, true) && otherZone->connectPath(guardPos, true))
			{
				if(monsterType)
				{
					Rmg::Object monster(*monsterType);
					monster.setPosition(guardPos);
					manager.placeObject(monster, false, true);
				}
				else
				{
					zone.areaPossible().erase(guardPos);
					zone.areaUsed().add(guardPos);
					map.setOccupied(guardPos, ETileType::FREE);
				}
				
				assert(zone.getModificator<RoadPlacer>());
				zone.getModificator<RoadPlacer>()->addRoadNode(guardPos);
				
				assert(otherZone->getModificator<RoadPlacer>());
				otherZone->getModificator<RoadPlacer>()->addRoadNode(guardPos);
				
				assert(otherZone->getModificator<ConnectionsPlacer>());
				otherZone->getModificator<ConnectionsPlacer>()->otherSideConnection(connection);
				
				success = true;
			}
		}
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
