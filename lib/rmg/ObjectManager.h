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

#include "Zone.h"
#include "RmgObject.h"
#include <boost/heap/priority_queue.hpp> //A*

VCMI_LIB_NAMESPACE_BEGIN

class CGObjectInstance;
class ObjectTemplate;
class CGCreature;

typedef std::pair<int3, float> TDistance;
struct DistanceMaximizeFunctor
{
	bool operator()(const TDistance & lhs, const TDistance & rhs) const
	{
		return (rhs.second > lhs.second);
	}
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

	void addRequiredObject(CGObjectInstance * obj, si32 guardStrength=0);
	void addCloseObject(CGObjectInstance * obj, si32 guardStrength = 0);
	void addNearbyObject(CGObjectInstance * obj, CGObjectInstance * nearbyTarget);

	bool createRequiredObjects();

	int3 findPlaceForObject(const rmg::Area & searchArea, rmg::Object & obj, si32 min_dist, OptimizeType optimizer) const;
	int3 findPlaceForObject(const rmg::Area & searchArea, rmg::Object & obj, const std::function<float(const int3)> & weightFunction, OptimizeType optimizer) const;

	rmg::Path placeAndConnectObject(const rmg::Area & searchArea, rmg::Object & obj, si32 min_dist, bool isGuarded, bool onlyStraight, OptimizeType optimizer) const;
	rmg::Path placeAndConnectObject(const rmg::Area & searchArea, rmg::Object & obj, const std::function<float(const int3)> & weightFunction, bool isGuarded, bool onlyStraight, OptimizeType optimizer) const;

	CGCreature * chooseGuard(si32 strength, bool zoneGuard = false);
	bool addGuard(rmg::Object & object, si32 strength, bool zoneGuard = false);
	void placeObject(rmg::Object & object, bool guarded, bool updateDistance);

	void updateDistances(const rmg::Object & obj);
	void createDistancesPriorityQueue();

	const rmg::Area & getVisitableArea() const;

	std::vector<CGObjectInstance*> getMines() const;
	
protected:
	//content info
	std::vector<std::pair<CGObjectInstance*, ui32>> requiredObjects;
	std::vector<std::pair<CGObjectInstance*, ui32>> closeObjects;
	std::vector<std::pair<CGObjectInstance*, int3>> instantObjects;
	std::vector<std::pair<CGObjectInstance*, CGObjectInstance*>> nearbyObjects;
	std::vector<CGObjectInstance*> objects;
	rmg::Area objectsVisitableArea;
	
	boost::heap::priority_queue<TDistance, boost::heap::compare<DistanceMaximizeFunctor>> tilesByDistance;
	
};

VCMI_LIB_NAMESPACE_END
