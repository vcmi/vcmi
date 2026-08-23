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

void ConnectionsPlacer::selfSideIndirectConnection(const rmg::ZoneConnection & connection)
{
	bool success = false;
	auto otherZoneId = (connection.getZoneA() == zone.getId() ? connection.getZoneB() : connection.getZoneA());
	auto & otherZone = map.getZones().at(otherZoneId);

	bool allowRoad = shouldGenerateRoad(connection);

	//3. place subterrain gates
	if(zone.isUnderground() != otherZone->isUnderground())
	{
		int3 zShift(0, 0, zone.getPos().z - otherZone->getPos().z);

		auto lock = lockZones(otherZone);

		std::scoped_lock doubleLock(zone.areaMutex, otherZone->areaMutex);
		auto commonArea = zone.areaPossible().get() * (otherZone->areaPossible().get() + zShift);
		const int maxGateDistance = generator.getConfig().zonePlacement.maxGateDistance;

		assert(zone.getModificator<ObjectManager>());
		auto & manager = *zone.getModificator<ObjectManager>();

		assert(otherZone->getModificator<ObjectManager>());
		auto & managerOther = *otherZone->getModificator<ObjectManager>();

		auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::SUBTERRANEAN_GATE, 0);
		auto gate1 = factory->create(map.mapInstance->cb, nullptr);
		auto gate2 = factory->create(map.mapInstance->cb, nullptr);
		rmg::Object rmgGate1(gate1);
		rmg::Object rmgGate2(gate2);
		rmgGate1.setTemplate(zone.getTerrainType(), zone.getRand());
		rmgGate2.setTemplate(otherZone->getTerrainType(), zone.getRand());
		bool guarded1 = manager.addGuard(rmgGate1, connection.getGuardStrength(), true);
		bool guarded2 = managerOther.addGuard(rmgGate2, connection.getGuardStrength(), true);
		const int minDist = 3;

		// Positions are chosen (placeAndConnectObject only searches - it does not mark tiles) before this
		// runs. Reserve the pairing so off-column gates stay each other's nearest gate, then place for real.
		auto commitGates = [&](rmg::Path & path1, rmg::Path & path2) -> bool
		{
			if(maxGateDistance > 0 && !generator.reserveGatePair(rmgGate1.getVisitablePosition(), rmgGate2.getVisitablePosition()))
				return false;

			manager.placeObject(rmgGate1, guarded1, true, allowRoad);
			managerOther.placeObject(rmgGate2, guarded2, true, allowRoad);

			replaceWithCurvedPath(path1, zone, rmgGate1.getVisitablePosition());
			replaceWithCurvedPath(path2, *otherZone, rmgGate2.getVisitablePosition());

			zone.connectPath(path1);
			otherZone->connectPath(path2);

			assert(otherZone->getModificator<ConnectionsPlacer>());
			otherZone->getModificator<ConnectionsPlacer>()->otherSideConnection(connection);
			return true;
		};

		// 3a. Preferred: both gates in a shared column (identical XY on both levels).
		if(!commonArea.empty())
		{
			rmg::Path path2(otherZone->area().get());
			rmg::Path path1 = manager.placeAndConnectObject(commonArea, rmgGate1, [this, minDist, &path2, &rmgGate1, &zShift, guarded2, &managerOther, &rmgGate2	](const int3 & tile)
			{
				auto ti = map.getTileInfo(tile);
				auto otherTi = map.getTileInfo(tile - zShift);
				float dist = ti.getNearestObjectDistance();
				float otherDist = otherTi.getNearestObjectDistance();
				if(dist < minDist || otherDist < minDist)
					return -1.f;

				//This could fail is accessibleArea is below the map
				rmg::Area toPlace(rmgGate1.getArea());
				toPlace.unite(toPlace.getBorderOutside()); // Add a bit of extra space around
				toPlace.erase_if([this](const int3 & tile)
				{
					return !map.isOnMap(tile);
				});
				toPlace.translate(-zShift);

				path2 = managerOther.placeAndConnectObject(toPlace, rmgGate2, minDist, guarded2, true, ObjectManager::OptimizeType::NONE);

				return path2.valid() ? (dist * otherDist) : -1.f;
			}, guarded1, true, ObjectManager::OptimizeType::DISTANCE);

			if(path1.valid() && path2.valid())
				success = commitGates(path1, path2);
		}

		// 3b. Fallback: gates need not share a column. Place them on the nearest suitable tiles in each
		// zone, at most maxGateDistance apart, as long as they stay each other's nearest gate. This rescues
		// cross-level connections whose zones never overlapped in XY (previously an immediate monolith).
		if(!success && maxGateDistance > 0)
		{
			const int maxDistSqr = maxGateDistance * maxGateDistance;
			const rmg::Area otherPossible = otherZone->areaPossible().get();

			// 2D disk offsets within maxGateDistance, used to gather gate2 candidates around gate1's column.
			std::vector<int3> disk;
			for(int dy = -maxGateDistance; dy <= maxGateDistance; ++dy)
				for(int dx = -maxGateDistance; dx <= maxGateDistance; ++dx)
					if(dx * dx + dy * dy <= maxDistSqr)
						disk.emplace_back(dx, dy, 0);

			// Restrict gate1 candidates to this zone's tiles within reach of the other zone's projection.
			rmg::Area otherProjected(otherPossible);
			otherProjected.translate(zShift);
			rmg::Area gate1Search = zone.areaPossible().get().getSubarea([&otherProjected, maxDistSqr](const int3 & t)
			{
				return otherProjected.distanceSqr(t) <= maxDistSqr;
			});

			if(!gate1Search.empty())
			{
				rmg::Path path2(otherZone->area().get());
				rmg::Path path1 = manager.placeAndConnectObject(gate1Search, rmgGate1, [this, &otherPossible, &disk, minDist, &path2, &rmgGate1, &zShift, guarded2, &managerOther, &rmgGate2](const int3 & tile)
				{
					float dist = map.getTileInfo(tile).getNearestObjectDistance();
					if(dist < minDist)
						return -1.f;

					// Gather the other zone's possible tiles within maxGateDistance of gate1's column point.
					const int3 anchor = tile - zShift;
					rmg::Area gate2Search;
					for(const auto & off : disk)
					{
						const int3 t = anchor + off;
						if(otherPossible.contains(t))
							gate2Search.add(t);
					}
					if(gate2Search.empty())
						return -1.f;

					path2 = managerOther.placeAndConnectObject(gate2Search, rmgGate2, minDist, guarded2, true, ObjectManager::OptimizeType::NONE);
					if(!path2.valid())
						return -1.f;

					float otherDist = map.getTileInfo(rmgGate2.getVisitablePosition()).getNearestObjectDistance();
					return dist * otherDist;
				}, guarded1, true, ObjectManager::OptimizeType::DISTANCE);

				if(path1.valid() && path2.valid())
					success = commitGates(path1, path2);
			}
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
