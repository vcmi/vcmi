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
#include <stack>
#include "../CRandomGenerator.h"
#include "CZonePlacer.h"
#include "../TerrainHandler.h"
#include "../mapping/CMap.h"
#include "../mapping/CMapEditManager.h"
#include "CMapGenOptions.h"
#include "RmgMap.h"
#include "Zone.h"
#include "Functions.h"

VCMI_LIB_NAMESPACE_BEGIN

class CRandomGenerator;

CZonePlacer::CZonePlacer(RmgMap & map)
	: width(0), height(0), scaleX(0), scaleY(0), mapSize(0), gravityConstant(0), stiffnessConstant(0),
	map(map)
{

}

int3 CZonePlacer::cords(const float3 & f) const
{
	return int3(static_cast<si32>(std::max(0.f, (f.x * map.map().width) - 1)), static_cast<si32>(std::max(0.f, (f.y * map.map().height - 1))), f.z);
}

float CZonePlacer::getDistance (float distance) const
{
	return (distance ? distance * distance : 1e-6f);
}

void CZonePlacer::findPathsBetweenZones()
{
	typedef std::pair<int, int> ConnectionIndex;

	auto zones = map.getZones();

	std::set<std::shared_ptr<Zone>> zonesToCheck;

	//Initialize direct connections
	for (auto zone : zones)
	{
		auto zoneId = zone.second->getId();
		for (auto connection : zone.second->getConnections())
		{
			if (!vstd::contains(distancesBetweenZones[zoneId], connection))
			{
				distancesBetweenZones[zoneId][connection] = 1;
				distancesBetweenZones[connection][zoneId] = 1;
			}
		}
	}

	for (auto startZone : zones)
	{
		size_t start = startZone.second->getId();

		for (auto endZone : zones)
		{
			size_t end = endZone.second->getId();
			
			if (start != end)
			{
				auto currentEnd = end;
				while (!vstd::contains(distancesBetweenZones[start], end))
				{
					size_t distance = 10; //Some large but not infinite number to not blow up the weights
					std::stack<int> nearbyZones;
					std::set<int> checkedZones;
					
					//FIXME: we may know the path from previous iterations, but can't be sure if it's optimal :?

					for (auto nearbyZone : startZone.second->getConnections())
					{
						nearbyZones.push(nearbyZone);
					}

					while (!nearbyZones.empty())
					{
						auto currentZone = nearbyZones.top();
						nearbyZones.pop();

						checkedZones.insert(currentZone);

						for (auto neighbourZone : distancesBetweenZones[currentZone])
						{
							if (neighbourZone.first == currentEnd)
							{
								//This zone has connection to our end zone

								if (!vstd::contains(distancesBetweenZones[currentZone], currentEnd))
								{
									//Initialize the connection of adjacent zones
									distancesBetweenZones[currentZone][currentEnd] = 1;
								}

								if ((distancesBetweenZones[currentZone][currentEnd] + 1) < distance)
								{
									//We found new, shorter path
									distance = distancesBetweenZones[currentZone][currentEnd] + 1;

									//Add just found connection
									distancesBetweenZones[start][currentEnd] = distance;
									//Connection is bidirectional
									distancesBetweenZones[currentEnd][start] = distance;

									//Unwind the stack, find the path between start previous-to-last zone
									currentEnd = currentZone;
								}
							}
							else
							{
								if (!vstd::contains(checkedZones, neighbourZone.first))
								{
									//We didn't check that zone yet
									nearbyZones.push(neighbourZone.first);
								}
							}
						}
					}
					//At the very least after this step we will find 1 more step connecting the two zones
				}
			}
		}
	}

	//Dump debug
	for (auto startZone : zones)
	{
		auto startId = startZone.second->getId();

		for (auto endZone : zones)
		{
			auto endId = endZone.second->getId();

			if (startId >= endId)
			{
				//Print only conections in one way
				continue;
			}
			logGlobal->info((boost::format("Distance between zone %2d and %2d: %d")
				% startId % endId % distancesBetweenZones[startId][endId]).str());
		}
	}
}

void CZonePlacer::placeOnGrid(CRandomGenerator* rand)
{
	auto zones = map.getZones();
	assert(zones.size());

	//TODO: determine all the distances between zones on a graph

	//Make sure there are at least as many grid fields as the number of zones
	size_t gridSize = std::ceil(std::sqrt(zones.size()));

	typedef boost::multi_array<std::shared_ptr<Zone>, 2> GridType;
	GridType grid(boost::extents[gridSize][gridSize]);

	TZoneVector zonesVector(zones.begin(), zones.end());
	RandomGeneratorUtil::randomShuffle(zonesVector, *rand);

	//Place first zone

	auto firstZone = zonesVector[0].second;
	size_t x = 0, y = 0;

	auto getRandomEdge = [rand, gridSize](size_t& x, size_t& y)
	{
		switch (rand->nextInt() % 4)
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
			if (firstZone->getConnections().size() > 2)
			{
				getRandomEdge(x, y);
			}
			else
			{
				//Random corner
				if (rand->nextInt() % 2)
				{
					x = 0;
				}
				else
				{
					x = gridSize - 1;
				}
				if (rand->nextInt() % 2)
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
			if (gridSize && 1) //odd
			{
				x = y = (gridSize / 2);
			}
			else
			{
				//One of 4 squares in the middle
				x = (gridSize / 2) - 1 + rand->nextInt() % 2;
				y = (gridSize / 2) - 1 + rand->nextInt() % 2;
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
		auto connections = zone->getConnections();

		float maxDistance = 0.0;
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
					for (size_t existingX = 0; existingX < gridSize; ++existingX)
					{
						for (size_t existingY = 0; existingY < gridSize; ++existingY)
						{
							float distance = 0.0;
							auto existingZone = grid[existingX][existingY];
							if (existingZone )
							{
								//There is already zone here

								if (distancesBetweenZones[zone->getId()][existingZone->getId()] > 1)
								{
									//No direct connection
									distance += potentialPos.dist2d(int3(existingX, existingY, 0));
									//TODO: Multiply by weight - the distance from A*
								}
								else
								{
									//Has direct connection
									distance -= (gridSize - 1);
								}

								//TODO: Multiply if zones belong to players, especially humans.
								//Starting zones should be as far away from eahc other as possible

								if (distance > maxDistance)
								{
									distance = maxDistance;
									mostDistantPlace = potentialPos;
								}
							}
						}
					}
				}
			}
		}

		//Place in a free slot
		grid[mostDistantPlace.x][mostDistantPlace.y] = zone;
	}

	//TODO: toggle with a flag
	logGlobal->info("Initial zone grid:");
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
		logGlobal->info(s);
	}

	//Set initial position for zones - random position in square centered around (x, y)
	for (size_t x = 0; x < gridSize; ++x)
	{
		for (size_t y = 0; y < gridSize; ++y)
		{
			auto zone = grid[x][y];
			if (zone)
			{
				auto targetX = rand->nextDouble(x - 0.5f, x + 0.5f);
				vstd::clamp(targetX, 0, gridSize);
				auto targetY = rand->nextDouble(y - 0.5f, y + 0.5f);
				vstd::clamp(targetY, 0, gridSize);

				zone->setCenter(float3(targetX / gridSize, targetY / gridSize, zone->getPos().z));
			}
		}
	}
}

void CZonePlacer::placeZones(CRandomGenerator * rand)
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
	gravity-based algorithm

	let's assume we try to fit N circular zones with radius = size on a map
	*/

	gravityConstant = 4e-3f;
	stiffnessConstant = 4e-3f;

	TZoneVector zonesVector(zones.begin(), zones.end());
	assert (zonesVector.size());

	RandomGeneratorUtil::randomShuffle(zonesVector, *rand);

	//0. set zone sizes and surface / underground level
	prepareZones(zones, zonesVector, underground, rand);

	//gravity-based algorithm. connected zones attract, intersecting zones and map boundaries push back

	//remember best solution
	float bestTotalDistance = 1e10;
	float bestTotalOverlap = 1e10;

	std::map<std::shared_ptr<Zone>, float3> bestSolution;

	TForceVector forces;
	TForceVector totalForces; //  both attraction and pushback, overcomplicated?
	TDistanceVector distances;
	TDistanceVector overlaps;

	const int MAX_ITERATIONS = 100;
	for (int i = 0; i < MAX_ITERATIONS; ++i) //until zones reach their desired size and fill the map tightly
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

		//3. now perform drastic movement of zone that is completely not linked

		moveOneZone(zones, totalForces, distances, overlaps);

		//4. NOW after everything was moved, re-evaluate zone positions
		attractConnectedZones(zones, forces, distances);
		separateOverlappingZones(zones, forces, overlaps);

		float totalDistance = 0;
		float totalOverlap = 0;
		for(const auto & zone : distances) //find most misplaced zone
		{
			totalDistance += zone.second;
			float overlap = overlaps[zone.first];
			totalOverlap += overlap;
		}

		//check fitness function
		bool improvement = false;
		if (bestTotalDistance > 0 && bestTotalOverlap > 0)
		{
			if (totalDistance * totalOverlap < bestTotalDistance * bestTotalOverlap) //multiplication is better for auto-scaling, but stops working if one factor is 0
				improvement = true;
		}
		else
		{
			if (totalDistance + totalOverlap < bestTotalDistance + bestTotalOverlap)
				improvement = true;
		}

		logGlobal->trace("Total distance between zones after this iteration: %2.4f, Total overlap: %2.4f, Improved: %s", totalDistance, totalOverlap , improvement);

		//save best solution
		if (improvement)
		{
			bestTotalDistance = totalDistance;
			bestTotalOverlap = totalOverlap;

			for(const auto & zone : zones)
				bestSolution[zone.second] = zone.second->getCenter();
		}
	}

	logGlobal->trace("Best fitness reached: total distance %2.4f, total overlap %2.4f", bestTotalDistance, bestTotalOverlap);
	for(const auto & zone : zones) //finalize zone positions
	{
		zone.second->setPos (cords (bestSolution[zone.second]));
		logGlobal->trace("Placed zone %d at relative position %s and coordinates %s", zone.first, zone.second->getCenter().toString(), zone.second->getPos().toString());
	}
}

void CZonePlacer::prepareZones(TZoneMap &zones, TZoneVector &zonesVector, const bool underground, CRandomGenerator * rand)
{
	std::vector<float> totalSize = { 0, 0 }; //make sure that sum of zone sizes on surface and uderground match size of the map

	const float radius = 0.4f;
	const float pi2 = 6.28f;

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
			if (boost::optional<int> owner = zone.second->getOwner())
			{
				auto player = PlayerColor(*owner - 1);
				auto playerSettings = map.getMapGenOptions().getPlayersSettings();
				si32 faction = CMapGenOptions::CPlayerSettings::RANDOM_TOWN;
				if (vstd::contains(playerSettings, player))
					faction = playerSettings[player].getStartingTown();
				else
					logGlobal->error("Can't find info for player %d (starting zone)", player.getNum());

				if (faction == CMapGenOptions::CPlayerSettings::RANDOM_TOWN) //TODO: check this after a town has already been randomized
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
		auto randomAngle = static_cast<float>(rand->nextDouble(0, pi2));
		zone.second->setCenter(float3(0.5f + std::sin(randomAngle) * radius, 0.5f + std::cos(randomAngle) * radius, level)); //place zones around circle
	}

	/*
	prescale zones

	formula: sum((prescaler*n)^2)*pi = WH

	prescaler = sqrt((WH)/(sum(n^2)*pi))
	*/

	std::vector<float> prescaler = { 0, 0 };
	for (int i = 0; i < 2; i++)
		prescaler[i] = std::sqrt((width * height) / (totalSize[i] * 3.14f));
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

		for (auto con : zone.second->getConnections())
		{
			auto otherZone = zones[con];
			float3 otherZoneCenter = otherZone->getCenter();
			auto distance = static_cast<float>(pos.dist2d(otherZoneCenter));
			float minDistance = 0;

			if (pos.z != otherZoneCenter.z)
				minDistance = 0; //zones on different levels can overlap completely
			else
				minDistance = (zone.second->getSize() + otherZone->getSize()) / mapSize; //scale down to (0,1) coordinates

			if (distance > minDistance)
			{
				//WARNING: compiler used to 'optimize' that line so it never actually worked
				float overlapMultiplier = (pos.z == otherZoneCenter.z) ? (minDistance / distance) : 1.0f;
				forceVector += ((otherZoneCenter - pos)* overlapMultiplier / getDistance(distance)) * gravityConstant; //positive value
				totalDistance += (distance - minDistance);
			}
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
				forceVector -= (((otherZoneCenter - pos)*(minDistance / (distance ? distance : 1e-3f))) / getDistance(distance)) * stiffnessConstant; //negative value
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
			forceVector -= (boundary - pos) * (size - distance) / this->getDistance(distance) * this->stiffnessConstant; //negative value
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
		overlaps[zone.second] = overlap;
		forceVector.z = 0; //operator - doesn't preserve z coordinate :/
		forces[zone.second] = forceVector;
	}
}

void CZonePlacer::moveOneZone(TZoneMap & zones, TForceVector & totalForces, TDistanceVector & distances, TDistanceVector & overlaps) const
{
	float maxRatio = 0;
	const int maxDistanceMovementRatio = static_cast<int>(zones.size() * zones.size()); //experimental - the more zones, the greater total distance expected
	std::shared_ptr<Zone> misplacedZone;

	float totalDistance = 0;
	float totalOverlap = 0;
	for(const auto & zone : distances) //find most misplaced zone
	{
		totalDistance += zone.second;
		float overlap = overlaps[zone.first];
		totalOverlap += overlap;
		float ratio = (zone.second + overlap) / static_cast<float>(totalForces[zone.first].mag()); //if distance to actual movement is long, the zone is misplaced
		if (ratio > maxRatio)
		{
			maxRatio = ratio;
			misplacedZone = zone.first;
		}
	}
	logGlobal->trace("Worst misplacement/movement ratio: %3.2f", maxRatio);

	if (maxRatio > maxDistanceMovementRatio && misplacedZone)
	{
		std::shared_ptr<Zone> targetZone;
		float3 ourCenter = misplacedZone->getCenter();

		if (totalDistance > totalOverlap)
		{
			//find most distant zone that should be attracted and move inside it
			float maxDistance = 0;
			for (auto con : misplacedZone->getConnections())
			{
				auto otherZone = zones[con];
				float distance = static_cast<float>(otherZone->getCenter().dist2dSQ(ourCenter));
				if (distance > maxDistance)
				{
					maxDistance = distance;
					targetZone = otherZone;
				}
			}
			if (targetZone) //TODO: consider refactoring duplicated code
			{
				float3 vec = targetZone->getCenter() - ourCenter;
				float newDistanceBetweenZones = (std::max(misplacedZone->getSize(), targetZone->getSize())) / mapSize;
				logGlobal->trace("Trying to move zone %d %s towards %d %s. Old distance %f", misplacedZone->getId(), ourCenter.toString(), targetZone->getId(), targetZone->getCenter().toString(), maxDistance);
				logGlobal->trace("direction is %s", vec.toString());

				misplacedZone->setCenter(targetZone->getCenter() - vec.unitVector() * newDistanceBetweenZones); //zones should now overlap by half size
				logGlobal->trace("New distance %f", targetZone->getCenter().dist2d(misplacedZone->getCenter()));
			}
		}
		else
		{
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
				logGlobal->trace("Trying to move zone %d %s away from %d %s. Old distance %f", misplacedZone->getId(), ourCenter.toString(), targetZone->getId(), targetZone->getCenter().toString(), maxOverlap);
				logGlobal->trace("direction is %s", vec.toString());

				misplacedZone->setCenter(targetZone->getCenter() + vec.unitVector() * newDistanceBetweenZones); //zones should now be just separated
				logGlobal->trace("New distance %f", targetZone->getCenter().dist2d(misplacedZone->getCenter()));
			}
		}
	}
}

float CZonePlacer::metric (const int3 &A, const int3 &B) const
{
/*

Matlab code

	dx = abs(A(1) - B(1)); %distance must be symmetric
	dy = abs(A(2) - B(2));

d = 0.01 * dx^3 - 0.1618 * dx^2 + 1 * dx + ...
    0.01618 * dy^3 + 0.1 * dy^2 + 0.168 * dy;
*/

	float dx = abs(A.x - B.x) * scaleX;
	float dy = abs(A.y - B.y) * scaleY;

	//Horner scheme
	return dx * (1.0f + dx * (0.1f + dx * 0.01f)) + dy * (1.618f + dy * (-0.1618f + dy * 0.01618f));
}

void CZonePlacer::assignZones(CRandomGenerator * rand)
{
	logGlobal->info("Starting zone colouring");

	auto width = map.getMapGenOptions().getWidth();
	auto height = map.getMapGenOptions().getHeight();

	//scale to Medium map to ensure smooth results
	scaleX = 72.f / width;
	scaleY = 72.f / height;

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

	auto moveZoneToCenterOfMass = [](const std::shared_ptr<Zone> & zone) -> void
	{
		int3 total(0, 0, 0);
		auto tiles = zone->area().getTiles();
		for(const auto & tile : tiles)
		{
			total += tile;
		}
		int size = static_cast<int>(tiles.size());
		assert(size);
		zone->setPos(int3(total.x / size, total.y / size, total.z / size));
	};

	int levels = map.map().levels();

	/*
	1. Create Voronoi diagram
	2. find current center of mass for each zone. Move zone to that center to balance zones sizes
	*/

	int3 pos;
	for(pos.z = 0; pos.z < levels; pos.z++)
	{
		for(pos.x = 0; pos.x < width; pos.x++)
		{
			for(pos.y = 0; pos.y < height; pos.y++)
			{
				distances.clear();
				for(const auto & zone : zones)
				{
					if (zone.second->getPos().z == pos.z)
						distances.emplace_back(zone.second, static_cast<float>(pos.dist2dSQ(zone.second->getPos())));
					else
						distances.emplace_back(zone.second, std::numeric_limits<float>::max());
				}
				boost::min_element(distances, compareByDistance)->first->area().add(pos); //closest tile belongs to zone
			}
		}
	}

	for(const auto & zone : zones)
	{
		if(zone.second->area().empty())
			throw rmgException("Empty zone is generated, probably RMG template is inappropriate for map size");
		
		moveZoneToCenterOfMass(zone.second);
	}

	//assign actual tiles to each zone using nonlinear norm for fine edges

	for(const auto & zone : zones)
		zone.second->clearTiles(); //now populate them again

	for (pos.z = 0; pos.z < levels; pos.z++)
	{
		for (pos.x = 0; pos.x < width; pos.x++)
		{
			for (pos.y = 0; pos.y < height; pos.y++)
			{
				distances.clear();
				for(const auto & zone : zones)
				{
					if (zone.second->getPos().z == pos.z)
						distances.emplace_back(zone.second, metric(pos, zone.second->getPos()));
					else
						distances.emplace_back(zone.second, std::numeric_limits<float>::max());
				}
				auto zone = boost::min_element(distances, compareByDistance)->first; //closest tile belongs to zone
				zone->area().add(pos);
				map.setZoneID(pos, zone->getId());
			}
		}
	}
	//set position (town position) to center of mass of irregular zone
	for(const auto & zone : zones)
	{
		moveZoneToCenterOfMass(zone.second);

		//TODO: similiar for islands
		#define	CREATE_FULL_UNDERGROUND true //consider linking this with water amount
		if (zone.second->isUnderground())
		{
			if (!CREATE_FULL_UNDERGROUND)
			{
				auto discardTiles = collectDistantTiles(*zone.second, zone.second->getSize() + 1.f);
				for(const auto & t : discardTiles)
					zone.second->area().erase(t);
			}

			//make sure that terrain inside zone is not a rock
			//FIXME: reorder actions?
			paintZoneTerrain(*zone.second, *rand, map, ETerrainId::SUBTERRANEAN);
		}
	}
	logGlobal->info("Finished zone colouring");
}

VCMI_LIB_NAMESPACE_END
