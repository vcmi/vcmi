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
	
	void setWaterZone(TRmgTemplateZoneId water);
	
protected:
	void createWater(EWaterContent::EWaterContent waterContent);
	void reinitWaterZone(Zone & zone);
	
protected:
	RmgMap & map;
	CMapGenerator & generator;
	Zone & zone;
	
	TRmgTemplateZoneId waterZoneId;
	std::map<int3, int> distanceMap;
	std::map<int, Rmg::Tileset> reverseDistanceMap;
};
