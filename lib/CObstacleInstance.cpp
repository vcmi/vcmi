#include "StdInc.h"
#include "CObstacleInstance.h"
#include "CHeroHandler.h"
#include "VCMI_Lib.h"

CObstacleInstance::CObstacleInstance()
{
	obstacleType = USUAL;
	casterSide = -1;
	spellLevel = -1;
	turnsRemaining = -1;
	visibleForAnotherSide = -1;
}

const CObstacleInfo & CObstacleInstance::getInfo() const
{
	switch(obstacleType)
	{
	case ABSOLUTE_OBSTACLE:
		return VLC->heroh->absoluteObstacles[ID];
	case USUAL:
		return VLC->heroh->obstacles[ID];
	default:
		assert(0);
	}

}

std::vector<BattleHex> CObstacleInstance::getBlocked() const
{
	switch(obstacleType)
	{
	case ABSOLUTE_OBSTACLE:
	case USUAL:
		return getInfo().getBlocked(pos);
		//TODO Force Field
	}

	return std::vector<BattleHex>();
}

std::vector<BattleHex> CObstacleInstance::getAffectedTiles() const
{
	switch(obstacleType)
	{
	case QUICKSAND:
	case LAND_MINE:
		return std::vector<BattleHex>(1, pos);
		//TODO Fire Wall
	}
	return std::vector<BattleHex>();
}

bool CObstacleInstance::spellGenerated() const
{
	if(obstacleType == USUAL  ||  obstacleType == ABSOLUTE_OBSTACLE)
		return false;

	return true;
}

bool CObstacleInstance::visibleForSide(ui8 side) const
{
	switch(obstacleType)
	{
	case ABSOLUTE_OBSTACLE:
	case USUAL:
		return true;
	default:
		//we hide mines and not discovered quicksands
		return casterSide == side  ||  visibleForAnotherSide;
	}
}
