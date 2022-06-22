//
//  TileInfo.hpp
//  vcmi
//
//  Created by nordsoft on 22.06.2022.
//

#pragma once

#include "../GameConstants.h"
#include "../Terrain.h"

class DLL_LINKAGE CTileInfo
{
public:
	
	CTileInfo();
	
	float getNearestObjectDistance() const;
	void setNearestObjectDistance(float value);
	bool isBlocked() const;
	bool shouldBeBlocked() const;
	bool isPossible() const;
	bool isFree() const;
	bool isUsed() const;
	bool isRoad() const;
	void setOccupied(ETileType::ETileType value);
	Terrain getTerrainType() const;
	ETileType::ETileType getTileType() const;
	void setTerrainType(Terrain value);
	
	void setRoadType(const std::string & value);
private:
	float nearestObjectDistance;
	ETileType::ETileType occupied;
	Terrain terrain;
	std::string roadType;
};
