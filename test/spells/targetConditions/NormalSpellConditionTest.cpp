/*
 * NormalSpellConditionTest.cpp, part of VCMI engine
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

class NormalSpellConditionTest : public TargetConditionItemTest, public WithParamInterface<tuple<int32_t, int32_t> >
{
public:
	int32_t immuneSpell;
	int32_t castSpell;

	void setDefaultExpectations()
	{
		EXPECT_CALL(unitMock, getAllBonuses(_, _, _, _)).Times(AtLeast(1));
		EXPECT_CALL(unitMock, getTreeVersion()).Times(AtLeast(0));
		EXPECT_CALL(mechanicsMock, getSpellIndex()).WillRepeatedly(Return(castSpell));
	}

	void SetUp() override
	{
		TargetConditionItemTest::SetUp();

		immuneSpell = ::testing::get<0>(GetParam());
		castSpell = ::testing::get<1>(GetParam());

		subject = TargetConditionItemFactory::getDefault()->createNormalSpell();
	}
};

TEST_P(NormalSpellConditionTest, ChecksAbsoluteCase)
{
	setDefaultExpectations();
	auto bonus = std::make_shared<Bonus>(Bonus::ONE_BATTLE, Bonus::SPELL_IMMUNITY, Bonus::OTHER, 4, 0, immuneSpell);
	bonus->additionalInfo = 1;

	unitBonuses.addNewBonus(bonus);

	if(immuneSpell == castSpell)
		EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));
	else
		EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_P(NormalSpellConditionTest, ChecksNormalCase)
{
	setDefaultExpectations();
	auto bonus = std::make_shared<Bonus>(Bonus::ONE_BATTLE, Bonus::SPELL_IMMUNITY, Bonus::OTHER, 4, 0, immuneSpell);
	unitBonuses.addNewBonus(bonus);
	if(immuneSpell == castSpell)
		EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));
	else
		EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

INSTANTIATE_TEST_SUITE_P
(
	BySpells,
	NormalSpellConditionTest,
	Combine
	(
		Values(1,2),
		Values(1,2)
	)
);

}
