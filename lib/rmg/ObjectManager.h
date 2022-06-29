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

class CGObjectInstance;
class ObjectTemplate;
class CGCreature;

class DLL_LINKAGE ObjectManager: public Modificator
{
public:
	MODIFICATOR(ObjectManager);
	
	void process() override;
	void init() override;
	
	void addRequiredObject(CGObjectInstance * obj, si32 guardStrength=0);
	void addCloseObject(CGObjectInstance * obj, si32 guardStrength = 0);
	void addNearbyObject(CGObjectInstance * obj, CGObjectInstance * nearbyTarget);
		
	bool createRequiredObjects();
	
	int3 findPlaceForObject(const rmg::Area & searchArea, rmg::Object & obj, si32 min_dist, bool optimizer) const;
	int3 findPlaceForObject(const rmg::Area & searchArea, rmg::Object & obj, std::function<float(const int3)> weightFunction, bool optimizaer) const;
	
	bool placeAndConnectObject(const rmg::Area & searchArea, rmg::Object & obj, si32 min_dist, bool isGuarded, bool onlyStraight, bool optimizer) const;
	bool placeAndConnectObject(const rmg::Area & searchArea, rmg::Object & obj, std::function<float(const int3)> weightFunction, bool isGuarded, bool onlyStraight, bool optimizer) const;
	
	CGCreature * chooseGuard(si32 str, bool zoneGuard = false);
	bool addGuard(rmg::Object & object, si32 str, bool zoneGuard = false);
	void placeObject(rmg::Object & object, bool guarded, bool updateDistance);
	void placeObject(CGObjectInstance* object, const int3 & pos, bool updateDistance);
	
	void updateDistances(const int3 & pos);
	
protected:
	//content info
	std::vector<std::pair<CGObjectInstance*, ui32>> requiredObjects;
	std::vector<std::pair<CGObjectInstance*, ui32>> closeObjects;
	std::vector<std::pair<CGObjectInstance*, int3>> instantObjects;
	std::vector<std::pair<CGObjectInstance*, CGObjectInstance*>> nearbyObjects;
	std::vector<CGObjectInstance*> objects;
};
