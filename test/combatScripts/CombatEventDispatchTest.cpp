/*
 * CombatEventDispatchTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../spells/effects/EffectFixture.h"

#include <vstd/RNG.h>

#include "../../lib/GameLibrary.h"
#include "../../lib/battle/CUnitState.h"
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

/// Drives a combat script the way BattleActionProcessor does, to cover the dispatch itself:
/// event to script method, and the no-op inherited for events the script does not implement.
/// The vcmi-test testSpikes fixture damages the attacker on AFTER_ATTACKED and nothing else.
class CombatEventDispatchTest : public Test, public EffectFixture
{
public:
	CombatEventDispatchTest()
		: EffectFixture("core:damage") // unused, this test drives a combat script instead
	{
	}

	std::shared_ptr<ICombatEventScript> script;
	UnitEnvironmentMock unitEnvironmentMock;

protected:
	void SetUp() override
	{
		EffectFixture::setUp();

		auto index = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "combatScript", "testSpikes", false);
		ASSERT_TRUE(index.has_value());

		script = LIBRARY->combatScripts()->get(CombatScriptID(*index));
		ASSERT_TRUE(script);
	}
};

TEST_F(CombatEventDispatchTest, ImplementedEventRunsScript)
{
	const int32_t unitAmount = 25;
	const int32_t unitHP = 100;
	const uint32_t attackerId = 42;

	auto & victim = unitsFake.add(BattleSide::DEFENDER);
	auto & attacker = unitsFake.add(BattleSide::ATTACKER);

	attacker.addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::STACK_HEALTH, BonusSource::CREATURE_ABILITY, unitHP, BonusSourceID()));
	EXPECT_CALL(attacker, unitId()).WillRepeatedly(Return(attackerId));
	EXPECT_CALL(attacker, unitBaseAmount()).WillRepeatedly(Return(unitAmount));
	EXPECT_CALL(attacker, alive()).WillRepeatedly(Return(true));

	unitsFake.setDefaultBonusExpectations();

	auto attackerState = std::make_shared<::battle::CUnitStateDetached>(&attacker, &attacker);
	attackerState->localInit(&unitEnvironmentMock);
	EXPECT_CALL(attacker, acquireState()).WillOnce(Return(attackerState));
	EXPECT_CALL(*battleFake, updateUnit(Eq(attackerId), _, Lt(0))).Times(1);
	EXPECT_CALL(serverMock, apply(Matcher<StacksInjured &>(_))).Times(1);

	setupDefaultRNG();

	JsonNode parameters;
	parameters["damage"].Integer() = unitHP;

	script->run(&serverMock, *battleFake, CombatEventType::AFTER_ATTACKED, &victim, &attacker, parameters);

	EXPECT_EQ(attackerState->getCount(), unitAmount - 1);
}

TEST_F(CombatEventDispatchTest, UnimplementedEventIsNoOp)
{
	auto & victim = unitsFake.add(BattleSide::DEFENDER);
	auto & attacker = unitsFake.add(BattleSide::ATTACKER);

	JsonNode parameters;
	parameters["damage"].Integer() = 100;

	// serverMock is strict, so any mutation attempted by the script fails the test
	script->run(&serverMock, *battleFake, CombatEventType::WAIT, &victim, &attacker, parameters);
}

}
