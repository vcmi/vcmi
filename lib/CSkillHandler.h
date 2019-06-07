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

#include "../lib/HeroBonus.h"
#include "GameConstants.h"
#include "IHandlerBase.h"

class DLL_LINKAGE CSkill : public Skill
{
public:
	struct LevelInfo
	{
		std::string description; //descriptions of spell for skill level
		std::string iconSmall;
		std::string iconMedium;
		std::string iconLarge;
		std::vector<std::shared_ptr<Bonus>> effects;

		LevelInfo();
		~LevelInfo();

		template <typename Handler> void serialize(Handler & h, const int version)
		{
			h & description;
			if(version >= 785)
			{
				h & iconSmall;
				h & iconMedium;
				h & iconLarge;
			}
			h & effects;
		}
	};

private:
	std::vector<LevelInfo> levels; // bonuses provided by basic, advanced and expert level
	void addNewBonus(const std::shared_ptr<Bonus> & b, int level);

public:
	CSkill(SecondarySkill id = SecondarySkill::DEFAULT, std::string identifier = "default");
	~CSkill();

	int32_t getIndex() const override;
	const std::string & getName() const override;
	const std::string & getJsonKey() const override;
	void registerIcons(const IconRegistar & cb) const override;
	SecondarySkill getId() const override;

	const LevelInfo & at(int level) const;
	LevelInfo & at(int level);

	std::string toString() const;

	SecondarySkill id;
	std::string identifier;
	std::string name; //as displayed in GUI
	std::array<si32, 2> gainChance; // gainChance[0/1] = default gain chance on level-up for might/magic heroes

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & id;
		h & identifier;
		h & name;
		if(version >= 785)
		{
			h & gainChance;
		}
		h & levels;
	}

	friend class CSkillHandler;
	friend DLL_LINKAGE std::ostream & operator<<(std::ostream & out, const CSkill & skill);
	friend DLL_LINKAGE std::ostream & operator<<(std::ostream & out, const CSkill::LevelInfo & info);
};

class DLL_LINKAGE CSkillHandler: public CHandlerBase<SecondarySkill, CSkill>, public SkillService
{
public:
	CSkillHandler();
	virtual ~CSkillHandler();

	///IHandler base
	std::vector<JsonNode> loadLegacyData(size_t dataSize) override;
	void afterLoadFinalization() override;
	void beforeValidate(JsonNode & object) override;

	std::vector<bool> getDefaultAllowed() const override;
	const std::vector<std::string> & getTypeNames() const override;

	const std::string & skillInfo(int skill, int level) const;
	const std::string & skillName(int skill) const;

	const Entity * getBaseByIndex(const int32_t index) const override;

	const Skill * getById(const SecondarySkill & id) const override;
	const Skill * getByIndex(const int32_t index) const override;

	void forEachBase(const std::function<void(const Entity * entity, bool & stop)> & cb) const override;
	void forEach(const std::function<void(const Skill * entity, bool & stop)> & cb) const override;

	///json serialization helpers
	static si32 decodeSkill(const std::string & identifier);
	static std::string encodeSkill(const si32 index);
	static std::string encodeSkillWithType(const si32 index);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & objects;
	}


protected:
	CSkill * loadFromJson(const JsonNode & json, const std::string & identifier, size_t index) override;
};
