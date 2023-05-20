/*
 * AbilityCasterTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "mock/mock_battle_Unit.h"
#include "mock/mock_BonusBearer.h"
#include "mock/mock_spells_Spell.h"

#include "../../lib/NetPacksBase.h"
#include "../../lib/spells/AbilityCaster.h"

namespace test
{
using namespace ::spells;
using namespace ::testing;


class AbilityCasterTest : public Test
{
public:
	std::shared_ptr<AbilityCaster> subject;

	StrictMock<UnitMock> actualCaster;
	BonusBearerMock casterBonuses;
	StrictMock<SpellMock> spellMock;

protected:
	void SetUp() override
	{
		ON_CALL(actualCaster, getAllBonuses(_, _, _, _)).WillByDefault(Invoke(&casterBonuses, &BonusBearerMock::getAllBonuses));
		ON_CALL(actualCaster, getTreeVersion()).WillByDefault(Invoke(&casterBonuses, &BonusBearerMock::getTreeVersion));
	}

	void setupSubject(int skillLevel)
	{
		subject = std::make_shared<AbilityCaster>(&actualCaster, skillLevel);
	}
};

TEST_F(AbilityCasterTest, NonMagicAbilityIgnoresBonuses)
{
	EXPECT_CALL(spellMock, getLevel()).WillRepeatedly(Return(0));
	setupSubject(1);

	EXPECT_EQ(subject->getSpellSchoolLevel(&spellMock), 1);
}

TEST_F(AbilityCasterTest, MagicAbilityAffectedByGenericBonus)
{
	EXPECT_CALL(spellMock, getLevel()).WillRepeatedly(Return(1));

	casterBonuses.addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::MAGIC_SCHOOL_SKILL, BonusSource::OTHER, 2, 0, 0));

	EXPECT_CALL(actualCaster, getAllBonuses(_, _, _, _)).Times(AtLeast(1));
	EXPECT_CALL(actualCaster, getTreeVersion()).Times(AtLeast(0));

	setupSubject(1);

	EXPECT_EQ(subject->getSpellSchoolLevel(&spellMock), 2);
}

TEST_F(AbilityCasterTest, MagicAbilityIngoresSchoolBonus)
{
	EXPECT_CALL(spellMock, getLevel()).WillRepeatedly(Return(1));

	casterBonuses.addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::MAGIC_SCHOOL_SKILL, BonusSource::OTHER, 2, 0, 1));

	EXPECT_CALL(actualCaster, getAllBonuses(_, _, _, _)).Times(AtLeast(1));
	EXPECT_CALL(actualCaster, getTreeVersion()).Times(AtLeast(0));

	setupSubject(1);

	EXPECT_EQ(subject->getSpellSchoolLevel(&spellMock), 1);
}


}
