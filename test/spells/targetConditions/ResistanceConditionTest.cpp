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

class ResistanceConditionTest : public TargetConditionItemTest, public WithParamInterface<bool>
{
public:
	bool isPositive;
	void setDefaultExpectations()
	{
		EXPECT_CALL(unitMock, getTreeVersion()).Times(AtLeast(0));
		EXPECT_CALL(mechanicsMock, isPositiveSpell()).WillRepeatedly(Return(isPositive));
	}

	void SetUp() override
	{
		TargetConditionItemTest::SetUp();
		subject = TargetConditionItemFactory::getDefault()->createResistance();

		isPositive = GetParam();
	}
};

TEST_P(ResistanceConditionTest, ReceptiveIfNoBonus)
{
	setDefaultExpectations();

	EXPECT_CALL(unitMock, magicResistance()).Times(AtLeast(0)).WillRepeatedly(Return(0));

	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_P(ResistanceConditionTest, DependsOnPositivness)
{
	setDefaultExpectations();

	EXPECT_CALL(unitMock, magicResistance()).Times(AtLeast(0)).WillRepeatedly(Return(100));

	EXPECT_EQ(isPositive, subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_P(ResistanceConditionTest, ReceptiveIfResistanceIsLessThanHundred)
{
	setDefaultExpectations();

	EXPECT_CALL(unitMock, magicResistance()).Times(AtLeast(0)).WillRepeatedly(Return(99));

	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

INSTANTIATE_TEST_SUITE_P
(
	ByPositiveness,
	ResistanceConditionTest,
	Values(false, true)
);

}
