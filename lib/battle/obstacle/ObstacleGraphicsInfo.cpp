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
	setGraphics(graphicsName);
	setOffsetGraphicsInX(offsetX);
	setOffsetGraphicsInY(offsetY);
}


void ObstacleGraphicsInfo::setGraphics(const std::string &name, ObstacleState state)
{
	switch(state)
	{
		case ObstacleState::Appear:
			graphics[0] = name;
			break;
		case ObstacleState::Interaction:
			graphics[2] = name;
			break;
		case ObstacleState::Disappear:
			graphics[3] = name;
			break;
		default:
			graphics[1] = name;
			break;
	}
}

void ObstacleGraphicsInfo::setOffsetGraphicsInX(int32_t value)
{
	offsetGraphicsInX = value;
}

void ObstacleGraphicsInfo::setOffsetGraphicsInY(int32_t value)
{
	offsetGraphicsInY = value;
}

std::string ObstacleGraphicsInfo::getGraphics(ObstacleState state) const
{
	switch(state)
	{
		case ObstacleState::Appear:
			return graphics[0];
		case ObstacleState::Interaction:
			return graphics[2];
		case ObstacleState::Disappear:
			return graphics[3];
		default:
			return graphics[1];
	}
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
	handler.serializeString("appear", graphics[0]);
	handler.serializeString("default", graphics[1]);
	handler.serializeString("interaction", graphics[2]);
	handler.serializeString("disappear", graphics[3]);
}