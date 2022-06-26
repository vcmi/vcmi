//
//  ObjectManager.hpp
//  vcmi
//
//  Created by nordsoft on 23.06.2022.
//

#pragma once

#include "Zone.h"
#include "CRmgObject.h"

class CGObjectInstance;
class ObjectTemplate;
class CGCreature;

class DLL_LINKAGE ObjectManager: public Modificator
{
public:
	ObjectManager(Zone & zone, RmgMap & map, CMapGenerator & generator);
	
	void process() override;
	
	void addRequiredObject(CGObjectInstance * obj, si32 guardStrength=0);
	void addCloseObject(CGObjectInstance * obj, si32 guardStrength = 0);
	void addNearbyObject(CGObjectInstance * obj, CGObjectInstance * nearbyTarget);
	void addObjectAtPosition(CGObjectInstance * obj, const int3 & position, si32 guardStrength=0);
		
	bool createRequiredObjects();
	
	int3 findPlaceForObject(const Rmg::Area & searchArea, Rmg::Object & obj, si32 min_dist) const;
	int3 findPlaceForObject(const Rmg::Area & searchArea, Rmg::Object & obj, std::function<float(const int3)> weightFunction) const;
	
	bool placeAndConnectObject(const Rmg::Area & searchArea, Rmg::Object & obj, si32 min_dist, bool isGuarded, bool onlyStraight) const;
	bool placeAndConnectObject(const Rmg::Area & searchArea, Rmg::Object & obj, std::function<float(const int3)> weightFunction, bool isGuarded, bool onlyStraight) const;
	
	CGCreature * chooseGuard(si32 str, bool zoneGuard = false);
	bool addGuard(Rmg::Object & object, si32 str, bool zoneGuard = false);
	void placeObject(Rmg::Object & object, bool guarded, bool updateDistance);
	void placeObject(CGObjectInstance* object, const int3 & pos, bool updateDistance);
	
	void updateDistances(const int3 & pos);
	
protected:
	RmgMap & map;
	CMapGenerator & generator;
	Zone & zone;
	
	//content info
	std::vector<std::pair<CGObjectInstance*, ui32>> requiredObjects;
	std::vector<std::pair<CGObjectInstance*, ui32>> closeObjects;
	std::vector<std::pair<CGObjectInstance*, int3>> instantObjects;
	std::vector<std::pair<CGObjectInstance*, CGObjectInstance*>> nearbyObjects;
	std::vector<CGObjectInstance*> objects;
};
