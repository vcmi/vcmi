//
//  WaterRoutes.cpp
//  vcmi
//
//  Created by nordsoft on 27.06.2022.
//

#include "WaterRoutes.h"
#include "WaterProxy.h"
#include "CMapGenerator.h"
#include "RmgMap.h"
#include "../mapping/CMap.h"
#include "../mapping/CMapEditManager.h"
#include "../mapObjects/CObjectClassesHandler.h"
#include "CRmgPath.h"
#include "CRmgObject.h"
#include "ObjectManager.h"
#include "Functions.h"
#include "RoadPlacer.h"
#include "TreasurePlacer.h"
#include "TownPlacer.h"
#include "ConnectionsPlacer.h"
#include "TileInfo.h"
#include "WaterAdopter.h"
#include "CRmgArea.h"

WaterRoutes::WaterRoutes(Zone & zone, RmgMap & map, CMapGenerator & generator) : zone(zone), map(map), generator(generator)
{
}

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
		postfunction(z.second->getModificator<TreasurePlacer>());
		postfunction(z.second->getModificator<ObjectManager>());
	}
	dependency(zone.getModificator<WaterProxy>());
	postfunction(zone.getModificator<TreasurePlacer>());
}
