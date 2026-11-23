/*
 * ObjectManager.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../Zone.h"
#include "../RmgObject.h"
#include "../TileDistanceIndex.h"

class CGObjectInstance;
class ObjectTemplate;
class CGCreature;

struct RequiredObjectInfo
{
	RequiredObjectInfo();
	RequiredObjectInfo(std::shared_ptr<CGObjectInstance> obj, ui32 guardStrength = 0, bool createRoad = false, CGObjectInstance* nearbyTarget = nullptr);

	std::shared_ptr<CGObjectInstance> obj;
	CGObjectInstance* nearbyTarget;
	int3 pos;
	ui32 guardStrength;
	bool createRoad;
};

class ObjectManager: public Modificator
{
public:
	enum OptimizeType
	{
		NONE = 0x00000000,
		WEIGHT = 0x00000001,
		DISTANCE = 0x00000010,
		BOTH = 0x00000011
	};

public:
	MODIFICATOR(ObjectManager);

	void process() override;
	void init() override;

	void addRequiredObject(const RequiredObjectInfo & info);
	void addCloseObject(const RequiredObjectInfo & info);
	void addNearbyObject(const RequiredObjectInfo & info);

	bool createMonoliths();
	bool createRequiredObjects();

	/// VCM-169: `stopDistance` ends the DISTANCE scan at the first tile whose nearest-object distance is below it.
	/// The caller guarantees that `searchArea` holds no such tile, so the result does not change.
	int3 findPlaceForObject(const rmg::Area & searchArea, rmg::Object & obj, si32 min_dist, OptimizeType optimizer, float stopDistance = 0.f) const;
	int3 findPlaceForObject(const rmg::Area & searchArea, rmg::Object & obj, const std::function<float(const int3)> & weightFunction, OptimizeType optimizer, float stopDistance = 0.f) const;

	rmg::Path placeAndConnectObject(const rmg::Area & searchArea, rmg::Object & obj, si32 min_dist, bool isGuarded, bool onlyStraight, OptimizeType optimizer, float stopDistance = 0.f) const;
	rmg::Path placeAndConnectObject(const rmg::Area & searchArea, rmg::Object & obj, const std::function<float(const int3)> & weightFunction, bool isGuarded, bool onlyStraight, OptimizeType optimizer, float stopDistance = 0.f) const;

	/// VCM-169: search `searchArea` restricted to tiles `filter` accepts, without copying it.
	/// These hold zone.areaMutex for the whole call; `searchArea` may be a live area of this zone.
	using TileFilter = std::function<bool(const int3 &)>;
	int3 findPlaceForObject(const rmg::Area & searchArea, const TileFilter & filter, rmg::Object & obj, const std::function<float(const int3)> & weightFunction, OptimizeType optimizer, float stopDistance = 0.f) const;
	rmg::Path placeAndConnectObject(const rmg::Area & searchArea, const TileFilter & filter, rmg::Object & obj, si32 min_dist, bool isGuarded, bool onlyStraight, OptimizeType optimizer, float stopDistance = 0.f) const;
	rmg::Path placeAndConnectObject(const rmg::Area & searchArea, const TileFilter & filter, rmg::Object & obj, const std::function<float(const int3)> & weightFunction, bool isGuarded, bool onlyStraight, OptimizeType optimizer, float stopDistance = 0.f) const;

	std::shared_ptr<CGCreature> chooseGuard(si32 strength, bool zoneGuard = false);
	bool addGuard(rmg::Object & object, si32 strength, bool zoneGuard = false);
	void placeObject(rmg::Object & object, bool guarded, bool updateDistance, bool createRoad = false);

	void updateDistances(const rmg::Object & obj);
	/// Remembers footprint (occupied and entrance tiles) of a placed object for the multi-entrance distance rule
	void addFootprint(const rmg::Object & obj);
	/// Footprint of candidate keeps minDistance from footprints of placed objects whenever a multi-entrance object is involved
	bool keepsFootprintDistance(const rmg::Object & candidate, float minDistance) const;
	void updateDistances(const int3& pos);
	void createDistancesPriorityQueue();

	const rmg::Area & getVisitableArea() const;

	std::vector<CGObjectInstance*> getMines() const;
	
protected:
	void updateDistancesFromTiles(const std::vector<int3> & source);

	//content info
	std::vector<RequiredObjectInfo> requiredObjects;
	std::vector<RequiredObjectInfo> closeObjects;
	std::vector<RequiredObjectInfo> instantObjects;
	std::vector<RequiredObjectInfo> nearbyObjects;
	std::vector<CGObjectInstance*> objects;
	rmg::Area objectsVisitableArea;
	/// Occupied and entrance tiles of all placed objects, and of the placed objects with multiple entrances
	rmg::Area objectsFootprint;
	rmg::Area multiEntranceFootprint;
	mutable std::mutex footprintMutex;
	
	rmg::TileDistanceIndex tilesByDistance;
	
};
