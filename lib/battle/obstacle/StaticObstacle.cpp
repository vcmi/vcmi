/*
 * StaticObstacle.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "StaticObstacle.h"

StaticObstacle::StaticObstacle()
{
}

StaticObstacle::StaticObstacle(const ObstacleJson & info, int16_t position)
{
	if(!info.randomPosition())
		position = info.getPosition();
	setArea(ObstacleArea(info.getArea(), position));
	canBeRemovedBySpell = info.canBeRemovedBySpell();
	setGraphicsInfo(info.getGraphicsInfo());
}

StaticObstacle::StaticObstacle(const ObstacleJson * info, int16_t position)
{
	if(!info->randomPosition())
		position = info->getPosition();
	setArea(ObstacleArea(info->getArea(), position));
	canBeRemovedBySpell = info->canBeRemovedBySpell();
	setGraphicsInfo(info->getGraphicsInfo());
}

StaticObstacle::~StaticObstacle()
{

}

ObstacleType StaticObstacle::getType() const
{
	return ObstacleType::STATIC;
}


bool StaticObstacle::visibleForSide(ui8 side, bool hasNativeStack) const
{
	return true;
}

void StaticObstacle::battleTurnPassed()
{

}

bool StaticObstacle::blocksTiles() const
{
	return true;
}

bool StaticObstacle::stopsMovement() const
{
	return false;
}

bool StaticObstacle::triggersEffects() const
{
	return false;
}
