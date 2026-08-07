/*
 * SoulStealScriptTest.cpp, part of VCMI engine
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
#include "../../lib/combatScripts/CombatEventPayload.h"
#include "../../lib/combatScripts/CombatScriptService.h"
#include "../../lib/combatScripts/ICombatEventScript.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/modding/ModScope.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"

#include "mock/mock_UnitEnvironment.h"

namespace test
{
using namespace ::testing;

/// Covers the built-in soulSteal combat script - the scripted equivalent of the SOUL_STEAL bonus.
/// Unlike life drain it grows the stack past its original size, so it needs no wound to work.
class SoulStealScriptTest : public Test, public EffectFixture
{
public:
	static constexpr int32_t creaturesPerKill = 2;
	static constexpr int32_t unitAmount = 10;
	static constexpr int32_t unitHP = 100;

	SoulStealScriptTest()
		: EffectFixture("core:damage") // unused, this test drives a combat script instead
	{
	}

	std::shared_ptr<ICombatEventScript> script;
	UnitEnvironmentMock unitEnvironmentMock;
	std::shared_ptr<::battle::CUnitStateDetached> attackerState;

	JsonNode parameters(bool permanent)
	{
		JsonNode result;
		result["creaturesPerKill"].Integer() = creaturesPerKill;
		result["permanent"].Bool() = permanent;
		return result;
	}

	CombatEventPayload payload(const std::vector<std::pair<const ::battle::Unit *, int>> & targets)
	{
		CombatEventPayload result;
		for(const auto & [unit, killed] : targets)
		{
			AttackedTarget entry;
			entry.unit = unit;
			entry.killed = killed;
			result.targets.push_back(entry);
		}
		return result;
	}

	battle::UnitFake & addAttacker()
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

		EXPECT_CALL(attacker, acquireState()).WillRepeatedly(Return(attackerState));
		EXPECT_CALL(attacker, acquire()).WillRepeatedly(Return(attackerState));
		EXPECT_CALL(attacker, getCount()).WillRepeatedly(Return(attackerState->getCount()));
		return attacker;
	}

	battle::UnitFake & addVictim(bool living)
	{
		auto & victim = unitsFake.add(BattleSide::DEFENDER);
		if(living)
			victim.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::LIVING, BonusSource::CREATURE_ABILITY, 1, BonusSourceID()));
		victim.expectAnyBonusSystemCall();
		return victim;
	}

	void expectHeal(int64_t * healedBy)
	{
		EXPECT_CALL(*battleFake, updateUnit(Eq(1u), _, Gt(0))).WillOnce(SaveArg<2>(healedBy));
		EXPECT_CALL(serverMock, apply(Matcher<BattleUnitsChanged &>(_))).Times(1);
		EXPECT_CALL(serverMock, apply(Matcher<CPackForClient &>(_))).Times(1); // the animation
		EXPECT_CALL(serverMock, apply(Matcher<BattleLogMessage &>(_))).Times(1);
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();

		auto index = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "combatScript", "soulSteal", false);
		ASSERT_TRUE(index.has_value());

		script = LIBRARY->combatScripts()->get(CombatScriptID(*index));
		ASSERT_TRUE(script);
	}
};

TEST_F(SoulStealScriptTest, GrowsStackByCreaturesPerKill)
{
	auto & attacker = addAttacker();

	int64_t healedBy = 0;
	expectHeal(&healedBy);

	script->run(&serverMock, *battleFake, CombatEventType::ATTACK_RESOLVED, &attacker, nullptr,
		parameters(true), payload({{&addVictim(true), 3}}));

	// an unwounded stack grows past its original size, unlike life drain
	EXPECT_EQ(healedBy, 3 * creaturesPerKill * unitHP);
}

TEST_F(SoulStealScriptTest, IgnoresKillsAmongNonLivingTargets)
{
	auto & attacker = addAttacker();

	int64_t healedBy = 0;
	expectHeal(&healedBy);

	script->run(&serverMock, *battleFake, CombatEventType::ATTACK_RESOLVED, &attacker, nullptr,
		parameters(true), payload({{&addVictim(true), 3}, {&addVictim(false), 5}}));

	EXPECT_EQ(healedBy, 3 * creaturesPerKill * unitHP);
}

TEST_F(SoulStealScriptTest, NoKillsChangeNothing)
{
	auto & attacker = addAttacker();

	// serverMock is strict, so any mutation attempted by the script fails the test
	script->run(&serverMock, *battleFake, CombatEventType::ATTACK_RESOLVED, &attacker, nullptr,
		parameters(true), payload({{&addVictim(true), 0}}));
}

}
