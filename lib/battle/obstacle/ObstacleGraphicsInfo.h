/*
 * ObstacleGraphicsInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ObstacleState.h"

class DLL_LINKAGE ObstacleGraphicsInfo
{
public:
	ObstacleGraphicsInfo();
	ObstacleGraphicsInfo(std::string graphicsName, int32_t offsetX, int32_t offsetY);

	void setGraphics(const std::string &name, ObstacleState state = ObstacleState::Default);
	void setOffsetGraphicsInX(int32_t value);
	void setOffsetGraphicsInY(int32_t value);

	std::string getGraphics(ObstacleState state = ObstacleState::Default) const;
	int32_t getOffsetGraphicsInX() const;
	int32_t getOffsetGraphicsInY() const;
	
	void serializeJson(JsonSerializeFormat & handler);
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & offsetGraphicsInX;
		h & offsetGraphicsInY;
		h & graphics;
	}
private:
	int32_t offsetGraphicsInY = 0;
	int32_t offsetGraphicsInX = 0;
	std::string graphics[4];
};
