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
	TTerrainId getTerrainType() const;
	ETileType::ETileType getTileType() const;
	void setTerrainType(TTerrainId value);
	
	void setRoadType(TRoadId type);
private:
	float nearestObjectDistance;
	ETileType::ETileType occupied;
	TTerrainId terrain;
	TRoadId roadType;
};

VCMI_LIB_NAMESPACE_END
