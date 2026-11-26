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
#include "ZonePlacementLog.h"
#include "../int3.h"
#include "../GameConstants.h"

namespace vstd
{
class RNG;
}

namespace rmg
{
class ZoneConnection;
}

class CZoneGraph;
class CMap;
class RmgMap;
class Zone;
class Point2D;

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
	void prepareZones(const TZoneMap & zones, const TZoneVector & zonesVector, const int mapLevels);
	void attractConnectedZones(TZoneMap & zones, TForceVector & forces, TDistanceVector & distances) const;

	// Collect the player starting zones, and among them the ones belonging to players in the game.
	void findPlayerZones(const TZoneMap & zones);

	// Roll a random dihedral symmetry and apply it to the finished zone centers, so that a fixed template
	// graph does not always yield the same-looking layout. Connectivity is preserved - it is an isometry.
	void applyRandomOrientation(const TZoneMap & zones, vstd::RNG * rand);
	// How close the player start zones of one layout are to each other, see playerCrowding().
	float measurePlayerCrowding(const TZoneMap & zones, const TZoneCenterMap & solution) const;
	// Swap interchangeable player start zones so that lower player indexes hold the farthest centers.
	void arrangePlayerZones(const TZoneMap & zones) const;
	// Apply the stored orientation to a point in normalized [0,1] space (identity if none was rolled).
	std::pair<float, float> orientNormalized(float x, float y) const;
	void separateOverlappingZones(TZoneMap &zones, TForceVector &forces, TDistanceVector &overlaps);
	void moveOneZone(TZoneMap & zones, TForceVector & totalForces, TDistanceVector & distances, TDistanceVector & overlaps);

	// Predicted connectivity of a finished layout, used to pick the best of several placement attempts.
	std::vector<ConnectionPrediction> classifyConnections(const TZoneMap & zones, const TZoneCenterMap & solution) const;
	ConnectionPrediction predictConnection(const rmg::ZoneConnection & connection, float touching, const float3 & selfPos, const float3 & otherPos) const;

	// One level's tile assignment: iterate a per-zone additive weight so each zone's claimed tile count
	// converges to its target share, then paint the tiles.
	void assignTilesCapacityBalanced(int level, const std::vector<std::shared_ptr<Zone>> & levelZones, const std::set<Point2D> & vertices) const;

private:
	ZonePlacementLog placementLog;
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

	std::set<TRmgTemplateZoneId> playerStartZones;
	std::set<TRmgTemplateZoneId> participatingPlayerZones; //subset of playerStartZones

	//distance [a][b] = number of zone connections required to travel between the zones
	TDistanceMap distancesBetweenZones;
	std::set<TRmgTemplateZoneId> lastSwappedZones;
	RmgMap & map;
};
