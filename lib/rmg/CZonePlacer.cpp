/*
 * CZonePlacer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CZonePlacer.h"
#include "CZoneGridPlacer.h"

#include "../TerrainHandler.h"
#include "../entities/faction/CFaction.h"
#include "../entities/faction/CTownHandler.h"
#include "../mapping/CMap.h"
#include "../mapping/CMapEditManager.h"
#include "../GameLibrary.h"
#include "CMapGenOptions.h"
#include "CRmgTemplate.h"
#include "RmgMap.h"
#include "Zone.h"
#include "Functions.h"
#include "PenroseTiling.h"

#include <vstd/RNG.h>

#include <limits>

CZonePlacer::CZonePlacer(RmgMap & map,
	int placementAttempts, int scoreDirect, int scoreGate, int scoreMonolith, bool hexGrid, bool hubFirst, bool saPolish, float crossAlignWeight,
	bool capacityBalance, int capacityIterations, float capacityGain, bool randomOrientation)
	: width(0), height(0), mapSize(0),
	gravityConstant(1e-3f),
	stiffnessConstant(3e-3f),
	stifness(0),
	stiffnessIncreaseFactor(1.03f),
	hexGrid(hexGrid),
	hubFirst(hubFirst),
	saPolish(saPolish),
	crossAlignWeight(crossAlignWeight),
	capacityBalance(capacityBalance),
	capacityIterations(capacityIterations),
	capacityGain(capacityGain),
	randomOrientation(randomOrientation),
	attractionReachScore(1.5f),
	placementAttempts(placementAttempts),
	scoreDirect(scoreDirect),
	scoreGate(scoreGate),
	scoreMonolith(scoreMonolith),
	bestTotalDistance(1e10),
	bestTotalOverlap(1e10),
	map(map)
{
}

int3 CZonePlacer::cords(const float3 & f) const
{
	return int3(static_cast<si32>(std::max(0.f, (f.x * map.width()) - 1)), static_cast<si32>(std::max(0.f, (f.y * map.height() - 1))), f.z);
}

float CZonePlacer::getDistance (float distance) const
{
	return (distance ? distance * distance : 1e-6f);
}

void CZonePlacer::findPathsBetweenZones()
{
	auto zones = map.getZones();

	std::set<std::shared_ptr<Zone>> zonesToCheck;

	// Iterate through each pair of nodes in the graph

	for (const auto& zone : zones)
	{
		int start = zone.first;
		distancesBetweenZones[start][start] = 0; // Distance from a node to itself is 0

		std::queue<int> q;
		std::map<int, bool> visited;
		visited[start] = true;
		q.push(start);

		// Perform Breadth-First Search from the starting node
		while (!q.empty())
		{
			int current = q.front();
			q.pop();

			const auto& currentZone = zones.at(current);
			const auto& connectedZoneIds = currentZone->getConnections();

			for (auto & connection : connectedZoneIds)
			{
				switch (connection.getConnectionType())
				{
					//Do not consider virtual connections for graph distance
					case rmg::EConnectionType::REPULSIVE:
					case rmg::EConnectionType::FORCE_PORTAL:
						continue;
				}
				auto neighbor = connection.getOtherZoneId(current);

				if (current == neighbor)
				{
					//Do not consider self-connections
					continue;
				}

				if (!visited[neighbor])
				{
					visited[neighbor] = true;
					q.push(neighbor);
					distancesBetweenZones[start][neighbor] = distancesBetweenZones[start][current] + 1;
				}
			}
		}
	}
}

float CZonePlacer::scaleForceBetweenZones(const std::shared_ptr<Zone> zoneA, const std::shared_ptr<Zone> zoneB) const
{
	if (zoneA->getOwner() && zoneB->getOwner()) //Players participate in game
	{
		int firstPlayer = zoneA->getOwner().value();
		int secondPlayer = zoneB->getOwner().value();

		//Players with lower indexes (especially 1 and 2) will be placed further apart

		return (1.0f + (2.0f / (firstPlayer * secondPlayer)));
	}
	else
	{
		return 1;
	}
}

void CZonePlacer::placeZones(vstd::RNG * rand)
{
	logGlobal->info("Starting zone placement");

	width = map.getMapGenOptions().getWidth();
	height = map.getMapGenOptions().getHeight();

	auto zones = map.getZones();

	// Water zones do not go through the land-zone tessellation below - their area gets carved
	// out of land zones later, via WaterAdopter/WaterProxy. They still need a valid level though,
	// otherwise they keep the Zone default of level 0, which is wrong whenever level 0 isn't the
	// surface layer. Put them on the surface layer (falling back to level 0 if there is none).
	{
		const auto & mapLayers = map.getMapGenOptions().getLevelMapLayers();
		int waterLevel = 0;
		for(size_t i = 0; i < mapLayers.size(); i++)
		{
			if(mapLayers[i] == MapLayerId::SURFACE)
			{
				waterLevel = static_cast<int>(i);
				break;
			}
		}
		for(const auto & zonePair : zones)
		{
			if(zonePair.second->getType() == ETemplateZoneType::WATER)
			{
				zonePair.second->setPos(int3(0, 0, waterLevel));
				zonePair.second->setCenter(float3(0.f, 0.f, static_cast<float>(waterLevel)));
			}
		}
	}

	vstd::erase_if(zones, [](const std::pair<TRmgTemplateZoneId, std::shared_ptr<Zone>> & pr)
	{
		return pr.second->getType() == ETemplateZoneType::WATER;
	});
	int mapLevels = map.getMapGenOptions().getLevels();

	findPathsBetweenZones();

	TZoneVector zonesVector(zones.begin(), zones.end());
	assert (zonesVector.size());

	// Snapshot the pristine per-zone state (original, un-prescaled sizes and centers) so every
	// attempt starts identically. prepareZones prescales sizes in place, so each attempt must
	// restore the original size before prescaling again - otherwise sizes compound across attempts.
	// Independent attempts also keep this parallelizable later.
	std::map<std::shared_ptr<Zone>, float3> pristineCenter;
	std::map<std::shared_ptr<Zone>, int> pristineSize;
	for(const auto & zone : zones)
	{
		pristineCenter[zone.second] = zone.second->getCenter();
		pristineSize[zone.second] = zone.second->getSize();
	}

	CZoneGridPlacer gridPlacer(
		map,
		distancesBetweenZones,
		[this](const std::shared_ptr<Zone> & zoneA, const std::shared_ptr<Zone> & zoneB)
		{
			return scaleForceBetweenZones(zoneA, zoneB);
		},
		hexGrid, hubFirst, saPolish, crossAlignWeight);

	// Winning layout across all attempts (positions and per-attempt prescaled sizes).
	std::map<std::shared_ptr<Zone>, float3> winningCenters;
	std::map<std::shared_ptr<Zone>, int> winningSizes;
	float winningScore = std::numeric_limits<float>::max();

	const int attempts = std::max(1, placementAttempts);
	for(int attempt = 0; attempt < attempts; ++attempt)
	{
		//restore pristine state and reset per-attempt accumulators
		for(const auto & zone : zones)
		{
			zone.second->setCenter(pristineCenter[zone.second]);
			zone.second->setSize(pristineSize[zone.second]);
		}
		bestTotalDistance = 1e10;
		bestTotalOverlap = 1e10;
		lastSwappedZones.clear();

		//re-roll the initial layout: zone order drives level split and grid placement
		RandomGeneratorUtil::randomShuffle(zonesVector, *rand);
		prepareZones(zones, zonesVector, mapLevels, rand);
		gridPlacer.placeOnGrid(zones, rand);

		std::map<std::shared_ptr<Zone>, float3> bestSolution;

		TForceVector forces;
		TForceVector totalForces; //  both attraction and pushback, overcomplicated?
		TDistanceVector distances;
		TDistanceVector overlaps;

		auto evaluateSolution = [this, zones, &distances, &overlaps, &bestSolution]() -> bool
		{
			bool improvement = false;

			float totalDistance = 0;
			float totalOverlap = 0;
			for (const auto& zone : distances) //find most misplaced zone
			{
				totalDistance += zone.second;
				float overlap = overlaps[zone.first];
				totalOverlap += overlap;
			}

			//check fitness function
			if ((totalDistance + 1) * (totalOverlap + 1) < (bestTotalDistance + 1) * (bestTotalOverlap + 1))
			{
				//multiplication is better for auto-scaling, but stops working if one factor is 0
				improvement = true;
			}

			//Save best solution
			if (improvement)
			{
				bestTotalDistance = totalDistance;
				bestTotalOverlap = totalOverlap;

				for (const auto& zone : zones)
					bestSolution[zone.second] = zone.second->getCenter();
			}

#ifdef ZONE_PLACEMENT_LOG
			logGlobal->trace("Total distance between zones after this iteration: %2.4f, Total overlap: %2.4f, Improved: %s", totalDistance, totalOverlap , improvement);
#endif

			return improvement;
		};

		 //Start with low stiffness. Bigger graphs need more time and more flexibility
		for (stifness = stiffnessConstant / zones.size(); stifness <= stiffnessConstant;)
		{
			//1. attract connected zones
			attractConnectedZones(zones, forces, distances);
			for(const auto & zone : forces)
			{
				zone.first->setCenter (zone.first->getCenter() + zone.second);
				totalForces[zone.first] = zone.second; //override
			}

			//2. separate overlapping zones
			separateOverlappingZones(zones, forces, overlaps);
			for(const auto & zone : forces)
			{
				zone.first->setCenter (zone.first->getCenter() + zone.second);
				totalForces[zone.first] += zone.second; //accumulate
			}

			bool improved = evaluateSolution();

			if (!improved)
			{
				//3. now perform drastic movement of zone that is completely not linked
				//TODO: Don't do this is fitness was improved
				moveOneZone(zones, totalForces, distances, overlaps);

				improved |= evaluateSolution();
			}

			if (!improved)
			{
				//Only cool down if we didn't see any improvement
				stifness *= stiffnessIncreaseFactor;
			}

		}

		ConnectivityCounts counts = classifyConnections(zones, bestSolution);
		float score = static_cast<float>(counts.direct * scoreDirect + counts.gates * scoreGate + counts.monoliths * scoreMonolith);
		logGlobal->info("Zone placement attempt %d/%d: score %d (direct %d, gates %d, monoliths %d); fitness distance %2.4f overlap %2.4f",
			attempt + 1, attempts, static_cast<int>(score), counts.direct, counts.gates, counts.monoliths, bestTotalDistance, bestTotalOverlap);

		if(score < winningScore)
		{
			winningScore = score;
			winningCenters = bestSolution;
			winningSizes.clear();
			for(const auto & zone : zones)
				winningSizes[zone.second] = zone.second->getSize();
		}
	}

	logGlobal->info("Best zone placement score: %d", static_cast<int>(winningScore));
	for(const auto & zone : zones) //apply the winning layout
	{
		zone.second->setSize(winningSizes[zone.second]);
		zone.second->setCenter(winningCenters[zone.second]);
		zone.second->setPos(cords(winningCenters[zone.second]));
	}

	if(randomOrientation)
		applyRandomOrientation(zones, rand);
}

std::pair<float, float> CZonePlacer::orientNormalized(float x, float y) const
{
	// Dihedral symmetry about (0.5, 0.5) in normalized space; maps [0,1]^2 onto itself.
	switch(orientRotation)
	{
		case 1: { const float t = x; x = y;         y = 1.f - t; break; } // 90
		case 2: {                    x = 1.f - x;    y = 1.f - y; break; } // 180
		case 3: { const float t = x; x = 1.f - y;    y = t;       break; } // 270
	}
	if(orientFlipH)
		x = 1.f - x;
	if(orientFlipV)
		y = 1.f - y;
	return {x, y};
}

void CZonePlacer::applyRandomOrientation(const TZoneMap & zones, vstd::RNG * rand)
{
	// Roll one of 4 rotations plus independent horizontal/vertical flips (4x2x2 = 16 combinations).
	// On a square map every combination is an isometry about the map center. A non-square map cannot
	// rotate 90/270 without distorting aspect, so restrict it to 0/180 there.
	const bool square = map.getMapGenOptions().getWidth() == map.getMapGenOptions().getHeight();
	orientRotation = square ? rand->nextInt(0, 3) : rand->nextInt(0, 1) * 2;
	orientFlipH = rand->nextInt(0, 1) == 1;
	orientFlipV = rand->nextInt(0, 1) == 1;
	if(orientRotation == 0 && !orientFlipH && !orientFlipV)
		return; //identity

	// Reorient center/pos (they feed the tessellation); assignZones replays the same isometry onto the
	// Penrose vertices so footprints - and thus cross-level gate overlaps - transform with the zones
	// instead of being reshuffled against a fixed vertex field. Diagnostic snapshots (grid cells,
	// placement centers) are left as-is: their consumers compare zones relatively, which any isometry preserves.
	for(const auto & zonePair : zones)
	{
		const auto & zone = zonePair.second;
		const auto [nx, ny] = orientNormalized(zone->getCenter().x, zone->getCenter().y);
		const float3 newCenter(nx, ny, zone->getCenter().z);
		zone->setCenter(newCenter);
		zone->setPos(cords(newCenter));
	}
}

void CZonePlacer::prepareZones(TZoneMap &zones, TZoneVector &zonesVector, const int mapLevels, vstd::RNG * rand)
{
	std::map<int, float> totalSize; //make sure that sum of zone sizes on surface and uderground match size of the map

	std::map<int, int> zonesOnLevel;
	for (int i = 0; i < mapLevels; i++)
		zonesOnLevel[i] = 0;

	// map.getMapGenOptions().getLevelMapLayers() is always sized to match getLevels() == mapLevels
	const auto & mapLayers = map.getMapGenOptions().getLevelMapLayers();

	auto findLevelByLayer = [&](MapLayerId layer) -> int {
		for (int i = 0; i < mapLevels; i++)
			if (mapLayers[i] == layer) return i;
		return -1;
	};

	//even distribution for surface / underground zones. Surface zones always have priority.
	//NOTE: only SURFACE and UNDERGROUND layers get special treatment here, any other (custom) layer
	//is treated the same as underground for the purposes of this distribution

	TZoneVector zonesToPlace;
	std::map<TRmgTemplateZoneId, int> levels;

	auto addZoneEqually = [&](auto & zone, bool ignoreUnderground = false) {
		// Only restrict to surface levels if a surface layer actually exists on this map,
		// otherwise fall back to distributing across all levels
		bool restrictToSurface = ignoreUnderground && findLevelByLayer(MapLayerId::SURFACE) >= 0;

		int chosenLevel = 0;
		int minCount = std::numeric_limits<int>::max();

		for (const auto& [level, count] : zonesOnLevel) {
			if (restrictToSurface && mapLayers[level] != MapLayerId::SURFACE)
				continue;

			if (count < minCount ||
				(count == minCount && mapLayers[level] == MapLayerId::SURFACE && mapLayers[chosenLevel] != MapLayerId::SURFACE) ||
				(count == minCount && mapLayers[chosenLevel] != MapLayerId::SURFACE && level < chosenLevel))
			{
				chosenLevel = level;
				minCount = count;
			}
		}

		levels[zone.first] = chosenLevel;
		zonesOnLevel[chosenLevel]++;
	};

	//first pass - determine fixed surface for zones
	for(const auto & zone : zonesVector)
	{
		if (mapLevels == 1) //this step is ignored
		{
			zonesToPlace.push_back(zone);
			continue;
		}

		// Check if zone has forced level assignment
		auto forcedLevel = zone.second->getForcedLevel();
		if (forcedLevel == EZoneLevel::SURFACE)
		{
			// Force to surface
			int targetLevel = findLevelByLayer(MapLayerId::SURFACE);
			if (targetLevel < 0) targetLevel = 0;
			levels[zone.first] = targetLevel;
			zonesOnLevel[targetLevel]++;
			continue;
		}
		else if (forcedLevel == EZoneLevel::UNDERGROUND)
		{
			// Force to underground
			int targetLevel = findLevelByLayer(MapLayerId::UNDERGROUND);
			if (targetLevel < 0) targetLevel = 0;
			levels[zone.first] = targetLevel;
			zonesOnLevel[targetLevel]++;
			continue;
		}
		// forcedLevel == AUTOMATIC - continue with normal logic

		//place players depending on their factions
		if(std::optional<int> owner = zone.second->getOwner())
		{
			auto player = PlayerColor(*owner - 1);
			auto playerSettings = map.getMapGenOptions().getPlayersSettings();
			FactionID faction = FactionID::RANDOM;
			if (playerSettings.size() > player.getNum())
			{
				faction = std::next(playerSettings.begin(), player.getNum())->second.getStartingTown();
			}
			else
			{
				logGlobal->trace("Player %d (starting zone %d) does not participate in game", player.getNum(), zone.first);
			}

			if (faction == FactionID::RANDOM) //TODO: check this after a town has already been randomized
				zonesToPlace.push_back(zone);
			else
			{
				const auto & factionObject = (*LIBRARY->townh)[faction];
				const auto & nativeTerrains = factionObject->nativeTerrains;
				if(nativeTerrains.empty())
				{
					//any / random
					zonesToPlace.push_back(zone);
				}
				else
				{
					bool hasUndergroundTerrain = false;
					bool hasSurfaceTerrain = false;
					for (const auto & terrainId : nativeTerrains)
					{
						const auto & terrainType = LIBRARY->terrainTypeHandler->getById(terrainId);
						if (terrainType->isUnderground())
							hasUndergroundTerrain = true;
						if (terrainType->isSurface())
							hasSurfaceTerrain = true;
					}

					if(hasUndergroundTerrain && !hasSurfaceTerrain)
					{
						// underground only
						int targetLevel = findLevelByLayer(MapLayerId::UNDERGROUND);
						if (targetLevel < 0)
							targetLevel = 0;
						zonesOnLevel[targetLevel]++;
						levels[zone.first] = targetLevel;
					}
					else
					{
						// surface-only or mixed surface+underground
						// mixed should not be forced to surface
						addZoneEqually(zone, !hasUndergroundTerrain);
					}
				}
			}
		}
		else //no starting zone or no underground altogether
		{
			zonesToPlace.push_back(zone);
		}
	}
	for(const auto & zone : zonesToPlace)
	{
		if (mapLevels > 1) //only then consider underground zones
			addZoneEqually(zone);
		else
			levels[zone.first] = 0;
	}

	for(const auto & zone : zonesVector)
	{
		int level = levels[zone.first];
		totalSize[level] += (zone.second->getSize() * zone.second->getSize());
		float3 center = zone.second->getCenter();
		center.z = level;
		zone.second->setCenter(center);
	}

	/*
	prescale zones

	formula: sum((prescaler*n)^2)*pi = WH

	prescaler = sqrt((WH)/(sum(n^2)*pi))
	*/

	std::map<int, float> prescaler;
	for (int i = 0; i < mapLevels; i++)
		prescaler[i] = std::sqrt((width * height) / (totalSize[i] * PI_CONSTANT));
	mapSize = static_cast<float>(sqrt(width * height));
	for(const auto & zone : zones)
	{
		zone.second->setSize(static_cast<int>(zone.second->getSize() * prescaler[zone.second->getCenter().z]));
	}
}

void CZonePlacer::attractConnectedZones(TZoneMap & zones, TForceVector & forces, TDistanceVector & distances) const
{
	for(const auto & zone : zones)
	{
		float3 forceVector(0, 0, 0);
		float3 pos = zone.second->getCenter();
		float totalDistance = 0;

		for (const auto & connection : zone.second->getConnections())
		{
			switch (connection.getConnectionType())
			{
				//Do not consider virtual connections for graph distance
				case rmg::EConnectionType::REPULSIVE:
				case rmg::EConnectionType::FORCE_PORTAL:
					continue;
			}
			if (connection.getZoneA() == connection.getZoneB())
			{
				//Do not consider self-connections
				continue;
			}

			auto otherZone = zones[connection.getOtherZoneId(zone.second->getId())];
			float3 otherZoneCenter = otherZone->getCenter();
			auto distance = static_cast<float>(pos.dist2d(otherZoneCenter));
			float scale = scaleForceBetweenZones(zone.second, otherZone);

			//zones on different levels can overlap completely
			float minDistance = (pos.z != otherZoneCenter.z) ? 0.f
				: (zone.second->getSize() + otherZone->getSize()) / mapSize; //scale down to (0,1) coordinates

			forceVector += (otherZoneCenter - pos) * distance * gravityConstant * scale; //positive value

			if (distance > minDistance)
				totalDistance += (distance - minDistance);
		}
		distances[zone.second] = totalDistance;
		forceVector.z = 0; //operator - doesn't preserve z coordinate :/
		forces[zone.second] = forceVector;
	}
}

void CZonePlacer::separateOverlappingZones(TZoneMap &zones, TForceVector &forces, TDistanceVector &overlaps)
{
	for(const auto & zone : zones)
	{
		float3 forceVector(0, 0, 0);
		float3 pos = zone.second->getCenter();

		float overlap = 0;
		//separate overlapping zones
		for(const auto & otherZone : zones)
		{
			float3 otherZoneCenter = otherZone.second->getCenter();
			//zones on different levels don't push away
			if (zone == otherZone || pos.z != otherZoneCenter.z)
				continue;

			auto distance = static_cast<float>(pos.dist2d(otherZoneCenter));
			float minDistance = (zone.second->getSize() + otherZone.second->getSize()) / mapSize;
			if (distance < minDistance)
			{
				float3 localForce = (((otherZoneCenter - pos)*(minDistance / (distance ? distance : 1e-3f))) / getDistance(distance)) * stifness;
				//negative value
				localForce *= scaleForceBetweenZones(zone.second, otherZone.second);
				forceVector -= localForce * (distancesBetweenZones[zone.second->getId()][otherZone.second->getId()] / 2.0f);
				overlap += (minDistance - distance); //overlapping of small zones hurts us more
			}
		}

		//move zones away from boundaries
		//do not scale boundary distance - zones tend to get squashed
		float size = zone.second->getSize() / mapSize;

		auto pushAwayFromBoundary = [&forceVector, pos, size, &overlap, this](float x, float y)
		{
			float3 boundary = float3(x, y, pos.z);
			auto distance = static_cast<float>(pos.dist2d(boundary));
			overlap += std::max<float>(0, distance - size); //check if we're closer to map boundary than value of zone size
			forceVector -= (boundary - pos) * (size - distance) / this->getDistance(distance) * this->stifness; //negative value
		};
		if (pos.x < size)
		{
			pushAwayFromBoundary(0, pos.y);
		}
		if (pos.x > 1 - size)
		{
			pushAwayFromBoundary(1, pos.y);
		}
		if (pos.y < size)
		{
			pushAwayFromBoundary(pos.x, 0);
		}
		if (pos.y > 1 - size)
		{
			pushAwayFromBoundary(pos.x, 1);
		}

		//Always move repulsive zones away, no matter their distance
		//TODO: Consider z plane?
		for (auto& connection : zone.second->getConnections())
		{
			if (connection.getConnectionType() == rmg::EConnectionType::REPULSIVE)
			{
				auto & otherZone = zones[connection.getOtherZoneId(zone.second->getId())];
				float3 otherZoneCenter = otherZone->getCenter();

				//TODO: Roll into lambda?
				auto distance = static_cast<float>(pos.dist2d(otherZoneCenter));
				float minDistance = (zone.second->getSize() + otherZone->getSize()) / mapSize;
				float3 localForce = (((otherZoneCenter - pos)*(minDistance / (distance ? distance : 1e-3f))) / getDistance(distance)) * stifness;
				localForce *= (distancesBetweenZones[zone.second->getId()][otherZone->getId()]);
				forceVector -= localForce * scaleForceBetweenZones(zone.second, otherZone);
			}
		}

		overlaps[zone.second] = overlap;
		forceVector.z = 0; //operator - doesn't preserve z coordinate :/
		forces[zone.second] = forceVector;
	}
}

CZonePlacer::ConnectivityCounts CZonePlacer::classifyConnections(const TZoneMap & zones, const std::map<std::shared_ptr<Zone>, float3> & solution) const
{
	//Predict, from geometry alone, how each connection would be realized. This is the signal
	//placement can actually influence; the real outcome is decided later by ConnectionsPlacer.
	ConnectivityCounts counts;
	std::set<int> seen;
	for(const auto & zonePair : zones)
	{
		for(const auto & connection : zonePair.second->getConnections())
		{
			switch(connection.getConnectionType())
			{
				//virtual connections never need a passage
				case rmg::EConnectionType::REPULSIVE:
				case rmg::EConnectionType::FORCE_PORTAL:
					continue;
			}
			if(connection.getZoneA() == connection.getZoneB())
				continue; //self-connection is an intended portal, not a failure
			if(!seen.insert(connection.getId()).second)
				continue; //count each undirected connection once

			auto otherIt = zones.find(connection.getOtherZoneId(zonePair.first));
			if(otherIt == zones.end())
				continue; //e.g. water zone, not part of tessellation

			auto selfPos = solution.find(zonePair.second);
			auto otherPos = solution.find(otherIt->second);
			if(selfPos == solution.end() || otherPos == solution.end())
				continue;

			float touching = (zonePair.second->getSize() + otherIt->second->getSize()) / mapSize;
			auto distance = static_cast<float>(selfPos->second.dist2d(otherPos->second));

			if(selfPos->second.z != otherPos->second.z)
			{
				//cross-level: a subterranean gate needs the footprints to overlap in XY
				if(distance <= touching)
					counts.gates++;
				else
					counts.monoliths++;
			}
			else if(distance <= touching * attractionReachScore)
				counts.direct++; //adjacent / overlapping -> a direct guarded passage is achievable
			else
				counts.monoliths++;
		}
	}
	return counts;
}

void CZonePlacer::moveOneZone(TZoneMap& zones, TForceVector& totalForces, TDistanceVector& distances, TDistanceVector& overlaps)
{
	//The more zones, the greater total distance expected
	//Also, higher stiffness make expected movement lower
	const int maxDistanceMovementRatio = zones.size() * zones.size() * (stiffnessConstant / stifness);

	typedef std::pair<float, std::shared_ptr<Zone>> Misplacement;
	std::vector<Misplacement> misplacedZones;

	float totalDistance = 0;
	float totalOverlap = 0;
	for (const auto& zone : distances) //find most misplaced zone
	{
		if (vstd::contains(lastSwappedZones, zone.first->getId()))
		{
			continue;
		}
		totalDistance += zone.second;
		float overlap = overlaps[zone.first];
		totalOverlap += overlap;
		//if distance to actual movement is long, the zone is misplaced
		float ratio = (zone.second + overlap) / static_cast<float>(totalForces[zone.first].mag());
		if (ratio > maxDistanceMovementRatio)
		{
			misplacedZones.emplace_back(std::make_pair(ratio, zone.first));
		}
	}

	if (misplacedZones.empty())
		return;

	std::ranges::sort(misplacedZones, [](const Misplacement& lhs, Misplacement& rhs)
	{
		return lhs.first > rhs.first; //Largest displacement first
	});

#ifdef ZONE_PLACEMENT_LOG
	logGlobal->trace("Worst misplacement/movement ratio: %3.2f", misplacedZones.front().first);
#endif

	if (misplacedZones.size() >= 2)
	{
		//Swap 2 misplaced zones

		auto firstZone = misplacedZones.front().second;
		std::shared_ptr<Zone> secondZone;
		std::set<TRmgTemplateZoneId> connectedZones;
		for (const auto& connection : firstZone->getConnections())
		{
			switch (connection.getConnectionType())
			{
				//Do not consider virtual connections for graph distance
				case rmg::EConnectionType::REPULSIVE:
				case rmg::EConnectionType::FORCE_PORTAL:
					continue;
			}
			if (connection.getZoneA() == connection.getZoneB())
			{
				//Do not consider self-connections
				continue;
			}
			connectedZones.insert(connection.getOtherZoneId(firstZone->getId()));
		}

		auto level = firstZone->getCenter().z;
		for (size_t i = 1; i < misplacedZones.size(); i++)
		{
			//Only swap zones on the same level
			//Don't swap zones that should be connected (Jebus)

			if (misplacedZones[i].second->getCenter().z == level &&
				!vstd::contains(connectedZones, misplacedZones[i].second->getId()))
			{
				secondZone = misplacedZones[i].second;
				break;
			}
		}
		if (secondZone)
		{
#ifdef ZONE_PLACEMENT_LOG
			logGlobal->trace("Swapping two misplaced zones %d and %d", firstZone->getId(), secondZone->getId());
#endif

			auto firstCenter = firstZone->getCenter();
			auto secondCenter = secondZone->getCenter();
			firstZone->setCenter(secondCenter);
			secondZone->setCenter(firstCenter);

			lastSwappedZones.insert(firstZone->getId());
			lastSwappedZones.insert(secondZone->getId());
			return;
		}
	}
	lastSwappedZones.clear(); //If we didn't swap zones in this iteration, we can do it in the next

	//find most distant zone that should be attracted and move inside it
	std::shared_ptr<Zone> targetZone;
	auto misplacedZone = misplacedZones.front().second;
	float3 ourCenter = misplacedZone->getCenter();
		
	if ((totalDistance / (bestTotalDistance + 1)) > (totalOverlap / (bestTotalOverlap + 1)))
	{
		//Move one zone towards most distant zone to reduce distance

		float maxDistance = 0;
		for (const auto & con : misplacedZone->getConnections())
		{
			if (con.getConnectionType() == rmg::EConnectionType::REPULSIVE)
			{
				continue;
			}

			auto otherZone = zones[con.getOtherZoneId(misplacedZone->getId())];
			float3 otherCenter = otherZone->getCenter();

			float distance = static_cast<float>(otherCenter.dist2dSQ(ourCenter));
			if (distance > maxDistance)
			{
				maxDistance = distance;
				targetZone = otherZone;
			}
		}
		if (targetZone)
		{
			float3 vec = targetZone->getCenter() - ourCenter;
			float newDistanceBetweenZones = (std::max(misplacedZone->getSize(), targetZone->getSize())) / mapSize;
#ifdef ZONE_PLACEMENT_LOG
			logGlobal->trace("Trying to move zone %d %s towards %d %s. Direction is %s", misplacedZone->getId(), ourCenter.toString(), targetZone->getId(), targetZone->getCenter().toString(), vec.toString());
#endif

			misplacedZone->setCenter(targetZone->getCenter() - vec.unitVector() * newDistanceBetweenZones); //zones should now overlap by half size
		}
	}
	else
	{
		//Move misplaced zone away from overlapping zone

		float maxOverlap = 0;
		for(const auto & otherZone : zones)
		{
			float3 otherZoneCenter = otherZone.second->getCenter();

			if (otherZone.second == misplacedZone || otherZoneCenter.z != ourCenter.z)
				continue;

			auto distance = static_cast<float>(otherZoneCenter.dist2dSQ(ourCenter));
			if (distance > maxOverlap)
			{
				maxOverlap = distance;
				targetZone = otherZone.second;
			}
		}
		if (targetZone)
		{
			float3 vec = ourCenter - targetZone->getCenter();
			float newDistanceBetweenZones = (misplacedZone->getSize() + targetZone->getSize()) / mapSize;
#ifdef ZONE_PLACEMENT_LOG
			logGlobal->trace("Trying to move zone %d %s away from %d %s. Direction is %s", misplacedZone->getId(), ourCenter.toString(), targetZone->getId(), targetZone->getCenter().toString(), vec.toString());
#endif

			misplacedZone->setCenter(targetZone->getCenter() + vec.unitVector() * newDistanceBetweenZones); //zones should now be just separated
		}
	}
	//Don't swap that zone in next iteration
	lastSwappedZones.insert(misplacedZone->getId());
}

float CZonePlacer::metric (const int3 &A, const int3 &B) const
{
	return A.dist2dSQ(B);

}

void CZonePlacer::assignZones(vstd::RNG * rand)
{
	logGlobal->info("Starting zone colouring");

	auto width = map.getMapGenOptions().getWidth();
	auto height = map.getMapGenOptions().getHeight();


	auto zones = map.getZones();
	vstd::erase_if(zones, [](const std::pair<TRmgTemplateZoneId, std::shared_ptr<Zone>> & pr)
	{
		return pr.second->getType() == ETemplateZoneType::WATER;
	});

	using Dpair = std::pair<std::shared_ptr<Zone>, float>;
	std::vector <Dpair> distances;
	distances.reserve(zones.size());

	//now place zones correctly and assign tiles to each zone

	auto compareByDistance = [](const Dpair & lhs, const Dpair & rhs) -> bool
	{
		//bigger zones have smaller distance
		return lhs.second / lhs.first->getSize() < rhs.second / rhs.first->getSize();
	};

	auto simpleCompareByDistance = [](const Dpair & lhs, const Dpair & rhs) -> bool
	{
		//bigger zones have smaller distance
		return lhs.second < rhs.second;
	};

	int levels = map.levels();

	// Find current center of mass for each zone. Move zone to that center to balance zones sizes
	std::vector<RmgMap::Zones> zonesOnLevel;
	for(int level = 0; level < levels; level++)
	{
		zonesOnLevel.push_back(map.getZonesOnLevel(level));
	}

	int3 pos;

	for(pos.z = 0; pos.z < levels; pos.z++)
	{
		for(pos.x = 0; pos.x < width; pos.x++)
		{
			for(pos.y = 0; pos.y < height; pos.y++)
			{
				distances.clear();
				for(const auto & zone : zonesOnLevel[pos.z])
				{
					distances.emplace_back(zone.second, static_cast<float>(pos.dist2dSQ(zone.second->getPos())));
				}
				std::ranges::min_element(distances, compareByDistance)->first->area()->add(pos); //closest tile belongs to zone
			}
		}
	}

	for(const auto & zone : zones)
	{
		if(zone.second->area()->empty())
			throw rmgException("Empty zone is generated, probably RMG template is inappropriate for map size");
		
		zone.second->moveToCenterOfMass();
	}

	for(const auto & zone : zones)
		zone.second->clearTiles(); //now populate them again

	PenroseTiling penrose;
	for (int level = 0; level < levels; level++)
	{
		//Create different tiling for each level

		auto vertices = penrose.generatePenroseTiling(zonesOnLevel[level].size(), rand);

		// Replay the orientation rolled in applyRandomOrientation onto the vertex field, so oriented zones
		// claim the rotated-equivalent vertices. Keeps the tessellation an exact isometry of the un-oriented
		// one, preserving cross-level overlaps that would otherwise be reshuffled into monoliths.
		if(orientRotation != 0 || orientFlipH || orientFlipV)
		{
			std::set<Point2D> oriented;
			for(const auto & v : vertices)
			{
				const auto [nx, ny] = orientNormalized(v.x(), v.y());
				oriented.insert(Point2D(nx, ny));
			}
			vertices = std::move(oriented);
		}

		if(capacityBalance)
		{
			std::vector<std::shared_ptr<Zone>> levelZones;
			levelZones.reserve(zonesOnLevel[level].size());
			for(const auto & zone : zonesOnLevel[level])
				levelZones.push_back(zone.second);
			assignTilesCapacityBalanced(level, width, height, levelZones, vertices);
		}
		else
		{
			// Assign zones to closest Penrose vertex
			std::map<std::shared_ptr<Zone>, std::set<int3>> vertexMapping;

			for (const auto & vertex : vertices)
			{
				distances.clear();
				for(const auto & zone : zonesOnLevel[level])
				{
					distances.emplace_back(zone.second, zone.second->getCenter().dist2dSQ(float3(vertex.x(), vertex.y(), level)));
				}
				auto closestZone = std::ranges::min_element(distances, compareByDistance)->first;

				vertexMapping[closestZone].insert(int3(vertex.x() * width, vertex.y() * height, level)); //Closest vertex belongs to zone
			}

			//Assign actual tiles to each zone
			pos.z = level;
			for (pos.x = 0; pos.x < width; pos.x++)
			{
				for (pos.y = 0; pos.y < height; pos.y++)
				{
					distances.clear();
					for(const auto & zoneVertex : vertexMapping)
					{
						auto zone = zoneVertex.first;
						for (const auto & vertex : zoneVertex.second)
						{
							distances.emplace_back(zone, metric(pos, vertex));
						}
					}

					//Tile closest to vertex belongs to zone
					auto closestZone = std::ranges::min_element(distances, simpleCompareByDistance)->first;
					closestZone->area()->add(pos);
					map.setZoneID(pos, closestZone->getId());
				}
			}
		}

		for(const auto & zone : zonesOnLevel[level])
		{
			if(zone.second->area()->empty())
			{
				// FIXME: Some vertices are duplicated, but it's not a source of problem
				logGlobal->error("Zone %d at %s is empty, dumping Penrose tiling", zone.second->getId(), zone.second->getCenter().toString());
				for (const auto & vertex : vertices)
				{
					logGlobal->warn("Penrose Vertex: %s", vertex.toString());
				}
				throw rmgException("Empty zone after Penrose tiling");
			}
		}
	}

	//set position (town position) to center of mass of irregular zone
	for(const auto & zone : zones)
	{
		zone.second->moveToCenterOfMass();

		//TODO: similar for islands
		#define	CREATE_FULL_UNDERGROUND true //consider linking this with water amount
		if (zone.second->isUnderground())
		{
			if (!CREATE_FULL_UNDERGROUND)
			{
				auto discardTiles = collectDistantTiles(*zone.second, zone.second->getSize() + 1.f);
				for(const auto & t : discardTiles)
					zone.second->area()->erase(t);
			}

			//make sure that terrain inside zone is not a rock

			auto v = zone.second->area()->getTilesVector();
			map.getMapProxy()->drawTerrain(*rand, v, ETerrainId::SUBTERRANEAN);
		}
	}
	logGlobal->info("Finished zone colouring");
}

void CZonePlacer::assignTilesCapacityBalanced(int level, int width, int height,
	const std::vector<std::shared_ptr<Zone>> & levelZones, const std::set<Point2D> & vertices) const
{
	const size_t numZones = levelZones.size();
	if(numZones == 0 || vertices.empty())
		return;

	// Penrose vertices in both tile and normalized [0,1] coordinates.
	std::vector<int3> vertexTile;
	std::vector<float3> vertexNorm;
	vertexTile.reserve(vertices.size());
	vertexNorm.reserve(vertices.size());
	for(const auto & v : vertices)
	{
		vertexTile.emplace_back(static_cast<si32>(v.x() * width), static_cast<si32>(v.y() * height), level);
		vertexNorm.emplace_back(v.x(), v.y(), static_cast<float>(level));
	}
	const size_t numVertices = vertexTile.size();

	// Each zone's target share of the level, proportional to size (size is a linear area weight:
	// a size-30 zone should claim twice the tiles of a size-15 one).
	std::vector<double> target(numZones, 0.0);
	double sumSize = 0;
	for(size_t z = 0; z < numZones; ++z)
	{
		const double s = static_cast<double>(levelZones[z]->getSize());
		target[z] = s;
		sumSize += s;
	}
	if(sumSize <= 0)
		return;
	for(auto & t : target)
		t /= sumSize;

	// Nearest Penrose vertex for every tile. Independent of weights, so precompute once; each vertex
	// then owns a fixed block of tiles and a zone's area is just the sum over the vertices it holds.
	std::vector<int> nearestVertex(static_cast<size_t>(width) * height, 0);
	std::vector<double> vertexTileCount(numVertices, 0.0);
	int3 pos(0, 0, level);
	for(pos.x = 0; pos.x < width; ++pos.x)
	{
		for(pos.y = 0; pos.y < height; ++pos.y)
		{
			float best = std::numeric_limits<float>::infinity();
			int bestVertex = 0;
			for(size_t v = 0; v < numVertices; ++v)
			{
				const float d = static_cast<float>(pos.dist2dSQ(vertexTile[v]));
				if(d < best)
				{
					best = d;
					bestVertex = static_cast<int>(v);
				}
			}
			nearestVertex[static_cast<size_t>(pos.x) * height + pos.y] = bestVertex;
			vertexTileCount[bestVertex] += 1.0;
		}
	}
	const double totalTiles = static_cast<double>(width) * height;

	// Precompute the normalized vertex<->center distances (constant across passes).
	std::vector<std::vector<double>> vertexZoneDist(numVertices, std::vector<double>(numZones, 0.0));
	for(size_t v = 0; v < numVertices; ++v)
		for(size_t z = 0; z < numZones; ++z)
			vertexZoneDist[v][z] = vertexNorm[v].dist2dSQ(levelZones[z]->getCenter());

	// Balance: assign every vertex to the zone with the smallest power distance (dist^2 - weight),
	// measure each zone's claimed area, and raise the weight of under-sized zones for the next pass.
	// The update oscillates near the optimum, so keep the assignment with the smallest total area error.
	std::vector<double> weight(numZones, 0.0);
	std::vector<int> vertexZone(numVertices, 0);
	std::vector<int> bestVertexZone(numVertices, 0);
	double bestError = std::numeric_limits<double>::infinity();
	for(int iter = 0; iter < capacityIterations; ++iter)
	{
		std::vector<double> area(numZones, 0.0);
		for(size_t v = 0; v < numVertices; ++v)
		{
			double best = std::numeric_limits<double>::infinity();
			int bestZone = 0;
			for(size_t z = 0; z < numZones; ++z)
			{
				const double d = vertexZoneDist[v][z] - weight[z];
				if(d < best)
				{
					best = d;
					bestZone = static_cast<int>(z);
				}
			}
			vertexZone[v] = bestZone;
			area[bestZone] += vertexTileCount[v];
		}

		double error = 0;
		bool allNonEmpty = true;
		for(size_t z = 0; z < numZones; ++z)
		{
			error += std::abs(target[z] - area[z] / totalTiles);
			if(area[z] <= 0)
				allNonEmpty = false;
		}
		if(allNonEmpty && error < bestError)
		{
			bestError = error;
			bestVertexZone = vertexZone;
		}
		for(size_t z = 0; z < numZones; ++z)
			weight[z] += capacityGain * (target[z] - area[z] / totalTiles);
	}

	// If no pass ever filled every zone, fall back to the last assignment; the guard below repairs gaps.
	if(bestError == std::numeric_limits<double>::infinity())
		bestVertexZone = vertexZone;
	vertexZone = bestVertexZone;

	// Safety: guarantee no zone is empty (the Penrose step throws otherwise). Give any empty zone the
	// tile-bearing vertex nearest its center, stealing it from whichever zone currently holds it.
	std::vector<double> ownedTiles(numZones, 0.0);
	for(size_t v = 0; v < numVertices; ++v)
		ownedTiles[vertexZone[v]] += vertexTileCount[v];
	for(size_t z = 0; z < numZones; ++z)
	{
		if(ownedTiles[z] > 0)
			continue;
		int chosen = -1;
		double bestDist = std::numeric_limits<double>::infinity();
		for(size_t v = 0; v < numVertices; ++v)
		{
			if(vertexTileCount[v] <= 0)
				continue;
			if(vertexZoneDist[v][z] < bestDist)
			{
				bestDist = vertexZoneDist[v][z];
				chosen = static_cast<int>(v);
			}
		}
		if(chosen >= 0)
			vertexZone[chosen] = static_cast<int>(z);
	}

	// Paint tiles using the final vertex->zone assignment.
	for(pos.x = 0; pos.x < width; ++pos.x)
	{
		for(pos.y = 0; pos.y < height; ++pos.y)
		{
			const auto & zone = levelZones[vertexZone[nearestVertex[static_cast<size_t>(pos.x) * height + pos.y]]];
			zone->area()->add(pos);
			map.setZoneID(pos, zone->getId());
		}
	}
}

void CZonePlacer::RemoveRoadsForWideConnections()
{
	auto zones = map.getZones();
	
	for(auto & zonePtr : zones)
	{
		for(auto & connection : zonePtr.second->getConnections())
		{
			if(connection.getConnectionType() == rmg::EConnectionType::WIDE)
			{
				zonePtr.second->setRoadOption(connection.getId(), rmg::ERoadOption::ROAD_FALSE);
			}
		}
	}
}



const TDistanceMap& CZonePlacer::getDistanceMap()
{
	return distancesBetweenZones;
}
