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
#include "../../mapObjects/CGCreature.h"
#include "../../mapping/CMapEditManager.h"
#include "../RmgObject.h"
#include "ObjectManager.h"
#include "../Functions.h"
#include "RoadPlacer.h"
#include "../TileInfo.h"
#include "WaterAdopter.h"
#include "WaterProxy.h"
#include "TownPlacer.h"

#include <vstd/RNG.h>

static constexpr int GATE_MIN_DIST = 3; //keep subterranean gates this far from other objects

std::pair<Zone::Lock, Zone::Lock> ConnectionsPlacer::lockZones(std::shared_ptr<Zone> otherZone)
{
	if (zone.getId() == otherZone->getId())
		return {};

	while (true)
	{
		auto lock1 = Zone::Lock(zone.areaMutex, std::try_to_lock);
		auto lock2 = Zone::Lock(otherZone->areaMutex, std::try_to_lock);

		if (lock1.owns_lock() && lock2.owns_lock())
		{
			return { std::move(lock1), std::move(lock2) };
		}
	}
}

void ConnectionsPlacer::process()
{
	collectNeighbourZones();

	auto diningPhilosophers = [this](std::function<void(const rmg::ZoneConnection&)> f)
	{
		for (auto& c : dConnections)
		{
			if (c.getZoneA() == c.getZoneB())
			{
				// Zone can always be connected to itself, but only by monolith pair
				RecursiveLock lock(externalAccessMutex);
				if (!vstd::contains(dCompleted, c))
				{
					placeMonolithConnection(c);
					continue;
				}
			}

			auto otherZone = map.getZones().at(c.getZoneB());
			auto* cp = otherZone->getModificator<ConnectionsPlacer>();

			while (cp)
			{
				RecursiveLock lock1(externalAccessMutex, std::try_to_lock);
				RecursiveLock lock2(cp->externalAccessMutex, std::try_to_lock);
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
		forcePortalConnection(c);
	});

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
	
	for (auto c : zone.getConnections())
	{
		addConnection(c);
	}
}

void ConnectionsPlacer::addConnection(const rmg::ZoneConnection& connection)
{
	dConnections.push_back(connection);
}

void ConnectionsPlacer::otherSideConnection(const rmg::ZoneConnection & connection)
{
	dCompleted.push_back(connection);
}

void ConnectionsPlacer::forcePortalConnection(const rmg::ZoneConnection & connection)
{
	// This should always succeed
	if (connection.getConnectionType() == rmg::EConnectionType::FORCE_PORTAL)
	{
		placeMonolithConnection(connection);
	}
}

void ConnectionsPlacer::selfSideDirectConnection(const rmg::ZoneConnection & connection)
{
	bool success = false;
	auto otherZoneId = connection.getOtherZoneId(zone.getId());
	auto & otherZone = map.getZones().at(otherZoneId);
	bool createRoad = shouldGenerateRoad(connection);
	
	//1. Try to make direct connection
	//Do if it's not prohibited by terrain settings
	const auto * ourTerrain   = LIBRARY->terrainTypeHandler->getById(zone.getTerrainType());
	const auto * otherTerrain = LIBRARY->terrainTypeHandler->getById(otherZone->getTerrainType());

	bool directProhibited = vstd::contains(ourTerrain->prohibitTransitions, otherZone->getTerrainType())
						 || vstd::contains(otherTerrain->prohibitTransitions, zone.getTerrainType());

	auto lock = lockZones(otherZone);

	auto directConnectionIterator = dNeighbourZones.find(otherZoneId);

	if (directConnectionIterator != dNeighbourZones.end())
	{
		if (connection.getConnectionType() == rmg::EConnectionType::WIDE)
		{
			for (auto borderPos : directConnectionIterator->second)
			{
				//TODO: Refactor common code with direct connection
				int3 potentialPos = zone.areaPossible()->nearest(borderPos);
				assert(borderPos != potentialPos);

				auto safetyGap = rmg::Area({ potentialPos });
				safetyGap.unite(safetyGap.getBorderOutside());
				safetyGap.intersect(zone.areaPossible().get());
				if (!safetyGap.empty())
				{
					safetyGap.intersect(otherZone->areaPossible().get());
					if (safetyGap.empty())
					{
						rmg::Area border(zone.area()->getBorder());
						border.unite(otherZone->area()->getBorder());

						auto costFunction = [&border](const int3& s, const int3& d)
						{
							return 1.f / (1.f + border.distanceSqr(d));
						};

						auto ourArea = zone.areaForRoads();
						auto theirArea = otherZone->areaForRoads();
						theirArea.add(potentialPos);
						rmg::Path ourPath(ourArea);
						rmg::Path theirPath(theirArea);
						ourPath.connect(zone.freePaths().get());
						ourPath = ourPath.search(potentialPos, true, costFunction);
						theirPath.connect(otherZone->freePaths().get());
						theirPath = theirPath.search(potentialPos, true, costFunction);

						if (ourPath.valid() && theirPath.valid())
						{
							zone.connectPath(ourPath);
							otherZone->connectPath(theirPath);
							otherZone->getModificator<ObjectManager>()->updateDistances(potentialPos);

							success = true;
							break;
						}
					}
				}
			}
		}
	}

	if (connection.getConnectionType() == rmg::EConnectionType::FICTIVE || 
		connection.getConnectionType() == rmg::EConnectionType::REPULSIVE)
	{
		//Fictive or repulsive connections are not real, take no action
		dCompleted.push_back(connection);
		return;
	}

	float maxDist = -10e6;
	if(!success && !directProhibited && directConnectionIterator != dNeighbourZones.end())
	{
		int3 guardPos(-1, -1, -1);
		int3 roadNode;
		for (auto borderPos : directConnectionIterator->second)
		{
			int3 potentialPos = zone.areaPossible()->nearest(borderPos);
			assert(borderPos != potentialPos);

			//Check if guard pos doesn't touch any 3rd zone. This would create unwanted passage to 3rd zone
			bool adjacentZone = false;
			map.foreach_neighbour(potentialPos, [this, &adjacentZone, otherZoneId](int3 & pos)
			{
				auto zoneId = map.getZoneID(pos);
				if (zoneId != zone.getId() && zoneId != otherZoneId)
				{
					adjacentZone = true;
				}
			});
			if (adjacentZone)
			{
				continue;
			}

			//Take into account distance to objects from both sides
			float dist = std::min(map.getTileInfo(potentialPos).getNearestObjectDistance(),
				map.getTileInfo(borderPos).getNearestObjectDistance());
			if (dist > 3) //Don't place guards at adjacent tiles
			{

				auto safetyGap = rmg::Area({ potentialPos });
				safetyGap.unite(safetyGap.getBorderOutside());
				safetyGap.intersect(zone.areaPossible().get());
				if (!safetyGap.empty())
				{
					safetyGap.intersect(otherZone->areaPossible().get());
					if (safetyGap.empty())
					{
						float distanceToCenter = zone.getPos().dist2d(potentialPos) * otherZone->getPos().dist2d(potentialPos);

						auto localDist = (dist - distanceToCenter) * //Prefer close to zone center
							(std::max(distanceToCenter, dist) / std::min(distanceToCenter, dist));
						//Distance to center dominates and is negative, so imbalanced proportions will result in huge penalty
						if (localDist > maxDist)
						{
							maxDist = localDist;
							guardPos = potentialPos;
							roadNode = borderPos;
						}
					}
				}
			}
		}
		
		if(guardPos.isValid())
		{
			assert(zone.getModificator<ObjectManager>());
			auto & manager = *zone.getModificator<ObjectManager>();
			auto monsterType = manager.chooseGuard(connection.getGuardStrength(), true);
		
			rmg::Area border(zone.area()->getBorder());
			border.unite(otherZone->area()->getBorder());

			auto localCostFunction = rmg::Path::createCurvedCostFunction(zone.area()->getBorder());
			auto otherCostFunction = rmg::Path::createCurvedCostFunction(otherZone->area()->getBorder());

			auto ourArea = zone.areaForRoads();
			auto theirArea = otherZone->areaForRoads();
			theirArea.add(guardPos);
			rmg::Path ourPath(ourArea);
			rmg::Path theirPath(theirArea);
			ourPath.connect(zone.freePaths().get());
			ourPath = ourPath.search(guardPos, true, localCostFunction);
			theirPath.connect(otherZone->freePaths().get());
			theirPath = theirPath.search(guardPos, true, otherCostFunction);
			
			if(ourPath.valid() && theirPath.valid())
			{
				zone.connectPath(ourPath);
				otherZone->connectPath(theirPath);
				
				if(monsterType)
				{
					rmg::Object monster(monsterType);
					monster.setPosition(guardPos);
					manager.placeObject(monster, false, true);
					//Place objects away from the monster in the other zone, too
					otherZone->getModificator<ObjectManager>()->updateDistances(monster);
				}
				else
				{
					//Update distances from empty passage, too
					zone.areaPossible()->erase(guardPos);
					zone.freePaths()->add(guardPos);
					map.setOccupied(guardPos, ETileType::FREE);
					manager.updateDistances(guardPos);
					otherZone->getModificator<ObjectManager>()->updateDistances(guardPos);
				}
				
				if (createRoad)
				{
					assert(zone.getModificator<RoadPlacer>());
					zone.getModificator<RoadPlacer>()->addRoadNode(guardPos);

					assert(otherZone->getModificator<RoadPlacer>());
					otherZone->getModificator<RoadPlacer>()->addRoadNode(roadNode);
				}
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
			if(generator.getZoneWater()->getModificator<WaterProxy>()->waterKeepConnection(connection, createRoad))
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

bool ConnectionsPlacer::gatePairingOk(const GatePair & gates) const
{
	// Checked during the search, so that a conflicting tile is merely skipped instead of costing the
	// whole connection.
	return generator.canReserveGatePair(gates.gate1.getVisitablePosition(), gates.gate2.getVisitablePosition());
}

bool ConnectionsPlacer::commitGatePair(const rmg::ZoneConnection & connection, GatePair & gates, rmg::Path & path1, rmg::Path & path2)
{
	// placeAndConnectObject only searched, it did not mark any tiles yet. Another zone may have reserved a
	// conflicting pair in the meantime, so reserving can still fail here.
	if(!generator.reserveGatePair(gates.gate1.getVisitablePosition(), gates.gate2.getVisitablePosition()))
		return false;

	gates.manager.placeObject(gates.gate1, gates.guarded1, true, gates.allowRoad);
	gates.managerOther.placeObject(gates.gate2, gates.guarded2, true, gates.allowRoad);

	replaceWithCurvedPath(path1, zone, gates.gate1.getVisitablePosition());
	replaceWithCurvedPath(path2, gates.otherZone, gates.gate2.getVisitablePosition());

	zone.connectPath(path1);
	gates.otherZone.connectPath(path2);

	assert(gates.otherZone.getModificator<ConnectionsPlacer>());
	gates.otherZone.getModificator<ConnectionsPlacer>()->otherSideConnection(connection);
	return true;
}

bool ConnectionsPlacer::placeGatePairInColumn(const rmg::ZoneConnection & connection, GatePair & gates, const rmg::Area & commonArea)
{
	rmg::Path path2(gates.otherZone.area().get());
	rmg::Path path1 = gates.manager.placeAndConnectObject(commonArea, gates.gate1, [this, &gates, &path2](const int3 & tile)
	{
		auto ti = map.getTileInfo(tile);
		auto otherTi = map.getTileInfo(tile - gates.zShift);
		float dist = ti.getNearestObjectDistance();
		float otherDist = otherTi.getNearestObjectDistance();
		if(dist < GATE_MIN_DIST || otherDist < GATE_MIN_DIST)
			return -1.f;

		//This could fail is accessibleArea is below the map
		rmg::Area toPlace(gates.gate1.getArea());
		toPlace.unite(toPlace.getBorderOutside()); // Add a bit of extra space around
		toPlace.erase_if([this](const int3 & tile)
		{
			return !map.isOnMap(tile);
		});
		toPlace.translate(-gates.zShift);

		path2 = gates.managerOther.placeAndConnectObject(toPlace, gates.gate2, GATE_MIN_DIST, gates.guarded2, true, ObjectManager::OptimizeType::NONE);

		return path2.valid() && gatePairingOk(gates) ? (dist * otherDist) : -1.f;
	}, gates.guarded1, true, ObjectManager::OptimizeType::DISTANCE);

	return path1.valid() && path2.valid() && commitGatePair(connection, gates, path1, path2);
}

bool ConnectionsPlacer::placeGatePairOffColumn(const rmg::ZoneConnection & connection, GatePair & gates, int maxGateDistance)
{
	const int maxDistSqr = maxGateDistance * maxGateDistance;
	const rmg::Area otherPossible = gates.otherZone.areaPossible().get();

	// 2D disk offsets within maxGateDistance, used to gather gate2 candidates around gate1's column.
	std::vector<int3> disk;
	for(int dy = -maxGateDistance; dy <= maxGateDistance; ++dy)
		for(int dx = -maxGateDistance; dx <= maxGateDistance; ++dx)
			if(dx * dx + dy * dy <= maxDistSqr)
				disk.emplace_back(dx, dy, 0);

	// Any tile of this zone may host gate1 - the weight function rejects those with no partner in reach.
	// Each evaluation runs a full nested object placement, so scoring every candidate would cost one of
	// those per tile. DISTANCE stops at the first fit, walking outwards from the zone center rather than
	// in coordinate order, which would pin every gate to a corner.
	rmg::Path path2(gates.otherZone.area().get());
	rmg::Path path1 = gates.manager.placeAndConnectObject(zone.areaPossible().get(), gates.gate1, [this, &gates, &otherPossible, &disk, &path2](const int3 & tile)
	{
		float dist = map.getTileInfo(tile).getNearestObjectDistance();
		if(dist < GATE_MIN_DIST)
			return -1.f;

		// Gather the other zone's possible tiles within maxGateDistance of gate1's column point.
		const int3 anchor = tile - gates.zShift;
		rmg::Area gate2Search;
		for(const auto & off : disk)
		{
			const int3 t = anchor + off;
			if(otherPossible.contains(t))
				gate2Search.add(t);
		}
		if(gate2Search.empty())
			return -1.f;

		path2 = gates.managerOther.placeAndConnectObject(gate2Search, gates.gate2, GATE_MIN_DIST, gates.guarded2, true, ObjectManager::OptimizeType::NONE);
		if(!path2.valid() || !gatePairingOk(gates))
			return -1.f;

		float otherDist = map.getTileInfo(gates.gate2.getVisitablePosition()).getNearestObjectDistance();
		return dist * otherDist;
	}, gates.guarded1, true, ObjectManager::OptimizeType::DISTANCE);

	return path1.valid() && path2.valid() && commitGatePair(connection, gates, path1, path2);
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

		auto lock = lockZones(otherZone);

		std::scoped_lock doubleLock(zone.areaMutex, otherZone->areaMutex);
		auto commonArea = zone.areaPossible().get() * (otherZone->areaPossible().get() + zShift);
		const int maxGateDistance = generator.getConfig().zonePlacement.maxGateDistance;

		// Nothing to search for if the zones never overlap in XY and off-column gates are disabled.
		if(!commonArea.empty() || maxGateDistance > 0)
		{
			assert(zone.getModificator<ObjectManager>());
			assert(otherZone->getModificator<ObjectManager>());
			auto & manager = *zone.getModificator<ObjectManager>();
			auto & managerOther = *otherZone->getModificator<ObjectManager>();

			auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::SUBTERRANEAN_GATE, 0);
			rmg::Object rmgGate1(factory->create(map.mapInstance->cb, nullptr));
			rmg::Object rmgGate2(factory->create(map.mapInstance->cb, nullptr));
			rmgGate1.setTemplate(zone.getTerrainType(), zone.getRand());
			rmgGate2.setTemplate(otherZone->getTerrainType(), zone.getRand());

			bool guarded1 = manager.addGuard(rmgGate1, connection.getGuardStrength(), true);
			bool guarded2 = managerOther.addGuard(rmgGate2, connection.getGuardStrength(), true);

			GatePair gates{*otherZone, manager, managerOther, rmgGate1, rmgGate2, zShift,
				guarded1, guarded2, shouldGenerateRoad(connection)};

			// Preferred: both gates in a shared column, so they are trivially each other's nearest.
			if(!commonArea.empty())
				success = placeGatePairInColumn(connection, gates, commonArea);

			// Fallback: rescues cross-level connections whose zones never overlapped in XY, which
			// previously became a monolith outright.
			if(!success && maxGateDistance > 0)
				success = placeGatePairOffColumn(connection, gates, maxGateDistance);
		}
	}

	//4. place monoliths/portals
	if(!success)
	{
		placeMonolithConnection(connection);
	}
}

void ConnectionsPlacer::placeMonolithConnection(const rmg::ZoneConnection & connection)
{
	auto otherZoneId = (connection.getZoneA() == zone.getId() ? connection.getZoneB() : connection.getZoneA());
	auto & otherZone = map.getZones().at(otherZoneId);

	bool allowRoad = shouldGenerateRoad(connection);

	auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::MONOLITH_TWO_WAY, generator.getNextMonlithIndex());
	auto teleport1 = factory->create(map.mapInstance->cb, nullptr);
	auto teleport2 = factory->create(map.mapInstance->cb, nullptr);

	RequiredObjectInfo obj1(teleport1, connection.getGuardStrength(), allowRoad);
	RequiredObjectInfo obj2(teleport2, connection.getGuardStrength(), allowRoad);
	zone.getModificator<ObjectManager>()->addRequiredObject(obj1);
	otherZone->getModificator<ObjectManager>()->addRequiredObject(obj2);

	dCompleted.push_back(connection);
	
	assert(otherZone->getModificator<ConnectionsPlacer>());
	otherZone->getModificator<ConnectionsPlacer>()->otherSideConnection(connection);
}

void ConnectionsPlacer::collectNeighbourZones()
{
	auto border = zone.area()->getBorderOutside();
	for(const auto & i : border)
	{
		if(!map.isOnMap(i))
			continue;
		
		auto zid = map.getZoneID(i);
		assert(zid != zone.getId());
		dNeighbourZones[zid].insert(i);
	}
}

bool ConnectionsPlacer::shouldGenerateRoad(const rmg::ZoneConnection& connection) const
{
	if (connection.getRoadOption() == rmg::ERoadOption::ROAD_RANDOM)
		logGlobal->error("Random road between zones %d and %d", connection.getZoneA(), connection.getZoneB());
	else
		logGlobal->info("Should generate road between zones %d and %d: %d", connection.getZoneA(), connection.getZoneB(), connection.getRoadOption() == rmg::ERoadOption::ROAD_TRUE);
	return connection.getRoadOption() == rmg::ERoadOption::ROAD_TRUE;
}

void ConnectionsPlacer::createBorder()
{
	rmg::Area borderArea(zone.area()->getBorder());
	rmg::Area borderOutsideArea(zone.area()->getBorderOutside());
	auto blockBorder = borderArea.getSubarea([this, &borderOutsideArea](const int3 & t)
	{
		auto tile = borderOutsideArea.nearest(t);
		return map.isOnMap(tile) && map.getZones()[map.getZoneID(tile)]->getType() != ETemplateZoneType::WATER;
	});

	//No border for wide connections
	for (auto& connection : zone.getConnections()) // We actually placed that connection already
	{
		auto otherZone = connection.getOtherZoneId(zone.getId());

		if (connection.getConnectionType() == rmg::EConnectionType::WIDE)
		{
			auto sharedBorder = borderArea.getSubarea([this, otherZone, &borderOutsideArea](const int3 & t)
			{
				auto tile = borderOutsideArea.nearest(t);
				return map.isOnMap(tile) && map.getZones()[map.getZoneID(tile)]->getId() == otherZone;
			});

			blockBorder.subtract(sharedBorder);
		}
	};

	auto areaPossible = zone.areaPossible();
	for(const auto & tile : blockBorder.getTilesVector())
	{
		if(map.isPossible(tile))
		{
			map.setOccupied(tile, ETileType::BLOCKED);
			areaPossible->erase(tile);
		}

		map.foreachDirectNeighbour(tile, [this, &areaPossible](int3 &nearbyPos)
		{
			if(map.isPossible(nearbyPos) && map.getZoneID(nearbyPos) == zone.getId())
			{
				map.setOccupied(nearbyPos, ETileType::BLOCKED);
				areaPossible->erase(nearbyPos);
			}
		});
	}
}
