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
#include "ZonePlacementConfig.h"
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
	explicit CZonePlacer(RmgMap & map, const ZonePlacementConfig & config);
	int3 cords(const float3 & f) const;
	float getDistance(float distance) const; //additional scaling without 0 division
	~CZonePlacer() = default;

	void placeZones(vstd::RNG * rand);
	void findPathsBetweenZones();
	void assignZones(vstd::RNG * rand);
	void RemoveRoadsForWideConnections();

	const TDistanceMap & getDistanceMap();
	
private:
	void prepareZones(TZoneMap &zones, TZoneVector &zonesVector, const int mapLevels);
	void attractConnectedZones(TZoneMap & zones, TForceVector & forces, TDistanceVector & distances) const;

	// Roll and store a random dihedral symmetry (one of 4 rotations plus optional horizontal/vertical
	// flip) and apply it to the finished zone centers. On a square map these are isometries, so same-level
	// connectivity is preserved - the map just faces a random way. Without this a fixed template graph
	// always yields the same layout. The stored orientation is replayed onto the Penrose vertex field in
	// assignZones so cross-level footprint overlaps (subterranean gates) are preserved too.
	void applyRandomOrientation(const TZoneMap & zones, vstd::RNG * rand);
	// Apply the stored orientation to a point in normalized [0,1] space (identity if none was rolled).
	std::pair<float, float> orientNormalized(float x, float y) const;
	void separateOverlappingZones(TZoneMap &zones, TForceVector &forces, TDistanceVector &overlaps);
	void moveOneZone(TZoneMap & zones, TForceVector & totalForces, TDistanceVector & distances, TDistanceVector & overlaps);

	// Predicted connectivity of a finished layout, used to pick the best of several placement attempts.
	struct ConnectivityCounts { int direct = 0; int gates = 0; int monoliths = 0; };
	ConnectivityCounts classifyConnections(const TZoneMap & zones, const std::map<std::shared_ptr<Zone>, float3> & solution) const;

	// One level's tile assignment: iterate a per-zone additive weight so each zone's claimed tile count
	// converges to its target (proportional to size, a linear area weight), then paint the tiles.
	// Keeps the Penrose vertices (organic borders) and the zone centers (adjacency).
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

	ZonePlacementConfig config;

	// Orientation rolled by applyRandomOrientation, replayed onto the Penrose vertices in assignZones.
	int orientRotation = 0;    // 0/1/2/3 = 0/90/180/270 degrees
	bool orientFlipH = false;
	bool orientFlipV = false;

	//remember best solution
	float bestTotalDistance;
	float bestTotalOverlap;

	//distance [a][b] = number of zone connections required to travel between the zones
	TDistanceMap distancesBetweenZones;
	std::set<TRmgTemplateZoneId> lastSwappedZones;
	RmgMap & map;
};
