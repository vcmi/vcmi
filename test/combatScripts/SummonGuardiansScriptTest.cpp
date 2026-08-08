/*
 * SummonGuardiansScriptTest.cpp, part of VCMI engine
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
#include "../../lib/combatScripts/CombatEventPayload.h"
#include "../../lib/combatScripts/ICombatEventScript.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/modding/ModScope.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"

namespace test
{
using namespace ::testing;

/// Covers the built-in summonGuardians combat script - the scripted equivalent of the
/// SUMMON_GUARDIANS bonus. The double-wide cases exercise the ported placement math, which is
/// the only non-trivial part of the script.
class SummonGuardiansScriptTest : public Test, public EffectFixture
{
public:
	static constexpr int32_t percentage = 20;
	static constexpr int32_t guardedCount = 50;
	static constexpr uint32_t newUnitId = 77;
	const CreatureID guardianCreature = CreatureID(42);

	SummonGuardiansScriptTest()
		: EffectFixture("core:damage") // unused, this test drives a combat script instead
	{
	}

	std::shared_ptr<ICombatEventScript> script;

	JsonNode parameters()
	{
		JsonNode result;
		result["creature"].String() = "core:goldGolem";
		result["val"].Integer() = percentage;
		return result;
	}

	/// The unit to be guarded. It is the only unit in the battle, so every hex other than the one
	/// it stands on is free and placement depends purely on the script's own hex math.
	battle::UnitFake & addGuarded(BattleSide side, const BattleHex & position, bool doubleWide)
	{
		auto & guarded = unitsFake.add(side);
		EXPECT_CALL(guarded, alive()).WillRepeatedly(Return(true));
		EXPECT_CALL(guarded, isGhost()).WillRepeatedly(Return(false));
		EXPECT_CALL(guarded, isValidTarget(_)).WillRepeatedly(Return(true));
		EXPECT_CALL(guarded, unitSide()).WillRepeatedly(Return(side));
		EXPECT_CALL(guarded, getPosition()).WillRepeatedly(Return(position));
		EXPECT_CALL(guarded, doubleWide()).WillRepeatedly(Return(doubleWide));
		EXPECT_CALL(guarded, getCount()).WillRepeatedly(Return(guardedCount));
		EXPECT_CALL(guarded, creatureId()).WillRepeatedly(Return(CreatureID(0)));
		unitsFake.setDefaultBonusExpectations();
		return guarded;
	}

	void expectGuardianCreature(bool doubleWide)
	{
		EXPECT_CALL(creatureServiceMock, getByName("core:goldGolem")).WillRepeatedly(Return(&creatureStub));
		EXPECT_CALL(creatureStub, getId()).WillRepeatedly(Return(guardianCreature));
		EXPECT_CALL(creatureStub, isDoubleWide()).WillRepeatedly(Return(doubleWide));
	}

	/// Collects the position of every unit the script asked the server to add, in placement order.
	std::vector<BattleHex> runAndCollectPlacements(battle::UnitFake & guarded)
	{
		std::vector<BattleHex> placements;

		EXPECT_CALL(*battleFake, nextUnitId()).WillRepeatedly(Return(newUnitId));
		EXPECT_CALL(serverMock, apply(Matcher<BattleUnitsChanged &>(_))).Times(AnyNumber())
			.WillRepeatedly(Invoke([&placements](BattleUnitsChanged & pack)
			{
				for(const auto & change : pack.changedStacks)
				{
					::battle::UnitInfo added;
					added.load(newUnitId, change.data);
					placements.push_back(added.position);
				}
			}));
		EXPECT_CALL(serverMock, apply(Matcher<StacksInjured &>(_))).Times(AnyNumber());

		script->run(&serverMock, *battleFake, CombatEventType::BATTLE_START, &guarded, nullptr, parameters(), CombatEventPayload());
		return placements;
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
		battleFake->setupEmptyBattlefield();

		auto index = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "combatScript", "summonGuardians", false);
		ASSERT_TRUE(index.has_value());

		script = LIBRARY->combatScripts()->get(CombatScriptID(*index));
		ASSERT_TRUE(script);
	}
};

TEST_F(SummonGuardiansScriptTest, SingleHexGuardiansSurroundTheUnit)
{
	// hex 56 is column 5 of row 3, far enough from every edge for all six neighbours to be free
	auto & guarded = addGuarded(BattleSide::ATTACKER, BattleHex(56), false);
	expectGuardianCreature(false);

	auto placements = runAndCollectPlacements(guarded);

	EXPECT_THAT(placements, UnorderedElementsAre(
		BattleHex(38), BattleHex(39), BattleHex(57), BattleHex(73), BattleHex(72), BattleHex(55)));
}

TEST_F(SummonGuardiansScriptTest, DoubleWideGuardiansUseComputedHexes)
{
	auto & guarded = addGuarded(BattleSide::ATTACKER, BattleHex(56), false);
	expectGuardianCreature(true);

	auto placements = runAndCollectPlacements(guarded);

	// front guardian two hexes east, one per side, and a back guard to the west
	EXPECT_THAT(placements, ElementsAre(BattleHex(58), BattleHex(39), BattleHex(73), BattleHex(55)));
}

TEST_F(SummonGuardiansScriptTest, DoubleWideGuardiansMirrorForDefender)
{
	auto & guarded = addGuarded(BattleSide::DEFENDER, BattleHex(56), false);
	expectGuardianCreature(true);

	auto placements = runAndCollectPlacements(guarded);

	EXPECT_THAT(placements, ElementsAre(BattleHex(54), BattleHex(38), BattleHex(72), BattleHex(57)));
}

TEST_F(SummonGuardiansScriptTest, StackSizeIsPercentageOfGuardedUnitAndAtLeastOne)
{
	auto & guarded = addGuarded(BattleSide::ATTACKER, BattleHex(56), false);
	expectGuardianCreature(true);

	std::vector<::battle::UnitInfo> added;

	EXPECT_CALL(*battleFake, nextUnitId()).WillRepeatedly(Return(newUnitId));
	EXPECT_CALL(serverMock, apply(Matcher<BattleUnitsChanged &>(_))).Times(AnyNumber())
		.WillRepeatedly(Invoke([&added](BattleUnitsChanged & pack)
		{
			for(const auto & change : pack.changedStacks)
			{
				::battle::UnitInfo info;
				info.load(newUnitId, change.data);
				added.push_back(info);
			}
		}));
	EXPECT_CALL(serverMock, apply(Matcher<StacksInjured &>(_))).Times(AnyNumber());

	script->run(&serverMock, *battleFake, CombatEventType::BATTLE_START, &guarded, nullptr, parameters(), CombatEventPayload());

	ASSERT_FALSE(added.empty());
	for(const auto & info : added)
	{
		EXPECT_EQ(info.count, guardedCount * percentage / 100);
		EXPECT_EQ(info.type, guardianCreature);
		EXPECT_EQ(info.side, BattleSide::ATTACKER);
		EXPECT_TRUE(info.summoned);
	}
}

}
