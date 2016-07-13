
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
#include "../CRandomGenerator.h"
#include "CZonePlacer.h"
#include "CRmgTemplateZone.h"
#include "../mapping/CMap.h"

#include "CZoneGraphGenerator.h"

class CRandomGenerator;

CPlacedZone::CPlacedZone(const CRmgTemplateZone * zone)
{

}

CZonePlacer::CZonePlacer(CMapGenerator * Gen) : gen(Gen)
{

}

CZonePlacer::~CZonePlacer()
{

}

int3 CZonePlacer::cords (const float3 f) const
{
	return int3(std::max(0.f, (f.x * gen->map->width)-1), std::max(0.f, (f.y * gen->map->height-1)), f.z);
}

float CZonePlacer::getDistance (float distance) const
{
	return (distance ? distance * distance : 1e-6);
}

void CZonePlacer::placeZones(const CMapGenOptions * mapGenOptions, CRandomGenerator * rand)
{
	logGlobal->infoStream() << "Starting zone placement";

	width = mapGenOptions->getWidth();
	height = mapGenOptions->getHeight();

	auto zones = gen->getZones();
	bool underground = mapGenOptions->getHasTwoLevels();

	/*
	gravity-based algorithm

	let's assume we try to fit N circular zones with radius = size on a map
	*/

	gravityConstant = 4e-3;
	stiffnessConstant = 4e-3;

	TZoneVector zonesVector(zones.begin(), zones.end());
	assert (zonesVector.size());

	RandomGeneratorUtil::randomShuffle(zonesVector, *rand);

	//0. set zone sizes and surface / underground level
	prepareZones(zones, zonesVector, underground, rand);

	//gravity-based algorithm. connected zones attract, intersceting zones and map boundaries push back

	//remember best solution
	float bestTotalDistance = 1e10;
	float bestTotalOverlap = 1e10;

	std::map<CRmgTemplateZone *, float3> bestSolution;

	TForceVector forces;
	TForceVector totalForces; //  both attraction and pushback, overcomplicated?
	TDistanceVector distances;
	TDistanceVector overlaps;

	const int MAX_ITERATIONS = 100;
	for (int i = 0; i < MAX_ITERATIONS; ++i) //until zones reach their desired size and fill the map tightly
	{
		//1. attract connected zones
		attractConnectedZones(zones, forces, distances);
		for (auto zone : forces)
		{
			zone.first->setCenter (zone.first->getCenter() + zone.second);
			totalForces[zone.first] = zone.second; //override
		}

		//2. separate overlapping zones
		separateOverlappingZones(zones, forces, overlaps);
		for (auto zone : forces)
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
		for (auto zone : distances) //find most misplaced zone
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
			if (totalDistance + totalOverlap < bestTotalDistance + bestTotalOverlap)
				improvement = true;

		logGlobal->traceStream() << boost::format("Total distance between zones after this iteration: %2.4f, Total overlap: %2.4f, Improved: %s") % totalDistance % totalOverlap % improvement;

		//save best solution
		if (improvement)
		{
			bestTotalDistance = totalDistance;
			bestTotalOverlap = totalOverlap;

			for (auto zone : zones)
				bestSolution[zone.second] = zone.second->getCenter();
		}
	}

	logGlobal->traceStream() << boost::format("Best fitness reached: total distance %2.4f, total overlap %2.4f") % bestTotalDistance % bestTotalOverlap;
	for (auto zone : zones) //finalize zone positions
	{
		zone.second->setPos (cords (bestSolution[zone.second]));
		logGlobal->traceStream() << boost::format ("Placed zone %d at relative position %s and coordinates %s") % zone.first % zone.second->getCenter() % zone.second->getPos();
	}
}

void CZonePlacer::prepareZones(TZoneMap &zones, TZoneVector &zonesVector, const bool underground, CRandomGenerator * rand)
{	
	TRmgTemplateZoneId firstZone = zones.begin()->first; //we want lowest ID here

	std::vector<float> totalSize = { 0, 0 }; //make sure that sum of zone sizes on surface and uderground match size of the map

	const float radius = 0.4f;
	const float pi2 = 6.28f;

	int zonesOnLevel[2] = { 0, 0 };

	//even distribution for surface / underground zones. Surface zones always have priority.

	TZoneVector zonesToPlace;
	std::map<TRmgTemplateZoneId, int> levels;

	//first pass - determine fixed surface for zones
	for (auto zone : zonesVector)
	{
		//TODO: place players depending on their factions
		if (zone.first == firstZone)
		{
			zonesOnLevel[0]++;
			levels[zone.first] = 0;
		}
		else
		{
			zonesToPlace.push_back(zone);
		}
	}
	for (auto zone : zonesToPlace)
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
	for (auto zone : zonesVector)
	{
		int level = levels[zone.first];
		totalSize[level] += (zone.second->getSize() * zone.second->getSize());
		float randomAngle = rand->nextDouble(0, pi2);
		zone.second->setCenter(float3(0.5f + std::sin(randomAngle) * radius, 0.5f + std::cos(randomAngle) * radius, level)); //place zones around circle
	}

	/*
	prescale zones

	formula: sum((prescaler*n)^2)*pi = WH

	prescaler = sqrt((WH)/(sum(n^2)*pi))
	*/

	std::vector<float> prescaler = { 0, 0 };
	for (int i = 0; i < 2; i++)
		prescaler[i] = sqrt((width * height) / (totalSize[i] * 3.14f));
	mapSize = sqrt(width * height);
	for (auto zone : zones)
	{
		zone.second->setSize(zone.second->getSize() * prescaler[zone.second->getCenter().z]);
	}
}

void CZonePlacer::attractConnectedZones(TZoneMap &zones, TForceVector &forces, TDistanceVector &distances)
{
	for (auto zone : zones)
	{
		float3 forceVector(0, 0, 0);
		float3 pos = zone.second->getCenter();
		float totalDistance = 0;

		for (auto con : zone.second->getConnections())
		{
			auto otherZone = zones[con];
			float3 otherZoneCenter = otherZone->getCenter();
			float distance = pos.dist2d(otherZoneCenter);
			float minDistance = 0;

			if (pos.z != otherZoneCenter.z)
				minDistance = 0; //zones on different levels can overlap completely
			else
				minDistance = (zone.second->getSize() + otherZone->getSize()) / mapSize; //scale down to (0,1) coordinates

			if (distance > minDistance)
			{
				//WARNING: compiler used to 'optimize' that line so it never actually worked
				float overlapMultiplier = (pos.z == otherZoneCenter.z) ? (minDistance / distance) : 1.0f;
				forceVector += (((otherZoneCenter - pos)* overlapMultiplier / getDistance(distance))) * gravityConstant; //positive value
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
	for (auto zone : zones)
	{
		float3 forceVector(0, 0, 0);
		float3 pos = zone.second->getCenter();

		float overlap = 0;
		//separate overlaping zones
		for (auto otherZone : zones)
		{
			float3 otherZoneCenter = otherZone.second->getCenter();
			//zones on different levels don't push away
			if (zone == otherZone || pos.z != otherZoneCenter.z)
				continue;

			float distance = pos.dist2d(otherZoneCenter);
			float minDistance = (zone.second->getSize() + otherZone.second->getSize()) / mapSize;
			if (distance < minDistance)
			{
				forceVector -= (((otherZoneCenter - pos)*(minDistance / (distance ? distance : 1e-3))) / getDistance(distance)) * stiffnessConstant; //negative value
				overlap += (minDistance - distance); //overlapping of small zones hurts us more
			}
		}

		//move zones away from boundaries
		//do not scale boundary distance - zones tend to get squashed
		float size = zone.second->getSize() / mapSize;

		auto pushAwayFromBoundary = [&forceVector, pos, size, &overlap, this](float x, float y)
		{
			float3 boundary = float3(x, y, pos.z);
			float distance = pos.dist2d(boundary);
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

void CZonePlacer::moveOneZone(TZoneMap &zones, TForceVector &totalForces, TDistanceVector &distances, TDistanceVector &overlaps)
{
	float maxRatio = 0;
	const int maxDistanceMovementRatio = zones.size() * zones.size(); //experimental - the more zones, the greater total distance expected
	CRmgTemplateZone * misplacedZone = nullptr;

	float totalDistance = 0;
	float totalOverlap = 0;
	for (auto zone : distances) //find most misplaced zone
	{
		totalDistance += zone.second;
		float overlap = overlaps[zone.first];
		totalOverlap += overlap;
		float ratio = (zone.second + overlap) / totalForces[zone.first].mag(); //if distance to actual movement is long, the zone is misplaced
		if (ratio > maxRatio)
		{
			maxRatio = ratio;
			misplacedZone = zone.first;
		}
	}
	logGlobal->traceStream() << boost::format("Worst misplacement/movement ratio: %3.2f") % maxRatio;

	if (maxRatio > maxDistanceMovementRatio)
	{
		CRmgTemplateZone * targetZone = nullptr;
		float3 ourCenter = misplacedZone->getCenter();

		if (totalDistance > totalOverlap)
		{
			//find most distant zone that should be attracted and move inside it
			float maxDistance = 0;
			for (auto con : misplacedZone->getConnections())
			{
				auto otherZone = zones[con];
				float distance = otherZone->getCenter().dist2dSQ(ourCenter);
				if (distance > maxDistance)
				{
					maxDistance = distance;
					targetZone = otherZone;
				}
			}
			float3 vec = targetZone->getCenter() - ourCenter;
			float newDistanceBetweenZones = (std::max(misplacedZone->getSize(), targetZone->getSize())) / mapSize;
			logGlobal->traceStream() << boost::format("Trying to move zone %d %s towards %d %s. Old distance %f") %
				misplacedZone->getId() % ourCenter() % targetZone->getId() % targetZone->getCenter()() % maxDistance;
			logGlobal->traceStream() << boost::format("direction is %s") % vec();

			misplacedZone->setCenter(targetZone->getCenter() - vec.unitVector() * newDistanceBetweenZones); //zones should now overlap by half size
			logGlobal->traceStream() << boost::format("New distance %f") % targetZone->getCenter().dist2d(misplacedZone->getCenter());
		}
		else
		{
			float maxOverlap = 0;
			for (auto otherZone : zones)
			{
				float3 otherZoneCenter = otherZone.second->getCenter();

				if (otherZone.second == misplacedZone || otherZoneCenter.z != ourCenter.z)
					continue;

				float distance = otherZoneCenter.dist2dSQ(ourCenter);
				if (distance > maxOverlap)
				{
					maxOverlap = distance;
					targetZone = otherZone.second;
				}
			}
			float3 vec = ourCenter - targetZone->getCenter();
			float newDistanceBetweenZones = (misplacedZone->getSize() + targetZone->getSize()) / mapSize;
			logGlobal->traceStream() << boost::format("Trying to move zone %d %s away from %d %s. Old distance %f") %
				misplacedZone->getId() % ourCenter() % targetZone->getId() % targetZone->getCenter()() % maxOverlap;
			logGlobal->traceStream() << boost::format("direction is %s") % vec();

			misplacedZone->setCenter(targetZone->getCenter() + vec.unitVector() * newDistanceBetweenZones); //zones should now be just separated
			logGlobal->traceStream() << boost::format("New distance %f") % targetZone->getCenter().dist2d(misplacedZone->getCenter());
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
	return dx * (1 + dx * (0.1 + dx * 0.01)) + dy * (1.618 + dy * (-0.1618 + dy * 0.01618));
}

void CZonePlacer::assignZones(const CMapGenOptions * mapGenOptions)
{
	logGlobal->infoStream() << "Starting zone colouring";

	auto width = mapGenOptions->getWidth();
	auto height = mapGenOptions->getHeight();

	//scale to Medium map to ensure smooth results
	scaleX = 72.f / width;
	scaleY = 72.f / height;

	auto zones = gen->getZones();

	typedef std::pair<CRmgTemplateZone *, float> Dpair;
	std::vector <Dpair> distances;
	distances.reserve(zones.size());

	//now place zones correctly and assign tiles to each zone

	auto compareByDistance = [](const Dpair & lhs, const Dpair & rhs) -> bool
	{
		//bigger zones have smaller distance
		return lhs.second / lhs.first->getSize() < rhs.second / rhs.first->getSize();
	};

	auto moveZoneToCenterOfMass = [](CRmgTemplateZone * zone) -> void
	{
		int3 total(0, 0, 0);
		auto tiles = zone->getTileInfo();
		for (auto tile : tiles)
		{
			total += tile;
		}
		int size = tiles.size();
		assert(size);
		zone->setPos(int3(total.x / size, total.y / size, total.z / size));
	};

	int levels = gen->map->twoLevel ? 2 : 1;

	/*
	1. Create Voronoi diagram
	2. find current center of mass for each zone. Move zone to that center to balance zones sizes
	*/

	for (int i = 0; i<width; i++)
	{
		for (int j = 0; j<height; j++)
		{
			for (int k = 0; k < levels; k++)
			{
				distances.clear();
				int3 pos(i, j, k);
				for (auto zone : zones)
				{
					if (zone.second->getPos().z == k)
						distances.push_back(std::make_pair(zone.second, pos.dist2dSQ(zone.second->getPos())));
					else
						distances.push_back(std::make_pair(zone.second, std::numeric_limits<float>::max()));
				}
				boost::min_element(distances, compareByDistance)->first->addTile(pos); //closest tile belongs to zone
			}
		}
	}

	for (auto zone : zones)
		moveZoneToCenterOfMass(zone.second);

	//assign actual tiles to each zone using nonlinear norm for fine edges

	for (auto zone : zones)
		zone.second->clearTiles(); //now populate them again

	for (int i=0; i<width; i++)
	{
		for(int j=0; j<height; j++)
		{
			for (int k = 0; k < levels; k++)
			{
				distances.clear();
				int3 pos(i, j, k);
				for (auto zone : zones)
				{
					if (zone.second->getPos().z == k)
						distances.push_back (std::make_pair(zone.second, metric(pos, zone.second->getPos())));
					else
						distances.push_back (std::make_pair(zone.second, std::numeric_limits<float>::max()));
				}
				boost::min_element(distances, compareByDistance)->first->addTile(pos); //closest tile belongs to zone
			}
		}
	}
	//set position (town position) to center of mass of irregular zone
	for (auto zone : zones)
	{
		moveZoneToCenterOfMass(zone.second);

		//TODO: similiar for islands
		#define	CREATE_FULL_UNDERGROUND true //consider linking this with water amount
		if (zone.second->getPos().z)
		{
			if (!CREATE_FULL_UNDERGROUND)
				zone.second->discardDistantTiles(gen, zone.second->getSize() + 1);

			//make sure that terrain inside zone is not a rock
			//FIXME: reorder actions?
			zone.second->paintZoneTerrain (gen, ETerrainType::SUBTERRANEAN);
		}
	}
	logGlobal->infoStream() << "Finished zone colouring";
}
