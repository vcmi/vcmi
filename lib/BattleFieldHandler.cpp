/*
 * BattleFieldHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include <vcmi/Entity.h>
#include "BattleFieldHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

BattleFieldInfo * BattleFieldHandler::loadFromJson(const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index)
{
	BattleFieldInfo * info = new BattleFieldInfo(BattleField(index), identifier);

	if(json["graphics"].getType() == JsonNode::JsonType::DATA_STRING)
	{
		info->graphics = json["graphics"].String();
	}

	if(json["icon"].getType() == JsonNode::JsonType::DATA_STRING)
	{
		info->icon = json["icon"].String();
	}

	if(json["name"].getType() == JsonNode::JsonType::DATA_STRING)
	{
		info->name = json["name"].String();
	}

	if(json["bonuses"].getType() == JsonNode::JsonType::DATA_VECTOR)
	{
		for(auto b : json["bonuses"].Vector())
		{
			auto bonus = JsonUtils::parseBonus(b);

			bonus->source = Bonus::TERRAIN_OVERLAY;
			bonus->sid = info->getIndex();
			bonus->duration = Bonus::ONE_BATTLE;

			info->bonuses.push_back(bonus);
		}
	}

	if(json["isSpecial"].getType() == JsonNode::JsonType::DATA_BOOL)
	{
		info->isSpecial = json["isSpecial"].Bool();
	}

	if(json["impassableHexes"].getType() == JsonNode::JsonType::DATA_VECTOR)
	{
		for(auto node : json["impassableHexes"].Vector())
			info->impassableHexes.push_back(BattleHex(node.Integer()));
	}


	return info;
}

std::vector<JsonNode> BattleFieldHandler::loadLegacyData(size_t dataSize)
{
	return std::vector<JsonNode>();
}

const std::vector<std::string> & BattleFieldHandler::getTypeNames() const
{
	static const  std::vector<std::string> types = std::vector<std::string> { "battlefield" };

	return types;
}

std::vector<bool> BattleFieldHandler::getDefaultAllowed() const
{
	return std::vector<bool>();
}

int32_t BattleFieldInfo::getIndex() const
{
	return battlefield.getNum();
}

int32_t BattleFieldInfo::getIconIndex() const
{
	return iconIndex;
}

const std::string & BattleFieldInfo::getName() const
{
	return name;
}

const std::string & BattleFieldInfo::getJsonKey() const
{
	return identifier;
}

void BattleFieldInfo::registerIcons(const IconRegistar & cb) const
{
	//cb(getIconIndex(), "BATTLEFIELD", icon);
}

BattleField BattleFieldInfo::getId() const
{
	return battlefield;
}

VCMI_LIB_NAMESPACE_END
