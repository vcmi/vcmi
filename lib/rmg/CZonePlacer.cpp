
/*
 * CZonePlacer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CZonePlacer.h"

#include "CZoneGraphGenerator.h"

CPlacedZone::CPlacedZone(CRmgTemplateZone * zone) : zone(zone)
{

}

CZonePlacer::CZonePlacer() : map(nullptr), gen(nullptr)
{

}

CZonePlacer::~CZonePlacer()
{

}

void CZonePlacer::placeZones(CMap * map, unique_ptr<CZoneGraph> graph, CRandomGenerator * gen)
{

}
