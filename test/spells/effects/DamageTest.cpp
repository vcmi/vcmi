/*
 * DamageTest.cpp, part of VCMI engine
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

#include "mock/mock_UnitEnvironment.h"

namespace test
{
using namespace ::spells;
using namespace ::spells::effects;
using namespace ::testing;

class DamageTest : public Test, public EffectFixture
{
public:
	DamageTest()
		: EffectFixture("core:damage")
	{
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
		EffectFixture::setupEffect(JsonNode());
	}
};

TEST_F(DamageTest, ApplicableToAliveUnit)
{
	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillOnce(Return(true));

	EXPECT_CALL(mechanicsMock, isSmart()).WillOnce(Return(false));

	EffectTarget target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_TRUE(subject->applicable(problemMock, &mechanicsMock, target));
}

TEST_F(DamageTest, NotApplicableToDeadUnit)
{
	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillOnce(Return(false));

	EXPECT_CALL(mechanicsMock, isSmart()).Times(AtMost(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).Times(AtMost(1)).WillRepeatedly(Return(true));

	EffectTarget target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicable(problemMock, &mechanicsMock, target));
}

class DamageApplyTest : public Test, public EffectFixture
{
public:
	UnitEnvironmentMock unitEnvironmentMock;

	DamageApplyTest()
		: EffectFixture("core:damage")
	{
	}


protected:
	void SetUp() override
	{
		EffectFixture::setUp();
	}
};

TEST_F(DamageApplyTest, DoesDamageToAliveUnit)
{
	EffectFixture::setupEffect(JsonNode());
	using namespace ::battle;

	const int64_t effectValue = 123;
	const int32_t unitAmount = 25;
	const int32_t unitHP = 100;
	const uint32_t unitId = 42;
	auto & targetUnit = unitsFake.add(BattleSide::ATTACKER);

	targetUnit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, unitHP, 0));
	EXPECT_CALL(targetUnit, unitId()).WillRepeatedly(Return(unitId));
	EXPECT_CALL(targetUnit, unitBaseAmount()).WillRepeatedly(Return(unitAmount));
	EXPECT_CALL(targetUnit, alive()).WillRepeatedly(Return(true));

	EXPECT_CALL(mechanicsMock, adjustEffectValue(Eq(&targetUnit))).WillOnce(Return(effectValue));

	unitsFake.setDefaultBonusExpectations();

	std::shared_ptr<CUnitState> targetUnitState = std::make_shared<CUnitStateDetached>(&targetUnit, &targetUnit);
	targetUnitState->localInit(&unitEnvironmentMock);
	EXPECT_CALL(targetUnit, acquireState()).WillOnce(Return(targetUnitState));
	EXPECT_CALL(*battleFake, setUnitState(Eq(unitId),_, Lt(0))).Times(1);
	EXPECT_CALL(serverMock, apply(Matcher<StacksInjured *>(_))).Times(1);
	EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));

	setupDefaultRNG();

	EffectTarget target;
	target.emplace_back(&targetUnit, BattleHex());

	subject->apply(&serverMock, &mechanicsMock, target);

	EXPECT_EQ(targetUnitState->getCount(), unitAmount - 1);
}

TEST_F(DamageApplyTest, IgnoresDeadUnit)
{
	EffectFixture::setupEffect(JsonNode());
	using namespace ::battle;

	auto & targetUnit = unitsFake.add(BattleSide::ATTACKER);

	EXPECT_CALL(targetUnit, alive()).WillRepeatedly(Return(false));
	EXPECT_CALL(targetUnit, acquireState()).Times(0);
	EXPECT_CALL(*battleFake, setUnitState(_,_,_)).Times(0);
	EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));

	setupDefaultRNG();

	EffectTarget target;
	target.emplace_back(&targetUnit, BattleHex());

	subject->apply(&serverMock, &mechanicsMock, target);
}

TEST_F(DamageApplyTest, DoesDamageByPercent)
{
	using namespace ::battle;

	{
		JsonNode config(JsonNode::JsonType::DATA_STRUCT);
		config["killByPercentage"].Bool() = true;
		EffectFixture::setupEffect(config);
	}

	const int64_t effectValue = 27;
	const int32_t unitAmount = 200;
	const int32_t unitHP = 100;
	const uint32_t unitId = 42;
	auto & targetUnit = unitsFake.add(BattleSide::ATTACKER);

	targetUnit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, unitHP, 0));
	EXPECT_CALL(targetUnit, unitId()).WillRepeatedly(Return(unitId));
	EXPECT_CALL(targetUnit, unitBaseAmount()).WillRepeatedly(Return(unitAmount));
	EXPECT_CALL(targetUnit, getCount()).WillOnce(Return(unitAmount));
	EXPECT_CALL(targetUnit, alive()).WillRepeatedly(Return(true));

	EXPECT_CALL(mechanicsMock, adjustEffectValue(Eq(&targetUnit))).Times(0);
	EXPECT_CALL(mechanicsMock, getEffectValue()).WillOnce(Return(effectValue));

	unitsFake.setDefaultBonusExpectations();

	std::shared_ptr<CUnitState> targetUnitState = std::make_shared<CUnitStateDetached>(&targetUnit, &targetUnit);
	targetUnitState->localInit(&unitEnvironmentMock);
	EXPECT_CALL(targetUnit, acquireState()).WillOnce(Return(targetUnitState));

	EXPECT_CALL(*battleFake, setUnitState(Eq(unitId),_, Lt(0))).Times(1);
	EXPECT_CALL(serverMock, apply(Matcher<StacksInjured *>(_))).Times(1);
	EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));

	setupDefaultRNG();

	EffectTarget target;
	target.emplace_back(&targetUnit, BattleHex());

	subject->apply(&serverMock, &mechanicsMock, target);

	EXPECT_EQ(targetUnitState->getCount(), unitAmount - (unitAmount * effectValue / 100));
}

TEST_F(DamageApplyTest, DoesDamageByCount)
{
	using namespace ::battle;

	{
		JsonNode config(JsonNode::JsonType::DATA_STRUCT);
		config["killByCount"].Bool() = true;
		EffectFixture::setupEffect(config);
	}

	const int64_t effectValue = 27;
	const int32_t unitAmount = 200;
	const int32_t unitHP = 100;
	const uint32_t unitId = 42;
	auto & targetUnit = unitsFake.add(BattleSide::ATTACKER);

	targetUnit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, unitHP, 0));
	EXPECT_CALL(targetUnit, unitId()).WillRepeatedly(Return(unitId));
	EXPECT_CALL(targetUnit, unitBaseAmount()).WillRepeatedly(Return(unitAmount));
	EXPECT_CALL(targetUnit, alive()).WillRepeatedly(Return(true));

	EXPECT_CALL(mechanicsMock, adjustEffectValue(Eq(&targetUnit))).Times(0);
	EXPECT_CALL(mechanicsMock, getEffectValue()).WillOnce(Return(effectValue));

	unitsFake.setDefaultBonusExpectations();

	std::shared_ptr<CUnitState> targetUnitState = std::make_shared<CUnitStateDetached>(&targetUnit, &targetUnit);
	targetUnitState->localInit(&unitEnvironmentMock);
	EXPECT_CALL(targetUnit, acquireState()).WillOnce(Return(targetUnitState));

	EXPECT_CALL(*battleFake, setUnitState(Eq(unitId), _, Lt(0))).Times(1);
	EXPECT_CALL(serverMock, apply(Matcher<StacksInjured *>(_))).Times(1);
	EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));

	setupDefaultRNG();

	EffectTarget target;
	target.emplace_back(&targetUnit, BattleHex());

	subject->apply(&serverMock, &mechanicsMock, target);

	EXPECT_EQ(targetUnitState->getCount(), unitAmount - effectValue);
}

}
