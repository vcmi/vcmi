/*
 * AbsoluteLevelConditionTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "TargetConditionItemFixture.h"

namespace test
{
using namespace ::spells;
using namespace ::testing;


class AbsoluteLevelConditionTest : public TargetConditionItemTest
{
public:

	void setDefaultExpectations()
	{
		EXPECT_CALL(mechanicsMock, isMagicalEffect()).WillRepeatedly(Return(true));
		EXPECT_CALL(unitMock, getAllBonuses(_, _, _, _)).Times(AtLeast(1));
		EXPECT_CALL(unitMock, getTreeVersion()).Times(AtLeast(0));
	}

	void SetUp() override
	{
		TargetConditionItemTest::SetUp();

		subject = TargetConditionItemFactory::getDefault()->createAbsoluteLevel();
	}
};

TEST_F(AbsoluteLevelConditionTest, DefaultForAbility)
{
	setDefaultExpectations();
	EXPECT_CALL(mechanicsMock, getSpellLevel()).WillRepeatedly(Return(0));
	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_F(AbsoluteLevelConditionTest, DefaultForNormalSpell)
{
	setDefaultExpectations();
	EXPECT_CALL(mechanicsMock, getSpellLevel()).WillRepeatedly(Return(1));
	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_F(AbsoluteLevelConditionTest, ReceptiveNormalSpell)
{
	setDefaultExpectations();

	auto bonus = std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::LEVEL_SPELL_IMMUNITY, BonusSource::OTHER, 3, 0);
	bonus->additionalInfo = 1;
	unitBonuses.addNewBonus(bonus);

	EXPECT_CALL(mechanicsMock, getSpellLevel()).Times(AtLeast(1)).WillRepeatedly(Return(4));
	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

//this tests covers fact that creature abilities ignored (by spell level == 0)
TEST_F(AbsoluteLevelConditionTest, ReceptiveAbility)
{
	setDefaultExpectations();

	auto bonus = std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::LEVEL_SPELL_IMMUNITY, BonusSource::OTHER, 5, 0);
	bonus->additionalInfo = 1;
	unitBonuses.addNewBonus(bonus);

	EXPECT_CALL(mechanicsMock, getSpellLevel()).Times(AtLeast(1)).WillRepeatedly(Return(0));
	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_F(AbsoluteLevelConditionTest, ImmuneNormalSpell)
{
	setDefaultExpectations();

	auto bonus = std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::LEVEL_SPELL_IMMUNITY, BonusSource::OTHER, 4, 0);
	bonus->additionalInfo = 1;
	unitBonuses.addNewBonus(bonus);

	EXPECT_CALL(mechanicsMock, getSpellLevel()).Times(AtLeast(1)).WillRepeatedly(Return(2));
	EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_F(AbsoluteLevelConditionTest, IgnoresNormalCase)
{
	setDefaultExpectations();
	auto bonus = std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::LEVEL_SPELL_IMMUNITY, BonusSource::OTHER, 4, 0);
	unitBonuses.addNewBonus(bonus);
	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}


}

