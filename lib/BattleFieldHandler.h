/*
 * BattleFieldHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/EntityService.h>
#include <vcmi/Entity.h>
#include "HeroBonus.h"
#include "GameConstants.h"
#include "IHandlerBase.h"
#include "battle/BattleHex.h"

VCMI_LIB_NAMESPACE_BEGIN

class BattleFieldInfo : public EntityT<BattleField>
{
public:
	BattleField battlefield;
	std::vector<std::shared_ptr<Bonus>> bonuses;
	bool isSpecial;
	std::string graphics;
	std::string name;
	std::string identifier;
	std::string icon;
	si32 iconIndex;
	std::vector<BattleHex> impassableHexes;

	BattleFieldInfo() 
		: BattleFieldInfo(BattleField::NONE, "")
	{
	}

	BattleFieldInfo(BattleField battlefield, std::string identifier)
		:bonuses(), isSpecial(false), battlefield(battlefield), identifier(identifier), graphics(), icon(), iconIndex(battlefield.getNum()), impassableHexes(), name(identifier)
	{
	}

	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	const std::string & getName() const override;
	const std::string & getJsonKey() const override;
	void registerIcons(const IconRegistar & cb) const override;
	BattleField getId() const override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & name;
		h & identifier;
		h & isSpecial;
		h & graphics;
		h & icon;
		h & iconIndex;
		h & battlefield;
		h & impassableHexes;

	}
};

class DLL_LINKAGE BattleFieldService : public EntityServiceT<BattleField, BattleFieldInfo>
{
public:
};

class BattleFieldHandler : public CHandlerBase<BattleField, BattleFieldInfo, BattleFieldInfo, BattleFieldService>
{
public:
	virtual BattleFieldInfo * loadFromJson(
		const std::string & scope,
		const JsonNode & json,
		const std::string & identifier,
		size_t index) override;

	virtual const std::vector<std::string> & getTypeNames() const override;
	virtual std::vector<JsonNode> loadLegacyData(size_t dataSize) override;
	virtual std::vector<bool> getDefaultAllowed() const override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & objects;
	}
};

VCMI_LIB_NAMESPACE_END
