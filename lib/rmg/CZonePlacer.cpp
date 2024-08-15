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

#include "../TerrainHandler.h"
#include "../entities/faction/CFaction.h"
#include "../entities/faction/CTownHandler.h"
#include "../mapping/CMap.h"
#include "../mapping/CMapEditManager.h"
#include "../VCMI_Lib.h"
#include "CMapGenOptions.h"
#include "RmgMap.h"
#include "Zone.h"
#include "Functions.h"
#include "PenroseTiling.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

//#define ZONE_PLACEMENT_LOG true

CZonePlacer::CZonePlacer(RmgMap & map)
	: width(0), height(0), mapSize(0),
	gravityConstant(1e-3f),
	stiffnessConstant(3e-3f),
	stifness(0),
	stiffnessIncreaseFactor(1.03f),
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

void CZonePlacer::placeOnGrid(vstd::RNG* rand)
{
	auto zones = map.getZones();
	assert(zones.size());

	//Make sure there are at least as many grid fields as the number of zones
	size_t gridSize = std::ceil(std::sqrt(zones.size()));

	typedef boost::multi_array<std::shared_ptr<Zone>, 2> GridType;
	GridType grid(boost::extents[gridSize][gridSize]);

	TZoneVector zonesVector(zones.begin(), zones.end());

	//Place first zone

	auto firstZone = zonesVector[0].second;
	size_t x = 0;
	size_t y = 0;

	auto getRandomEdge = [rand, gridSize](size_t& x, size_t& y)
	{
		switch (rand->nextInt(0, 3) % 4)
		{
		case 0:
			x = 0;
			y = gridSize / 2;
			break;
		case 1:
			x = gridSize - 1;
			y = gridSize / 2;
			break;
		case 2:
			x = gridSize / 2;
			y = 0;
			break;
		case 3:
			x = gridSize / 2;
			y = gridSize - 1;
			break;
		}
	};

	switch (firstZone->getType())
	{
		case ETemplateZoneType::PLAYER_START:
		case ETemplateZoneType::CPU_START:
			if (firstZone->getConnectedZoneIds().size() > 2)
			{
				getRandomEdge(x, y);
			}
			else
			{
				//Random corner
				if (rand->nextInt(0, 1) == 1)
				{
					x = 0;
				}
				else
				{
					x = gridSize - 1;
				}
				if (rand->nextInt(0, 1) == 1)
				{
					y = 0;
				}
				else
				{
					y = gridSize - 1;
				}
			}
			break;
		case ETemplateZoneType::TREASURE:
			if (gridSize & 1) //odd
			{
				x = y = (gridSize / 2);
			}
			else
			{
				//One of 4 squares in the middle
				x = (gridSize / 2) - 1 + rand->nextInt(0, 1);
				y = (gridSize / 2) - 1 + rand->nextInt(0, 1);
			}
			break;
		case ETemplateZoneType::JUNCTION:
			getRandomEdge(x, y);
			break;
	}
	grid[x][y] = firstZone;

	//Ignore z placement for simplicity

	for (size_t i = 1; i < zones.size(); i++)
	{
		auto zone = zonesVector[i].second;
		auto connectedZoneIds = zone->getConnectedZoneIds();

		float maxDistance = -1000.0;
		int3 mostDistantPlace;

		//Iterate over free positions
		for (size_t freeX = 0; freeX < gridSize; ++freeX)
		{
			for (size_t freeY = 0; freeY < gridSize; ++freeY)
			{
				if (!grid[freeX][freeY])
				{
					//There is free space left here
					int3 potentialPos(freeX, freeY, 0);
					
					//Compute distance to every existing zone

					float distance = 0;
					for (size_t existingX = 0; existingX < gridSize; ++existingX)
					{
						for (size_t existingY = 0; existingY < gridSize; ++existingY)
						{
							auto existingZone = grid[existingX][existingY];
							if (existingZone)
							{
								//There is already zone here
								float localDistance = 0.0f;

								auto graphDistance = distancesBetweenZones[zone->getId()][existingZone->getId()];
								if (graphDistance > 1)
								{
									//No direct connection
									localDistance = potentialPos.dist2d(int3(existingX, existingY, 0)) * graphDistance;
								}
								else
								{
									//Has direct connection - place as close as possible
									localDistance = -potentialPos.dist2d(int3(existingX, existingY, 0));
								}

								localDistance *= scaleForceBetweenZones(zone, existingZone);

								distance += localDistance;
							}
						}
					}
					if (distance > maxDistance)
					{
						maxDistance = distance;
						mostDistantPlace = potentialPos;
					}
				}
			}
		}

		//Place in a free slot
		grid[mostDistantPlace.x][mostDistantPlace.y] = zone;
	}

	//TODO: toggle with a flag
#ifdef ZONE_PLACEMENT_LOG
	logGlobal->trace("Initial zone grid:");
	for (size_t x = 0; x < gridSize; ++x)
	{
		std::string s;
		for (size_t y = 0; y < gridSize; ++y)
		{
			if (grid[x][y])
			{
				s += (boost::format("%3d ") % grid[x][y]->getId()).str();
			}
			else
			{
				s += " -- ";
			}
		}
		logGlobal->trace(s);
	}
#endif

	//Set initial position for zones - random position in square centered around (x, y)
	for (size_t x = 0; x < gridSize; ++x)
	{
		for (size_t y = 0; y < gridSize; ++y)
		{
			auto zone = grid[x][y];
			if (zone)
			{
				//i.e. for grid size 5 we get range (0.25 - 4.75)
				auto targetX = rand->nextDouble(x + 0.25f, x + 0.75f);
				vstd::abetween(targetX, 0.5, gridSize - 0.5);
				auto targetY = rand->nextDouble(y + 0.25f, y + 0.75f);
				vstd::abetween(targetY, 0.5, gridSize - 0.5);

				zone->setCenter(float3(targetX / gridSize, targetY / gridSize, zone->getPos().z));
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
	vstd::erase_if(zones, [](const std::pair<TRmgTemplateZoneId, std::shared_ptr<Zone>> & pr)
	{
		return pr.second->getType() == ETemplateZoneType::WATER;
	});
	bool underground = map.getMapGenOptions().getHasTwoLevels();

	findPathsBetweenZones();
	placeOnGrid(rand);

	/*
	Fruchterman-Reingold algorithm

	Let's assume we try to fit N circular zones with radius = size on a map
	Connected zones attract, intersecting zones and map boundaries push back
	*/

	TZoneVector zonesVector(zones.begin(), zones.end());
	assert (zonesVector.size());

	RandomGeneratorUtil::randomShuffle(zonesVector, *rand);

	//0. set zone sizes and surface / underground level
	prepareZones(zones, zonesVector, underground, rand);

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

	logGlobal->trace("Best fitness reached: total distance %2.4f, total overlap %2.4f", bestTotalDistance, bestTotalOverlap);
	for(const auto & zone : zones) //finalize zone positions
	{
		zone.second->setPos (cords (bestSolution[zone.second]));
#ifdef ZONE_PLACEMENT_LOG
		logGlobal->trace("Placed zone %d at relative position %s and coordinates %s", zone.first, zone.second->getCenter().toString(), zone.second->getPos().toString());
#endif
	}
}

void CZonePlacer::prepareZones(TZoneMap &zones, TZoneVector &zonesVector, const bool underground, vstd::RNG * rand)
{
	std::vector<float> totalSize = { 0, 0 }; //make sure that sum of zone sizes on surface and uderground match size of the map

	int zonesOnLevel[2] = { 0, 0 };

	//even distribution for surface / underground zones. Surface zones always have priority.

	TZoneVector zonesToPlace;
	std::map<TRmgTemplateZoneId, int> levels;

	//first pass - determine fixed surface for zones
	for(const auto & zone : zonesVector)
	{
		if (!underground) //this step is ignored
			zonesToPlace.push_back(zone);
		else //place players depending on their factions
		{
			if(std::optional<int> owner = zone.second->getOwner())
			{
				auto player = PlayerColor(*owner - 1);
				auto playerSettings = map.getMapGenOptions().getPlayersSettings();
				FactionID faction = FactionID::RANDOM;
				if (playerSettings.size() > player)
				{
					faction = std::next(playerSettings.begin(), player)->second.getStartingTown();
				}
				else
				{
					logGlobal->trace("Player %d (starting zone %d) does not participate in game", player.getNum(), zone.first);
				}

				if (faction == FactionID::RANDOM) //TODO: check this after a town has already been randomized
					zonesToPlace.push_back(zone);
				else
				{
					auto & tt = (*VLC->townh)[faction]->nativeTerrain;
					if(tt == ETerrainId::NONE)
					{
						//any / random
						zonesToPlace.push_back(zone);
					}
					else
					{
						const auto & terrainType = VLC->terrainTypeHandler->getById(tt);
						if(terrainType->isUnderground() && !terrainType->isSurface())
						{
							//underground only
							zonesOnLevel[1]++;
							levels[zone.first] = 1;
						}
						else
						{
							//surface
							zonesOnLevel[0]++;
							levels[zone.first] = 0;
						}
					}
				}
			}
			else //no starting zone or no underground altogether
			{
				zonesToPlace.push_back(zone);
			}
		}
	}
	for(const auto & zone : zonesToPlace)
	{
		if (underground) //only then consider underground zones
		{
			int level = 0;
			if (zonesOnLevel[1] < zonesOnLevel[0]) //only if there are less underground zones
				level = 1;
			else
				level = 0;

			levels[zone.first] = level;
			zonesOnLevel[level]++;
		}
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

	std::vector<float> prescaler = { 0, 0 };
	for (int i = 0; i < 2; i++)
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
			
			forceVector += (otherZoneCenter - pos) * distance * gravityConstant * scaleForceBetweenZones(zone.second, otherZone); //positive value

			//Attract zone centers always

			float minDistance = 0;

			if (pos.z != otherZoneCenter.z)
				minDistance = 0; //zones on different levels can overlap completely
			else
				minDistance = (zone.second->getSize() + otherZone->getSize()) / mapSize; //scale down to (0,1) coordinates

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

	boost::sort(misplacedZones, [](const Misplacement& lhs, Misplacement& rhs)
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
		for (auto con : misplacedZone->getConnections())
		{
			if (con.getConnectionType() == rmg::EConnectionType::REPULSIVE)
			{
				continue;
			}

			auto otherZone = zones[con.getOtherZoneId(misplacedZone->getId())];
			float distance = static_cast<float>(otherZone->getCenter().dist2dSQ(ourCenter));
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

	auto moveZoneToCenterOfMass = [width, height](const std::shared_ptr<Zone> & zone) -> void
	{
		int3 total(0, 0, 0);
		auto tiles = zone->area()->getTiles();
		for(const auto & tile : tiles)
		{
			total += tile;
		}
		int size = static_cast<int>(tiles.size());
		assert(size);
		auto newPos = int3(total.x / size, total.y / size, total.z / size);
		zone->setPos(newPos);
		zone->setCenter(float3(float(newPos.x) / width, float(newPos.y) / height, newPos.z));
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
				boost::min_element(distances, compareByDistance)->first->area()->add(pos); //closest tile belongs to zone
			}
		}
	}

	for(const auto & zone : zones)
	{
		if(zone.second->area()->empty())
			throw rmgException("Empty zone is generated, probably RMG template is inappropriate for map size");
		
		moveZoneToCenterOfMass(zone.second);
	}

	for(const auto & zone : zones)
		zone.second->clearTiles(); //now populate them again

	PenroseTiling penrose;
	for (int level = 0; level < levels; level++)
	{
		//Create different tiling for each level

		auto vertices = penrose.generatePenroseTiling(zonesOnLevel[level].size(), rand);

		// Assign zones to closest Penrose vertex
		std::map<std::shared_ptr<Zone>, std::set<int3>> vertexMapping;

		for (const auto & vertex : vertices)
		{
			distances.clear();
			for(const auto & zone : zonesOnLevel[level])
			{
				distances.emplace_back(zone.second, zone.second->getCenter().dist2dSQ(float3(vertex.x(), vertex.y(), level)));
			}
			auto closestZone = boost::min_element(distances, compareByDistance)->first;

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
				auto closestZone = boost::min_element(distances, simpleCompareByDistance)->first;
				closestZone->area()->add(pos);
				map.setZoneID(pos, closestZone->getId());
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
		moveZoneToCenterOfMass(zone.second);

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

const TDistanceMap& CZonePlacer::getDistanceMap()
{
	return distancesBetweenZones;
}

VCMI_LIB_NAMESPACE_END
