/*
 * ObstacleArea.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "../BattleHex.h"

class JsonSerializeFormat;

class DLL_LINKAGE ObstacleArea
{
public:
	void addField(BattleHex field);
	void setArea(std::vector<BattleHex> fields);
	void moveAreaToField(BattleHex offset);

	int getWidth() const;
	int getHeight() const;

	void setPosition(BattleHex hex);
	BattleHex getPosition() const;

	std::vector<BattleHex> getFields() const;
	ObstacleArea();
	ObstacleArea(std::vector<BattleHex> zone, BattleHex position);
	ObstacleArea(ObstacleArea zone, BattleHex position);
	
	void serializeJson(JsonSerializeFormat & handler);
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & area;
		h & position;
	}
private:
	std::vector<BattleHex> area;
	BattleHex position;
};
