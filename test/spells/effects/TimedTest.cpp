/*
 * TimedTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "EffectFixture.h"

#include <vstd/RNG.h>
#include "lib/modding/ModScope.h"

namespace test
{
using namespace ::spells;
using namespace ::spells::effects;
using namespace ::testing;

class TimedTest : public Test, public EffectFixture
{
public:
	TimedTest()
		: EffectFixture("core:timed")
	{
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();

	}
};

class TimedApplyTest : public TestWithParam<::testing::tuple<bool, bool>>, public EffectFixture
{
public:
	bool cumulative = false;
	bool firstCast = false;

	const int32_t spellIndex = 456;
	const int32_t duration = 57;

	TimedApplyTest()
		: EffectFixture("core:timed")
	{
	}

	void setDefaultExpectaions()
	{
		unitsFake.setDefaultBonusExpectations();
		EXPECT_CALL(mechanicsMock, getSpellIndex()).WillRepeatedly(Return(spellIndex));
		EXPECT_CALL(mechanicsMock, getEffectDuration()).WillRepeatedly(Return(duration));
		EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();

		cumulative = get<0>(GetParam());
		firstCast = get<1>(GetParam());

	}
};

TEST_P(TimedApplyTest, ChangesBonuses)
{
	Bonus testBonus1(BonusDuration::PERMANENT, BonusType::PRIMARY_SKILL, BonusSource::OTHER, 3, 0, PrimarySkill::KNOWLEDGE);

	Bonus testBonus2(BonusDuration::N_TURNS, BonusType::PRIMARY_SKILL, BonusSource::OTHER, 3, 0, PrimarySkill::KNOWLEDGE);
	testBonus2.turnsRemain = 4;

	JsonNode options(JsonNode::JsonType::DATA_STRUCT);
	options["cumulative"].Bool() = cumulative;
	options["bonus"]["test1"] = testBonus1.toJsonNode();
	options["bonus"]["test2"] = testBonus2.toJsonNode();
	options.setMeta(ModScope::scopeBuiltin());
	setupEffect(options);

	const uint32_t unitId = 42;
	auto & targetUnit = unitsFake.add(BattleSide::ATTACKER);
	targetUnit.makeAlive();
	EXPECT_CALL(targetUnit, unitId()).WillRepeatedly(Return(unitId));
	EXPECT_CALL(targetUnit, creatureLevel()).WillRepeatedly(Return(1));

	EffectTarget target;
	target.emplace_back(&targetUnit, BattleHex());

	std::vector<Bonus> actualBonus;
	std::vector<Bonus> expectedBonus;

	testBonus1.turnsRemain = duration;

	expectedBonus.push_back(testBonus1);
	expectedBonus.push_back(testBonus2);

	for(auto & bonus : expectedBonus)
	{
		bonus.source = BonusSource::SPELL_EFFECT;
		bonus.sid = spellIndex;
	}

	if(cumulative)
	{
		EXPECT_CALL(*battleFake, addUnitBonus(Eq(unitId),_)).WillOnce(SaveArg<1>(&actualBonus));
	}
	else
	{
		EXPECT_CALL(*battleFake, updateUnitBonus(Eq(unitId),_)).WillOnce(SaveArg<1>(&actualBonus));
	}

	setDefaultExpectaions();

	EXPECT_CALL(serverMock, apply(Matcher<SetStackEffect *>(_))).Times(1);

	subject->apply(&serverMock, &mechanicsMock, target);

	EXPECT_THAT(actualBonus, UnorderedElementsAreArray(expectedBonus));
}

INSTANTIATE_TEST_SUITE_P
(
	ByConfig,
	TimedApplyTest,
	Combine
	(
		Values(false, true),
		Values(false, true)
	)
);

}
