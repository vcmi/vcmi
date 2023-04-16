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

VCMI_LIB_NAMESPACE_BEGIN

class CHeroClass;
class CGHeroInstance;
struct BattleHex;
class JsonNode;
class CRandomGenerator;
class JsonSerializeFormat;
class BattleField;

enum class EHeroGender : uint8_t
{
	MALE = 0,
	FEMALE = 1,
	DEFAULT = 0xff // from h3m, instance has same gender as hero type
};

class DLL_LINKAGE CHero : public HeroType
{
	friend class CHeroHandler;

	HeroTypeID ID;
	std::string identifier;
	std::string modScope;

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
	si32 imageIndex = 0;

	std::vector<InitialArmyStack> initialArmy;

	CHeroClass * heroClass{};
	std::vector<std::pair<SecondarySkill, ui8> > secSkillsInit; //initial secondary skills; first - ID of skill, second - level of skill (1 - basic, 2 - adv., 3 - expert)
	BonusList specialty;
	std::set<SpellID> spells;
	bool haveSpellBook = false;
	bool special = false; // hero is special and won't be placed in game (unless preset on map), e.g. campaign heroes
	EHeroGender gender = EHeroGender::MALE; // default sex: 0=male, 1=female

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
	std::string getJsonKey() const override;
	HeroTypeID getId() const override;
	void registerIcons(const IconRegistar & cb) const override;

	std::string getNameTranslated() const override;
	std::string getBiographyTranslated() const override;
	std::string getSpecialtyNameTranslated() const override;
	std::string getSpecialtyDescriptionTranslated() const override;
	std::string getSpecialtyTooltipTranslated() const override;

	std::string getNameTextID() const override;
	std::string getBiographyTextID() const override;
	std::string getSpecialtyNameTextID() const override;
	std::string getSpecialtyDescriptionTextID() const override;
	std::string getSpecialtyTooltipTextID() const override;

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
		h & gender;
		h & special;
		h & iconSpecSmall;
		h & iconSpecLarge;
		h & portraitSmall;
		h & portraitLarge;
		h & identifier;
		h & modScope;
		h & battleImage;
	}
};

class DLL_LINKAGE CHeroClass : public HeroClass
{
	friend class CHeroClassHandler;
	HeroClassID id; // use getId instead
	std::string modScope;
	std::string identifier; // use getJsonKey instead

public:
	enum EClassAffinity
	{
		MIGHT,
		MAGIC
	};

	//double aggression; // not used in vcmi.
	FactionID faction;
	ui8 affinity; // affinity, using EClassAffinity enum

	// default chance for hero of specific class to appear in tavern, if field "tavern" was not set
	// resulting chance = sqrt(town.chance * heroClass.chance)
	ui32 defaultTavernChance;

	CCreature * commander;

	std::vector<int> primarySkillInitial;  // initial primary skills
	std::vector<int> primarySkillLowLevel; // probability (%) of getting point of primary skill when getting level
	std::vector<int> primarySkillHighLevel;// same for high levels (> 10)

	std::vector<int> secSkillProbability; //probabilities of gaining secondary skills (out of 112), in id order

	std::map<FactionID, int> selectionProbability; //probability of selection in towns

	std::string imageBattleMale;
	std::string imageBattleFemale;
	std::string imageMapMale;
	std::string imageMapFemale;

	CHeroClass();

	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	std::string getJsonKey() const override;
	HeroClassID getId() const override;
	void registerIcons(const IconRegistar & cb) const override;

	std::string getNameTranslated() const override;
	std::string getNameTextID() const override;

	bool isMagicHero() const;
	SecondarySkill chooseSecSkill(const std::set<SecondarySkill> & possibles, CRandomGenerator & rand) const; //picks secondary skill out from given possibilities

	void updateFrom(const JsonNode & data);
	void serializeJson(JsonSerializeFormat & handler);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & modScope;
		h & identifier;
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
	EAlignment getAlignment() const;
};

class DLL_LINKAGE CHeroClassHandler : public CHandlerBase<HeroClassID, HeroClass, CHeroClass, HeroClassService>
{
	void fillPrimarySkillData(const JsonNode & node, CHeroClass * heroClass, PrimarySkill::PrimarySkill pSkill) const;

public:
	std::vector<JsonNode> loadLegacyData() override;

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
	void loadHeroArmy(CHero * hero, const JsonNode & node) const;
	void loadHeroSkills(CHero * hero, const JsonNode & node) const;
	void loadHeroSpecialty(CHero * hero, const JsonNode & node);

	void loadExperience();

	std::vector<std::function<void()>> callAfterLoadFinalization;

public:
	CHeroClassHandler classes;

	//default costs of going through terrains. -1 means terrain is impassable
	std::map<TerrainId, int> terrCosts;

	ui32 level(ui64 experience) const; //calculates level corresponding to given experience amount
	ui64 reqExp(ui32 level) const; //calculates experience required for given level

	std::vector<JsonNode> loadLegacyData() override;

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
		h & terrCosts;
	}

protected:
	const std::vector<std::string> & getTypeNames() const override;
	CHero * loadFromJson(const std::string & scope, const JsonNode & node, const std::string & identifier, size_t index) override;
};

VCMI_LIB_NAMESPACE_END
