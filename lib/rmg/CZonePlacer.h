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

#include "float3.h"
#include "../int3.h"
#include "../GameConstants.h"

namespace vstd
{
class RNG;
}

class CZoneGraph;
class CMap;
class RmgMap;
class Zone;
class Point2D;

typedef std::vector<std::pair<TRmgTemplateZoneId, std::shared_ptr<Zone>>> TZoneVector;
typedef std::map<TRmgTemplateZoneId, std::shared_ptr<Zone>> TZoneMap;
typedef std::map<std::shared_ptr<Zone>, float3> TForceVector;
typedef std::map<std::shared_ptr<Zone>, float> TDistanceVector;
typedef std::map<int, std::map<int, size_t>> TDistanceMap;

class CZonePlacer
{
public:
	explicit CZonePlacer(RmgMap & map, bool hexGrid = false, bool hubFirst = false, bool saPolish = false, float crossAlignWeight = 0.0f,
		bool capacityBalance = false, int capacityIterations = 30, float capacityGain = 1.0f);
	int3 cords(const float3 & f) const;
	float metric (const int3 &a, const int3 &b) const;
	float getDistance(float distance) const; //additional scaling without 0 division
	~CZonePlacer() = default;

	void placeZones(vstd::RNG * rand);
	void findPathsBetweenZones();
	float scaleForceBetweenZones(const std::shared_ptr<Zone> zoneA, const std::shared_ptr<Zone> zoneB) const;
	void assignZones(vstd::RNG * rand);
	void RemoveRoadsForWideConnections();

	const TDistanceMap & getDistanceMap();
	
private:
	void prepareZones(TZoneMap &zones, TZoneVector &zonesVector, const int mapLevels, vstd::RNG * rand);
	void attractConnectedZones(TZoneMap & zones, TForceVector & forces, TDistanceVector & distances) const;
	void separateOverlappingZones(TZoneMap &zones, TForceVector &forces, TDistanceVector &overlaps);
	void moveOneZone(TZoneMap & zones, TForceVector & totalForces, TDistanceVector & distances, TDistanceVector & overlaps);

	// One level's tile assignment with capacity balancing: iterate a per-zone additive weight so each
	// zone's claimed tile count converges to its target (proportional to size, a linear area weight),
	// then paint the tiles. Keeps the Penrose vertices (organic borders) and the zone centers (adjacency).
	void assignTilesCapacityBalanced(int level, int width, int height,
		const std::vector<std::shared_ptr<Zone>> & levelZones, const std::set<Point2D> & vertices) const;

private:
	int width;
	int height;
	//metric coefficient
	float mapSize;

	float gravityConstant;
	float stiffnessConstant;
	float stifness;
	float stiffnessIncreaseFactor;

	// Experimental placement improvements, toggled from randomMap.json ("zonePlacement")
	bool hexGrid;              // seed the initial layout on a hex (6-neighbour) grid instead of square
	bool hubFirst;             // place zones highest-degree first, hub at grid centre
	bool saPolish;             // anneal the grid assignment to improve connected-zone adjacency
	float crossAlignWeight;    // SA reward for cross-level partners sharing a normalized cell
	bool capacityBalance;      // balance per-zone Voronoi weights so claimed area matches target (area~size)
	int capacityIterations;    // number of weight-balancing passes
	float capacityGain;        // step size for the weight update (fraction-of-area error -> normalized dist^2)

	//remember best solution
	float bestTotalDistance;
	float bestTotalOverlap;

	//distance [a][b] = number of zone connections required to travel between the zones
	TDistanceMap distancesBetweenZones;
	std::set<TRmgTemplateZoneId> lastSwappedZones;
	RmgMap & map;
};
