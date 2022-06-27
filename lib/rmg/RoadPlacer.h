/*
 * RoadPlacer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once
#include "Zone.h"

class DLL_LINKAGE RoadPlacer: public Modificator
{
public:
	RoadPlacer(Zone & zone, RmgMap & map, CMapGenerator & generator);
	
	void process() override;
	
	void addRoadNode(const int3 & node);
	void connectRoads(); //fills "roads" according to "roadNodes"
	
protected:
	bool createRoad(const rmg::Area & dst);
	void drawRoads(); //actually updates tiles
	
protected:
	RmgMap & map;
	CMapGenerator & generator;
	Zone & zone;
	
	rmg::Area roadNodes; //tiles to be connected with roads
	rmg::Area roads; //all tiles with roads
};
