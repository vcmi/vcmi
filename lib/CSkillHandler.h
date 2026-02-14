/*
 * CSkillHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/Skill.h>
#include <vcmi/SkillService.h>

#include "../lib/bonuses/Bonus.h"
#include "GameConstants.h"
#include "IHandlerBase.h"

VCMI_LIB_NAMESPACE_BEGIN

class JsonSerializeFormat;

class DLL_LINKAGE CSkill : public Skill
{
public:
	struct LevelInfo
	{
		std::string iconSmall;
		std::string iconMedium;
		std::string iconLarge;
		std::string scenarioBonus;
		std::vector<std::shared_ptr<Bonus>> effects;
	};

private:
	std::vector<LevelInfo> levels; // bonuses provided by basic, advanced and expert level
	void addNewBonus(const std::shared_ptr<Bonus> & b, int level);
	int32_t getIconIndex() const override;

	SecondarySkill id;
	std::string modScope;
	std::string identifier;

public:
	CSkill(const SecondarySkill & id = SecondarySkill::NONE, std::string identifier = "default", bool obligatoryMajor = false, bool obligatoryMinor = false);
	~CSkill() = default;

	enum class Obligatory : ui8
	{
		MAJOR = 0,
		MINOR = 1,
	};

	int32_t getIndex() const override;
	int32_t getIconIndex(uint8_t skillMasterLevel) const;
	std::string getJsonKey() const override;
	std::string getModScope() const override;
	void registerIcons(const IconRegistar & cb) const override;
	SecondarySkill getId() const override;

	std::string getNameTextID() const override;
	std::string getNameTranslated() const override;

	std::string getDescriptionTextID(int level) const override;
	std::string getDescriptionTranslated(int level) const override;

	const LevelInfo & at(int level) const;
	LevelInfo & at(int level);

	std::string toString() const;
	bool obligatory(Obligatory val) const { return val == Obligatory::MAJOR ? obligatoryMajor : obligatoryMinor; };

	std::array<si32, 2> gainChance; // gainChance[0/1] = default gain chance on level-up for might/magic heroes

	/// Bonuses that should be given to hero that specializes in this skill
	std::vector<std::shared_ptr<const Bonus>> specialtyTargetBonuses;

	void updateFrom(const JsonNode & data);
	void serializeJson(JsonSerializeFormat & handler);

	bool onlyOnWaterMap;
	bool special;
	bool banInMapObjects;

	friend class CSkillHandler;
	friend DLL_LINKAGE std::ostream & operator<<(std::ostream & out, const CSkill & skill);
	friend DLL_LINKAGE std::ostream & operator<<(std::ostream & out, const CSkill::LevelInfo & info);
private:
	bool obligatoryMajor;
	bool obligatoryMinor;
};

class DLL_LINKAGE CSkillHandler: public CHandlerBase<SecondarySkill, Skill, CSkill, SkillService>
{
public:
	///IHandler base
	std::vector<JsonNode> loadLegacyData() override;
	void afterLoadFinalization() override;
	void beforeValidate(JsonNode & object) override;

	std::set<SecondarySkill> getDefaultAllowed() const;

protected:
	const std::vector<std::string> & getTypeNames() const override;
	std::shared_ptr<CSkill> loadFromJson(const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index) override;
};

VCMI_LIB_NAMESPACE_END
