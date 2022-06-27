//
//  WaterRoutes.hpp
//  vcmi
//
//  Created by nordsoft on 27.06.2022.
//

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
