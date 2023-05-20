/*
 * CloneTest.cpp, part of VCMI engine
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

#include "../../../lib/battle/CUnitState.h"

namespace test
{
using namespace ::spells;
using namespace ::spells::effects;
using namespace ::testing;

class CloneTest : public Test, public EffectFixture
{
public:
	CloneTest()
		: EffectFixture("core:clone")
	{
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
	}
};

TEST_F(CloneTest, ApplicableToValidTarget)
{
	{
		JsonNode config(JsonNode::JsonType::DATA_STRUCT);
		config["maxTier"].Integer() = 7;
		EffectFixture::setupEffect(config);
	}

	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	EXPECT_CALL(unit, isValidTarget(Eq(false))).Times(AtLeast(1)).WillRepeatedly(Return(true));

	EXPECT_CALL(unit, hasClone()).WillRepeatedly(Return(false));
	EXPECT_CALL(unit, isClone()).Times(AtLeast(1)).WillRepeatedly(Return(false));

	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).Times(AtLeast(1)).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, isSmart()).Times(AtLeast(1)).WillRepeatedly(Return(true));

	EXPECT_CALL(unit, getPosition()).WillOnce(Return(BattleHex(5, 5)));
	EffectTarget target;
	target.emplace_back(&unit);

	EXPECT_TRUE(subject->applicable(problemMock, &mechanicsMock, target));
}

TEST_F(CloneTest, CloneIsNotClonable)
{
	setupEffect(JsonNode());
	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	EXPECT_CALL(unit, hasClone()).WillRepeatedly(Return(false));
	EXPECT_CALL(unit, isClone()).Times(AtLeast(1)).WillRepeatedly(Return(true));

	EXPECT_CALL(unit, getPosition()).WillOnce(Return(BattleHex(5, 5)));
	EffectTarget target;
	target.emplace_back(&unit);

	EXPECT_FALSE(subject->applicable(problemMock, &mechanicsMock, target));
}

TEST_F(CloneTest, SecondCloneRejected)
{
	setupEffect(JsonNode());
	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	EXPECT_CALL(unit, hasClone()).Times(AtLeast(1)).WillRepeatedly(Return(true));
	EXPECT_CALL(unit, isClone()).WillRepeatedly(Return(false));

	EXPECT_CALL(unit, getPosition()).WillOnce(Return(BattleHex(5, 5)));
	EffectTarget target;
	target.emplace_back(&unit);

	EXPECT_FALSE(subject->applicable(problemMock, &mechanicsMock, target));
}

class CloneApplyTest : public Test, public EffectFixture
{
public:
	std::shared_ptr<::battle::UnitInfo> cloneAddInfo;
	std::shared_ptr<::battle::CUnitState> cloneState;
	std::shared_ptr<::battle::CUnitState> originalState;

	const uint32_t originalId = 40;
	const uint32_t cloneId = 42;
	const int32_t expectedAmount = 10;
	const int32_t effectDuration = 6;
	const BattleHex originalPosition = BattleHex(5,5);

	EffectTarget target;

	CloneApplyTest()
		: EffectFixture("core:clone")
	{
	}

    void onUnitAdded(uint32_t id, const JsonNode & data)
	{
		using namespace ::battle;

		auto & clone = unitsFake.add(BattleSide::ATTACKER);

		EXPECT_CALL(clone, unitId()).WillRepeatedly(Return(id));

		cloneState = std::make_shared<CUnitStateDetached>(&clone, &clone);

		EXPECT_CALL(clone, acquireState()).WillOnce(Return(cloneState));

		cloneAddInfo->load(id, data);
	}

	void checkCloneLifetimeMarker(uint32_t id, const std::vector<Bonus> & bonus)
	{
		EXPECT_EQ(id, cloneId);

		GTEST_ASSERT_EQ(bonus.size(), 1);

		const Bonus & marker = bonus.front();

		EXPECT_EQ(marker.type, BonusType::NONE);
		EXPECT_EQ(marker.duration, BonusDuration::N_TURNS);
		EXPECT_EQ(marker.turnsRemain, effectDuration);
		EXPECT_EQ(marker.source, BonusSource::SPELL_EFFECT);
		EXPECT_EQ(marker.sid, SpellID::CLONE);
	}

	void setDefaultExpectations()
	{
		using namespace ::battle;

		battleFake->setupEmptyBattlefield();

		EXPECT_CALL(serverMock, apply(Matcher<BattleUnitsChanged *>(_))).Times(2);
		EXPECT_CALL(serverMock, apply(Matcher<SetStackEffect *>(_))).Times(1);

		EXPECT_CALL(mechanicsMock, getEffectDuration()).WillOnce(Return(effectDuration));
		EXPECT_CALL(*battleFake, getUnitsIf(_)).Times(AtLeast(1));

		EXPECT_CALL(*battleFake, nextUnitId()).WillOnce(Return(cloneId));
		EXPECT_CALL(*battleFake, addUnit(_, _)).WillOnce(Invoke(this, &CloneApplyTest::onUnitAdded));

		EXPECT_CALL(*battleFake, setUnitState(Eq(originalId), _, Eq(0))).Times(AtLeast(1));
		EXPECT_CALL(*battleFake, setUnitState(Eq(cloneId), _, Eq(0))).Times(AtLeast(1));

		auto & original = unitsFake.add(BattleSide::ATTACKER);
		EXPECT_CALL(original, isValidTarget(Eq(false))).Times(AtLeast(1)).WillRepeatedly(Return(true));
		EXPECT_CALL(original, getCount()).WillRepeatedly(Return(expectedAmount));
		EXPECT_CALL(original, unitId()).WillRepeatedly(Return(originalId));
		EXPECT_CALL(original, creatureId()).WillRepeatedly(Return(CreatureID(0)));
		EXPECT_CALL(original, creatureIndex()).WillRepeatedly(Return(0));
		EXPECT_CALL(original, doubleWide()).WillRepeatedly(Return(false));
		EXPECT_CALL(original, getPosition()).WillRepeatedly(Return(originalPosition));
		EXPECT_CALL(original, unitSide()).Times(AnyNumber());

		originalState = std::make_shared<CUnitStateDetached>(&original, &original);

		EXPECT_CALL(original, acquireState()).WillOnce(Return(originalState));

		target.emplace_back(&original);
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
		setupEffect(JsonNode());

		cloneAddInfo = std::make_shared<::battle::UnitInfo>();
	}

};

TEST_F(CloneApplyTest, AddsNewUnit)
{
	setDefaultExpectations();

	EXPECT_CALL(*battleFake, addUnitBonus(_,_)).Times(AtLeast(1));

	subject->apply(&serverMock, &mechanicsMock, target);

	EXPECT_EQ(cloneAddInfo->id, cloneId);
	EXPECT_EQ(cloneAddInfo->count, expectedAmount);
	EXPECT_EQ(cloneAddInfo->type, CreatureID(0));
	EXPECT_EQ(cloneAddInfo->side, BattleSide::ATTACKER);
	EXPECT_TRUE(cloneAddInfo->position.isValid());
	EXPECT_NE(cloneAddInfo->position, originalPosition);
	EXPECT_TRUE(cloneAddInfo->summoned);
}

TEST_F(CloneApplyTest, SetsClonedFlag)
{
	setDefaultExpectations();

	EXPECT_CALL(*battleFake, addUnitBonus(_,_)).Times(AtLeast(1));

	subject->apply(&serverMock, &mechanicsMock, target);

	GTEST_ASSERT_NE(cloneState, nullptr);

	EXPECT_TRUE(cloneState->cloned);
}

TEST_F(CloneApplyTest, SetsCloneLink)
{
	setDefaultExpectations();

	EXPECT_CALL(*battleFake, addUnitBonus(_,_)).Times(AtLeast(1));

	subject->apply(&serverMock, &mechanicsMock, target);

	EXPECT_EQ(originalState->cloneID, cloneId);
}

TEST_F(CloneApplyTest, SetsLifetimeMarker)
{
	setDefaultExpectations();

	EXPECT_CALL(*battleFake, addUnitBonus(_, _)).WillOnce(Invoke(this, &CloneApplyTest::checkCloneLifetimeMarker));

	subject->apply(&serverMock, &mechanicsMock, target);
}

}
