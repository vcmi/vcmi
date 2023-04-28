/*
 * ElementalConditionTest.cpp, part of VCMI engine
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

class ElementalConditionTest : public TargetConditionItemTest, public WithParamInterface<bool>
{
public:
	bool isPositive;
	void setDefaultExpectations()
	{
		EXPECT_CALL(unitMock, getAllBonuses(_, _, _, _)).Times(AtLeast(1));
		EXPECT_CALL(unitMock, getTreeVersion()).Times(AtLeast(0));

		std::vector<Bonus::BonusType> immunityList =
		{
			Bonus::AIR_IMMUNITY,
			Bonus::FIRE_IMMUNITY,
		};

		EXPECT_CALL(mechanicsMock, getElementalImmunity()).Times(AtLeast(1)).WillRepeatedly(Return(immunityList));
		EXPECT_CALL(mechanicsMock, isPositiveSpell()).WillRepeatedly(Return(isPositive));
	}

	void SetUp() override
	{
		TargetConditionItemTest::SetUp();
		subject = TargetConditionItemFactory::getDefault()->createElemental();

		isPositive = GetParam();
	}
};

TEST_P(ElementalConditionTest, ReceptiveIfNoBonus)
{
	setDefaultExpectations();

	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_P(ElementalConditionTest, ImmuneIfBonusMatches)
{
	setDefaultExpectations();
	unitBonuses.addNewBonus(std::make_shared<Bonus>(Bonus::ONE_BATTLE, Bonus::AIR_IMMUNITY, Bonus::SPELL_EFFECT, 0, 0, 0));

	EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_P(ElementalConditionTest, DependsOnPositivness)
{
	setDefaultExpectations();
	unitBonuses.addNewBonus(std::make_shared<Bonus>(Bonus::ONE_BATTLE, Bonus::AIR_IMMUNITY, Bonus::SPELL_EFFECT, 0, 0, 1));

	EXPECT_EQ(isPositive, subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_P(ElementalConditionTest, ImmuneIfBothBonusesPresent)
{
	setDefaultExpectations();
	unitBonuses.addNewBonus(std::make_shared<Bonus>(Bonus::ONE_BATTLE, Bonus::AIR_IMMUNITY, Bonus::SPELL_EFFECT, 0, 0, 0));
	unitBonuses.addNewBonus(std::make_shared<Bonus>(Bonus::ONE_BATTLE, Bonus::AIR_IMMUNITY, Bonus::SPELL_EFFECT, 0, 0, 1));

	EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));
}

INSTANTIATE_TEST_SUITE_P
(
	ByPositiveness,
	ElementalConditionTest,
	Values(false, true)
);

}
