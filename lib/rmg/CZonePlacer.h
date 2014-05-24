
/*
 * CZonePlacer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CMapGenerator.h"
#include "../mapping/CMap.h"

#include "float3.h"
#include "../int3.h"

class CZoneGraph;
class CMap;
class CRandomGenerator;
class CRmgTemplateZone;
class CMapGenerator;

class CPlacedZone
{
public:
	explicit CPlacedZone(const CRmgTemplateZone * Zone);

private:
	const CRmgTemplateZone * zone;

	//TODO exact outline data of zone
	//TODO perhaps further zone data, guards, obstacles, etc...
};

class CZonePlacer
{
public:
	explicit CZonePlacer(CMapGenerator * gen);
	int3 cords (float3 f) const;
	~CZonePlacer();

	void placeZones(shared_ptr<CMapGenOptions> mapGenOptions, CRandomGenerator * rand);

private:
	//CMap * map;
	//unique_ptr<CZoneGraph> graph;
	CMapGenerator * gen;
};
