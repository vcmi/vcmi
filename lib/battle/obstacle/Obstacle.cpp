/*
 * Obstacle.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "Obstacle.h"

ObstacleArea Obstacle::getArea() const
{
	return area;
}

void Obstacle::setArea(ObstacleArea zone)
{
	area = zone;
}

ObstacleGraphicsInfo Obstacle::getGraphicsInfo() const
{
	return graphicsInfo;
}

void Obstacle::setGraphicsInfo(ObstacleGraphicsInfo info)
{
	graphicsInfo = info;
}
