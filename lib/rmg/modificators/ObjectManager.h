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
#include <boost/heap/priority_queue.hpp> //A*

VCMI_LIB_NAMESPACE_BEGIN

class CGObjectInstance;
class ObjectTemplate;
class CGCreature;

using TDistance = std::pair<int3, float>;
struct DistanceMaximizeFunctor
{
	bool operator()(const TDistance & lhs, const TDistance & rhs) const
	{
		return (rhs.second > lhs.second);
	}
};

struct RequiredObjectInfo
{
	RequiredObjectInfo();
	RequiredObjectInfo(CGObjectInstance* obj, ui32 guardStrength = 0, bool allowRoad = true, CGObjectInstance* nearbyTarget = nullptr);

	CGObjectInstance* obj;
	CGObjectInstance* nearbyTarget;
	int3 pos;
	ui32 guardStrength;
	bool allowRoad;
};

class ObjectManager: public Modificator
{
public:
	enum OptimizeType
	{
		NONE = 0x00000000,
		WEIGHT = 0x00000001,
		DISTANCE = 0x00000010
	};

public:
	MODIFICATOR(ObjectManager);

	void process() override;
	void init() override;

	void addRequiredObject(const RequiredObjectInfo & info);
	void addCloseObject(const RequiredObjectInfo & info);
	void addNearbyObject(const RequiredObjectInfo & info);

	bool createRequiredObjects();

	int3 findPlaceForObject(const rmg::Area & searchArea, rmg::Object & obj, si32 min_dist, OptimizeType optimizer) const;
	int3 findPlaceForObject(const rmg::Area & searchArea, rmg::Object & obj, const std::function<float(const int3)> & weightFunction, OptimizeType optimizer) const;

	rmg::Path placeAndConnectObject(const rmg::Area & searchArea, rmg::Object & obj, si32 min_dist, bool isGuarded, bool onlyStraight, OptimizeType optimizer) const;
	rmg::Path placeAndConnectObject(const rmg::Area & searchArea, rmg::Object & obj, const std::function<float(const int3)> & weightFunction, bool isGuarded, bool onlyStraight, OptimizeType optimizer) const;

	CGCreature * chooseGuard(si32 strength, bool zoneGuard = false);
	bool addGuard(rmg::Object & object, si32 strength, bool zoneGuard = false);
	void placeObject(rmg::Object & object, bool guarded, bool updateDistance, bool allowRoad = true);

	void updateDistances(const rmg::Object & obj);
	void updateDistances(const int3& pos);
	void updateDistances(std::function<ui32(const int3 & tile)> distanceFunction);
	void createDistancesPriorityQueue();

	const rmg::Area & getVisitableArea() const;

	std::vector<CGObjectInstance*> getMines() const;
	
protected:
	//content info
	std::vector<RequiredObjectInfo> requiredObjects;
	std::vector<RequiredObjectInfo> closeObjects;
	std::vector<RequiredObjectInfo> instantObjects;
	std::vector<RequiredObjectInfo> nearbyObjects;
	std::vector<CGObjectInstance*> objects;
	rmg::Area objectsVisitableArea;
	
	boost::heap::priority_queue<TDistance, boost::heap::compare<DistanceMaximizeFunctor>> tilesByDistance;
	
};

VCMI_LIB_NAMESPACE_END
