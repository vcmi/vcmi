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

	int3 findPlaceForObject(const rmg::Area & searchArea, rmg::Object & obj, si32 min_dist, OptimizeType optimizer) const;
	int3 findPlaceForObject(const rmg::Area & searchArea, rmg::Object & obj, const std::function<float(const int3)> & weightFunction, OptimizeType optimizer) const;

	rmg::Path placeAndConnectObject(const rmg::Area & searchArea, rmg::Object & obj, si32 min_dist, bool isGuarded, bool onlyStraight, OptimizeType optimizer) const;
	rmg::Path placeAndConnectObject(const rmg::Area & searchArea, rmg::Object & obj, const std::function<float(const int3)> & weightFunction, bool isGuarded, bool onlyStraight, OptimizeType optimizer) const;

	std::shared_ptr<CGCreature> chooseGuard(si32 strength, bool zoneGuard = false);
	bool addGuard(rmg::Object & object, si32 strength, bool zoneGuard = false);
	void placeObject(rmg::Object & object, bool guarded, bool updateDistance, bool createRoad = false);

	void updateDistances(const rmg::Object & obj);
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
	
	rmg::TileDistanceIndex tilesByDistance;
	
};
