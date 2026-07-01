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
#include "../../../lib/json/JsonNode.h"

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

	Target target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_TRUE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(DamageTest, NotApplicableToDeadUnit)
{
	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillOnce(Return(false));

	EXPECT_CALL(mechanicsMock, isSmart()).Times(AtMost(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).Times(AtMost(1)).WillRepeatedly(Return(true));

	Target target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(DamageTest, AnySchoolImmunityNotReceptive)
{
	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	unit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::SPELL_DAMAGE_REDUCTION,
		BonusSource::CREATURE_ABILITY, 100, BonusSourceID(), BonusSubtypeID(SpellSchool::ANY)));
	unit.expectAnyBonusSystemCall();

	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillRepeatedly(Return(true));
	EXPECT_CALL(unit, isInvincible()).WillRepeatedly(Return(false));

	EXPECT_CALL(mechanicsMock, isReceptive(Eq(&unit))).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, getSpell()).WillRepeatedly(Return(&spellStub));
	EXPECT_CALL(mechanicsMock, adaptProblem(Eq(ESpellCastProblem::NO_APPROPRIATE_TARGET), Ref(problemMock))).WillOnce(Return(false));

	EXPECT_CALL(spellStub, isMagical()).WillRepeatedly(Return(true));
	EXPECT_CALL(spellStub, forEachSchool(_)).WillRepeatedly(Return());

	EXPECT_FALSE(subject->applicableGeneral(problemMock, &mechanicsMock));
}

TEST_F(DamageTest, SchoolImmunityNotReceptive)
{
	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	unit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::SPELL_DAMAGE_REDUCTION,
		BonusSource::CREATURE_ABILITY, 100, BonusSourceID(), BonusSubtypeID(SpellSchool::FIRE)));
	unit.expectAnyBonusSystemCall();

	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillRepeatedly(Return(true));
	EXPECT_CALL(unit, isInvincible()).WillRepeatedly(Return(false));

	EXPECT_CALL(mechanicsMock, isReceptive(Eq(&unit))).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, getSpell()).WillRepeatedly(Return(&spellStub));
	EXPECT_CALL(mechanicsMock, adaptProblem(Eq(ESpellCastProblem::NO_APPROPRIATE_TARGET), Ref(problemMock))).WillOnce(Return(false));

	EXPECT_CALL(spellStub, isMagical()).WillRepeatedly(Return(true));
	EXPECT_CALL(spellStub, forEachSchool(_)).WillRepeatedly(Invoke([](const Spell::SchoolCallback & cb)
	{
		bool stop = false;
		cb(SpellSchool::FIRE, stop);
	}));

	EXPECT_FALSE(subject->applicableGeneral(problemMock, &mechanicsMock));
}

TEST_F(DamageTest, PartialReductionNotImmune)
{
	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	unit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::SPELL_DAMAGE_REDUCTION,
		BonusSource::CREATURE_ABILITY, 99, BonusSourceID(), BonusSubtypeID(SpellSchool::ANY)));
	unit.expectAnyBonusSystemCall();

	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillRepeatedly(Return(true));
	EXPECT_CALL(unit, isInvincible()).WillRepeatedly(Return(false));

	EXPECT_CALL(mechanicsMock, isReceptive(Eq(&unit))).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, getSpell()).WillRepeatedly(Return(&spellStub));
	EXPECT_CALL(mechanicsMock, isSmart()).WillRepeatedly(Return(false));

	EXPECT_CALL(spellStub, isMagical()).WillRepeatedly(Return(true));
	EXPECT_CALL(spellStub, forEachSchool(_)).WillRepeatedly(Return());

	EXPECT_TRUE(subject->applicableGeneral(problemMock, &mechanicsMock));
}

TEST_F(DamageTest, NonMagicalSpellIgnoresDamageReduction)
{
	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	unit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::SPELL_DAMAGE_REDUCTION,
		BonusSource::CREATURE_ABILITY, 100, BonusSourceID(), BonusSubtypeID(SpellSchool::ANY)));
	unit.expectAnyBonusSystemCall();

	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillRepeatedly(Return(true));
	EXPECT_CALL(unit, isInvincible()).WillRepeatedly(Return(false));

	EXPECT_CALL(mechanicsMock, isReceptive(Eq(&unit))).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, getSpell()).WillRepeatedly(Return(&spellStub));
	EXPECT_CALL(mechanicsMock, isSmart()).WillRepeatedly(Return(false));

	EXPECT_CALL(spellStub, isMagical()).WillRepeatedly(Return(false));
	EXPECT_CALL(spellStub, forEachSchool(_)).WillRepeatedly(Return()); // no schools → no per-school immunity

	EXPECT_TRUE(subject->applicableGeneral(problemMock, &mechanicsMock));
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

	targetUnit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, unitHP, BonusSourceID()));
	EXPECT_CALL(targetUnit, unitId()).WillRepeatedly(Return(unitId));
	EXPECT_CALL(targetUnit, unitBaseAmount()).WillRepeatedly(Return(unitAmount));
	EXPECT_CALL(targetUnit, alive()).WillRepeatedly(Return(true));

	EXPECT_CALL(mechanicsMock, adjustEffectValue(Eq(&targetUnit))).WillOnce(Return(effectValue));

	unitsFake.setDefaultBonusExpectations();

	auto targetUnitState = std::make_shared<CUnitStateDetached>(&targetUnit, &targetUnit);
	targetUnitState->localInit(&unitEnvironmentMock);
	EXPECT_CALL(targetUnit, acquireState()).WillOnce(Return(targetUnitState));
	EXPECT_CALL(*battleFake, updateUnit(Eq(unitId),_, Lt(0))).Times(1);
	EXPECT_CALL(serverMock, apply(Matcher<StacksInjured &>(_))).Times(1);
	EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));

	setupDefaultRNG();

	Target target;
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
	EXPECT_CALL(*battleFake, updateUnit(_,_,_)).Times(0);
	EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));

	setupDefaultRNG();

	Target target;
	target.emplace_back(&targetUnit, BattleHex());

	subject->apply(&serverMock, &mechanicsMock, target);
}

TEST_F(DamageApplyTest, DoesDamageByPercent)
{
	using namespace ::battle;

	{
		JsonNode config;
		config["killByPercentage"].Bool() = true;
		EffectFixture::setupEffect(config);
	}

	const int64_t effectValue = 27;
	const int32_t unitAmount = 200;
	const int32_t unitHP = 100;
	const uint32_t unitId = 42;
	auto & targetUnit = unitsFake.add(BattleSide::ATTACKER);

	targetUnit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, unitHP, BonusSourceID()));
	EXPECT_CALL(targetUnit, unitId()).WillRepeatedly(Return(unitId));
	EXPECT_CALL(targetUnit, unitBaseAmount()).WillRepeatedly(Return(unitAmount));
	EXPECT_CALL(targetUnit, getCount()).WillOnce(Return(unitAmount));
	EXPECT_CALL(targetUnit, alive()).WillRepeatedly(Return(true));

	EXPECT_CALL(mechanicsMock, adjustEffectValue(Eq(&targetUnit))).Times(0);
	EXPECT_CALL(mechanicsMock, getEffectValue()).WillOnce(Return(effectValue));

	unitsFake.setDefaultBonusExpectations();

	auto targetUnitState = std::make_shared<CUnitStateDetached>(&targetUnit, &targetUnit);
	targetUnitState->localInit(&unitEnvironmentMock);
	EXPECT_CALL(targetUnit, acquireState()).WillOnce(Return(targetUnitState));

	EXPECT_CALL(*battleFake, updateUnit(Eq(unitId),_, Lt(0))).Times(1);
	EXPECT_CALL(serverMock, apply(Matcher<StacksInjured &>(_))).Times(1);
	EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));

	setupDefaultRNG();

	Target target;
	target.emplace_back(&targetUnit, BattleHex());

	subject->apply(&serverMock, &mechanicsMock, target);

	EXPECT_EQ(targetUnitState->getCount(), unitAmount - (unitAmount * effectValue / 100));
}

TEST_F(DamageApplyTest, DoesDamageByCount)
{
	using namespace ::battle;

	{
		JsonNode config;
		config["killByCount"].Bool() = true;
		EffectFixture::setupEffect(config);
	}

	const int64_t effectValue = 27;
	const int32_t unitAmount = 200;
	const int32_t unitHP = 100;
	const uint32_t unitId = 42;
	auto & targetUnit = unitsFake.add(BattleSide::ATTACKER);

	targetUnit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, unitHP, BonusSourceID()));
	EXPECT_CALL(targetUnit, unitId()).WillRepeatedly(Return(unitId));
	EXPECT_CALL(targetUnit, unitBaseAmount()).WillRepeatedly(Return(unitAmount));
	EXPECT_CALL(targetUnit, alive()).WillRepeatedly(Return(true));

	EXPECT_CALL(mechanicsMock, adjustEffectValue(Eq(&targetUnit))).Times(0);
	EXPECT_CALL(mechanicsMock, getEffectValue()).WillOnce(Return(effectValue));

	unitsFake.setDefaultBonusExpectations();

	auto targetUnitState = std::make_shared<CUnitStateDetached>(&targetUnit, &targetUnit);
	targetUnitState->localInit(&unitEnvironmentMock);
	EXPECT_CALL(targetUnit, acquireState()).WillOnce(Return(targetUnitState));

	EXPECT_CALL(*battleFake, updateUnit(Eq(unitId), _, Lt(0))).Times(1);
	EXPECT_CALL(serverMock, apply(Matcher<StacksInjured &>(_))).Times(1);
	EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));

	setupDefaultRNG();

	Target target;
	target.emplace_back(&targetUnit, BattleHex());

	subject->apply(&serverMock, &mechanicsMock, target);

	EXPECT_EQ(targetUnitState->getCount(), unitAmount - effectValue);
}

TEST_F(DamageApplyTest, GetHealthChangeAliveUnit)
{
	EffectFixture::setupEffect(JsonNode());
	using namespace ::battle;

	const int64_t effectValue = 123;
	const int32_t unitAmount = 25;
	const int32_t unitHP = 100;
	const uint32_t unitId = 42;
	auto & targetUnit = unitsFake.add(BattleSide::ATTACKER);

	targetUnit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, unitHP, BonusSourceID()));
	EXPECT_CALL(targetUnit, unitId()).WillRepeatedly(Return(unitId));
	EXPECT_CALL(targetUnit, unitBaseAmount()).WillRepeatedly(Return(unitAmount));
	EXPECT_CALL(targetUnit, alive()).WillRepeatedly(Return(true));

	EXPECT_CALL(mechanicsMock, adjustEffectValue(Eq(&targetUnit))).WillOnce(Return(effectValue));

	unitsFake.setDefaultBonusExpectations();

	auto targetUnitState = std::make_shared<CUnitStateDetached>(&targetUnit, &targetUnit);
	targetUnitState->localInit(&unitEnvironmentMock);
	EXPECT_CALL(targetUnit, acquireState()).WillOnce(Return(targetUnitState));

	Target target;
	target.emplace_back(&targetUnit, BattleHex());

	auto result = subject->getHealthChange(&mechanicsMock, target);

	EXPECT_EQ(result.hpDelta, -effectValue);
	EXPECT_EQ(result.unitsDelta, -1);
}

TEST_F(DamageApplyTest, AppliesChainFactorToSubsequentTargets)
{
	{
		JsonNode config;
		config["chainLength"].Integer() = 2;
		config["chainFactor"].Float() = 0.5;
		EffectFixture::setupEffect(config);
	}
	using namespace ::battle;

	const int64_t effectValue = 200;
	const int32_t unitAmount = 25;
	const int32_t unitHP = 100;

	auto & firstUnit = unitsFake.add(BattleSide::ATTACKER);
	firstUnit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, unitHP, BonusSourceID()));
	const uint32_t firstId = 41;
	EXPECT_CALL(firstUnit, unitId()).WillRepeatedly(Return(firstId));
	EXPECT_CALL(firstUnit, unitBaseAmount()).WillRepeatedly(Return(unitAmount));
	EXPECT_CALL(firstUnit, alive()).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, adjustEffectValue(Eq(&firstUnit))).WillOnce(Return(effectValue));
	auto firstState = std::make_shared<CUnitStateDetached>(&firstUnit, &firstUnit);
	firstState->localInit(&unitEnvironmentMock);
	EXPECT_CALL(firstUnit, acquireState()).WillOnce(Return(firstState));
	EXPECT_CALL(*battleFake, updateUnit(Eq(firstId), _, Lt(0))).Times(1);

	auto & secondUnit = unitsFake.add(BattleSide::ATTACKER);
	secondUnit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, unitHP, BonusSourceID()));
	const uint32_t secondId = 42;
	EXPECT_CALL(secondUnit, unitId()).WillRepeatedly(Return(secondId));
	EXPECT_CALL(secondUnit, unitBaseAmount()).WillRepeatedly(Return(unitAmount));
	EXPECT_CALL(secondUnit, alive()).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, adjustEffectValue(Eq(&secondUnit))).WillOnce(Return(effectValue));
	auto secondState = std::make_shared<CUnitStateDetached>(&secondUnit, &secondUnit);
	secondState->localInit(&unitEnvironmentMock);
	EXPECT_CALL(secondUnit, acquireState()).WillOnce(Return(secondState));
	EXPECT_CALL(*battleFake, updateUnit(Eq(secondId), _, Lt(0))).Times(1);

	unitsFake.setDefaultBonusExpectations();

	EXPECT_CALL(serverMock, apply(Matcher<StacksInjured &>(_))).Times(2);
	EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));

	setupDefaultRNG();

	Target target;
	target.emplace_back(&firstUnit, BattleHex());
	target.emplace_back(&secondUnit, BattleHex());

	subject->apply(&serverMock, &mechanicsMock, target);

	// First target takes full damage: 200 hp → kills 2 creatures (2 × 100 HP)
	EXPECT_EQ(firstState->getCount(), unitAmount - 2);
	// Second target takes half damage (chainFactor^1 = 0.5): 100 hp → kills 1 creature
	EXPECT_EQ(secondState->getCount(), unitAmount - 1);
}

}
