/*
 * WaterRoutes.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "WaterRoutes.h"
#include "WaterProxy.h"
#include "CMapGenerator.h"
#include "RmgMap.h"
#include "../mapping/CMap.h"
#include "../mapping/CMapEditManager.h"
#include "../mapObjects/CObjectClassesHandler.h"
#include "RmgPath.h"
#include "RmgObject.h"
#include "ObjectManager.h"
#include "Functions.h"
#include "RoadPlacer.h"
#include "TreasurePlacer.h"
#include "TownPlacer.h"
#include "ConnectionsPlacer.h"
#include "TileInfo.h"
#include "WaterAdopter.h"
#include "RmgArea.h"

void WaterRoutes::process()
{
	auto * wproxy = zone.getModificator<WaterProxy>();
	if(!wproxy)
		return;
	
	for(auto & z : map.getZones())
	{
		wproxy->waterRoute(*z.second);
	}
}

void WaterRoutes::init()
{
	for(auto & z : map.getZones())
	{
		dependency(z.second->getModificator<ConnectionsPlacer>());
		dependency(z.second->getModificator<ObjectManager>());
		postfunction(z.second->getModificator<TreasurePlacer>());
	}
	dependency(zone.getModificator<WaterProxy>());
	postfunction(zone.getModificator<TreasurePlacer>());
}

