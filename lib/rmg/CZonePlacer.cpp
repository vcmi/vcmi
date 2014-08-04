
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

void CZonePlacer::placeZones(const CMapGenOptions * mapGenOptions, CRandomGenerator * rand)
{
	//gravity-based algorithm

	float gravityConstant = 1e-2;
	float zoneScale = 0.5f; //zones starts small and then inflate
	const float inflateModifier = 1.02;

	logGlobal->infoStream() << "Starting zone placement";

	int width = mapGenOptions->getWidth();
	int height = mapGenOptions->getHeight();

	auto zones = gen->getZones();
	bool underground = mapGenOptions->getHasTwoLevels();

	/*
		let's assume we try to fit N circular zones with radius = size on a map

		formula: sum((prescaler*n)^2)*pi = WH

		prescaler = sqrt((WH)/(sum(n^2)*pi))
	*/
	std::vector<std::pair<TRmgTemplateZoneId, CRmgTemplateZone*>> zonesVector (zones.begin(), zones.end());
	assert (zonesVector.size());
	
	RandomGeneratorUtil::randomShuffle(zonesVector, *rand);
	TRmgTemplateZoneId firstZone = zones.begin()->first; //we want lowest ID here
	bool undergroundFlag = false;

	float totalSize = 0;
	for (auto zone : zonesVector)
	{
		//even distribution for surface / underground zones. Surface zones always have priority.
		int level = 0;
		if (underground) //only then consider underground zones
		{
			if (zone.first == firstZone)
			{
				level = 0;
			}
			else
			{
				level = undergroundFlag;
				undergroundFlag = !undergroundFlag; //toggle underground on/off
			}
		}

		totalSize += (zone.second->getSize() * zone.second->getSize());
		zone.second->setCenter (float3(rand->nextDouble(0.2, 0.8), rand->nextDouble(0.2, 0.8), level)); //start away from borders
	}
	//prescale zones
	if (underground) //map is twice as big, so zones occupy only half of normal space
		totalSize /= 2;
	float prescaler = sqrt ((width * height) / (totalSize * 3.14f)); 
	float mapSize = sqrt (width * height);
	for (auto zone : zones)
	{
		zone.second->setSize (zone.second->getSize() * prescaler);
	}

	//gravity-based algorithm. connected zones attract, intersceting zones and map boundaries push back

	auto getDistance = [](float distance) -> float
	{
		return (distance ? distance * distance : 1e-6);
	};

	std::map <CRmgTemplateZone *, float3> forces;
	while (zoneScale < 1) //until zones reach their desired size and fill the map tightly
	{
		for (auto zone : zones)
		{
			float3 forceVector(0,0,0);
			float3 pos = zone.second->getCenter();

			//attract connected zones
			for (auto con : zone.second->getConnections())
			{
				auto otherZone = zones[con];
				float3 otherZoneCenter = otherZone->getCenter();
				float distance = pos.dist2d (otherZoneCenter);
				float minDistance = (zone.second->getSize() + otherZone->getSize())/mapSize * zoneScale; //scale down to (0,1) coordinates
				if (distance > minDistance)
				{
					//WARNING: compiler used to 'optimize' that line so it never actually worked
					forceVector += (((otherZoneCenter - pos) / getDistance(distance))); //positive value
				}
			}
			//separate overlaping zones
			for (auto otherZone : zones)
			{
				float3 otherZoneCenter = otherZone.second->getCenter();
				//zones on different levels don't push away
				if (zone == otherZone || pos.z != otherZoneCenter.z)
					continue;

				float distance = pos.dist2d (otherZoneCenter);
				float minDistance = (zone.second->getSize() + otherZone.second->getSize())/mapSize * zoneScale;
				if (distance < minDistance)
				{
					forceVector -= (otherZoneCenter - pos) / getDistance(distance); //negative value
				}
			}

			//move zones away from boundaries
			//do not scale boundary distance - zones tend to get squashed
			float size = zone.second->getSize() / mapSize;

			auto pushAwayFromBoundary = [&forceVector, pos, &getDistance](float x, float y)
			{
				float3 boundary = float3 (x, y, pos.z);
				float distance = pos.dist2d(boundary);
				forceVector -= (boundary - pos) / getDistance(distance); //negative value
			};
			if (pos.x < size)
			{
				pushAwayFromBoundary(0, pos.y);
			}
			if (pos.x > 1-size)
			{
				pushAwayFromBoundary(1, pos.y);
			}
			if (pos.y < size)
			{
				pushAwayFromBoundary(pos.x, 0);
			}
			if (pos.y > 1-size)
			{
				pushAwayFromBoundary(pos.x, 1);
			}

			forceVector.z = 0; //operator - doesn't preserve z coordinate :/
			forces[zone.second] = forceVector * gravityConstant;
		}
		//update positions
		for (auto zone : forces)
		{
			zone.first->setCenter (zone.first->getCenter() + zone.second);
		}
		zoneScale *= inflateModifier; //increase size
	}
	for (auto zone : zones) //finalize zone positions
	{
		zone.second->setPos(cords(zone.second->getCenter()));
		logGlobal->infoStream() << boost::format ("Placed zone %d at relative position %s and coordinates %s") % zone.first % zone.second->getCenter() % zone.second->getPos();
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
	logGlobal->infoStream()  << "Starting zone colouring";

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
					if (zone.second->getPos().z == k)
						distances.push_back (std::make_pair(zone.second, metric(pos, zone.second->getPos())));
					else
						distances.push_back (std::make_pair(zone.second, std::numeric_limits<float>::max()));
				}
				boost::sort (distances, compareByDistance);
				distances.front().first->addTile(pos); //closest tile belongs to zone
			}
		}
	}
	//set position to center of mass
	for (auto zone : zones)
	{
		int3 total(0,0,0);
		auto tiles = zone.second->getTileInfo();
		for (auto tile : tiles)
		{
			total += tile;
		}
		int size = tiles.size();
		assert (size);
		zone.second->setPos (int3(total.x/size, total.y/size, total.z/size));

		//TODO: similiar for islands
		if (zone.second->getPos().z)
		{
			zone.second->discardDistantTiles(gen, zone.second->getSize() + 1);

			//make sure that terrain inside zone is not a rock
			//FIXME: reorder actions?
			zone.second->paintZoneTerrain (gen, ETerrainType::SUBTERRANEAN);
		}
	}
	logGlobal->infoStream() << "Finished zone colouring";
}
