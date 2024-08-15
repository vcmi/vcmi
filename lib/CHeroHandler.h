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

#include "ConstTransitivePtr.h"
#include "GameConstants.h"
#include "bonuses/Bonus.h"
#include "bonuses/BonusList.h"
#include "IHandlerBase.h"
#include "filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{
class RNG;
}

class CHeroClass;
class CGHeroInstance;
struct BattleHex;
class JsonNode;
class JsonSerializeFormat;
class BattleField;

enum class EHeroGender : int8_t
{
	DEFAULT = -1, // from h3m, instance has same gender as hero type
	MALE = 0,
	FEMALE = 1,
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
	};
	si32 imageIndex = 0;

	std::vector<InitialArmyStack> initialArmy;

	const CHeroClass * heroClass = nullptr;
	std::vector<std::pair<SecondarySkill, ui8> > secSkillsInit; //initial secondary skills; first - ID of skill, second - level of skill (1 - basic, 2 - adv., 3 - expert)
	BonusList specialty;
	std::set<SpellID> spells;
	bool haveSpellBook = false;
	bool special = false; // hero is special and won't be placed in game (unless preset on map), e.g. campaign heroes
	bool onlyOnWaterMap; // hero will be placed only if the map contains water
	bool onlyOnMapWithoutWater; // hero will be placed only if the map does not contain water
	EHeroGender gender = EHeroGender::MALE; // default sex: 0=male, 1=female

	/// Graphics
	std::string iconSpecSmall;
	std::string iconSpecLarge;
	std::string portraitSmall;
	std::string portraitLarge;
	AnimationPath battleImage;

	CHero();
	virtual ~CHero();

	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	std::string getJsonKey() const override;
	std::string getModScope() const override;
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

	CreatureID commander;

	std::vector<int> primarySkillInitial;  // initial primary skills
	std::vector<int> primarySkillLowLevel; // probability (%) of getting point of primary skill when getting level
	std::vector<int> primarySkillHighLevel;// same for high levels (> 10)

	std::map<SecondarySkill, int> secSkillProbability; //probabilities of gaining secondary skills (out of 112), in id order

	std::map<FactionID, int> selectionProbability; //probability of selection in towns

	AnimationPath imageBattleMale;
	AnimationPath imageBattleFemale;
	std::string imageMapMale;
	std::string imageMapFemale;

	CHeroClass();

	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	std::string getJsonKey() const override;
	std::string getModScope() const override;
	HeroClassID getId() const override;
	void registerIcons(const IconRegistar & cb) const override;

	std::string getNameTranslated() const override;
	std::string getNameTextID() const override;

	bool isMagicHero() const;
	SecondarySkill chooseSecSkill(const std::set<SecondarySkill> & possibles, vstd::RNG & rand) const; //picks secondary skill out from given possibilities

	void updateFrom(const JsonNode & data);
	void serializeJson(JsonSerializeFormat & handler);

	EAlignment getAlignment() const;

	int tavernProbability(FactionID faction) const;
};

class DLL_LINKAGE CHeroClassHandler : public CHandlerBase<HeroClassID, HeroClass, CHeroClass, HeroClassService>
{
	void fillPrimarySkillData(const JsonNode & node, CHeroClass * heroClass, PrimarySkill pSkill) const;

public:
	std::vector<JsonNode> loadLegacyData() override;

	void afterLoadFinalization() override;

	~CHeroClassHandler();

protected:
	const std::vector<std::string> & getTypeNames() const override;
	std::shared_ptr<CHeroClass> loadFromJson(const std::string & scope, const JsonNode & node, const std::string & identifier, size_t index) override;

};

class DLL_LINKAGE CHeroHandler : public CHandlerBase<HeroTypeID, HeroType, CHero, HeroTypeService>
{
	/// expPerLEvel[i] is amount of exp needed to reach level i;
	/// consists of 196 values. Any higher levels require experience larger that TExpType can hold
	std::vector<TExpType> expPerLevel;

	/// helpers for loading to avoid huge load functions
	void loadHeroArmy(CHero * hero, const JsonNode & node) const;
	void loadHeroSkills(CHero * hero, const JsonNode & node) const;
	void loadHeroSpecialty(CHero * hero, const JsonNode & node);

	void loadExperience();

	std::vector<std::function<void()>> callAfterLoadFinalization;

public:
	ui32 level(TExpType experience) const; //calculates level corresponding to given experience amount
	TExpType reqExp(ui32 level) const; //calculates experience required for given level
	ui32 maxSupportedLevel() const;

	std::vector<JsonNode> loadLegacyData() override;

	void beforeValidate(JsonNode & object) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;
	void afterLoadFinalization() override;

	CHeroHandler();
	~CHeroHandler();

	std::set<HeroTypeID> getDefaultAllowed() const;

protected:
	const std::vector<std::string> & getTypeNames() const override;
	std::shared_ptr<CHero> loadFromJson(const std::string & scope, const JsonNode & node, const std::string & identifier, size_t index) override;
};

VCMI_LIB_NAMESPACE_END
