/*
 * CHeroHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/HeroClass.h>
#include <vcmi/HeroClassService.h>
#include <vcmi/HeroType.h>
#include <vcmi/HeroTypeService.h>

#include "../lib/ConstTransitivePtr.h"
#include "GameConstants.h"
#include "HeroBonus.h"
#include "IHandlerBase.h"
#include "Terrain.h"

VCMI_LIB_NAMESPACE_BEGIN

class CHeroClass;
class CGHeroInstance;
struct BattleHex;
class JsonNode;
class CRandomGenerator;
class JsonSerializeFormat;
class BattleField;

struct SSpecialtyInfo
{	si32 type;
	si32 val;
	si32 subtype;
	si32 additionalinfo;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & type;
		h & val;
		h & subtype;
		h & additionalinfo;
	}
};

struct SSpecialtyBonus
/// temporary hold
{
	ui8 growsWithLevel;
	BonusList bonuses;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & growsWithLevel;
		h & bonuses;
	}
};

class DLL_LINKAGE CHero : public HeroType
{
public:
	struct InitialArmyStack
	{
		ui32 minAmount;
		ui32 maxAmount;
		CreatureID creature;

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & minAmount;
			h & maxAmount;
			h & creature;
		}
	};
	std::string identifier;
	HeroTypeID ID;
	si32 imageIndex;

	std::vector<InitialArmyStack> initialArmy;

	CHeroClass * heroClass;
	std::vector<std::pair<SecondarySkill, ui8> > secSkillsInit; //initial secondary skills; first - ID of skill, second - level of skill (1 - basic, 2 - adv., 3 - expert)
	std::vector<SSpecialtyInfo> specDeprecated;
	std::vector<SSpecialtyBonus> specialtyDeprecated;
	BonusList specialty;
	std::set<SpellID> spells;
	bool haveSpellBook;
	bool special; // hero is special and won't be placed in game (unless preset on map), e.g. campaign heroes
	ui8 sex; // default sex: 0=male, 1=female

	/// Localized texts
	std::string name; //name of hero
	std::string biography;
	std::string specName;
	std::string specDescr;
	std::string specTooltip;

	/// Graphics
	std::string iconSpecSmall;
	std::string iconSpecLarge;
	std::string portraitSmall;
	std::string portraitLarge;
	std::string battleImage;

	CHero();
	virtual ~CHero();

	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	const std::string & getName() const override;
	const std::string & getJsonKey() const override;
	HeroTypeID getId() const override;
	void registerIcons(const IconRegistar & cb) const override;

	void updateFrom(const JsonNode & data);
	void serializeJson(JsonSerializeFormat & handler);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ID;
		h & imageIndex;
		h & initialArmy;
		h & heroClass;
		h & secSkillsInit;
		h & specialty;
		h & spells;
		h & haveSpellBook;
		h & sex;
		h & special;
		h & name;
		h & biography;
		h & specName;
		h & specDescr;
		h & specTooltip;
		h & iconSpecSmall;
		h & iconSpecLarge;
		h & portraitSmall;
		h & portraitLarge;
		h & identifier;
		h & battleImage;
	}
};

// convert deprecated format
std::vector<std::shared_ptr<Bonus>> SpecialtyInfoToBonuses(const SSpecialtyInfo & spec, int sid = 0);
std::vector<std::shared_ptr<Bonus>> SpecialtyBonusToBonuses(const SSpecialtyBonus & spec, int sid = 0);

class DLL_LINKAGE CHeroClass : public HeroClass
{
public:
	enum EClassAffinity
	{
		MIGHT,
		MAGIC
	};

	std::string identifier;
	std::string name; // translatable
	//double aggression; // not used in vcmi.
	TFaction faction;
	HeroClassID id;
	ui8 affinity; // affinity, using EClassAffinity enum

	// default chance for hero of specific class to appear in tavern, if field "tavern" was not set
	// resulting chance = sqrt(town.chance * heroClass.chance)
	ui32 defaultTavernChance;

	CCreature * commander;

	std::vector<int> primarySkillInitial;  // initial primary skills
	std::vector<int> primarySkillLowLevel; // probability (%) of getting point of primary skill when getting level
	std::vector<int> primarySkillHighLevel;// same for high levels (> 10)

	std::vector<int> secSkillProbability; //probabilities of gaining secondary skills (out of 112), in id order

	std::map<TFaction, int> selectionProbability; //probability of selection in towns

	std::string imageBattleMale;
	std::string imageBattleFemale;
	std::string imageMapMale;
	std::string imageMapFemale;

	CHeroClass();

	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	const std::string & getName() const override;
	const std::string & getJsonKey() const override;
	HeroClassID getId() const override;
	void registerIcons(const IconRegistar & cb) const override;

	bool isMagicHero() const;
	SecondarySkill chooseSecSkill(const std::set<SecondarySkill> & possibles, CRandomGenerator & rand) const; //picks secondary skill out from given possibilities

	void updateFrom(const JsonNode & data);
	void serializeJson(JsonSerializeFormat & handler);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & identifier;
		h & name;
		h & faction;
		h & id;
		h & defaultTavernChance;
		h & primarySkillInitial;
		h & primarySkillLowLevel;
		h & primarySkillHighLevel;
		h & secSkillProbability;
		h & selectionProbability;
		h & affinity;
		h & commander;
		h & imageBattleMale;
		h & imageBattleFemale;
		h & imageMapMale;
		h & imageMapFemale;

		if(!h.saving)
		{
			for(auto i = 0; i < secSkillProbability.size(); i++)
				if(secSkillProbability[i] < 0)
					secSkillProbability[i] = 0;
	}
	}
	EAlignment::EAlignment getAlignment() const;
};

class DLL_LINKAGE CHeroClassHandler : public CHandlerBase<HeroClassID, HeroClass, CHeroClass, HeroClassService>
{
	void fillPrimarySkillData(const JsonNode & node, CHeroClass * heroClass, PrimarySkill::PrimarySkill pSkill);
public:
	std::vector<JsonNode> loadLegacyData(size_t dataSize) override;

	void afterLoadFinalization() override;

	std::vector<bool> getDefaultAllowed() const override;

	~CHeroClassHandler();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & objects;
	}

protected:
	const std::vector<std::string> & getTypeNames() const override;
	CHeroClass * loadFromJson(const std::string & scope, const JsonNode & node, const std::string & identifier, size_t index) override;

};

class DLL_LINKAGE CHeroHandler : public CHandlerBase<HeroTypeID, HeroType, CHero, HeroTypeService>
{
	/// expPerLEvel[i] is amount of exp needed to reach level i;
	/// consists of 201 values. Any higher levels require experience larger that ui64 can hold
	std::vector<ui64> expPerLevel;

	/// helpers for loading to avoid huge load functions
	void loadHeroArmy(CHero * hero, const JsonNode & node);
	void loadHeroSkills(CHero * hero, const JsonNode & node);
	void loadHeroSpecialty(CHero * hero, const JsonNode & node);

	void loadExperience();
	void loadBallistics();
	void loadTerrains();

public:
	CHeroClassHandler classes;

	//default costs of going through terrains. -1 means terrain is impassable
	std::map<TerrainId, int> terrCosts;

	struct SBallisticsLevelInfo
	{
		ui8 keep, tower, gate, wall; //chance to hit in percent (eg. 87 is 87%)
		ui8 shots; //how many shots we have
		ui8 noDmg, oneDmg, twoDmg; //chances for shot dealing certain dmg in percent (eg. 87 is 87%); must sum to 100
		ui8 sum; //I don't know if it is useful for anything, but it's in config file
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & keep;
			h & tower;
			h & gate;
			h & wall;
			h & shots;
			h & noDmg;
			h & oneDmg;
			h & twoDmg;
			h & sum;
		}
	};
	std::vector<SBallisticsLevelInfo> ballistics; //info about ballistics ability per level; [0] - none; [1] - basic; [2] - adv; [3] - expert

	ui32 level(ui64 experience) const; //calculates level corresponding to given experience amount
	ui64 reqExp(ui32 level) const; //calculates experience required for given level

	std::vector<JsonNode> loadLegacyData(size_t dataSize) override;

	void beforeValidate(JsonNode & object) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;
	void afterLoadFinalization() override;

	CHeroHandler();
	~CHeroHandler();

	std::vector<bool> getDefaultAllowed() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & classes;
		h & objects;
		h & expPerLevel;
		h & ballistics;
		h & terrCosts;
	}

protected:
	const std::vector<std::string> & getTypeNames() const override;
	CHero * loadFromJson(const std::string & scope, const JsonNode & node, const std::string & identifier, size_t index) override;
};

VCMI_LIB_NAMESPACE_END
