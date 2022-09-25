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

VCMI_LIB_NAMESPACE_BEGIN

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
	rmg::Area & riverProhibit();
	
protected:
	void drawRivers();
	
	void preprocess();
	void connectRiver(const int3 & tile);
	
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

VCMI_LIB_NAMESPACE_END
