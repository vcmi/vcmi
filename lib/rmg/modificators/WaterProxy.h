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
#include "../Zone.h"

VCMI_LIB_NAMESPACE_BEGIN

struct RouteInfo
{
	rmg::Area blocked;
	int3 visitable;
	int3 boarding;
	rmg::Area water;
};

class WaterProxy: public Modificator
{
public:
	MODIFICATOR(WaterProxy);
	
	//subclass to store disconnected parts of water zone
	struct Lake
	{
		rmg::Area area; //water tiles
		std::map<int3, int> distanceMap; //distance map for lake
		std::map<int, rmg::Tileset> reverseDistanceMap;
		std::map<TRmgTemplateZoneId, rmg::Area> neighbourZones; //zones boardered. Area - part of land
		std::set<TRmgTemplateZoneId> keepConnections;
		std::set<TRmgTemplateZoneId> keepRoads;
	};
		
	bool waterKeepConnection(const rmg::ZoneConnection & connection, bool createRoad);
	RouteInfo waterRoute(Zone & dst);
	
	void process() override;
	void init() override;
	char dump(const int3 &) override;
	const std::vector<Lake> & getLakes() const;
	
protected:
	void collectLakes();
	
	bool placeShipyard(Zone & land, const Lake & lake, si32 guard, bool createRoad, RouteInfo & info);
	bool placeBoat(Zone & land, const Lake & lake, bool createRoad, RouteInfo & info);

protected:
	std::vector<Lake> lakes; //disconnected parts of zone. Used to work with water zones
	std::map<int3, int> lakeMap; //map tile on lakeId which is position of lake in lakes array +1
};


VCMI_LIB_NAMESPACE_END
