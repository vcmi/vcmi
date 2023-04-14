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
	assert(identifier.find(':') == std::string::npos);

	auto * info = new BattleFieldInfo(BattleField(index), identifier);

	info->graphics = json["graphics"].String();
	info->icon = json["icon"].String();
	info->name = json["name"].String();
	for(const auto & b : json["bonuses"].Vector())
	{
		auto bonus = JsonUtils::parseBonus(b);

		bonus->source = Bonus::TERRAIN_OVERLAY;
		bonus->sid = info->getIndex();
		bonus->duration = Bonus::ONE_BATTLE;

		info->bonuses.push_back(bonus);
	}

	info->isSpecial = json["isSpecial"].Bool();
	for(auto node : json["impassableHexes"].Vector())
		info->impassableHexes.emplace_back(node.Integer());

	return info;
}

std::vector<JsonNode> BattleFieldHandler::loadLegacyData()
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

std::string BattleFieldInfo::getJsonKey() const
{
	return identifier;
}

std::string BattleFieldInfo::getNameTextID() const
{
	return name;
}

std::string BattleFieldInfo::getNameTranslated() const
{
	return name; // TODO?
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
