/*
 * RockFiller.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "RockFiller.h"
#include "RockPlacer.h"
#include "TreasurePlacer.h"
#include "ObjectManager.h"
#include "RiverPlacer.h"
#include "RoadPlacer.h"
#include "../RmgMap.h"
#include "../CMapGenerator.h"
#include "../Functions.h"
#include "../../TerrainHandler.h"
#include "../lib/mapping/CMapEditManager.h"
#include "../TileInfo.h"
#include "../MapProxy.h"

VCMI_LIB_NAMESPACE_BEGIN

class TileInfo;

void RockFiller::process()
{
	processMap();
}

void RockFiller::processMap()
{
	//Merge all areas
	for(auto & z : map.getZonesOnLevel(1))
	{
		auto zone = z.second;
		if(auto * m = zone->getModificator<RockPlacer>())
		{
			auto tiles = m->rockArea.getTilesVector();
			mapProxy->drawTerrain(zone->getRand(), tiles, m->rockTerrain);
		}
	}
	
	for(auto & z : map.getZonesOnLevel(1))
	{
		auto zone = z.second;
		if(auto * m = zone->getModificator<RockPlacer>())
		{
			//Now make sure all accessible tiles have no additional rock on them
			auto tiles = m->accessibleArea.getTilesVector();
			mapProxy->drawTerrain(zone->getRand(), tiles, zone->getTerrainType());

			m->postProcess();
		}

		// Draw roads after rock is placed
		if(auto * rp = zone->getModificator<RoadPlacer>())
		{
			rp->postProcess();
		}
	}
}

void RockFiller::init()
{
	DEPENDENCY_ALL(RockPlacer);
}

char RockFiller::dump(const int3 & t)
{
	if(!map.getTile(t).getTerrain()->isPassable())
	{
		return zone.area()->contains(t) ? 'R' : 'E';
	}
	return Modificator::dump(t);
}

VCMI_LIB_NAMESPACE_END
