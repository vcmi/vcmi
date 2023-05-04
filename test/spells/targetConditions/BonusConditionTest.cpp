/*
 * BonusConditionTest.cpp, part of VCMI engine
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

class BonusConditionTest : public TargetConditionItemTest
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
		subject = TargetConditionItemFactory::getDefault()->createConfigurable("", "bonus", "DIRECT_DAMAGE_IMMUNITY");
		GTEST_ASSERT_NE(subject, nullptr);
	}
};

TEST_F(BonusConditionTest, ImmuneByDefault)
{
	setDefaultExpectations();
	EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_F(BonusConditionTest, ReceptiveIfMatchesType)
{
	setDefaultExpectations();
	unitBonuses.addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::DIRECT_DAMAGE_IMMUNITY, BonusSource::OTHER, 0, 0));
	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_F(BonusConditionTest, ImmuneIfTypeMismatch)
{
	setDefaultExpectations();
	unitBonuses.addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::FIRE_IMMUNITY, BonusSource::OTHER, 0, 0));
	EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));
}

}
