/*
 * TileInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "TileInfo.h"

VCMI_LIB_NAMESPACE_BEGIN

TileInfo::TileInfo():nearestObjectDistance(float(INT_MAX)), terrain()
{
	occupied = ETileType::POSSIBLE; //all tiles are initially possible to place objects or passages
}

float TileInfo::getNearestObjectDistance() const
{
	return nearestObjectDistance;
}

void TileInfo::setNearestObjectDistance(float value)
{
	nearestObjectDistance = std::max<float>(0, value); //never negative (or unitialized)
}
bool TileInfo::shouldBeBlocked() const
{
	return occupied == ETileType::BLOCKED;
}
bool TileInfo::isBlocked() const
{
	return occupied == ETileType::BLOCKED || occupied == ETileType::USED;
}
bool TileInfo::isPossible() const
{
	return occupied == ETileType::POSSIBLE;
}
bool TileInfo::isFree() const
{
	return occupied == ETileType::FREE;
}

bool TileInfo::isRoad() const
{
	return roadType != Road::NO_ROAD;
}

bool TileInfo::isUsed() const
{
	return occupied == ETileType::USED;
}
void TileInfo::setOccupied(ETileType::ETileType value)
{
	occupied = value;
}

ETileType::ETileType TileInfo::getTileType() const
{
	return occupied;
}

TerrainId TileInfo::getTerrainType() const
{
	return terrain;
}

void TileInfo::setTerrainType(TerrainId type)
{
	terrain = type;
}

void TileInfo::setRoadType(RoadId type)
{
	roadType = type;
	//	setOccupied(ETileType::FREE);
}

VCMI_LIB_NAMESPACE_END
