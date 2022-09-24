/*
 * TileInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"
#include "../Terrain.h"

VCMI_LIB_NAMESPACE_BEGIN

class TileInfo
{
public:
	
	TileInfo();
	
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

VCMI_LIB_NAMESPACE_END
