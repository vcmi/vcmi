/*
 * mock_spells_Spell.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/spells/Spell.h>

namespace spells
{

class SpellMock : public Spell
{
public:
	MOCK_CONST_METHOD1(calculateDamage, int64_t(const Caster *));
	MOCK_CONST_METHOD0(getIndex, int32_t());
	MOCK_CONST_METHOD0(getIconIndex, int32_t());
	MOCK_CONST_METHOD0(getJsonKey, std::string());
	MOCK_CONST_METHOD0(getName, const std::string &());
	MOCK_CONST_METHOD0(getId, SpellID());
	MOCK_CONST_METHOD0(getLevel, int32_t());
	MOCK_CONST_METHOD1(getCost, int32_t(const int32_t));
	MOCK_CONST_METHOD0(getBasePower, int32_t());
	MOCK_CONST_METHOD1(getLevelPower, int32_t(const int32_t));
	MOCK_CONST_METHOD1(getLevelDescription, const std::string &(const int32_t));
	MOCK_CONST_METHOD0(getPositiveness, boost::logic::tribool());
	MOCK_CONST_METHOD0(isAdventure, bool());
	MOCK_CONST_METHOD0(isCombat, bool());
	MOCK_CONST_METHOD0(isCreatureAbility, bool());
	MOCK_CONST_METHOD0(isPositive, bool());
	MOCK_CONST_METHOD0(isNegative, bool());
	MOCK_CONST_METHOD0(isNeutral, bool());
	MOCK_CONST_METHOD0(isDamage, bool());
	MOCK_CONST_METHOD0(isOffensive, bool());
	MOCK_CONST_METHOD0(isSpecial, bool());
	MOCK_CONST_METHOD1(forEachSchool, void(const SchoolCallback &));
	MOCK_CONST_METHOD0(getCastSound, const std::string &());
	MOCK_CONST_METHOD1(registerIcons, void(const IconRegistar &));
	MOCK_CONST_METHOD0(getNameTranslated, std::string());
	MOCK_CONST_METHOD0(getNameTextID, std::string());
	MOCK_CONST_METHOD1(getDescriptionTextID, std::string(int32_t));
	MOCK_CONST_METHOD1(getDescriptionTranslated, std::string(int32_t));
	MOCK_CONST_METHOD0(isMagical, bool());
};

}

