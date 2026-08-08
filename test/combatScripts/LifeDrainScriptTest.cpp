/*
 * LifeDrainScriptTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../spells/effects/EffectFixture.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/battle/CUnitState.h"
#include "../../lib/battle/Unit.h"
#include "../../lib/combatScripts/CombatScriptService.h"
#include "../../lib/combatScripts/CombatEventPayload.h"
#include "../../lib/combatScripts/ICombatEventScript.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/modding/ModScope.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"

#include "mock/mock_UnitEnvironment.h"

namespace test
{
using namespace ::testing;

/// Covers the built-in lifeDrain combat script - the scripted equivalent of the LIFE_DRAIN bonus.
/// The payload is what a real attack would report, so these also pin down how the script reads it.
class LifeDrainScriptTest : public Test, public EffectFixture
{
public:
	static constexpr int32_t percentage = 50;
	static constexpr int32_t unitAmount = 10;
	static constexpr int32_t unitHP = 100;

	LifeDrainScriptTest()
		: EffectFixture("core:damage") // unused, this test drives a combat script instead
	{
	}

	std::shared_ptr<ICombatEventScript> script;
	UnitEnvironmentMock unitEnvironmentMock;

	std::shared_ptr<::battle::CUnitStateDetached> attackerState;

	JsonNode parameters()
	{
		JsonNode result;
		result["val"].Integer() = percentage;
		return result;
	}

	/// One entry per unit the attack hit, exactly as BattleActionProcessor builds it.
	CombatEventPayload payload(const std::vector<std::pair<const ::battle::Unit *, int>> & targets)
	{
		CombatEventPayload result;
		for(const auto & [unit, damage] : targets)
		{
			AttackedTarget entry;
			entry.unit = unit;
			entry.damage = damage;
			result.targets.push_back(entry);
		}
		return result;
	}

	/// A unit the attack hit. Only its living-ness matters to life drain.
	battle::UnitFake & addVictim(bool living)
	{
		auto & victim = unitsFake.add(BattleSide::DEFENDER);
		if(living)
			victim.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::LIVING, BonusSource::CREATURE_ABILITY, 1, BonusSourceID()));
		victim.expectAnyBonusSystemCall();
		return victim;
	}

	/// Wounded attacker - a stack at full health drains nothing, so every test needs the damage.
	battle::UnitFake & addAttacker(int64_t alreadyLost)
	{
		auto & attacker = unitsFake.add(BattleSide::ATTACKER);
		attacker.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, unitHP, BonusSourceID()));
		EXPECT_CALL(attacker, unitId()).WillRepeatedly(Return(1u));
		EXPECT_CALL(attacker, unitBaseAmount()).WillRepeatedly(Return(unitAmount));
		EXPECT_CALL(attacker, alive()).WillRepeatedly(Return(true));
		EXPECT_CALL(attacker, creatureId()).WillRepeatedly(Return(CreatureID(0)));
		EXPECT_CALL(attacker, getPosition()).WillRepeatedly(Return(BattleHex(50)));
		unitsFake.setDefaultBonusExpectations();

		attackerState = std::make_shared<::battle::CUnitStateDetached>(&attacker, &attacker);
		attackerState->localInit(&unitEnvironmentMock);
		attackerState->damage(alreadyLost);

		EXPECT_CALL(attacker, acquireState()).WillRepeatedly(Return(attackerState));
		EXPECT_CALL(attacker, acquire()).WillRepeatedly(Return(attackerState));
		EXPECT_CALL(attacker, getTotalHealth()).WillRepeatedly(Return(unitAmount * unitHP));
		EXPECT_CALL(attacker, getAvailableHealth()).WillRepeatedly(Return(unitAmount * unitHP - alreadyLost));
		EXPECT_CALL(attacker, getCount()).WillRepeatedly(Return(attackerState->getCount()));
		return attacker;
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();

		auto index = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "combatScript", "lifeDrain", false);
		ASSERT_TRUE(index.has_value());

		script = LIBRARY->combatScripts()->get(CombatScriptID(*index));
		ASSERT_TRUE(script);
	}
};

TEST_F(LifeDrainScriptTest, HealsShareOfDamageDealtToLivingTargets)
{
	auto & attacker = addAttacker(unitHP * 3);

	int64_t healedBy = 0;
	EXPECT_CALL(*battleFake, updateUnit(Eq(1u), _, Gt(0))).WillOnce(SaveArg<2>(&healedBy));
	EXPECT_CALL(serverMock, apply(Matcher<BattleUnitsChanged &>(_))).Times(1);
	EXPECT_CALL(serverMock, apply(Matcher<CPackForClient &>(_))).Times(1); // the drain animation
	EXPECT_CALL(serverMock, apply(Matcher<BattleLogMessage &>(_))).Times(1);

	script->run(&serverMock, *battleFake, CombatEventType::ATTACK_RESOLVED, &attacker, nullptr,
		parameters(), payload({{&addVictim(true), 200}}));

	EXPECT_EQ(healedBy, 200 * percentage / 100);
}

TEST_F(LifeDrainScriptTest, IgnoresDamageDealtToNonLivingTargets)
{
	auto & attacker = addAttacker(unitHP * 3);

	int64_t healedBy = 0;
	EXPECT_CALL(*battleFake, updateUnit(Eq(1u), _, Gt(0))).WillOnce(SaveArg<2>(&healedBy));
	EXPECT_CALL(serverMock, apply(Matcher<BattleUnitsChanged &>(_))).Times(1);
	EXPECT_CALL(serverMock, apply(Matcher<CPackForClient &>(_))).Times(1);
	EXPECT_CALL(serverMock, apply(Matcher<BattleLogMessage &>(_))).Times(1);

	// a multi-hex attack catching a living stack and an undead one
	script->run(&serverMock, *battleFake, CombatEventType::ATTACK_RESOLVED, &attacker, nullptr,
		parameters(), payload({{&addVictim(true), 200}, {&addVictim(false), 400}}));

	EXPECT_EQ(healedBy, 200 * percentage / 100);
}

TEST_F(LifeDrainScriptTest, UnwoundedAttackerDrainsNothing)
{
	auto & attacker = addAttacker(0);

	// serverMock is strict, so any mutation attempted by the script fails the test
	script->run(&serverMock, *battleFake, CombatEventType::ATTACK_RESOLVED, &attacker, nullptr,
		parameters(), payload({{&addVictim(true), 200}}));
}

TEST_F(LifeDrainScriptTest, NoDrainableDamageChangesNothing)
{
	auto & attacker = addAttacker(unitHP * 3);

	script->run(&serverMock, *battleFake, CombatEventType::ATTACK_RESOLVED, &attacker, nullptr,
		parameters(), payload({{&addVictim(false), 400}}));
}

}
