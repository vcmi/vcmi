
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

class CZoneGraph;
class CMap;
class CRandomGenerator;
class CRmgTemplateZone;

class CPlacedZone
{
public:
	explicit CPlacedZone(const CRmgTemplateZone * zone);

private:
	const CRmgTemplateZone * zone;

	//TODO exact outline data of zone
	//TODO perhaps further zone data, guards, obstacles, etc...
};

//TODO add voronoi helper classes(?), etc...

class CZonePlacer
{
public:
	CZonePlacer();
	~CZonePlacer();

	void placeZones(CMap * map, unique_ptr<CZoneGraph> graph, CRandomGenerator * gen);

private:
	CMap * map;
	unique_ptr<CZoneGraph> graph;
	CRandomGenerator * gen;
};
