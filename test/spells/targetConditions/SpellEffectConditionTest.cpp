/*
 * SpellEffectConditionTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "StdInc.h"

#include "TargetConditionItemFixture.h"

namespace test
{
using namespace ::spells;
using namespace ::testing;

class SpellEffectConditionTest : public TargetConditionItemTest
{
public:
	void setDefaultExpectations()
	{
		EXPECT_CALL(unitMock, getAllBonuses(_, _, _, _)).Times(AtLeast(1));
		EXPECT_CALL(unitMock, getTreeVersion()).Times(AtLeast(0));
	}

	void SetUp() override
	{
		TargetConditionItemTest::SetUp();

		subject = TargetConditionItemFactory::getDefault()->createConfigurable("core", "spell", "age");
		GTEST_ASSERT_NE(subject, nullptr);
	}
};

TEST_F(SpellEffectConditionTest, ImmuneByDefault)
{
	setDefaultExpectations();
	EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_F(SpellEffectConditionTest, ReceptiveIfHasEffectFromDesiredSpell)
{
	setDefaultExpectations();
	unitBonuses.addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::STACK_HEALTH, BonusSource::SPELL_EFFECT, 3, BonusSourceID(SpellID(SpellID::AGE))));

	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_F(SpellEffectConditionTest, ImmuneIfHasEffectFromOtherSpell)
{
	setDefaultExpectations();
	unitBonuses.addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::STACK_HEALTH, BonusSource::SPELL_EFFECT, 3, BonusSourceID(SpellID(SpellID::AIR_SHIELD))));
	EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_F(SpellEffectConditionTest, ImmuneIfHasNoSpellEffects)
{
	setDefaultExpectations();
	unitBonuses.addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, 3, BonusSourceID()));
	EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));
}

}

