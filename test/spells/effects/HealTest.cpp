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
#include "../../../lib/CCreatureHandler.h"
#include "../../../lib/json/JsonNode.h"

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

	Target target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicableTarget(problemMock, &mechanicsMock, target));
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

	Target target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_TRUE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(HealTest, ApplicableIfActuallyResurrects)
{
	{
		JsonNode config;
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

	unit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, 200, BonusSourceID()));
	unitsFake.setDefaultBonusExpectations();

	Target target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_TRUE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(HealTest, NotApplicableIfNotEnoughCasualties)
{
	{
		JsonNode config;
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

	unit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, 200, BonusSourceID()));
	unitsFake.setDefaultBonusExpectations();

	Target target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(HealTest, NotApplicableIfResurrectsLessThanRequired)
{
	{
		JsonNode config;
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

	unit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, 200, BonusSourceID()));
	unitsFake.setDefaultBonusExpectations();

	Target target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(HealTest, ApplicableToDeadUnit)
{
	{
		JsonNode config;
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

	EXPECT_CALL(*battleFake, getUnitsIf(_)).Times(AtLeast(0));

	Target target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_TRUE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(HealTest, NotApplicableIfDeadUnitIsBlocked)
{
	{
		JsonNode config;
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

	EXPECT_CALL(*battleFake, getUnitsIf(_)).Times(AtLeast(1));

	Target target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(HealTest, ApplicableWithAnotherDeadUnitInSamePosition)
{
	{
		JsonNode config;
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

	EXPECT_CALL(*battleFake, getUnitsIf(_)).Times(AtLeast(1));

	Target target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_TRUE(subject->applicableTarget(problemMock, &mechanicsMock, target));
}

TEST_F(HealTest, NotApplicableIfEffectValueTooLow)
{
	{
		JsonNode config;
		config["minFullUnits"].Integer() = 1;
		EffectFixture::setupEffect(config);
	}

	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillOnce(Return(true));
	EXPECT_CALL(unit, getTotalHealth()).WillOnce(Return(200));
	EXPECT_CALL(unit, getAvailableHealth()).WillOnce(Return(100));

	unit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, 200, BonusSourceID()));

	EXPECT_CALL(mechanicsMock, getEffectValue()).Times(AtLeast(1)).WillRepeatedly(Return(199));

	EXPECT_CALL(mechanicsMock, isSmart()).Times(AtMost(1)).WillRepeatedly(Return(false));

	unitsFake.setDefaultBonusExpectations();

	Target target;
	target.emplace_back(&unit, BattleHex());

	EXPECT_FALSE(subject->applicableTarget(problemMock, &mechanicsMock, target));
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
		JsonNode config;
		config["healLevel"].String() = HEAL_LEVEL_MAP.at(static_cast<size_t>(healLevel));
		config["healPower"].String() = HEAL_POWER_MAP.at(static_cast<size_t>(healPower));
		EffectFixture::setupEffect(config);
	}

	using namespace ::battle;

	const int64_t effectValue = 2000;
	const int32_t unitAmount = 24;
	const int32_t unitHP = 100;
	const uint32_t unitId = 42;
	const auto pikeman = CreatureID(unitId).toCreature();


	auto & targetUnit = unitsFake.add(BattleSide::ATTACKER);
	auto & actualCaster = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(targetUnit, unitBaseAmount()).WillRepeatedly(Return(unitAmount));
	EXPECT_CALL(targetUnit, unitId()).WillRepeatedly(Return(unitId));
	EXPECT_CALL(targetUnit, unitType()).WillRepeatedly(Return(pikeman));
	EXPECT_CALL(targetUnit, creatureId()).WillRepeatedly(Return(CreatureID(unitId)));

	targetUnit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, unitHP, BonusSourceID()));

	unitsFake.setDefaultBonusExpectations();

	auto targetUnitState = std::make_shared<CUnitStateDetached>(&targetUnit, &targetUnit);
	targetUnitState->localInit(&unitEnvironmentMock);
	{
		int64_t initialDmg = unitAmount * unitHP / 2 - 1;
		targetUnitState->health.damage(initialDmg);
	}

	mechanicsMock.caster = &actualCaster;
	EXPECT_CALL(mechanicsMock, getEffectValue()).WillRepeatedly(Return(effectValue));
	EXPECT_CALL(mechanicsMock, applySpellBonus(Eq(effectValue), Eq(&targetUnit))).WillRepeatedly(Return(effectValue));
	EXPECT_CALL(mechanicsMock, getUnitCaster()).WillRepeatedly(Return(&actualCaster));
	EXPECT_CALL(actualCaster, creatureId()).WillRepeatedly(Return(CreatureID(unitId)));
	EXPECT_CALL(actualCaster, getHeroCaster()).WillRepeatedly(Return(nullptr));

	GTEST_ASSERT_EQ(targetUnitState->getAvailableHealth(), unitAmount * unitHP / 2 + 1);
	GTEST_ASSERT_EQ(targetUnitState->getFirstHPleft(), 1);

	EXPECT_CALL(targetUnit, acquire()).WillRepeatedly(Return(targetUnitState));

	EXPECT_CALL(*battleFake, updateUnit(Eq(unitId), _, Gt(0))).Times(1);

	EXPECT_CALL(actualCaster, getCasterUnitId()).WillRepeatedly(Return(CreatureID(unitId)));

	EXPECT_CALL(serverMock, apply(Matcher<BattleUnitsChanged &>(_))).Times(1);
	EXPECT_CALL(serverMock, apply(Matcher<BattleLogMessage &>(_))).Times(AtLeast(1));

	setupDefaultRNG();

	Target target;
	target.emplace_back(&targetUnit, BattleHex());

	subject->apply(&serverMock, &mechanicsMock, target);

	switch(healLevel)
	{
	case EHealLevel::HEAL:
		EXPECT_EQ(targetUnitState->getFirstHPleft(), unitHP);
		break;
	case EHealLevel::RESURRECT:
		EXPECT_EQ(targetUnitState->getCount(), unitAmount);
		EXPECT_EQ(targetUnitState->getFirstHPleft(), unitHP);
		break;
	case EHealLevel::OVERHEAL:
		EXPECT_GT(targetUnitState->getAvailableHealth(), (int64_t)unitAmount * unitHP);
		EXPECT_GT(targetUnitState->getCount(), unitAmount);
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
		{
			const int32_t initialCount = unitAmount / 2 + 1;
			switch(healLevel)
			{
			case EHealLevel::HEAL:
				EXPECT_EQ(targetUnitState->health.getResurrected(), 0);
				break;
			case EHealLevel::RESURRECT:
				EXPECT_EQ(targetUnitState->health.getResurrected(), unitAmount - initialCount);
				break;
			case EHealLevel::OVERHEAL:
				{
					const int64_t available = static_cast<int64_t>(unitAmount) * unitHP / 2 + 1 + effectValue;
					EXPECT_EQ(targetUnitState->health.getResurrected(), (int32_t)((available + unitHP - 1) / unitHP) - initialCount);
				}
				break;
			default:
				break;
			}
		}
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

class HealApplyOneOffTest : public Test, public EffectFixture
{
public:
	UnitEnvironmentMock unitEnvironmentMock;

	HealApplyOneOffTest()
		: EffectFixture("core:heal")
	{
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
	}
};

TEST_F(HealApplyOneOffTest, GetHealthChangeReturnsHealedHP)
{
	EffectFixture::setupEffect(JsonNode());

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
	EXPECT_CALL(targetUnit, creatureId()).WillRepeatedly(Return(CreatureID(unitId)));

	targetUnit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, unitHP, BonusSourceID()));
	unitsFake.setDefaultBonusExpectations();

	auto targetUnitState = std::make_shared<CUnitStateDetached>(&targetUnit, &targetUnit);
	targetUnitState->localInit(&unitEnvironmentMock);
	{
		int64_t initialDmg = unitAmount * unitHP / 2 - 1;
		targetUnitState->health.damage(initialDmg);
	}

	EXPECT_CALL(mechanicsMock, getEffectValue()).WillRepeatedly(Return(effectValue));
	EXPECT_CALL(targetUnit, acquireState()).WillRepeatedly(Return(targetUnitState));

	Target target;
	target.emplace_back(&targetUnit, BattleHex());

	const auto result = subject->getHealthChange(&mechanicsMock, target);

	EXPECT_EQ(result.hpDelta, unitHP - 1);
	EXPECT_EQ(result.unitsDelta, 0);
	EXPECT_EQ(result.unitType, static_cast<const Creature *>(pikeman));
}

TEST_F(HealApplyOneOffTest, ApplyResurrectsFullyDeadUnit)
{
	{
		JsonNode config;
		config["healLevel"].String() = "resurrect";
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
	EXPECT_CALL(targetUnit, creatureId()).WillRepeatedly(Return(CreatureID(unitId)));

	targetUnit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, unitHP, BonusSourceID()));
	unitsFake.setDefaultBonusExpectations();

	auto targetUnitState = std::make_shared<CUnitStateDetached>(&targetUnit, &targetUnit);
	targetUnitState->localInit(&unitEnvironmentMock);
	{
		int64_t initialDmg = static_cast<int64_t>(unitAmount) * unitHP;
		targetUnitState->health.damage(initialDmg);
	}

	EXPECT_CALL(mechanicsMock, getEffectValue()).WillRepeatedly(Return(effectValue));
	EXPECT_CALL(targetUnit, acquire()).WillRepeatedly(Return(targetUnitState));

	EXPECT_CALL(*battleFake, updateUnit(Eq(unitId), _, Gt(0))).Times(1);
	EXPECT_CALL(serverMock, apply(Matcher<BattleUnitsChanged &>(_))).Times(1);
	EXPECT_CALL(serverMock, apply(Matcher<BattleLogMessage &>(_))).Times(AtLeast(1));

	setupDefaultRNG();

	GTEST_ASSERT_EQ(targetUnitState->getCount(), 0);

	Target target;
	target.emplace_back(&targetUnit, BattleHex());

	subject->apply(&serverMock, &mechanicsMock, target);

	EXPECT_EQ(targetUnitState->getCount(), effectValue / unitHP);
	EXPECT_EQ(targetUnitState->getFirstHPleft(), unitHP);
	EXPECT_EQ(targetUnitState->health.getResurrected(), 0);
}

TEST_F(HealApplyOneOffTest, ApplyHealsMultipleTargets)
{
	{
		JsonNode config;
		config["healLevel"].String() = "resurrect";
		EffectFixture::setupEffect(config);
	}

	using namespace ::battle;

	const int64_t effectValue = 1000;
	const int32_t unitAmount = 24;
	const int32_t unitHP = 100;
	const uint32_t unitId1 = 42;
	const uint32_t unitId2 = 43;
	const auto pikeman = CreatureID(unitId1).toCreature();

	auto & targetUnit1 = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(targetUnit1, unitBaseAmount()).WillRepeatedly(Return(unitAmount));
	EXPECT_CALL(targetUnit1, unitId()).WillRepeatedly(Return(unitId1));
	EXPECT_CALL(targetUnit1, unitType()).WillRepeatedly(Return(pikeman));
	EXPECT_CALL(targetUnit1, creatureId()).WillRepeatedly(Return(CreatureID(unitId1)));
	targetUnit1.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, unitHP, BonusSourceID()));

	auto & targetUnit2 = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(targetUnit2, unitBaseAmount()).WillRepeatedly(Return(unitAmount));
	EXPECT_CALL(targetUnit2, unitId()).WillRepeatedly(Return(unitId2));
	EXPECT_CALL(targetUnit2, unitType()).WillRepeatedly(Return(pikeman));
	EXPECT_CALL(targetUnit2, creatureId()).WillRepeatedly(Return(CreatureID(unitId2)));
	targetUnit2.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, unitHP, BonusSourceID()));

	unitsFake.setDefaultBonusExpectations();

	auto state1 = std::make_shared<CUnitStateDetached>(&targetUnit1, &targetUnit1);
	state1->localInit(&unitEnvironmentMock);
	{
		int64_t initialDmg = unitAmount * unitHP / 2 - 1;
		state1->health.damage(initialDmg);
	}

	auto state2 = std::make_shared<CUnitStateDetached>(&targetUnit2, &targetUnit2);
	state2->localInit(&unitEnvironmentMock);
	{
		int64_t initialDmg = unitAmount * unitHP / 2 - 1;
		state2->health.damage(initialDmg);
	}

	EXPECT_CALL(mechanicsMock, getEffectValue()).WillRepeatedly(Return(effectValue));
	EXPECT_CALL(targetUnit1, acquire()).WillRepeatedly(Return(state1));
	EXPECT_CALL(targetUnit2, acquire()).WillRepeatedly(Return(state2));

	EXPECT_CALL(*battleFake, updateUnit(Eq(unitId1), _, Gt(0))).Times(1);
	EXPECT_CALL(*battleFake, updateUnit(Eq(unitId2), _, Gt(0))).Times(1);
	EXPECT_CALL(serverMock, apply(Matcher<BattleUnitsChanged &>(_))).Times(2);
	EXPECT_CALL(serverMock, apply(Matcher<BattleLogMessage &>(_))).Times(AtLeast(1));

	setupDefaultRNG();

	Target target;
	target.emplace_back(&targetUnit1, BattleHex());
	target.emplace_back(&targetUnit2, BattleHex());

	subject->apply(&serverMock, &mechanicsMock, target);

	const int64_t available = static_cast<int64_t>(unitAmount) * unitHP / 2 + 1 + effectValue;
	const int32_t expectedCount = static_cast<int32_t>((available + unitHP - 1) / unitHP);
	EXPECT_EQ(state1->getCount(), expectedCount);
	EXPECT_EQ(state2->getCount(), expectedCount);
}

}
