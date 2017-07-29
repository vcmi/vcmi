/*
 * ObstacleGraphicsInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include <lib/serializer/JsonSerializeFormat.h>
#include "StdInc.h"
#include "ObstacleGraphicsInfo.h"

ObstacleGraphicsInfo::ObstacleGraphicsInfo()
{

}

ObstacleGraphicsInfo::ObstacleGraphicsInfo(std::string graphicsName, int32_t offsetX, int32_t offsetY)
{
	setGraphicsName(graphicsName);
	setOffsetGraphicsInX(offsetX);
	setOffsetGraphicsInY(offsetY);
}


void ObstacleGraphicsInfo::setGraphicsName(std::string name)
{
	graphicsName = name;
}

void ObstacleGraphicsInfo::setOffsetGraphicsInX(int32_t value)
{
	offsetGraphicsInX = value;
}

void ObstacleGraphicsInfo::setOffsetGraphicsInY(int32_t value)
{
	offsetGraphicsInY = value;
}

std::string ObstacleGraphicsInfo::getGraphicsName() const
{
	return graphicsName;
}

int32_t ObstacleGraphicsInfo::getOffsetGraphicsInX() const
{
	return offsetGraphicsInX;
}

int32_t ObstacleGraphicsInfo::getOffsetGraphicsInY() const
{
	return offsetGraphicsInY;
}

void ObstacleGraphicsInfo::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("offsetGraphicsInY", offsetGraphicsInY);
	handler.serializeInt("offsetGraphicsInX", offsetGraphicsInX);
	handler.serializeString("name", graphicsName);
	handler.serializeString("appearAnimation", appearAnimation);
}

const std::string & ObstacleGraphicsInfo::getAppearAnimation() const
{
	return appearAnimation;
}

void ObstacleGraphicsInfo::setAppearAnimation(const std::string & appearAnimation)
{
	ObstacleGraphicsInfo::appearAnimation = appearAnimation;
}

