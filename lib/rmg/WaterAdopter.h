//
//  WaterAdopter.hpp
//  vcmi
//
//  Created by nordsoft on 27.06.2022.
//

#pragma once
#include "Zone.h"

class DLL_LINKAGE WaterAdopter: public Modificator
{
public:
	WaterAdopter(Zone & zone, RmgMap & map, CMapGenerator & generator);
	
	void process() override;
	
protected:
	RmgMap & map;
	CMapGenerator & generator;
	Zone & zone;
};
