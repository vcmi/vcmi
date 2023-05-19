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
#include "RoadPlacer.h"
#include "RiverPlacer.h"
#include "RmgMap.h"
#include "CMapGenerator.h"
#include "Functions.h"
#include "../TerrainHandler.h"
#include "../CRandomGenerator.h"
#include "../mapping/CMapEditManager.h"
#include "TileInfo.h"
#include "threadpool/MapProxy.h"

VCMI_LIB_NAMESPACE_BEGIN

class TileInfo;

void RockFiller::process()
{
    //Do that only once
	auto lockVec = tryLockAll<RockFiller>();
	if (!lockVec.empty())
	{
		for(auto & z : map.getZones())
		{
			if(auto * m = z.second->getModificator<RockFiller>())
			{
				if(m->isFinished())
					return;
			}
		}
		logGlobal->info("Processing RockFiller for the whole map");
		processMap();
		finished = true; //Block other placers immediately
	}
}

void RockFiller::processMap()
{
	//Merge all areas
	for(auto & z : map.getZones())
	{
		if(auto * m = z.second->getModificator<RockPlacer>())
		{
			auto tiles = m->rockArea.getTilesVector();
			mapProxy->drawTerrain(generator.rand, tiles, m->rockTerrain);
		}
	}
	
	for(auto & z : map.getZones())
	{
		if(auto * m = z.second->getModificator<RockPlacer>())
		{
			//Now make sure all accessible tiles have no additional rock on them
			auto tiles = m->accessibleArea.getTilesVector();
			mapProxy->drawTerrain(generator.rand, tiles, z.second->getTerrainType());

			m->postProcess();
		}
	}
}

void RockFiller::init()
{
    DEPENDENCY_ALL(RockPlacer);
	POSTFUNCTION_ALL(RoadPlacer);
}

char RockFiller::dump(const int3 & t)
{
	if(!map.getTile(t).terType->isPassable())
	{
		return zone.area().contains(t) ? 'R' : 'E';
	}
	return Modificator::dump(t);
}

VCMI_LIB_NAMESPACE_END
