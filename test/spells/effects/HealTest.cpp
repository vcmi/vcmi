/*
 * HealTest.cpp, part of VCMI engine
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

class HealTest : public Test, public EffectFixture
{
public:

	HealTest()
		: EffectFixture("core:heal")
	{
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
	}
};

TEST_F(HealTest, NotApplicableToHealthyUnit)
{
	EffectFixture::setupEffect(JsonNode());
	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillOnce(Return(true));
	EXPECT_CALL(unit, getTotalHealth()).WillOnce(Return(200));
	EXPECT_CALL(unit, getAvailableHealth()).WillOnce(Return(200));

	EXPECT_CALL(mechanicsMock, isSmart()).Times(AtMost(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).Times(AtMost(1)).WillRepeatedly(Return(true));

	EffectTarget target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicable(problemMock, &mechanicsMock, target));
}

TEST_F(HealTest, ApplicableToWoundedUnit)
{
	EffectFixture::setupEffect(JsonNode());
	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	unit.makeAlive();
	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillOnce(Return(true));
	EXPECT_CALL(unit, getTotalHealth()).WillOnce(Return(200));
	EXPECT_CALL(unit, getAvailableHealth()).WillOnce(Return(100));

	EXPECT_CALL(mechanicsMock, isSmart()).WillOnce(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).WillOnce(Return(true));

	EffectTarget target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_TRUE(subject->applicable(problemMock, &mechanicsMock, target));
}

TEST_F(HealTest, ApplicableIfActuallyResurrects)
{
	{
		JsonNode config(JsonNode::JsonType::DATA_STRUCT);
		config["healLevel"].String() = "resurrect";
		config["minFullUnits"].Integer() = 5;
		EffectFixture::setupEffect(config);
	}

	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	unit.makeAlive();
	EXPECT_CALL(unit, isValidTarget(Eq(true))).WillOnce(Return(true));
	EXPECT_CALL(unit, getTotalHealth()).WillOnce(Return(20000));
	EXPECT_CALL(unit, getAvailableHealth()).WillOnce(Return(100));

	EXPECT_CALL(mechanicsMock, getEffectValue()).Times(AtLeast(1)).WillRepeatedly(Return(1000));
	EXPECT_CALL(mechanicsMock, isSmart()).WillOnce(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).WillOnce(Return(true));

	unit.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::STACK_HEALTH, Bonus::CREATURE_ABILITY, 200, 0));
	unitsFake.setDefaultBonusExpectations();

	EffectTarget target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_TRUE(subject->applicable(problemMock, &mechanicsMock, target));
}

TEST_F(HealTest, NotApplicableIfNotEnoughCasualties)
{
	{
		JsonNode config(JsonNode::JsonType::DATA_STRUCT);
		config["healLevel"].String() = "resurrect";
		config["minFullUnits"].Integer() = 1;
		EffectFixture::setupEffect(config);
	}

	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit, isValidTarget(Eq(true))).WillOnce(Return(true));
	EXPECT_CALL(unit, getTotalHealth()).WillOnce(Return(200));
	EXPECT_CALL(unit, getAvailableHealth()).WillOnce(Return(100));

	EXPECT_CALL(mechanicsMock, getEffectValue()).Times(AtLeast(1)).WillRepeatedly(Return(999));
	EXPECT_CALL(mechanicsMock, isSmart()).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).WillRepeatedly(Return(true));

	unit.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::STACK_HEALTH, Bonus::CREATURE_ABILITY, 200, 0));
	unitsFake.setDefaultBonusExpectations();

	EffectTarget target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicable(problemMock, &mechanicsMock, target));
}

TEST_F(HealTest, NotApplicableIfResurrectsLessThanRequired)
{
	{
		JsonNode config(JsonNode::JsonType::DATA_STRUCT);
		config["healLevel"].String() = "resurrect";
		config["minFullUnits"].Integer() = 5;
		EffectFixture::setupEffect(config);
	}

	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(unit, isValidTarget(Eq(true))).WillOnce(Return(true));
	EXPECT_CALL(unit, getTotalHealth()).WillOnce(Return(20000));
	EXPECT_CALL(unit, getAvailableHealth()).WillOnce(Return(100));

	EXPECT_CALL(mechanicsMock, getEffectValue()).Times(AtLeast(1)).WillRepeatedly(Return(999));
	EXPECT_CALL(mechanicsMock, isSmart()).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).WillRepeatedly(Return(true));

	unit.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::STACK_HEALTH, Bonus::CREATURE_ABILITY, 200, 0));
	unitsFake.setDefaultBonusExpectations();

	EffectTarget target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicable(problemMock, &mechanicsMock, target));
}

TEST_F(HealTest, ApplicableToDeadUnit)
{
	{
		JsonNode config(JsonNode::JsonType::DATA_STRUCT);
		config["healLevel"].String() = "resurrect";
		EffectFixture::setupEffect(config);
	}

	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	unit.makeDead();

	EXPECT_CALL(unit, isValidTarget(Eq(true))).WillOnce(Return(true));
	EXPECT_CALL(unit, getTotalHealth()).WillOnce(Return(200));
	EXPECT_CALL(unit, getAvailableHealth()).WillOnce(Return(0));
	EXPECT_CALL(unit, unitSide()).Times(AnyNumber());

	EXPECT_CALL(unit, getPosition()).WillRepeatedly(Return(BattleHex(5,5)));
	EXPECT_CALL(unit, doubleWide()).WillRepeatedly(Return(false));
	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillRepeatedly(Return(false));


	EXPECT_CALL(mechanicsMock, isSmart()).WillOnce(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).WillOnce(Return(true));

	EXPECT_CALL(*battleFake, getUnitsIf(_)).Times(AtLeast(0));

	EffectTarget target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_TRUE(subject->applicable(problemMock, &mechanicsMock, target));
}

TEST_F(HealTest, NotApplicableIfDeadUnitIsBlocked)
{
	{
		JsonNode config(JsonNode::JsonType::DATA_STRUCT);
		config["healLevel"].String() = "resurrect";
		EffectFixture::setupEffect(config);
	}

	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	unit.makeDead();

	EXPECT_CALL(unit, isValidTarget(Eq(true))).WillOnce(Return(true));
	EXPECT_CALL(unit, getTotalHealth()).WillOnce(Return(200));
	EXPECT_CALL(unit, getAvailableHealth()).WillOnce(Return(0));

	EXPECT_CALL(unit, getPosition()).WillRepeatedly(Return(BattleHex(5,5)));
	EXPECT_CALL(unit, doubleWide()).WillRepeatedly(Return(false));
	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillRepeatedly(Return(false));
	EXPECT_CALL(unit, unitSide()).Times(AnyNumber());

	auto & blockingUnit = unitsFake.add(BattleSide::ATTACKER);

	EXPECT_CALL(blockingUnit, getPosition()).Times(AtLeast(1)).WillRepeatedly(Return(BattleHex(5,5)));
	EXPECT_CALL(blockingUnit, doubleWide()).WillRepeatedly(Return(false));
	EXPECT_CALL(blockingUnit, isValidTarget(Eq(false))).Times(AtLeast(1)).WillRepeatedly(Return(true));
	EXPECT_CALL(blockingUnit, unitSide()).Times(AnyNumber());

	EXPECT_CALL(mechanicsMock, isSmart()).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).WillRepeatedly(Return(true));

	EXPECT_CALL(*battleFake, getUnitsIf(_)).Times(AtLeast(1));

	EffectTarget target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicable(problemMock, &mechanicsMock, target));
}

TEST_F(HealTest, ApplicableWithAnotherDeadUnitInSamePosition)
{
	{
		JsonNode config(JsonNode::JsonType::DATA_STRUCT);
		config["healLevel"].String() = "resurrect";
		EffectFixture::setupEffect(config);
	}

	auto & unit = unitsFake.add(BattleSide::ATTACKER);
	unit.makeDead();

	EXPECT_CALL(unit, isValidTarget(Eq(true))).WillOnce(Return(true));
	EXPECT_CALL(unit, getTotalHealth()).WillOnce(Return(200));
	EXPECT_CALL(unit, getAvailableHealth()).WillOnce(Return(0));

	EXPECT_CALL(unit, getPosition()).WillRepeatedly(Return(BattleHex(5,5)));
	EXPECT_CALL(unit, doubleWide()).WillRepeatedly(Return(false));
	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillRepeatedly(Return(false));
	EXPECT_CALL(unit, unitSide()).Times(AnyNumber());

	auto & blockingUnit = unitsFake.add(BattleSide::ATTACKER);

	EXPECT_CALL(blockingUnit, getPosition()).WillRepeatedly(Return(BattleHex(5,5)));
	EXPECT_CALL(blockingUnit, doubleWide()).WillRepeatedly(Return(false));
	EXPECT_CALL(blockingUnit, isValidTarget(Eq(false))).Times(AtLeast(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(blockingUnit, unitSide()).Times(AnyNumber());

	EXPECT_CALL(mechanicsMock, isSmart()).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).WillRepeatedly(Return(true));

	EXPECT_CALL(*battleFake, getUnitsIf(_)).Times(AtLeast(1));

	EffectTarget target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_TRUE(subject->applicable(problemMock, &mechanicsMock, target));
}

TEST_F(HealTest, NotApplicableIfEffectValueTooLow)
{
	{
		JsonNode config(JsonNode::JsonType::DATA_STRUCT);
		config["minFullUnits"].Integer() = 1;
		EffectFixture::setupEffect(config);
	}

	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillOnce(Return(true));
	EXPECT_CALL(unit, getTotalHealth()).WillOnce(Return(200));
	EXPECT_CALL(unit, getAvailableHealth()).WillOnce(Return(100));

	unit.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::STACK_HEALTH, Bonus::CREATURE_ABILITY, 200, 0));

	EXPECT_CALL(mechanicsMock, getEffectValue()).Times(AtLeast(1)).WillRepeatedly(Return(199));

	EXPECT_CALL(mechanicsMock, isSmart()).Times(AtMost(1)).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).Times(AtMost(1)).WillRepeatedly(Return(true));

	unitsFake.setDefaultBonusExpectations();

	EffectTarget target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicable(problemMock, &mechanicsMock, target));
}

class HealApplyTest : public TestWithParam<::testing::tuple<EHealLevel, EHealPower>>, public EffectFixture
{
public:
	UnitEnvironmentMock unitEnvironmentMock;

	EHealLevel healLevel;
	EHealPower healPower;

	std::array<std::string, 3> HEAL_LEVEL_MAP;

	std::array<std::string, 2> HEAL_POWER_MAP;

	HealApplyTest()
		: EffectFixture("core:heal")
	{
		HEAL_LEVEL_MAP =
		{
			"heal",
			"resurrect",
			"overHeal"
		};
		HEAL_POWER_MAP =
		{
			"oneBattle",
			"permanent"
		};
	}


protected:
	void SetUp() override
	{
		EffectFixture::setUp();

		healLevel = ::testing::get<0>(GetParam());
		healPower = ::testing::get<1>(GetParam());
	}
};

TEST_P(HealApplyTest, Heals)
{
	{
		JsonNode config(JsonNode::JsonType::DATA_STRUCT);
		config["healLevel"].String() = HEAL_LEVEL_MAP.at(static_cast<size_t>(healLevel));
		config["healPower"].String() = HEAL_POWER_MAP.at(static_cast<size_t>(healPower));
		EffectFixture::setupEffect(config);
	}

	using namespace ::battle;

	const int64_t effectValue = 1000;
	const int32_t unitAmount = 24;
	const int32_t unitHP = 100;
	const uint32_t unitId = 42;
	const auto pikeman = CreatureID(unitId).toCreature();


	auto & targetUnit = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(targetUnit, unitBaseAmount()).WillRepeatedly(Return(unitAmount));
	EXPECT_CALL(targetUnit, unitId()).WillRepeatedly(Return(unitId));
	EXPECT_CALL(targetUnit, unitType()).WillRepeatedly(Return(pikeman));

	targetUnit.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::STACK_HEALTH, Bonus::CREATURE_ABILITY, unitHP, 0));

	unitsFake.setDefaultBonusExpectations();

	std::shared_ptr<CUnitState> targetUnitState = std::make_shared<CUnitStateDetached>(&targetUnit, &targetUnit);
	targetUnitState->localInit(&unitEnvironmentMock);
	{
		int64_t initialDmg = unitAmount * unitHP / 2 - 1;
		targetUnitState->health.damage(initialDmg);
	}

	EXPECT_CALL(mechanicsMock, getEffectValue()).WillRepeatedly(Return(effectValue));
	EXPECT_CALL(mechanicsMock, applySpellBonus(Eq(effectValue), Eq(&targetUnit))).WillRepeatedly(Return(effectValue));

	GTEST_ASSERT_EQ(targetUnitState->getAvailableHealth(), unitAmount * unitHP / 2 + 1);
	GTEST_ASSERT_EQ(targetUnitState->getFirstHPleft(), 1);

	EXPECT_CALL(targetUnit, acquire()).WillOnce(Return(targetUnitState));

	EXPECT_CALL(*battleFake, setUnitState(Eq(unitId), _, Gt(0))).Times(1);

	//There should be battle log message if resurrect
	switch(healLevel) 
	{
	case EHealLevel::RESURRECT:
	case EHealLevel::OVERHEAL:
		EXPECT_CALL(serverMock, apply(Matcher<BattleLogMessage *>(_))).Times(AtLeast(1));
		break;
	default:
		break;
	}

	EXPECT_CALL(serverMock, apply(Matcher<BattleUnitsChanged *>(_))).Times(1);

	setupDefaultRNG();

	EffectTarget target;
	target.emplace_back(&targetUnit, BattleHex());

	subject->apply(&serverMock, &mechanicsMock, target);

	switch(healLevel)
	{
	case EHealLevel::HEAL:
		EXPECT_EQ(targetUnitState->getFirstHPleft(), unitHP);
		break;
	case EHealLevel::RESURRECT:
		EXPECT_EQ(targetUnitState->getFirstHPleft(), 1);
		break;
	case EHealLevel::OVERHEAL:
		EXPECT_EQ(targetUnitState->getFirstHPleft(), 1);
		break;
	default:
		break;
	}

	switch(healPower)
	{
	case EHealPower::PERMANENT:
		EXPECT_EQ(targetUnitState->health.getResurrected(), 0);
		break;
	case EHealPower::ONE_BATTLE:
		EXPECT_EQ(targetUnitState->health.getResurrected(), 10);
		break;
	default:
		break;
	}
}

INSTANTIATE_TEST_SUITE_P
(
	ByConfig1,
	HealApplyTest,
	Combine
	(
		Values(EHealLevel::HEAL, EHealLevel::RESURRECT, EHealLevel::OVERHEAL),
		Values(EHealPower::PERMANENT)
	)
);

INSTANTIATE_TEST_SUITE_P
(
	ByConfig2,
	HealApplyTest,
	Combine
	(
		Values(EHealLevel::RESURRECT, EHealLevel::OVERHEAL),
		Values(EHealPower::ONE_BATTLE)
	)
);



}


