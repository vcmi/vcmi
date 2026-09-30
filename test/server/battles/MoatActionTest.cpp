/*
 * MoatActionTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../lib/battle/BattleAction.h"
#include "../../../lib/battle/Destination.h"

namespace
{
// creatures
constexpr int pikeman = 0;
constexpr int archer = 2;
constexpr int stormElemental = 127; // casts Protection from Air

/// An action a unit takes without leaving its hex, against an enemy placed next to it or far away.
struct MoatCase
{
	const char * name;
	int creature;
	bool enemyAdjacent;
	BattleAction (*makeAction)(const CStack * unit, const CStack * enemy);
};

}

/// A unit that acts without leaving its hex while standing in the moat triggers it once.
class MoatActionTest : public BattleTestFixture, public ::testing::WithParamInterface<MoatCase>
{
public:
	static constexpr int32_t stackCount = 10;

	static inline const BattleHex unitHex = BattleHex(7, 5);
	static inline const BattleHex farHex = BattleHex(12, 5);
};

TEST_P(MoatActionTest, actingInsideMoatTriggersIt)
{
	const auto & scenario = GetParam();

	startGame();
	startBattle();

	const BattleHex enemyHex = scenario.enemyAdjacent ? unitHex.cloneInDirection(BattleHex::RIGHT) : farHex;

	CStack * unit = addStack(BattleSide::ATTACKER, CreatureID(scenario.creature), unitHex, stackCount);
	CStack * enemy = addStack(BattleSide::DEFENDER, CreatureID(pikeman), enemyHex, stackCount);
	addMoat(unitHex);

	// so that the only damage the unit takes is the moat's
	blockRetaliation(unit);
	const auto unitHealth = unit->getAvailableHealth();

	ASSERT_TRUE(act(scenario.makeAction(unit, enemy))) << scenario.name;

	const auto moatHits = server.castsOf(spellByName("core:castleMoatTrigger"));
	ASSERT_EQ(moatHits.size(), 1u) << scenario.name;
	EXPECT_EQ(moatHits.front().announcement.affectedCres, std::vector<ui32>{unit->unitId()}) << scenario.name;
	EXPECT_GT(moatHits.front().damage, 0) << scenario.name;
	EXPECT_EQ(unit->getAvailableHealth(), unitHealth - moatHits.front().damage) << scenario.name;
}

INSTANTIATE_TEST_SUITE_P(Actions, MoatActionTest, ::testing::Values(
	MoatCase{"wait", pikeman, false, [](const CStack * unit, const CStack *)
	{
		return BattleAction::makeWait(unit);
	}},
	MoatCase{"defend", pikeman, false, [](const CStack * unit, const CStack *)
	{
		return BattleAction::makeDefend(unit);
	}},
	MoatCase{"shoot", archer, false, [](const CStack * unit, const CStack * enemy)
	{
		return BattleAction::makeShotAttack(unit, enemy);
	}},
	MoatCase{"creatureSpell", stormElemental, false, [](const CStack * unit, const CStack *)
	{
		battle::Target target;
		target.emplace_back(unit);
		return BattleAction::makeCreatureSpellcast(unit, target, SpellID::PROTECTION_FROM_AIR);
	}},
	MoatCase{"attack", pikeman, true, [](const CStack * unit, const CStack * enemy)
	{
		return BattleAction::makeMeleeAttack(unit, enemy->getPosition(), unit->getPosition());
	}}
),
	[](const ::testing::TestParamInfo<MoatCase> & info) { return info.param.name; });
