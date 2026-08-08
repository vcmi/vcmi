/*
 * DestructionScriptTest.cpp, part of VCMI engine
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

/// Covers the built-in destruction combat script - the scripted equivalent of the DESTRUCTION
/// bonus. Nothing applies the packs back onto the battle, so the victim keeps its original size
/// no matter what the script did to it.
class DestructionScriptTest : public Test, public EffectFixture
{
public:
	static constexpr int32_t chance = 20;
	static constexpr int32_t victimCount = 10;
	static constexpr int32_t unitHP = 100;

	DestructionScriptTest()
		: EffectFixture("core:damage") // unused, this test drives a combat script instead
	{
	}

	std::shared_ptr<ICombatEventScript> script;
	UnitEnvironmentMock unitEnvironmentMock;
	std::shared_ptr<::battle::CUnitStateDetached> victimState;

	JsonNode parameters(const std::string & killBy, int amount)
	{
		JsonNode result;
		result["val"].Integer() = chance;
		result["killBy"].String() = killBy;
		result["amount"].Integer() = amount;
		return result;
	}

	battle::UnitFake & addAttacker()
	{
		return unitsFake.add(BattleSide::ATTACKER);
	}

	/// The victim needs a real unit state, since the kill count comes out of applying the damage
	battle::UnitFake & addVictim()
	{
		auto & victim = unitsFake.add(BattleSide::DEFENDER);
		victim.makeAlive();
		victim.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, unitHP, BonusSourceID()));
		EXPECT_CALL(victim, unitId()).WillRepeatedly(Return(2u));
		EXPECT_CALL(victim, unitBaseAmount()).WillRepeatedly(Return(victimCount));
		EXPECT_CALL(victim, getCount()).WillRepeatedly(Return(victimCount));
		EXPECT_CALL(victim, creatureId()).WillRepeatedly(Return(CreatureID(0)));
		unitsFake.setDefaultBonusExpectations();

		victimState = std::make_shared<::battle::CUnitStateDetached>(&victim, &victim);
		victimState->localInit(&unitEnvironmentMock);

		EXPECT_CALL(victim, acquireState()).WillRepeatedly(Return(victimState));
		EXPECT_CALL(serverMock, getRNG()).WillRepeatedly(Return(&rngMock));
		return victim;
	}

	/// Damage the script dealt and the creatures it killed, as reported to the clients
	std::pair<int64_t, uint32_t> runAndCollect(const JsonNode & params)
	{
		auto & attacker = addAttacker();
		auto & victim = addVictim();

		StacksInjured injured;
		EXPECT_CALL(serverMock, apply(Matcher<StacksInjured &>(_))).Times(1)
			.WillOnce(Invoke([&injured](StacksInjured & pack){ injured = pack; }));
		EXPECT_CALL(serverMock, apply(Matcher<CPackForClient &>(_))).Times(1); // the slayer animation
		EXPECT_CALL(serverMock, apply(Matcher<BattleLogMessage &>(_))).Times(1);

		script->run(&serverMock, *battleFake, CombatEventType::AFTER_ATTACK, &attacker, &victim, params, CombatEventPayload());

		EXPECT_EQ(injured.stacks.size(), 1u);
		if(injured.stacks.empty())
			return {0, 0};

		return {injured.stacks.front().damageAmount, injured.stacks.front().killedAmount};
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
		setupDefaultRNG();

		auto index = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "combatScript", "destruction", false);
		ASSERT_TRUE(index.has_value());

		script = LIBRARY->combatScripts()->get(CombatScriptID(*index));
		ASSERT_TRUE(script);
	}
};

TEST_F(DestructionScriptTest, KillsShareOfTheStack)
{
	EXPECT_CALL(serverMock, rollCombatAbility(_, _, Eq(chance))).WillOnce(Return(true));

	auto [damage, killed] = runAndCollect(parameters("percentage", 30));

	EXPECT_EQ(damage, 3 * unitHP);
	EXPECT_EQ(killed, 3u);
}

TEST_F(DestructionScriptTest, KillsFixedAmount)
{
	EXPECT_CALL(serverMock, rollCombatAbility(_, _, Eq(chance))).WillOnce(Return(true));

	auto [damage, killed] = runAndCollect(parameters("count", 4));

	EXPECT_EQ(damage, 4 * unitHP);
	EXPECT_EQ(killed, 4u);
}

TEST_F(DestructionScriptTest, FailedRollChangesNothing)
{
	auto & attacker = addAttacker();
	auto & victim = addVictim();

	EXPECT_CALL(serverMock, rollCombatAbility(_, _, Eq(chance))).WillOnce(Return(false));

	// serverMock is strict, so any mutation attempted by the script fails the test
	script->run(&serverMock, *battleFake, CombatEventType::AFTER_ATTACK, &attacker, &victim, parameters("count", 4), CombatEventPayload());
}

TEST_F(DestructionScriptTest, ShareTooSmallForOneCreatureKillsNobody)
{
	auto & attacker = addAttacker();
	auto & victim = addVictim();

	EXPECT_CALL(serverMock, rollCombatAbility(_, _, Eq(chance))).WillOnce(Return(true));

	script->run(&serverMock, *battleFake, CombatEventType::AFTER_ATTACK, &attacker, &victim, parameters("percentage", 5), CombatEventPayload());
}

}
