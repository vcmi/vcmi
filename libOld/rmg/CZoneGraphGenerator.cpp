
/*
 * CZoneGraphGenerator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CZoneGraphGenerator.h"

CZoneCell::CZoneCell(const CRmgTemplateZone * zone)// : zone(zone)
{

}


CZoneGraph::CZoneGraph()
{

}

CZoneGraphGenerator::CZoneGraphGenerator()// : gen(nullptr)
{

}

std::unique_ptr<CZoneGraph> CZoneGraphGenerator::generate(const CMapGenOptions & options, CRandomGenerator * gen)
{
	return make_unique<CZoneGraph>();
}
