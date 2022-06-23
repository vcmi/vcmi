//
//  TreasurePlacer.hpp
//  vcmi
//
//  Created by nordsoft on 23.06.2022.
//

#pragma once
#include "Zone.h"
#include "../mapObjects/ObjectTemplate.h"

class CGObjectInstance;
class ObjectManager;

struct DLL_LINKAGE ObjectInfo
{
	ObjectTemplate templ;
	ui32 value;
	ui16 probability;
	ui32 maxPerZone;
	//ui32 maxPerMap; //unused
	std::function<CGObjectInstance *()> generateObject;
	
	void setTemplate (si32 type, si32 subtype, Terrain terrain);
	
	ObjectInfo();
	
	bool operator==(const ObjectInfo& oi) const { return (templ == oi.templ); }
};

struct DLL_LINKAGE CTreasurePileInfo
{
	std::set<int3> visitableFromBottomPositions; //can be visited only from bottom or side
	std::set<int3> visitableFromTopPositions; //they can be visited from any direction
	std::set<int3> blockedPositions;
	std::set<int3> occupiedPositions; //blocked + visitable
	int3 nextTreasurePos;
};

class DLL_LINKAGE TreasurePlacer
{
public:
	TreasurePlacer(Zone & zone, CMapGenerator & gen);
	
	void createTreasures(ObjectManager & manager);
	
	void setQuestArtZone(std::shared_ptr<TreasurePlacer> otherZone);
	bool createTreasurePile(ObjectManager & manager, int3 &pos, float minDistance, const CTreasureInfo& treasureInfo);
	
protected:
	void addAllPossibleObjects(); //add objects, including zone-specific, to possibleObjects
	bool isGuardNeededForTreasure(int value);
	bool findPlaceForTreasurePile(float min_dist, int3 &pos, int value);
	ObjectInfo getRandomObject(const ObjectManager & manager, CTreasurePileInfo &info, ui32 desiredValue, ui32 maxValue, ui32 currentValue);
	
protected:
	CMapGenerator & gen;
	Zone & zone;
	
	std::vector<ObjectInfo> possibleObjects;
	int minGuardedValue;
	
	std::weak_ptr<TreasurePlacer> questArtZone; //artifacts required for Seer Huts will be placed here - or not if null
};
