/*
 * mock_Creature.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/Creature.h>

class IBonusBearer;
class FactionID;

class CreatureMock : public Creature
{
public:
	MOCK_CONST_METHOD0(getIndex, int32_t());
	MOCK_CONST_METHOD0(getIconIndex, int32_t());
	MOCK_CONST_METHOD0(getJsonKey, std::string());
	MOCK_CONST_METHOD0(getName, const std::string &());
	MOCK_CONST_METHOD0(getId, CreatureID());
	MOCK_CONST_METHOD0(getBonusBearer, const IBonusBearer *());
	MOCK_CONST_METHOD1(registerIcons, void(const IconRegistar &));

	MOCK_CONST_METHOD0(getPluralName, const std::string &());
	MOCK_CONST_METHOD0(getSingularName, const std::string &());
	MOCK_CONST_METHOD0(getMaxHealth, uint32_t());

	MOCK_CONST_METHOD0(getAdvMapAmountMin, int32_t());
	MOCK_CONST_METHOD0(getAdvMapAmountMax, int32_t());
	MOCK_CONST_METHOD0(getAIValue, int32_t());
	MOCK_CONST_METHOD0(getFightValue, int32_t());
	MOCK_CONST_METHOD0(getLevel, int32_t());
	MOCK_CONST_METHOD0(getGrowth, int32_t());
	MOCK_CONST_METHOD0(getHorde, int32_t());
	MOCK_CONST_METHOD0(getFaction, FactionID());

	MOCK_CONST_METHOD0(getBaseAttack, int32_t());
	MOCK_CONST_METHOD0(getBaseDefense, int32_t());
	MOCK_CONST_METHOD0(getBaseDamageMin, int32_t());
	MOCK_CONST_METHOD0(getBaseDamageMax, int32_t());
	MOCK_CONST_METHOD0(getBaseHitPoints, int32_t());
	MOCK_CONST_METHOD0(getBaseSpellPoints, int32_t());
	MOCK_CONST_METHOD0(getBaseSpeed, int32_t());
	MOCK_CONST_METHOD0(getBaseShots, int32_t());

	MOCK_CONST_METHOD1(getCost, int32_t(int32_t));
	MOCK_CONST_METHOD0(isDoubleWide, bool());
	
	MOCK_CONST_METHOD1(getRecruitCost, int32_t(Identifier<EGameResID>));
	MOCK_CONST_METHOD0(getFullRecruitCost, ResourceSet());
	MOCK_CONST_METHOD0(hasUpgrades, bool());
	
	MOCK_CONST_METHOD0(getNameTranslated, std::string());
	MOCK_CONST_METHOD0(getNameTextID, std::string());
	MOCK_CONST_METHOD0(getNamePluralTranslated, std::string());
	MOCK_CONST_METHOD0(getNameSingularTranslated, std::string());
	MOCK_CONST_METHOD0(getNamePluralTextID, std::string());
	MOCK_CONST_METHOD0(getNameSingularTextID, std::string());
};
