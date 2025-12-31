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
#include "json/JsonBonus.h"

VCMI_LIB_NAMESPACE_BEGIN

std::shared_ptr<BattleFieldInfo> BattleFieldHandler::loadFromJson(const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index)
{
	assert(identifier.find(':') == std::string::npos);

	auto info = std::make_shared<BattleFieldInfo>(BattleField(index), identifier);

	info->modScope = scope;
	info->graphics = ImagePath::fromJson(json["graphics"]);
	info->icon = json["icon"].String();
	info->name = json["name"].String();
	for(const auto & b : json["bonuses"].Vector())
	{
		auto bonus = JsonUtils::parseBonus(b);

		bonus->source = BonusSource::TERRAIN_OVERLAY;
		bonus->sid = BonusSourceID(info->getId());
		bonus->duration = BonusDuration::PERMANENT;

		info->bonuses.push_back(bonus);
	}

	info->isSpecial = json["isSpecial"].Bool();
	for(auto node : json["impassableHexes"].Vector())
		info->impassableHexes.insert(node.Integer());

	info->openingSoundFilename = AudioPath::fromJson(json["openingSound"]);
	info->musicFilename = AudioPath::fromJson(json["music"]);

	return info;
}

std::vector<JsonNode> BattleFieldHandler::loadLegacyData()
{
	return std::vector<JsonNode>();
}

const std::vector<std::string> & BattleFieldHandler::getTypeNames() const
{
	static const auto types = std::vector<std::string> { "battlefield" };

	return types;
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
	return modScope + ':' + identifier;
}

std::string BattleFieldInfo::getModScope() const
{
	return modScope;
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
