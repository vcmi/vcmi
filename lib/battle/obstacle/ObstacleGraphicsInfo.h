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

class DLL_LINKAGE ObstacleGraphicsInfo
{
public:
	enum class GraphicsType
	{
		Appear, Default, Interaction, Disappear
	};
	
	ObstacleGraphicsInfo();
	ObstacleGraphicsInfo(std::string graphicsName, int32_t offsetX, int32_t offsetY);

	void setGraphics(const std::string &name, GraphicsType type = GraphicsType::Default);
	void setOffsetGraphicsInX(int32_t value);
	void setOffsetGraphicsInY(int32_t value);

	std::string getGraphics(GraphicsType type = GraphicsType::Default) const;
	int32_t getOffsetGraphicsInX() const;
	int32_t getOffsetGraphicsInY() const;
	
	void serializeJson(JsonSerializeFormat & handler);
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & offsetGraphicsInX;
		h & offsetGraphicsInY;
		h & graphics[0];
		h & graphics[1];
		h & graphics[2];
		h & graphics[3];
	}
private:
	int32_t offsetGraphicsInY = 0;
	int32_t offsetGraphicsInX = 0;
	std::string graphics[4];
};
