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

		EXPECT_CALL(mechanicsMock, getSpell()).Times(AtLeast(1)).WillRepeatedly(Return(&spellMock));
		EXPECT_CALL(spellMock, forEachSchool(NotNull())).Times(AtLeast(1)).WillRepeatedly([](const spells::Spell::SchoolCallback & cb)
		{
			bool stop = false;
			cb(SpellSchool(ESpellSchool::AIR), stop);
			cb(SpellSchool(ESpellSchool::FIRE), stop);
		});

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
	unitBonuses.addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::SPELL_SCHOOL_IMMUNITY, BonusSource::SPELL_EFFECT, 0, 0, SpellSchool(ESpellSchool::AIR)));

	EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_P(ElementalConditionTest, NotImmuneIfBonusMismatches)
{
	setDefaultExpectations();
	unitBonuses.addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::SPELL_SCHOOL_IMMUNITY, BonusSource::SPELL_EFFECT, 0, 0, SpellSchool(ESpellSchool::WATER)));

	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_P(ElementalConditionTest, DependsOnPositivness)
{
	setDefaultExpectations();
	unitBonuses.addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::NEGATIVE_EFFECTS_IMMUNITY, BonusSource::SPELL_EFFECT, 0, 0, SpellSchool(ESpellSchool::AIR)));

	EXPECT_EQ(isPositive, subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_P(ElementalConditionTest, ImmuneIfBothBonusesPresent)
{
	setDefaultExpectations();
	unitBonuses.addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::SPELL_SCHOOL_IMMUNITY, BonusSource::SPELL_EFFECT, 0, 0, SpellSchool(ESpellSchool::AIR)));
	unitBonuses.addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::NEGATIVE_EFFECTS_IMMUNITY, BonusSource::SPELL_EFFECT, 0, 0, SpellSchool(ESpellSchool::AIR)));

	EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));
}

INSTANTIATE_TEST_SUITE_P
(
	ByPositiveness,
	ElementalConditionTest,
	Values(false, true)
);

}
