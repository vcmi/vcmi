#include "StdInc.h"
#include "CObstacleInstance.h"
#include "CHeroHandler.h"
#include "VCMI_Lib.h"

CObstacleInstance::CObstacleInstance()
{
	isAbsoluteObstacle = false;
}

const CObstacleInfo & CObstacleInstance::getInfo() const
{
	if(isAbsoluteObstacle)
		return VLC->heroh->absoluteObstacles[ID];

	return VLC->heroh->obstacles[ID];
}

std::vector<BattleHex> CObstacleInstance::getBlocked() const
{
	return getInfo().getBlocked(pos);
}