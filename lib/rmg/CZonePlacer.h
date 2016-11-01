
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
#include "float3.h"
#include "../int3.h"

class CZoneGraph;
class CMap;
class CRandomGenerator;
class CRmgTemplateZone;
class CMapGenerator;

typedef std::vector<std::pair<TRmgTemplateZoneId, CRmgTemplateZone*>> TZoneVector;
typedef std::map <TRmgTemplateZoneId, CRmgTemplateZone*> TZoneMap;
typedef std::map <CRmgTemplateZone *, float3> TForceVector;
typedef std::map <CRmgTemplateZone *, float> TDistanceVector;

class CPlacedZone
{
public:
	explicit CPlacedZone(const CRmgTemplateZone * Zone);

private:
    //const CRmgTemplateZone * zone;

	//TODO exact outline data of zone
	//TODO perhaps further zone data, guards, obstacles, etc...
};

class CZonePlacer
{
public:
	explicit CZonePlacer(CMapGenerator * gen);
	int3 cords (const float3 f) const;
	float metric (const int3 &a, const int3 &b) const;
	float getDistance(float distance) const; //additional scaling without 0 divison
	~CZonePlacer();

	void prepareZones(TZoneMap &zones, TZoneVector &zonesVector, const bool underground, CRandomGenerator * rand);
	void attractConnectedZones(TZoneMap &zones, TForceVector &forces, TDistanceVector &distances);
	void separateOverlappingZones(TZoneMap &zones, TForceVector &forces, TDistanceVector &overlaps);
	void moveOneZone(TZoneMap &zones, TForceVector &totalForces, TDistanceVector &distances, TDistanceVector &overlaps);
	void placeZones(const CMapGenOptions * mapGenOptions, CRandomGenerator * rand);
	void assignZones(const CMapGenOptions * mapGenOptions);

private:
	int width;
	int height;
	//metric coefiicients
	float scaleX;
	float scaleY;
	float mapSize;

	float gravityConstant;
	float stiffnessConstant;
    //float a1, b1, c1, a2, b2, c2;
	//CMap * map;
	//std::unique_ptr<CZoneGraph> graph;
	CMapGenerator * gen;
};
