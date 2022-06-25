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

class DLL_LINKAGE ObjectManager: public Modificator
{
public:
	
	enum EObjectPlacingResult
	{
		SUCCESS,
		CANNOT_FIT,
		SEALED_OFF
	};
	
	ObjectManager(Zone & zone, RmgMap & map, CRandomGenerator & generator);
	
	void process() override;
	
	void addRequiredObject(CGObjectInstance * obj, si32 guardStrength=0);
	void addCloseObject(CGObjectInstance * obj, si32 guardStrength = 0);
	void addNearbyObject(CGObjectInstance * obj, CGObjectInstance * nearbyTarget);
	void addObjectAtPosition(CGObjectInstance * obj, const int3 & position, si32 guardStrength=0);
	
	bool createRequiredObjects();
	
	bool isAccessibleFromSomewhere(ObjectTemplate & appearance, const int3 & tile) const;
	int3 getAccessibleOffset(ObjectTemplate & appearance, const int3 & tile) const;
	std::vector<int3> getAccessibleOffsets(const CGObjectInstance* object) const;
	
	bool areAllTilesAvailable(CGObjectInstance* obj, int3 & tile, const std::set<int3>& tilesBlockedByObject) const;
	bool findPlaceForObject(const Rmg::Area & searhArea, Rmg::Object & obj, si32 min_dist, int3 & pos);
	EObjectPlacingResult tryToPlaceObjectAndConnectToPath(CGObjectInstance * obj, const int3 & pos); //return true if the position can be connected
	
	void placeObject(Rmg::Object & object, bool updateDistance = true);
	void placeObject(CGObjectInstance* object, const int3 & pos, bool updateDistance = true);
	bool guardObject(CGObjectInstance* object, si32 str, bool zoneGuard = false, bool addToFreePaths = false);
	bool addMonster(int3 &pos, si32 strength, bool clearSurroundingTiles = true, bool zoneGuard = false);
	void setTemplateForObject(CGObjectInstance* obj);
	
	void checkAndPlaceObject(CGObjectInstance* object, const int3 & pos);
	void updateDistances(const int3 & pos);
	
protected:
	RmgMap & map;
	CRandomGenerator & generator;
	Zone & zone;
	
	//content info
	std::vector<std::pair<CGObjectInstance*, ui32>> requiredObjects;
	std::vector<std::pair<CGObjectInstance*, ui32>> closeObjects;
	std::vector<std::pair<CGObjectInstance*, int3>> instantObjects;
	std::vector<std::pair<CGObjectInstance*, CGObjectInstance*>> nearbyObjects;
	std::vector<CGObjectInstance*> objects;
	std::map<CGObjectInstance*, int3> requestedPositions;
};
