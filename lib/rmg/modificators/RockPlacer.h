/*
 * RockPlacer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once
#include "../Zone.h"

VCMI_LIB_NAMESPACE_BEGIN

class RockPlacer: public Modificator
{
	friend class RockFiller;
public:
	MODIFICATOR(RockPlacer);
	
	void process() override;
	void init() override;
	char dump(const int3 &) override;
	
	void blockRock();
	void postProcess();
	
protected:
	
	rmg::Area rockArea, accessibleArea;
	TerrainId rockTerrain;
};

VCMI_LIB_NAMESPACE_END
