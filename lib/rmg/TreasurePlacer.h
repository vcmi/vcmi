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
class RmgMap;
class CMapGenerator;

struct DLL_LINKAGE ObjectInfo
{
	ObjectTemplate templ;
	ui32 value;
	ui16 probability;
	ui32 maxPerZone;
	//ui32 maxPerMap; //unused
	std::function<CGObjectInstance *()> generateObject;
	
	void setTemplate(si32 type, si32 subtype, Terrain terrain);
	
	ObjectInfo();
	
	bool operator==(const ObjectInfo& oi) const { return (templ == oi.templ); }
};

class DLL_LINKAGE TreasurePlacer: public Modificator
{
public:
	TreasurePlacer(Zone & zone, RmgMap & map, CRandomGenerator & generator);
	
	void process() override;
	
	void createTreasures(ObjectManager & manager);
	
	void setQuestArtZone(Zone * otherZone);
	void addAllPossibleObjects(CMapGenerator & gen); //add objects, including zone-specific, to possibleObjects
	
protected:
	bool isGuardNeededForTreasure(int value);
	
	ObjectInfo getRandomObject(ui32 desiredValue, ui32 currentValue, bool allowLargeObjects);
	std::vector<ObjectInfo> prepareTreasurePile(const CTreasureInfo & treasureInfo);
	Rmg::Object constuctTreasurePile(const std::vector<ObjectInfo> & treasureInfos);
	
	
protected:
	RmgMap & map;
	CRandomGenerator & generator;
	Zone & zone;
	
	std::vector<ObjectInfo> possibleObjects;
	int minGuardedValue;
	
	Zone* questArtZone; //artifacts required for Seer Huts will be placed here - or not if null
};
