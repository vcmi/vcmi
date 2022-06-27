/*
 * WaterProxy.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once
#include "Zone.h"

class DLL_LINKAGE WaterProxy: public Modificator
{
public:
	//subclass to store disconnected parts of water zone
	struct Lake
	{
		rmg::Area area; //water tiles
		std::map<int3, int> distanceMap; //distance map for lake
		std::map<int, rmg::Tileset> reverseDistanceMap;
		std::map<TRmgTemplateZoneId, rmg::Area> neighbourZones; //zones boardered. Area - part of land
		std::set<TRmgTemplateZoneId> keepConnections;
	};
	
	WaterProxy(Zone & zone, RmgMap & map, CMapGenerator & generator);
	
	bool waterKeepConnection(TRmgTemplateZoneId zoneA, TRmgTemplateZoneId zoneB);
	void waterRoute(Zone & dst);
	
	void process() override;
	void init() override;
	
protected:
	void collectLakes();
	
	bool placeShipyard(Zone & land, const Lake & lake, si32 guard);
	bool placeBoat(Zone & land, const Lake & lake);
		
protected:
	RmgMap & map;
	CMapGenerator & generator;
	Zone & zone;
	
	std::vector<Lake> lakes; //disconnected parts of zone. Used to work with water zones
	std::map<int3, int> lakeMap; //map tile on lakeId which is position of lake in lakes array +1
};

