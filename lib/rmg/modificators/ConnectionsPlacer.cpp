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
#include "../ConnectionReport.h"
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
					generator.getConnectionReport().noteResolution(c, rmg::ConnectionReport::Resolution::PORTAL_INTENDED);
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
		generator.getConnectionReport().noteResolution(connection, rmg::ConnectionReport::Resolution::PORTAL_INTENDED);
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

							generator.getConnectionReport().noteResolution(connection, rmg::ConnectionReport::Resolution::WIDE);
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
		generator.getConnectionReport().noteResolution(connection, rmg::ConnectionReport::Resolution::VIRTUAL);
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

				generator.getConnectionReport().noteResolution(connection, rmg::ConnectionReport::Resolution::DIRECT);
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
				generator.getConnectionReport().noteResolution(connection, rmg::ConnectionReport::Resolution::WATER);
				success = true;
			}
		}
	}

	if(success)
		dCompleted.push_back(connection);
	else
	{
		//Record why a direct land passage could not be built; consumed later if this connection ends up a monolith
		auto reason = rmg::ConnectionReport::DirectFailure::NO_VALID_GUARD;
		if(directConnectionIterator == dNeighbourZones.end())
			reason = rmg::ConnectionReport::DirectFailure::NOT_ADJACENT;
		else if(directProhibited)
			reason = rmg::ConnectionReport::DirectFailure::TERRAIN_PROHIBITED;

		int sharedBorderTiles = directConnectionIterator != dNeighbourZones.end()
			? static_cast<int>(directConnectionIterator->second.size()) : 0;

		generator.getConnectionReport().noteDirectFailure(connection, reason, sharedBorderTiles, gridRelation(*otherZone));
	}
}

rmg::ConnectionReport::GridRelation ConnectionsPlacer::gridRelation(const Zone & otherZone) const
{
	using GridRelation = rmg::ConnectionReport::GridRelation;

	int3 gridA = zone.getGridPosition();
	int3 gridB = otherZone.getGridPosition();
	if(gridA.x < 0 || gridB.x < 0) //at least one zone was never placed on the grid (e.g. water)
		return GridRelation::UNKNOWN;

	if(gridA.z != gridB.z)
		return GridRelation::DIFFERENT_LEVEL;

	if(generator.getConfig().zonePlacementHexGrid)
	{
		// hex placement stores odd-r offset coords; a square dx/dy test misreads hex neighbours
		// (dx+dy==2) as DISTANT, so measure hex cube distance to match the placement metric
		auto toCube = [](const int3 & c)
		{
			const int x = c.x - (c.y - (c.y & 1)) / 2;
			const int z = c.y;
			return int3(x, -x - z, z);
		};
		const int3 ca = toCube(gridA);
		const int3 cb = toCube(gridB);
		const int hexDist = (std::abs(ca.x - cb.x) + std::abs(ca.y - cb.y) + std::abs(ca.z - cb.z)) / 2;
		if(hexDist == 1)
			return GridRelation::ORTHOGONAL; //adjacent hex cells - placement intended a shared border
		if(hexDist == 2)
			return GridRelation::DIAGONAL; //one ring too far - near miss
		return GridRelation::DISTANT; //more than one cell apart
	}

	int dx = std::abs(gridA.x - gridB.x);
	int dy = std::abs(gridA.y - gridB.y);
	if(dx + dy == 1)
		return GridRelation::ORTHOGONAL; //share a grid edge - placement intended a shared border
	if(dx == 1 && dy == 1)
		return GridRelation::DIAGONAL; //share only a grid corner
	return GridRelation::DISTANT; //more than one cell apart
}

void ConnectionsPlacer::selfSideIndirectConnection(const rmg::ZoneConnection & connection)
{
	bool success = false;
	auto otherZoneId = (connection.getZoneA() == zone.getId() ? connection.getZoneB() : connection.getZoneA());
	auto & otherZone = map.getZones().at(otherZoneId);

	bool allowRoad = shouldGenerateRoad(connection);

	// Diagnostics for cross-level connections that may degrade into a monolith
	auto gateFailure = rmg::ConnectionReport::GateFailure::NONE;
	bool crossLevel = zone.getPos().z != otherZone->getPos().z;
	int possibleOverlapTiles = 0;
	int fullOverlapTiles = 0;

	//3. place subterrain gates
	if(zone.isUnderground() != otherZone->isUnderground())
	{
		int3 zShift(0, 0, zone.getPos().z - otherZone->getPos().z);

		auto lock = lockZones(otherZone);

		std::scoped_lock doubleLock(zone.areaMutex, otherZone->areaMutex);
		auto commonArea = zone.areaPossible().get() * (otherZone->areaPossible().get() + zShift);
		fullOverlapTiles = static_cast<int>((zone.area().get() * (otherZone->area().get() + zShift)).getTiles().size());
		possibleOverlapTiles = static_cast<int>(commonArea.getTiles().size());
		const int maxGateDistance = generator.getConfig().zonePlacementMaxGateDistance;

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

			generator.getConnectionReport().noteResolution(connection, rmg::ConnectionReport::Resolution::SUBTERRANEAN_GATE);
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

		if(!success)
		{
			if(!commonArea.empty())
				gateFailure = rmg::ConnectionReport::GateFailure::GATE_PLACEMENT_FAILED;
			else
				//No shared possible-area: did the footprints ever overlap, or was the overlap consumed earlier?
				gateFailure = fullOverlapTiles > 0
					? rmg::ConnectionReport::GateFailure::OVERLAP_CONSUMED
					: rmg::ConnectionReport::GateFailure::NO_AREA_OVERLAP;
		}
	}
	else if(crossLevel)
	{
		//Different levels but same surface/underground state - a gate can never bridge them
		gateFailure = rmg::ConnectionReport::GateFailure::SAME_UNDERGROUND_STATE;
	}

	//4. place monoliths/portals
	if(!success)
	{
		if(crossLevel)
			logGateFailure(connection, *otherZone, gateFailure, fullOverlapTiles, possibleOverlapTiles);

		generator.getConnectionReport().noteGateFailure(connection, gateFailure);
		generator.getConnectionReport().noteResolution(connection, rmg::ConnectionReport::Resolution::MONOLITH);
		placeMonolithConnection(connection);
	}
}

void ConnectionsPlacer::logGateFailure(const rmg::ZoneConnection & connection, const Zone & otherZone, rmg::ConnectionReport::GateFailure reason, int fullOverlapTiles, int possibleOverlapTiles) const
{
	// Distance between zones as placement left them (pre-tiling) vs. where they ended up (post-tiling),
	// so we can tell "never aligned" from "aligned but tiling/relaxation pulled the footprints apart".
	auto placementDist = [this, &otherZone]() -> float
	{
		float3 a = zone.getPlacementCenter();
		float3 b = otherZone.getPlacementCenter();
		if(a.x < 0 || b.x < 0)
			return -1.f;
		float dx = (a.x - b.x) * map.width();
		float dy = (a.y - b.y) * map.height();
		return std::sqrt(dx * dx + dy * dy);
	}();

	float finalDist = static_cast<float>(zone.getPos().dist2d(otherZone.getPos())); //ignores z

	logGlobal->info(
		"Cross-level connection zones %d(level %d)<->%d(level %d) fell back to monolith. "
		"Footprint XY overlap: %d tiles (possible-area overlap: %d tiles). "
		"Center XY distance after placement: %.1f, after tiling: %.1f. Cause: %s",
		zone.getId(), zone.getPos().z, otherZone.getId(), otherZone.getPos().z,
		fullOverlapTiles, possibleOverlapTiles, placementDist, finalDist,
		rmg::ConnectionReport::describeGateFailure(reason));
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
