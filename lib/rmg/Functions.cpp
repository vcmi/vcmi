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
#include "../CTownHandler.h"
#include "../mapping/CMap.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../VCMI_Lib.h"

VCMI_LIB_NAMESPACE_BEGIN

rmg::Tileset collectDistantTiles(const Zone& zone, int distance)
{
	uint32_t distanceSq = distance * distance;
	auto subarea = zone.getArea().getSubarea([&zone, distanceSq](const int3 & t)
	{
		return t.dist2dSQ(zone.getPos()) > distanceSq;
	});
	return subarea.getTiles();
}

int chooseRandomAppearance(CRandomGenerator & generator, si32 ObjID, TerrainId terrain)
{
	auto factories = VLC->objtypeh->knownSubObjects(ObjID);
	vstd::erase_if(factories, [ObjID, &terrain](si32 f)
	{
		//TODO: Use templates with lowest number of terrains (most specific)
		return VLC->objtypeh->getHandlerFor(ObjID, f)->getTemplates(terrain).empty();
	});
	
	return *RandomGeneratorUtil::nextItem(factories, generator);
}

VCMI_LIB_NAMESPACE_END
