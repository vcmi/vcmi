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

#include "../lib/HeroBonus.h"
#include "GameConstants.h"
#include "IHandlerBase.h"

class DLL_LINKAGE CSkill // secondary skill
{
protected:
	struct LevelInfo
	{
		std::string description; //descriptions of spell for skill level
		std::vector<std::shared_ptr<Bonus>> effects;

		LevelInfo();
		~LevelInfo();

		template <typename Handler> void serialize(Handler & h, const int version)
		{
			h & description;
			h & effects;
		}
	};

	std::vector<LevelInfo> levels; // bonuses provided by basic, advanced and expert level

public:
	CSkill(SecondarySkill id = SecondarySkill::DEFAULT);
	~CSkill();

	void addNewBonus(const std::shared_ptr<Bonus> & b, int level);
	void setDescription(const std::string & desc, int level);
	const std::vector<std::shared_ptr<Bonus>> & getBonus(int level) const;
	const std::string & getDescription(int level) const;
	std::string toString() const;

	SecondarySkill id;
	std::string identifier;
	std::string name; //as displayed in GUI

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & id;
		h & identifier;
		h & name;
		h & levels;
	}

	friend class CSkillHandler;
	friend DLL_LINKAGE std::ostream & operator<<(std::ostream & out, const CSkill & skill);
	friend DLL_LINKAGE std::ostream & operator<<(std::ostream & out, const CSkill::LevelInfo & info);
};

class DLL_LINKAGE CSkillHandler: public CHandlerBase<SecondarySkill, CSkill>
{
public:
	CSkillHandler();
	virtual ~CSkillHandler();

	///IHandler base
	std::vector<JsonNode> loadLegacyData(size_t dataSize) override;
	void afterLoadFinalization() override;
	void beforeValidate(JsonNode & object) override;

	std::vector<bool> getDefaultAllowed() const override;
	const std::string getTypeName() const override;

	const std::string & skillInfo(int skill, int level) const;
	const std::string & skillName(int skill) const;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & objects;
	}

protected:
	CSkill * loadFromJson(const JsonNode & json, const std::string & identifier, size_t index) override;
};
