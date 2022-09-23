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
	TTerrain getTerrainType() const;
	ETileType::ETileType getTileType() const;
	void setTerrainType(TTerrain value);
	
	void setRoadType(TRoad type);
private:
	float nearestObjectDistance;
	ETileType::ETileType occupied;
	TTerrain terrain;
	TRoad roadType;
};
