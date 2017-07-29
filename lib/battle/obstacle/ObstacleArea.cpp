/*
 * ObstacleArea.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ObstacleArea.h"
#include "GameConstants.h"
#include "NetPacks.h"
#include "../../serializer/JsonDeserializer.h"
#include "../../serializer/JsonSerializer.h"
#include "../../serializer/JsonSerializeFormat.h"

void ObstacleArea::addField(BattleHex field)
{
	if(!vstd::contains(area, field))
		area.push_back(field);
}

void ObstacleArea::setArea(std::vector<BattleHex> fields)
{
	area.clear();
	for(auto &i : fields)
		addField(i);
}

int ObstacleArea::getWidth() const
{
	if(area.empty())
		return 0;
	int maxX = 0;
	int minX = GameConstants::BFIELD_WIDTH;
	for(auto i : area)
	{
		while(i < 0)
			i.hex += GameConstants::BFIELD_WIDTH;
		if(i.getX() > maxX)
			maxX = i.getX();
		if(i.getX() < minX)
			minX = i.getX();
	}
	return maxX - minX + 1;
}

int ObstacleArea::getHeight() const
{
	if(area.empty())
		return 0;
	BattleHex maxY = area.at(0);
	BattleHex minY = area.at(0);
	for(auto i : area)
	{
		if(i > maxY)
			maxY = i;
		if(i < minY)
			minY = i;
	}
	while(minY < 0)
	{
		maxY.hex += GameConstants::BFIELD_WIDTH;
		minY.hex += GameConstants::BFIELD_WIDTH;
	}
	return maxY.getY() - minY.getY() + 1;
}

void ObstacleArea::setPosition(BattleHex hex)
{
	position = hex;
}

BattleHex ObstacleArea::getPosition() const
{
	return position;
}

std::vector<BattleHex> ObstacleArea::getFields() const
{
	return area;
}

void ObstacleArea::moveAreaToField(BattleHex offset)
{
	if(position == offset)
		return;
	for(auto &field : area)
	{
		field.hex += offset.hex - position;
		if(position.getY() % 2 != offset.getY() % 2)
		{
			if(offset.getY() % 2 == 0 && field.getY() % 2 == 1)
				field.moveInDirection(BattleHex::RIGHT);
			if(offset.getY() % 2 == 1 && field.getY() % 2 == 0)
				field.moveInDirection(BattleHex::LEFT);
		}
	}
	setPosition(offset);
}

ObstacleArea::ObstacleArea()
{
	setPosition(0);
}

ObstacleArea::ObstacleArea(std::vector<BattleHex> zone, BattleHex position)
{
	setArea(zone);
	setPosition(0);
	moveAreaToField(position);
}

ObstacleArea::ObstacleArea(ObstacleArea zone, BattleHex position)
{
	setArea(zone.getFields());
	setPosition(zone.getPosition());
	moveAreaToField(position);
}

void ObstacleArea::serializeJson(JsonSerializeFormat &handler)
{
	handler.serializeInt("pos", position);
	
	
	if(handler.saving)
	{
		JsonArraySerializer outer = handler.enterArray("area");
		outer.syncSize(area, JsonNode::JsonType::DATA_VECTOR);
		for(size_t outerIndex = 0; outerIndex < outer.size(); outerIndex++)
		{
			outer.serializeInt(outerIndex, area.at(outerIndex));
		}
	}
	else
	{
		JsonArraySerializer customSizeJson = handler.enterArray("area");
		for(size_t index = 0; index < customSizeJson.size(); index++)
		{
			BattleHex hex;
			customSizeJson.serializeInt(index, hex);
			addField(hex);
		}
	}
}
