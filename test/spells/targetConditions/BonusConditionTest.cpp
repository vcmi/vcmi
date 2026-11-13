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

#include "../../../lib/GameLibrary.h"

namespace test
{
using namespace ::spells;
using namespace ::testing;

class BonusConditionTest : public TargetConditionItemTest
{
public:
	void setDefaultExpectations()
	{
		EXPECT_CALL(unitMock, getAllBonuses(_, _)).Times(AtLeast(1));
		EXPECT_CALL(unitMock, getTreeVersion()).Times(AtLeast(0));
	}

	void SetUp() override
	{
		TargetConditionItemTest::SetUp();
		subject = TargetConditionItemFactory::create(*LIBRARY->identifiers())->createConfigurable("game", "bonus", "UNDEAD");
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
	unitBonuses.addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::UNDEAD, BonusSource::OTHER, 0, BonusSourceID()));
	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

class BonusConditionFromJsonTest : public BonusConditionTest
{
public:
	void SetUp() override
	{
		TargetConditionItemTest::SetUp();
	}

	void createSubject(int32_t minValue, int32_t maxValue)
	{
		JsonNode config;
		config["type"].String() = "bonus";
		config["mode"].String() = "normal";
		config["bonusType"].String() = "SPELL_SCHOOL_IMMUNITY";
		config["bonusSubtype"].String() = "fire";
		if(minValue != std::numeric_limits<int32_t>::min())
			config["bonusMinValue"].Integer() = minValue;
		if(maxValue != std::numeric_limits<int32_t>::max())
			config["bonusMaxValue"].Integer() = maxValue;
		config.setModScope("game");

		subject = TargetConditionItemFactory::create(*LIBRARY->identifiers())->createFromJson(config);
		GTEST_ASSERT_NE(subject, nullptr);
	}

	void addImmunity(SpellSchool school, int value)
	{
		unitBonuses.addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::SPELL_SCHOOL_IMMUNITY, BonusSource::OTHER, value, BonusSourceID(), BonusSubtypeID(school)));
	}
};

TEST_F(BonusConditionFromJsonTest, ReceptiveIfMatchesSubtype)
{
	createSubject(std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());
	setDefaultExpectations();
	addImmunity(SpellSchool::FIRE, 0);
	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_F(BonusConditionFromJsonTest, ImmuneIfSubtypeMismatch)
{
	createSubject(std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());
	setDefaultExpectations();
	addImmunity(SpellSchool::WATER, 0);
	EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_F(BonusConditionFromJsonTest, ChecksTotalValueOfMatchingBonuses)
{
	createSubject(5, 10);
	setDefaultExpectations();
	addImmunity(SpellSchool::FIRE, 3);
	addImmunity(SpellSchool::WATER, 3);
	EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));

	addImmunity(SpellSchool::FIRE, 3);
	EXPECT_TRUE(subject->isReceptive(&mechanicsMock, &unitMock));

	addImmunity(SpellSchool::FIRE, 5);
	EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));
}

TEST_F(BonusConditionTest, ImmuneIfTypeMismatch)
{
	setDefaultExpectations();
	unitBonuses.addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::SPELL_SCHOOL_IMMUNITY, BonusSource::OTHER, 0, BonusSourceID(), BonusSubtypeID(SpellSchool::FIRE)));
	EXPECT_FALSE(subject->isReceptive(&mechanicsMock, &unitMock));
}

}
