
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

#include "CZoneGraphGenerator.h"

class CRandomGenerator;

CPlacedZone::CPlacedZone(const CRmgTemplateZone * zone) : zone(zone)
{

}

CZonePlacer::CZonePlacer(CMapGenerator * Gen) : gen(Gen)
{

}

CZonePlacer::~CZonePlacer()
{

}

int3 CZonePlacer::cords (float3 f) const
{
	return int3(f.x * gen->map->width, f.y * gen->map->height, f.z);
}

void CZonePlacer::placeZones(shared_ptr<CMapGenOptions> mapGenOptions, CRandomGenerator * rand)
{
	//some relaxation-simmulated annealing algorithm

	const int iterations = 5;
	float temperature = 1;
	const float temperatureModifier = 0.9;

	logGlobal->infoStream() << "Starting zone placement";

	int width = mapGenOptions->getWidth();
	int height = mapGenOptions->getHeight();

	auto zones = gen->getZones();

	//TODO: consider underground zones

	float totalSize = 0;
	for (auto zone : zones)
	{
		totalSize += zone.second->getSize();
		zone.second->setCenter (float3(rand->nextDouble(0,1), rand->nextDouble(0,1), 0));
	}
	//prescale zones
	float prescaler = sqrt (width * height / totalSize) / 3.14f; //let's assume we try to fit N circular zones with radius = size on a map
	float mapSize = sqrt (width * height);
	for (auto zone : zones)
	{
		zone.second->setSize (zone.second->getSize() * prescaler);
	}

	for (int i = 0; i < iterations; ++i)
	{
		for (auto zone : zones)
		{
			//attract connected zones
			for (auto con : zone.second->getConnections())
			{
				auto otherZone = zones[con];
				float distance = zone.second->getCenter().dist2d (otherZone->getCenter());
				float minDistance = (zone.second->getSize() + otherZone->getSize())/mapSize; //scale down to (0,1) coordinates
				if (distance > minDistance)
				{
					//attract our zone
					float scaler = (distance - minDistance)/distance * temperature; //positive
					auto positionVector = (otherZone->getCenter() - zone.second->getCenter()); //positive value
					zone.second->setCenter (zone.second->getCenter() + positionVector * scaler); //positive movement
				}
			}
		}
		for (auto zone : zones)
		{
			//separate overlaping zones
			for (auto otherZone : zones)
			{
				if (zone == otherZone)
					continue;

				float distance = zone.second->getCenter().dist2d (otherZone.second->getCenter());
				float minDistance = (zone.second->getSize() + otherZone.second->getSize())/mapSize;
				if (distance < minDistance)
				{
					//move our zone away
					float scaler = (distance ? (distance - minDistance)/distance : 1) * temperature; //negative
					auto positionVector = (otherZone.second->getCenter() - zone.second->getCenter()); //positive value
					zone.second->setCenter (zone.second->getCenter() + positionVector * scaler); //negative movement
				}
			}
		}
		temperature *= temperatureModifier;
	}
	for (auto zone : zones) //finalize zone positions
	{
		zone.second->setPos(cords(zone.second->getCenter()));
		logGlobal->infoStream() << boost::format ("Placed zone %d at relative position %s and coordinates %s") % zone.first % zone.second->getCenter() % zone.second->getPos();
	}
}

float CZonePlacer::metric (int3 &A, int3 &B) const
{
/*

Matlab code

	dx = abs(A(1) - B(1)); %distance must be symmetric
	dy = abs(A(2) - B(2));

	d = 0.01 * dx^3 + 0.1 * dx^2 + 1 * dx + ...
    0.03 * dy^3 - 0.3 * dy^2 + 0.3 * dy;
*/

	float dx = abs(A.x - B.x) * scaleX;
	float dy = abs(A.y - B.y) * scaleY;

	//Horner scheme
	return dx * (1 + dx * (0.1 + dx * 0.01)) + dy * (0.3 + dy * (-0.3 + dy * 0.03));
}

void CZonePlacer::assignZones(shared_ptr<CMapGenOptions> mapGenOptions)
{
	auto width = mapGenOptions->getWidth();
	auto height = mapGenOptions->getHeight();

	//scale to Medium map to ensure smooth results
	scaleX = 72.f / width;
	scaleY = 72.f / height;

	auto zones = gen->getZones();

	typedef std::pair<CRmgTemplateZone *, float> Dpair;
	std::vector <Dpair> distances;
	distances.reserve(zones.size());

	auto compareByDistance = [](const Dpair & lhs, const Dpair & rhs) -> bool
	{
		return lhs.second < rhs.second;
	};

	int levels = gen->map->twoLevel ? 2 : 1;
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
					distances.push_back (std::make_pair(zone.second, metric(pos, zone.second->getPos())));
				}
				boost::sort (distances, compareByDistance);
				distances.front().first->addTile(pos); //closest tile belongs to zone
			}
		}
	}
}
