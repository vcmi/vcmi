/*
 * HealthValueConditionTest.cpp, part of VCMI engine
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

class HealthValueConditionTest : public TargetConditionItemTest
{
public:
	const int64_t UNIT_HP = 4242;
	const int64_t EFFECT_VALUE = 101;
	void setDefaultExpectations()
	{
		EXPECT_CALL(unitMock, getAllBonuses(_, _)).Times(0);
		EXPECT_CALL(unitMock, getTreeVersion()).Times(0);
		EXPECT_CALL(unitMock, getAvailableHealth()).WillOnce(Return(UNIT_HP));
		EXPECT_CALL(mechanicsMock, getEffectValue()).WillOnce(Return(EFFECT_VALUE));
	}

	void SetUp() override
	{
		TargetConditionItemTest::SetUp();

		subject = TargetConditionItemFactory::getDefault()->createConfigurable("", "healthValueSpecial", "");
		GTEST_ASSERT_NE(subject, nullptr);
	}
};

TEST_F(HealthValueConditionTest, ReceptiveIfEq)
{
	setDefaultExpectations();

	EXPECT_CALL(mechanicsMock, applySpellBonus(Eq(EFFECT_VALUE), Eq(&unitMock))).WillOnce(Return(UNIT_HP));

	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_F(HealthValueConditionTest, ReceptiveIfGt)
{
	setDefaultExpectations();

	EXPECT_CALL(mechanicsMock, applySpellBonus(Eq(EFFECT_VALUE), Eq(&unitMock))).WillOnce(Return(UNIT_HP+1));

	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_F(HealthValueConditionTest, ImmuneIfLt)
{
	setDefaultExpectations();

	EXPECT_CALL(mechanicsMock, applySpellBonus(Eq(EFFECT_VALUE), Eq(&unitMock))).WillOnce(Return(UNIT_HP-1));

	EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));
}

}
