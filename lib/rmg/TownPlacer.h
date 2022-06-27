//
//  TownPlacer.hpp
//  vcmi
//
//  Created by nordsoft on 27.06.2022.
//

#pragma once
#include "Zone.h"

class ObjectManager;

class DLL_LINKAGE TownPlacer: public Modificator
{
public:
	TownPlacer(Zone & zone, RmgMap & map, CMapGenerator & generator);
	
	void process() override;
	void init() override;
	
	int getTotalTowns() const;
	
protected:
	void cleanupBoundaries(const Rmg::Object & rmgObject);
	void addNewTowns(int count, bool hasFort, PlayerColor player, ObjectManager & manager);
	si32 getRandomTownType(bool matchUndergroundType = false);
	void placeTowns(ObjectManager & manager);
	bool placeMines(ObjectManager & manager);
	
protected:
	RmgMap & map;
	CMapGenerator & generator;
	Zone & zone;
	
	int totalTowns;
};
