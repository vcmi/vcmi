/*
 * RockPlacer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "RockPlacer.h"
#include "TreasurePlacer.h"
#include "ObjectManager.h"
#include "RoadPlacer.h"
#include "RiverPlacer.h"
#include "../RmgMap.h"
#include "../CMapGenerator.h"
#include "../Functions.h"
#include "../../TerrainHandler.h"
#include "../../CRandomGenerator.h"
#include "../../mapping/CMapEditManager.h"
#include "../TileInfo.h"

VCMI_LIB_NAMESPACE_BEGIN

class TileInfo;

void RockPlacer::process()
{
	blockRock();
}
void RockPlacer::blockRock()
{
	rockTerrain = VLC->terrainTypeHandler->getById(zone.getTerrainType())->rockTerrain;
	assert(!VLC->terrainTypeHandler->getById(rockTerrain)->isPassable());

	accessibleArea = zone.freePaths() + zone.areaUsed();
	if(auto * m = zone.getModificator<ObjectManager>())
		accessibleArea.unite(m->getVisitableArea());

	//negative approach - create rock tiles first, then make sure all accessible tiles have no rock
	rockArea = zone.area().getSubarea([this](const int3 & t)
	{
		return map.shouldBeBlocked(t);
	});
}

void RockPlacer::postProcess()
{
	Zone::Lock lock(zone.areaMutex);
	//Finally mark rock tiles as occupied, spawn no obstacles there
	rockArea = zone.area().getSubarea([this](const int3 & t)
	{
		return !map.getTile(t).terType->isPassable();
	});
	
	zone.areaUsed().unite(rockArea);
	zone.areaPossible().subtract(rockArea);

	//TODO: Might need mutex here as well
	if(auto * m = zone.getModificator<RiverPlacer>())
		m->riverProhibit().unite(rockArea);
	if(auto * m = zone.getModificator<RoadPlacer>())
		m->areaIsolated().unite(rockArea);
}

void RockPlacer::init()
{
	DEPENDENCY_ALL(TreasurePlacer);
}

char RockPlacer::dump(const int3 & t)
{
	if(!map.getTile(t).terType->isPassable())
	{
		return zone.area().contains(t) ? 'R' : 'E';
	}
	return Modificator::dump(t);
}

VCMI_LIB_NAMESPACE_END
