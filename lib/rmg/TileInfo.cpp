//
//  TileInfo.cpp
//  vcmi
//
//  Created by nordsoft on 22.06.2022.
//

#include "TileInfo.h"

CTileInfo::CTileInfo():nearestObjectDistance(float(INT_MAX)), terrain()
{
	occupied = ETileType::POSSIBLE; //all tiles are initially possible to place objects or passages
}

float CTileInfo::getNearestObjectDistance() const
{
	return nearestObjectDistance;
}

void CTileInfo::setNearestObjectDistance(float value)
{
	nearestObjectDistance = std::max<float>(0, value); //never negative (or unitialized)
}
bool CTileInfo::shouldBeBlocked() const
{
	return occupied == ETileType::BLOCKED;
}
bool CTileInfo::isBlocked() const
{
	return occupied == ETileType::BLOCKED || occupied == ETileType::USED;
}
bool CTileInfo::isPossible() const
{
	return occupied == ETileType::POSSIBLE;
}
bool CTileInfo::isFree() const
{
	return occupied == ETileType::FREE;
}

bool CTileInfo::isRoad() const
{
	return roadType != ROAD_NAMES[0];
}

bool CTileInfo::isUsed() const
{
	return occupied == ETileType::USED;
}
void CTileInfo::setOccupied(ETileType::ETileType value)
{
	occupied = value;
}

ETileType::ETileType CTileInfo::getTileType() const
{
	return occupied;
}

Terrain CTileInfo::getTerrainType() const
{
	return terrain;
}

void CTileInfo::setTerrainType(Terrain value)
{
	terrain = value;
}

void CTileInfo::setRoadType(const std::string & value)
{
	roadType = value;
	//	setOccupied(ETileType::FREE);
}
