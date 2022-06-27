/*
 * WaterRoutes.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once
#include "Zone.h"

class DLL_LINKAGE WaterRoutes: public Modificator
{
public:
	WaterRoutes(Zone & zone, RmgMap & map, CMapGenerator & generator);
	
	void process() override;
	void init() override;
	
protected:
	RmgMap & map;
	CMapGenerator & generator;
	Zone & zone;
};
