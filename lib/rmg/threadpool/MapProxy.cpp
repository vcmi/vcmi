/*
 * MapProxy.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "MapProxy.h"
#include "../../TerrainHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

MapProxy::MapProxy(RmgMap & map):
    map(map)
{
}

void MapProxy::insertObject(CGObjectInstance * obj)
{
    Lock lock(mx);
    map.getEditManager()->insertObject(obj);
}

void MapProxy::insertObjects(std::set<CGObjectInstance*>& objects)
{
    Lock lock(mx);
    map.getEditManager()->insertObjects(objects);
}

void MapProxy::removeObject(CGObjectInstance * obj)
{
    Lock lock(mx);
    map.getEditManager()->removeObject(obj);
}

void MapProxy::drawTerrain(CRandomGenerator & generator, std::vector<int3> & tiles, TerrainId terrain)
{
    Lock lock(mx);
	map.getEditManager()->getTerrainSelection().setSelection(tiles);
	map.getEditManager()->drawTerrain(terrain, &generator);
}

void MapProxy::drawRivers(CRandomGenerator & generator, std::vector<int3> & tiles, TerrainId terrain)
{
    Lock lock(mx);
	map.getEditManager()->getTerrainSelection().setSelection(tiles);
	map.getEditManager()->drawRiver(VLC->terrainTypeHandler->getById(terrain)->river, &generator);
}

void MapProxy::drawRoads(CRandomGenerator & generator, std::vector<int3> & tiles, RoadId roadType)
{
    Lock lock(mx);
    map.getEditManager()->getTerrainSelection().setSelection(tiles);
	map.getEditManager()->drawRoad(roadType, &generator);
}

VCMI_LIB_NAMESPACE_END