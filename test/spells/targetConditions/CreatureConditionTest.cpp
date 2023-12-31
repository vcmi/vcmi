/*
 * CreatureConditionTest.cpp, part of VCMI engine
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

class CreatureConditionTest : public TargetConditionItemTest
{
public:
	void setDefaultExpectations()
	{
		EXPECT_CALL(unitMock, getAllBonuses(_, _, _, _)).Times(0);
		EXPECT_CALL(unitMock, getTreeVersion()).Times(0);
	}

	void SetUp() override
	{
		TargetConditionItemTest::SetUp();

		subject = TargetConditionItemFactory::getDefault()->createConfigurable("core", "creature", "ammoCart");
		GTEST_ASSERT_NE(subject, nullptr);
	}
};

TEST_F(CreatureConditionTest, ReceptiveIfMatchesType)
{
	setDefaultExpectations();
	EXPECT_CALL(unitMock, creatureId()).WillOnce(Return(CreatureID::AMMO_CART));
	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_F(CreatureConditionTest, ImmuneIfTypeMismatch)
{
	setDefaultExpectations();
	EXPECT_CALL(unitMock, creatureId()).WillOnce(Return(CreatureID::BALLISTA));
	EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));
}

}

