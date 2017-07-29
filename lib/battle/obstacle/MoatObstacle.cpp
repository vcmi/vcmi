/*
 * MoatObstacle.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "MoatObstacle.h"

ObstacleType MoatObstacle::getType() const
{
	return ObstacleType::MOAT;
}

bool MoatObstacle::blocksTiles() const
{
	return false;
}

bool MoatObstacle::stopsMovement() const
{
	return true;
}

MoatObstacle::MoatObstacle()
{

}

MoatObstacle::MoatObstacle(ObstacleJson info, int16_t position)  : StaticObstacle(info, position)
{
	setDamage(info.getDamage());
}

int32_t MoatObstacle::getDamage() const
{
	return damage;
}

void MoatObstacle::setDamage(int32_t value)
{
	damage = value;
}
