/*
 * MapProxy.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "StdInc.h"
#include "mapping/CMap.h"
#include "rmg/RmgMap.h"
#include "mapping/CMapEditManager.h"

VCMI_LIB_NAMESPACE_BEGIN

class RmgMap;

class MapProxy
{
public:
    MapProxy(RmgMap & map);

    void insertObject(CGObjectInstance * obj);
    void insertObjects(std::set<CGObjectInstance*>& objects);
    void removeObject(CGObjectInstance* obj);

    void drawTerrain(CRandomGenerator & generator, std::vector<int3> & tiles, TerrainId terrain);
    void drawRivers(CRandomGenerator & generator, std::vector<int3> & tiles, TerrainId terrain);
    void drawRoads(CRandomGenerator & generator, std::vector<int3> & tiles, RoadId roadType);

private:
    mutable boost::shared_mutex mx;
    using Lock = boost::unique_lock<boost::shared_mutex>;

    RmgMap & map;
};

VCMI_LIB_NAMESPACE_END
