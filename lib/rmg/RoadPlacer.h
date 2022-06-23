//
//  RoadPlacer.hpp
//  vcmi
//
//  Created by nordsoft on 23.06.2022.
//

#pragma once
#include "Zone.h"

class DLL_LINKAGE RoadPlacer
{
public:
	RoadPlacer(Zone & zone, RmgMap & map, CRandomGenerator & generator);
	
	void addRoadNode(const int3 & node);
	void connectRoads(); //fills "roads" according to "roadNodes"
	
protected:
	bool createRoad(const int3 & src, const int3 & dst);
	void drawRoads(); //actually updates tiles
	
protected:
	RmgMap & map;
	CRandomGenerator & generator;
	Zone & zone;
	
	std::set<int3> roadNodes; //tiles to be connected with roads
	std::set<int3> roads; //all tiles with roads
};
