/*
 * RiverPlacer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once
#include "Zone.h"

class RiverPlacer: public Modificator
{
public:
	MODIFICATOR(RiverPlacer);
	
	void process() override;
	void init() override;
	char dump(const int3 &) override;
	
	void addRiverNode(const int3 & node);
	
	rmg::Area & riverSource();
	rmg::Area & riverSink();
	
protected:
	void drawRivers();
	
	void preprocess();
	void connectRiver(const int3 & tile);
	
	void prepareBorderHeightmap();
	void prepareBorderHeightmap(std::vector<int3>::iterator l, std::vector<int3>::iterator r);
	void prepareHeightmap();
	
private:
	rmg::Area rivers;
	rmg::Area source;
	rmg::Area sink;
	rmg::Area prohibit;
	rmg::Tileset riverNodes;
	rmg::Area deltaSink;
	std::map<int3, int3> deltaPositions;
	std::map<int3, int> deltaOrientations;
	
	std::map<int3, int> heightMap;
};
