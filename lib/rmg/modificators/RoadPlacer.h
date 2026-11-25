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
#include "../Zone.h"

const float VISITABLE_PENALTY = 1.33f;

class RoadPlacer: public Modificator
{
public:
	MODIFICATOR(RoadPlacer);
	
	void process() override;
	void postProcess();
	void init() override;
	char dump(const int3 &) override;
	std::string dumpLegend() const override;
	
	void addRoadNode(const int3 & node);
	void setNodeCurve(const int3 & node, const rmg::BezierCurve & curve);
	void connectRoads(); //fills "roads" according to "roadNodes"
	
	// TODO: Use setters?
	rmg::Area & areaForRoads();
	rmg::Area & areaIsolated();
	rmg::Area & areaVisitable();
	const rmg::Area & getRoads() const;
	
protected:
	bool createRoad(const int3 & dst);
	rmg::Path createRoadDesperate(rmg::Path & path, const int3 & destination);
	void drawRoads(bool secondary = false); //actually updates tiles
	float isolatedMovePenalty(const int3 & src, const int3 & dst);

protected:
	rmg::Tileset roadNodes; //tiles to be connected with roads
	std::map<int3, rmg::BezierCurve> nodeCurves;
	rmg::Area roads; //all tiles with roads
	rmg::Area areaRoads;
	rmg::Area isolated;
	rmg::Area visitableTiles; // Tiles occupied by removable or passable objects

	bool noRoadNodes = false;
};
