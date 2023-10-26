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
	void setOccupied(ETileType value);
	TerrainId getTerrainType() const;
	ETileType getTileType() const;
	void setTerrainType(TerrainId value);
	
	void setRoadType(RoadId type);
private:
	float nearestObjectDistance;
	ETileType occupied;
	TerrainId terrain;
	RoadId roadType;
};

VCMI_LIB_NAMESPACE_END
