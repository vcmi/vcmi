/*
 * NormalLevelConditionTest.cpp, part of VCMI engine
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


class NormalLevelConditionTest : public TargetConditionItemTest, public WithParamInterface<bool>
{
public:
	bool isMagicalEffect;

	void setDefaultExpectations()
	{
		isMagicalEffect = GetParam();
		EXPECT_CALL(mechanicsMock, isMagicalEffect()).WillRepeatedly(Return(isMagicalEffect));
		if(isMagicalEffect)
			EXPECT_CALL(unitMock, getAllBonuses(_, _, _, _)).Times(AtLeast(1));
		EXPECT_CALL(unitMock, getTreeVersion()).Times(AtLeast(0));
	}

	void SetUp() override
	{
		TargetConditionItemTest::SetUp();

		subject = TargetConditionItemFactory::getDefault()->createNormalLevel();
	}
};

TEST_P(NormalLevelConditionTest, DefaultForAbility)
{
	setDefaultExpectations();
	EXPECT_CALL(mechanicsMock, getSpellLevel()).WillRepeatedly(Return(0));
	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_P(NormalLevelConditionTest, DefaultForNormal)
{
	setDefaultExpectations();
	EXPECT_CALL(mechanicsMock, getSpellLevel()).WillRepeatedly(Return(1));
	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_P(NormalLevelConditionTest, ReceptiveNormal)
{
	setDefaultExpectations();
	unitBonuses.addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::LEVEL_SPELL_IMMUNITY, BonusSource::OTHER, 3, 0));
	if(isMagicalEffect)
		EXPECT_CALL(mechanicsMock, getSpellLevel()).Times(AtLeast(1)).WillRepeatedly(Return(4));
	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

//TODO: this tests covers fact that creature abilities ignored (by spell level == 0), should this be done by ability flag or by cast mode?
TEST_P(NormalLevelConditionTest, ReceptiveAbility)
{
	setDefaultExpectations();
	unitBonuses.addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::LEVEL_SPELL_IMMUNITY, BonusSource::OTHER, 5, 0));
	if(isMagicalEffect)
		EXPECT_CALL(mechanicsMock, getSpellLevel()).Times(AtLeast(1)).WillRepeatedly(Return(0));
	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_P(NormalLevelConditionTest, ImmuneNormal)
{
	setDefaultExpectations();
	unitBonuses.addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::LEVEL_SPELL_IMMUNITY, BonusSource::OTHER, 4, 0));
	if(isMagicalEffect)
		EXPECT_CALL(mechanicsMock, getSpellLevel()).Times(AtLeast(1)).WillRepeatedly(Return(2));
	EXPECT_EQ(!isMagicalEffect, subject->isReceptive(&mechanicsMock, &unitMock));
}

INSTANTIATE_TEST_SUITE_P
(
	ByMagicalEffect,
	NormalLevelConditionTest,
	Values(false, true)
);

}
