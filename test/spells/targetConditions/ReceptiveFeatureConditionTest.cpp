/*
 * ReceptiveFeatureConditionTest.cpp, part of VCMI engine
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

class ReceptiveFeatureConditionTest : public TargetConditionItemTest, public WithParamInterface<tuple<bool, bool> >
{
public:
	bool isPositive;
	bool hasBonus;

	void setDefaultExpectations()
	{
		isPositive = ::testing::get<0>(GetParam());
		hasBonus = ::testing::get<1>(GetParam());

		EXPECT_CALL(unitMock, getAllBonuses(_, _, _, _)).Times(AtLeast(0));
		EXPECT_CALL(unitMock, getTreeVersion()).Times(AtLeast(0));
		EXPECT_CALL(mechanicsMock, isPositiveSpell()).WillRepeatedly(Return(isPositive));
		if(hasBonus)
			unitBonuses.addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::RECEPTIVE, BonusSource::OTHER, 0, 0));
	}

protected:
	void SetUp() override
	{
		TargetConditionItemTest::SetUp();
		subject = TargetConditionItemFactory::getDefault()->createReceptiveFeature();
		GTEST_ASSERT_NE(subject, nullptr);
	}
};

TEST_P(ReceptiveFeatureConditionTest, isReceptive)
{
	setDefaultExpectations();

	EXPECT_EQ(isPositive && hasBonus, subject->isReceptive(&mechanicsMock, &unitMock));
}

INSTANTIATE_TEST_SUITE_P
(
	ByFlags,
	ReceptiveFeatureConditionTest,
	Combine
	(
		Values(false, true),
		Values(false, true)
	)
);


}
