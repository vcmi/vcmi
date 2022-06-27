//
//  WaterAdopter.cpp
//  vcmi
//
//  Created by nordsoft on 27.06.2022.
//

#include "WaterAdopter.h"
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
#include "TileInfo.h"

WaterAdopter::WaterAdopter(Zone & zone, RmgMap & map, CMapGenerator & generator) : zone(zone), map(map), generator(generator)
{
}

void WaterAdopter::process()
{
}
