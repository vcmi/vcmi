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

VCMI_LIB_NAMESPACE_BEGIN

class RoadPlacer: public Modificator
{
public:
	MODIFICATOR(RoadPlacer);
	
	void process() override;
	char dump(const int3 &) override;
	
	void addRoadNode(const int3 & node);
	void connectRoads(); //fills "roads" according to "roadNodes"
	
	rmg::Area & areaForRoads();
	rmg::Area & areaIsolated();
	const rmg::Area & getRoads() const;
	
protected:
	bool createRoad(const int3 & dst);
	void drawRoads(bool secondary = false); //actually updates tiles

protected:
	rmg::Tileset roadNodes; //tiles to be connected with roads
	rmg::Area roads; //all tiles with roads
	rmg::Area areaRoads, isolated;
};

VCMI_LIB_NAMESPACE_END
