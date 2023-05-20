/*
 * SacrificeTest.cpp, part of VCMI engine
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

class SacrificeTest : public Test, public EffectFixture
{
public:

	SacrificeTest()
		: EffectFixture("core:sacrifice")
	{
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();

		{
			JsonNode config(JsonNode::JsonType::DATA_STRUCT);
			config["healLevel"].String() = "resurrect";
			EffectFixture::setupEffect(config);
		}
	}
};

TEST_F(SacrificeTest, ApplicableForTwoTargets)
{
	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	EXPECT_CALL(unit, getPosition()).WillRepeatedly(Return(BattleHex(5,5)));
	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillRepeatedly(Return(false));
	EXPECT_CALL(unit, isValidTarget(Eq(true))).WillRepeatedly(Return(true));

	EXPECT_CALL(unit, getTotalHealth()).WillRepeatedly(Return(0));
	EXPECT_CALL(unit, getAvailableHealth()).WillRepeatedly(Return(200));

	auto & victim = unitsFake.add(BattleSide::ATTACKER);

	victim.makeAlive();

	EXPECT_CALL(victim, getPosition()).WillRepeatedly(Return(BattleHex(5,10)));
	EXPECT_CALL(victim, isValidTarget(_)).WillRepeatedly(Return(true));
	EXPECT_CALL(victim, getTotalHealth()).WillRepeatedly(Return(300));
	EXPECT_CALL(victim, getAvailableHealth()).WillRepeatedly(Return(300));

	EXPECT_CALL(mechanicsMock, isSmart()).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, alwaysHitFirstTarget()).WillRepeatedly(Return(false));

	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).Times(AtLeast(1)).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, isReceptive(Eq(&unit))).Times(AtLeast(1)).WillRepeatedly(Return(true));

	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&victim))).Times(AtLeast(1)).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, isReceptive(Eq(&victim))).Times(AtLeast(1)).WillRepeatedly(Return(true));


	EffectTarget aimPoint;
	aimPoint.emplace_back(&unit, BattleHex());
	aimPoint.emplace_back(&victim, BattleHex());

	EffectTarget spellTarget;
	spellTarget.emplace_back(&unit, BattleHex());

	EffectTarget transformed = subject->transformTarget(&mechanicsMock, aimPoint, spellTarget);

	EXPECT_TRUE(subject->applicable(problemMock, &mechanicsMock, transformed));
}

#if 0

TEST_F(SacrificeTest, NotApplicableWithoutVictim)
{
	{
		JsonNode config(JsonNode::JsonType::DATA_STRUCT);
		config["healLevel"].String() = "resurrect";
		EffectFixture::setupEffect(config);
	}

	auto & unit = unitsFake.add(BattleSide::ATTACKER);

	EXPECT_CALL(unit, getPosition()).WillRepeatedly(Return(BattleHex(5,5)));
	EXPECT_CALL(unit, isValidTarget(Eq(false))).WillRepeatedly(Return(false));
	EXPECT_CALL(unit, isValidTarget(Eq(true))).WillRepeatedly(Return(true));

	EXPECT_CALL(unit, getTotalHealth()).WillRepeatedly(Return(0));
	EXPECT_CALL(unit, getAvailableHealth()).WillRepeatedly(Return(200));

	EXPECT_CALL(mechanicsMock, isSmart()).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, isMassive()).WillRepeatedly(Return(false));
	EXPECT_CALL(mechanicsMock, alwaysHitFirstTarget()).WillRepeatedly(Return(false));

	EXPECT_CALL(mechanicsMock, ownerMatches(Eq(&unit))).WillRepeatedly(Return(true));
	EXPECT_CALL(mechanicsMock, isReceptive(Eq(&unit))).WillRepeatedly(Return(true));

	EffectTarget aimPoint;
	aimPoint.emplace_back(&unit, BattleHex());

	EffectTarget transformed = subject->transformTarget(&mechanicsMock, aimPoint, aimPoint);

	EXPECT_FALSE(subject->applicable(problemMock, &mechanicsMock, transformed));
}

#endif

class SacrificeApplyTest : public Test, public EffectFixture
{
public:
	UnitEnvironmentMock unitEnvironmentMock;

	SacrificeApplyTest()
		: EffectFixture("core:sacrifice")
	{
	}

	void setDefaultExpectations()
	{

	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();

		{
			JsonNode config(JsonNode::JsonType::DATA_STRUCT);
			config["healLevel"].String() = "resurrect";
			config["healPower"].String() = "permanent";
			EffectFixture::setupEffect(config);
		}
	}
};

TEST_F(SacrificeApplyTest, ResurrectsTarget)
{
	using namespace ::battle;

	const int32_t effectPower = 10;
	const int64_t effectValue = 500;
	const uint32_t unitId = 42;
	const int32_t unitAmount = 100;
	const int32_t unitHP = 1000;

	const uint32_t victimId = 4242;
	const int32_t victimCount = 5;
	const int32_t victimUnitHP = 100;
	const auto pikeman = CreatureID(unitId).toCreature();

	const int64_t expectedHealValue = (effectPower + victimUnitHP + effectValue) * victimCount;

	auto & targetUnit = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(targetUnit, unitBaseAmount()).WillRepeatedly(Return(unitAmount));
	EXPECT_CALL(targetUnit, unitId()).WillRepeatedly(Return(unitId));
	EXPECT_CALL(targetUnit, unitType()).WillRepeatedly(Return(pikeman));

	EXPECT_CALL(mechanicsMock, getEffectPower()).Times(AtLeast(1)).WillRepeatedly(Return(effectPower));
	EXPECT_CALL(mechanicsMock, applySpellBonus(_, Eq(&targetUnit))).WillOnce(ReturnArg<0>());
	EXPECT_CALL(mechanicsMock, calculateRawEffectValue(_,_)).WillOnce(Return(effectValue));

	targetUnit.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, unitHP, 0));

	auto & victim = unitsFake.add(BattleSide::ATTACKER);
	EXPECT_CALL(victim, unitId()).Times(AtLeast(1)).WillRepeatedly(Return(victimId));
	EXPECT_CALL(victim, getCount()).Times(AtLeast(1)).WillRepeatedly(Return(victimCount));

	victim.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, victimUnitHP, 0));

	EXPECT_CALL(*battleFake, setUnitState(Eq(unitId), _, Eq(expectedHealValue))).Times(1);

	EXPECT_CALL(*battleFake, removeUnit(Eq(victimId))).Times(1);

	unitsFake.setDefaultBonusExpectations();

	std::shared_ptr<CUnitState> targetUnitState = std::make_shared<CUnitStateDetached>(&targetUnit, &targetUnit);
	targetUnitState->localInit(&unitEnvironmentMock);
	{
		int64_t initialDmg = targetUnitState->getAvailableHealth();
		targetUnitState->damage(initialDmg);
	}

	EXPECT_CALL(targetUnit, acquire()).WillOnce(Return(targetUnitState));

	EXPECT_CALL(serverMock, apply(Matcher<BattleUnitsChanged *>(_))).Times(AtLeast(1));
	EXPECT_CALL(serverMock, apply(Matcher<BattleLogMessage *>(_))).Times(AtLeast(1));

	setupDefaultRNG();

	EffectTarget target;
	target.emplace_back(&targetUnit, BattleHex());
	target.emplace_back(&victim, BattleHex());

	subject->apply(&serverMock, &mechanicsMock, target);

	EXPECT_EQ(targetUnitState->getAvailableHealth(), expectedHealValue);
}

}
