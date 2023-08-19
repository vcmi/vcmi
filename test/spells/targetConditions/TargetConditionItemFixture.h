/*
 * TargetConditionItemFixture.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vstd/RNG.h>

#include "../../../lib/NetPacksBase.h"
#include "../../../lib/spells/TargetCondition.h"


#include "mock/mock_spells_Mechanics.h"
#include "mock/mock_spells_Spell.h"
#include "mock/mock_BonusBearer.h"
#include "mock/mock_battle_Unit.h"

namespace test
{

class TargetConditionItemTest : public ::testing::Test
{
public:
	std::shared_ptr<spells::TargetConditionItem> subject;

	::testing::StrictMock<spells::MechanicsMock> mechanicsMock;
	::testing::StrictMock<UnitMock> unitMock;
	::testing::StrictMock<spells::SpellMock> spellMock;

	BonusBearerMock unitBonuses;
protected:
	void SetUp() override
	{
		using namespace ::testing;
		ON_CALL(unitMock, getAllBonuses(_, _, _, _)).WillByDefault(Invoke(&unitBonuses, &BonusBearerMock::getAllBonuses));
		ON_CALL(unitMock, getTreeVersion()).WillByDefault(Invoke(&unitBonuses, &BonusBearerMock::getTreeVersion));
	}
};

}
