/*
 * Functions.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Functions.h"
#include "CMapGenerator.h"
#include "RmgMap.h"
#include "TileInfo.h"
#include "RmgPath.h"
#include "../TerrainHandler.h"
#include "../mapping/CMap.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../GameLibrary.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

void replaceWithCurvedPath(rmg::Path & path, Zone & zone, const int3 & src, bool onlyStraight /* = true */)
{
	// Get random control points from within the zone
	auto & rng = zone.getRand();
	const auto & tiles = zone.area()->getTilesVector();

	if(tiles.size() < 2)
	{
		logGlobal->warn("Zone too small for curved path");
		return;
	}

	// Pick two random control points (Q1, Q2) from zone tiles
	int3 control1 = tiles[rng.nextInt(0, static_cast<int>(tiles.size()) - 1)];
	int3 control2 = tiles[rng.nextInt(0, static_cast<int>(tiles.size()) - 1)];

	// Create Bezier cost function: Q0=start, Q1=control1, Q2=control2, Q3=src
	auto costFunction = rmg::Path::createBezierCostFunction(src, control1, control2, zone.getPos());

	auto pathArea = zone.areaForRoads();
	rmg::Path curvedPath(pathArea);
	curvedPath.connect(zone.freePaths().get());
	curvedPath = curvedPath.search(src, onlyStraight, costFunction);
	if (curvedPath.valid())
	{
		path = curvedPath;
	}
	else
	{
		logGlobal->warn("Failed to create curved path to %s", src.toString());
	}
}

rmg::Tileset collectDistantTiles(const Zone& zone, int distance)
{
	uint32_t distanceSq = distance * distance;

	auto subarea = zone.area()->getSubarea([&zone, distanceSq](const int3 & t)
	{
		return t.dist2dSQ(zone.getPos()) > distanceSq;
	});
	return subarea.getTiles();
}

int chooseRandomAppearance(vstd::RNG & generator, si32 ObjID, TerrainId terrain)
{
	auto factories = LIBRARY->objtypeh->knownSubObjects(ObjID);
	vstd::erase_if(factories, [ObjID, &terrain](si32 f)
	{
		//TODO: Use templates with lowest number of terrains (most specific)
		return LIBRARY->objtypeh->getHandlerFor(ObjID, f)->getTemplates(terrain).empty();
	});
	
	return *RandomGeneratorUtil::nextItem(factories, generator);
}

VCMI_LIB_NAMESPACE_END
