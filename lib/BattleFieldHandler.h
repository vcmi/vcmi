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
#include "bonuses/Bonus.h"
#include "GameConstants.h"
#include "IHandlerBase.h"
#include "battle/BattleHex.h"
#include "filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN

class BattleFieldInfo : public EntityT<BattleField>
{
public:
	BattleField battlefield;
	std::vector<std::shared_ptr<Bonus>> bonuses;
	bool isSpecial;
	ImagePath graphics;
	std::string name;
	std::string modScope;
	std::string identifier;
	std::string icon;
	si32 iconIndex;
	std::vector<BattleHex> impassableHexes;

	BattleFieldInfo() 
		: BattleFieldInfo(BattleField::NONE, "")
	{
	}

	BattleFieldInfo(BattleField battlefield, std::string identifier):
		isSpecial(false),
		battlefield(battlefield),
		identifier(identifier),
		iconIndex(battlefield.getNum()),
		name(identifier)
	{
	}

	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	std::string getJsonKey() const override;
	std::string getNameTextID() const override;
	std::string getNameTranslated() const override;
	void registerIcons(const IconRegistar & cb) const override;
	BattleField getId() const override;
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

	const std::vector<std::string> & getTypeNames() const override;
	std::vector<JsonNode> loadLegacyData() override;
};

VCMI_LIB_NAMESPACE_END
