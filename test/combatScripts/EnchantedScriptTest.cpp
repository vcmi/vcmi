/*
 * EnchantedScriptTest.cpp, part of VCMI engine
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
#include "../../lib/battle/Unit.h"
#include "../../lib/combatScripts/CombatScriptService.h"
#include "../../lib/combatScripts/ICombatEventScript.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/modding/ModScope.h"
#include "../../lib/networkPacks/SetStackEffect.h"

namespace test
{
using namespace ::testing;

/// Covers the built-in enchanted combat script - the scripted equivalent of the ENCHANTED bonus.
/// A real timed spell is applied, so the SetStackEffect it produces shows exactly which units the
/// script selected as targets.
class EnchantedScriptTest : public Test, public EffectFixture
{
public:
	static constexpr int32_t duration = 50;
	static constexpr uint32_t bearerId = 10;
	static constexpr uint32_t allyId = 11;
	static constexpr uint32_t enemyId = 12;

	EnchantedScriptTest()
		: EffectFixture("core:damage") // unused, this test drives a combat script instead
	{
	}

	std::shared_ptr<ICombatEventScript> script;

	void allowUninterestingServerQueries()
	{
		EXPECT_CALL(serverMock, describeChanges()).WillRepeatedly(Return(false));
	}

	JsonNode parameters(bool massive)
	{
		JsonNode result;
		result["spell"].String() = "core:bloodlust";
		result["level"].Integer() = 3;
		result["duration"].Integer() = duration;
		if(massive)
			result["massive"].Bool() = true;
		return result;
	}

	battle::UnitFake & addUnit(BattleSide side, uint32_t id)
	{
		auto & unit = unitsFake.add(side);
		EXPECT_CALL(unit, alive()).WillRepeatedly(Return(true));
		EXPECT_CALL(unit, isGhost()).WillRepeatedly(Return(false));
		EXPECT_CALL(unit, isValidTarget(_)).WillRepeatedly(Return(true));
		EXPECT_CALL(unit, unitId()).WillRepeatedly(Return(id));
		EXPECT_CALL(unit, unitSide()).WillRepeatedly(Return(side));
		EXPECT_CALL(unit, creatureId()).WillRepeatedly(Return(CreatureID(0)));
		return unit;
	}

	/// Ids of the units that received a bonus, across every effect pack the script produced.
	std::vector<uint32_t> runAndCollectAffected(CombatEventType event, const battle::UnitFake & bearer, bool massive)
	{
		std::vector<uint32_t> affected;
		allowUninterestingServerQueries();

		EXPECT_CALL(serverMock, apply(Matcher<SetStackEffect &>(_))).Times(AnyNumber())
			.WillRepeatedly(Invoke([&affected](SetStackEffect & pack)
			{
				// timed effects are not cumulative, so they arrive as updates rather than additions
				for(const auto & entry : pack.toUpdate)
					affected.push_back(entry.first);
			}));

		script->run(&serverMock, *battleFake, event, &bearer, nullptr, parameters(massive));
		return affected;
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
		battleFake->setupEmptyBattlefield();

		auto index = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "combatScript", "enchanted", false);
		ASSERT_TRUE(index.has_value());

		script = LIBRARY->combatScripts()->get(CombatScriptID(*index));
		ASSERT_TRUE(script);
	}
};

TEST_F(EnchantedScriptTest, AffectsOnlyTheBearerByDefault)
{
	auto & bearer = addUnit(BattleSide::ATTACKER, bearerId);
	addUnit(BattleSide::ATTACKER, allyId);
	addUnit(BattleSide::DEFENDER, enemyId);
	unitsFake.setDefaultBonusExpectations();

	auto affected = runAndCollectAffected(CombatEventType::BATTLE_START, bearer, false);

	EXPECT_THAT(affected, ElementsAre(bearerId));
}

TEST_F(EnchantedScriptTest, MassiveAffectsEveryAllyButNoEnemy)
{
	auto & bearer = addUnit(BattleSide::ATTACKER, bearerId);
	addUnit(BattleSide::ATTACKER, allyId);
	addUnit(BattleSide::DEFENDER, enemyId);
	unitsFake.setDefaultBonusExpectations();

	auto affected = runAndCollectAffected(CombatEventType::BATTLE_START, bearer, true);

	EXPECT_THAT(affected, UnorderedElementsAre(bearerId, allyId));
}

TEST_F(EnchantedScriptTest, ReappliesAtStartOfEveryRound)
{
	auto & bearer = addUnit(BattleSide::ATTACKER, bearerId);
	unitsFake.setDefaultBonusExpectations();

	auto affected = runAndCollectAffected(CombatEventType::ROUND_START, bearer, false);

	EXPECT_THAT(affected, ElementsAre(bearerId));
}

TEST_F(EnchantedScriptTest, GrantedBonusUsesConfiguredDuration)
{
	auto & bearer = addUnit(BattleSide::ATTACKER, bearerId);
	unitsFake.setDefaultBonusExpectations();

	std::vector<Bonus> granted;
	allowUninterestingServerQueries();
	EXPECT_CALL(serverMock, apply(Matcher<SetStackEffect &>(_))).Times(AnyNumber())
		.WillRepeatedly(Invoke([&granted](SetStackEffect & pack)
		{
			for(const auto & entry : pack.toUpdate)
				granted.insert(granted.end(), entry.second.begin(), entry.second.end());
		}));

	script->run(&serverMock, *battleFake, CombatEventType::BATTLE_START, &bearer, nullptr, parameters(false));

	ASSERT_FALSE(granted.empty());
	for(const auto & bonus : granted)
	{
		EXPECT_EQ(bonus.turnsRemain, duration);
		EXPECT_EQ(bonus.sid, BonusSourceID(SpellID(SpellID::BLOODLUST)));
	}
}

}
