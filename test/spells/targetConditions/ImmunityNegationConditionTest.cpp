/*
 * ImmunityNegationConditionTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "TargetConditionItemFixture.h"

//FIXME: Orb of vulnerability mechanics is not such trivial (mantis issue 1791)
//TODO: NEGATE_ALL_NATURAL_IMMUNITIES special cases: dispel, chain lightning
//Tests are incomplete and only covers actual implementation for now

namespace test
{
using namespace ::spells;
using namespace ::testing;

class ImmunityNegationConditionTest : public TargetConditionItemTest, public WithParamInterface<std::tuple<bool, bool>>
{
public:
	bool ownerMatches;
	bool isMagicalEffect;

	void setDefaultExpectations()
	{
		ownerMatches = ::testing::get<0>(GetParam());
		isMagicalEffect = ::testing::get<1>(GetParam());
		EXPECT_CALL(unitMock, getAllBonuses(_, _, _, _)).Times(AtLeast(0));
		EXPECT_CALL(unitMock, getTreeVersion()).Times(AtLeast(0));
		EXPECT_CALL(mechanicsMock, isMagicalEffect()).Times(AtLeast(0)).WillRepeatedly(Return(isMagicalEffect));
		EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unitMock), Field(&boost::logic::tribool::value, boost::logic::tribool::false_value))).WillRepeatedly(Return(ownerMatches));
	}

protected:
	void SetUp() override
	{
		TargetConditionItemTest::SetUp();
		subject = TargetConditionItemFactory::getDefault()->createImmunityNegation();
		GTEST_ASSERT_NE(subject, nullptr);
	}
};

TEST_P(ImmunityNegationConditionTest, NotReceptiveByDefault)
{
	setDefaultExpectations();

	EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));
}


TEST_P(ImmunityNegationConditionTest, WithHeroNegation)
{
	setDefaultExpectations();

	unitBonuses.addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::NEGATE_ALL_NATURAL_IMMUNITIES, BonusSource::OTHER, 0, 0, 1));

	EXPECT_EQ(isMagicalEffect, subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_P(ImmunityNegationConditionTest, WithBattleWideNegation)
{
	setDefaultExpectations();

	unitBonuses.addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::NEGATE_ALL_NATURAL_IMMUNITIES, BonusSource::OTHER, 0, 0, 0));

	//This should return if ownerMatches, because anyone should cast onto owner's stacks, but not on enemyStacks
	EXPECT_EQ(ownerMatches && isMagicalEffect, subject->isReceptive(&mechanicsMock, &unitMock));
}

INSTANTIATE_TEST_SUITE_P
(
	ByUnitOwner,
	ImmunityNegationConditionTest,
	Combine
	(
		Values(false, true),
		Values(false, true)
	)
);

}

