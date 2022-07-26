/*
 * TerrainPainter.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "TerrainPainter.h"
#include "TownPlacer.h"
#include "WaterAdopter.h"
#include "WaterProxy.h"
#include "ConnectionsPlacer.h"
#include "ObjectManager.h"
#include "Functions.h"
#include "CMapGenerator.h"
#include "RmgMap.h"

VCMI_LIB_NAMESPACE_BEGIN

void TerrainPainter::process()
{
	initTerrainType(zone, generator);
	paintZoneTerrain(zone, generator.rand, map, zone.getTerrainType());
}

void TerrainPainter::init()
{
	DEPENDENCY(TownPlacer);
	DEPENDENCY_ALL(WaterAdopter);
	POSTFUNCTION_ALL(WaterProxy);
	POSTFUNCTION_ALL(ConnectionsPlacer);
	POSTFUNCTION(ObjectManager);
}

VCMI_LIB_NAMESPACE_END
