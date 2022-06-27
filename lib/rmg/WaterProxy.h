//
//  WaterProxy.hpp
//  vcmi
//
//  Created by nordsoft on 27.06.2022.
//

#pragma once
#include "Zone.h"

class DLL_LINKAGE WaterProxy: public Modificator
{
public:
	//subclass to store disconnected parts of water zone
	struct Lake
	{
		Rmg::Area area; //water tiles
		std::map<int3, int> distanceMap; //distance map for lake
		std::map<int, Rmg::Tileset> reverseDistanceMap;
		std::map<TRmgTemplateZoneId, Rmg::Area> neighbourZones; //zones boardered. Area - internal part of area
		std::set<TRmgTemplateZoneId> keepConnections;
	};
	
	WaterProxy(Zone & zone, RmgMap & map, CMapGenerator & generator);
	
	bool waterKeepConnection(TRmgTemplateZoneId zoneA, TRmgTemplateZoneId zoneB);
	void waterRoute(Zone & dst);
	
	void process() override;
	void init() override;
	
protected:
	void collectLakes();
		
protected:
	RmgMap & map;
	CMapGenerator & generator;
	Zone & zone;
	
	std::vector<Lake> lakes; //disconnected parts of zone. Used to work with water zones
	std::map<int3, int> lakeMap; //map tile on lakeId which is position of lake in lakes array +1
};

