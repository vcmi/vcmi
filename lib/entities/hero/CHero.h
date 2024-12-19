/*
 * CHero.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/HeroType.h>

#include "EHeroGender.h"

#include "../../bonuses/BonusList.h"
#include "../../constants/EntityIdentifiers.h"
#include "../../filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN

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

	//initial secondary skills; first - ID of skill, second - level of skill (1 - basic, 2 - adv., 3 - expert)
	std::vector<std::pair<SecondarySkill, ui8>> secSkillsInit;

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

VCMI_LIB_NAMESPACE_END
