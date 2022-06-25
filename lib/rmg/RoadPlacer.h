//
//  RoadPlacer.hpp
//  vcmi
//
//  Created by nordsoft on 23.06.2022.
//

#pragma once
#include "Zone.h"

class DLL_LINKAGE RoadPlacer: public Modificator
{
public:
	RoadPlacer(Zone & zone, RmgMap & map, CRandomGenerator & generator);
	
	void process() override;
	
	void addRoadNode(const int3 & node);
	void connectRoads(); //fills "roads" according to "roadNodes"
	
protected:
	bool createRoad(const Rmg::Area & dst);
	void drawRoads(); //actually updates tiles
	
protected:
	RmgMap & map;
	CRandomGenerator & generator;
	Zone & zone;
	
	Rmg::Area roadNodes; //tiles to be connected with roads
	Rmg::Area roads; //all tiles with roads
};
