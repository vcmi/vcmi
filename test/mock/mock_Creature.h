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

class CreatureMock : public Creature
{
public:
	MOCK_CONST_METHOD0(getIndex, int32_t());
	MOCK_CONST_METHOD0(getIconIndex, int32_t());
	MOCK_CONST_METHOD0(getJsonKey, const std::string &());
	MOCK_CONST_METHOD0(getName, const std::string &());
	MOCK_CONST_METHOD0(getId, CreatureID());
	MOCK_CONST_METHOD0(accessBonuses, const IBonusBearer *());
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
	MOCK_CONST_METHOD0(getFactionIndex, int32_t());

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
};
