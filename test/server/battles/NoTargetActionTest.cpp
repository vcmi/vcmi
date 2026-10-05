/*
 * NoTargetActionTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../lib/CPlayerState.h"
#include "../../../lib/battle/BattleAction.h"
#include "../../../lib/bonuses/Bonus.h"
#include "../../../lib/gameState/CGameState.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../server/CGameHandler.h"
#include "../../../server/activities/ActivityProcessor.h"
#include "../../../server/activities/BattleActivities.h"

namespace
{

constexpr int pikeman = 0;

/// How the start of one action changes the turn state of the unit making it.
struct StartStateCase
{
	const char * name;
	EActionType action;
	const char * spell; ///< full identifier, or nullptr for an action without one
	bool waiting;
	bool defending;
	bool movedThisRound;
	bool castSpellThisTurn;
};

}

/// StartAction is applied before every action runs, and sets the waiting, defending and moved
/// flags of the acting unit.
class ActionStartStateTest : public BattleTestFixture, public ::testing::WithParamInterface<StartStateCase>
{
};

TEST_P(ActionStartStateTest, setsTurnStateOfActingUnit)
{
	const auto & scenario = GetParam();

	startGame();
	startBattle();

	CStack * unit = addStack(BattleSide::ATTACKER, CreatureID(pikeman), BattleHex(leftHex), 10);
	ASSERT_NE(unit, nullptr);

	// tells an action that ends waiting apart from one that leaves it alone
	unit->waiting = true;

	BattleAction action;
	action.side = BattleSide::ATTACKER;
	action.stackNumber = unit->unitId();
	action.actionType = scenario.action;
	action.spell = scenario.spell ? spellByName(scenario.spell) : SpellID::NONE;

	StartAction pack(action);
	pack.battleID = BattleID(0);
	gameHandler->sendAndApply(pack);

	EXPECT_EQ(unit->waiting, scenario.waiting) << scenario.name;
	EXPECT_EQ(unit->defending, scenario.defending) << scenario.name;
	EXPECT_EQ(unit->movedThisRound, scenario.movedThisRound) << scenario.name;
	EXPECT_EQ(unit->castSpellThisTurn, scenario.castSpellThisTurn) << scenario.name;
	EXPECT_EQ(unit->waitedThisTurn, scenario.action == EActionType::WAIT) << scenario.name;
}

INSTANTIATE_TEST_SUITE_P(Actions, ActionStartStateTest, ::testing::Values(
	StartStateCase{"wait",            EActionType::WAIT,            nullptr,                         true,  false, false, false},
	StartStateCase{"defend",          EActionType::DEFEND,          nullptr,                         false, true,  false, false},
	StartStateCase{"creatureSpell",   EActionType::MONSTER_SPELL,   "core:bless",                    false, false, true,  true},
	// a genie action carries no spell, the server rolls it while applying the action
	StartStateCase{"randomSpell",     EActionType::MONSTER_SPELL,   nullptr,                         false, false, true,  true},
	StartStateCase{"castWithoutSkip", EActionType::MONSTER_SPELL,   "vcmi-test:testCastWithoutSkip", true,  false, false, true},
	StartStateCase{"walk",            EActionType::WALK,            nullptr,                         false, false, true,  false},
	StartStateCase{"attack",          EActionType::WALK_AND_ATTACK, nullptr,                         false, false, true,  false},
	StartStateCase{"shoot",           EActionType::SHOOT,           nullptr,                         false, false, true,  false},
	StartStateCase{"catapult",        EActionType::CATAPULT,        nullptr,                         false, false, true,  false},
	StartStateCase{"heal",            EActionType::STACK_HEAL,      nullptr,                         false, false, true,  false},
	// WALK_AND_CAST leaves castSpellThisTurn unset
	StartStateCase{"walkAndCast",     EActionType::WALK_AND_CAST,   "core:bless",                    false, false, true,  false},
	StartStateCase{"badMorale",       EActionType::BAD_MORALE,      nullptr,                         false, false, true,  false},
	StartStateCase{"noAction",        EActionType::NO_ACTION,       nullptr,                         false, false, true,  false}
),
	[](const ::testing::TestParamInfo<StartStateCase> & info) { return info.param.name; });

/// Actions that name no target: the stances a unit can take instead of acting, and the ways to
/// leave the battle. The end of the tactics phase is covered by every test that calls beginCombat.
class NoTargetActionTest : public BattleTestFixture
{
public:
	/// A retreat or surrender is only accepted while a unit of the player is the active one.
	CStack * activateUnitOfAttacker()
	{
		CStack * unit = addStack(BattleSide::ATTACKER, CreatureID(pikeman), BattleHex(leftHex), 10);
		battle()->activeStack = unit->unitId();
		return unit;
	}

	/// The server ends a battle through the battle activity of its players, which a battle started
	/// from a BattleStart pack does not have.
	void addBattleActivity()
	{
		gameHandler->activities->addActivity(std::make_shared<BattleActivity>(gameHandler.get(), battle()));
	}
};

namespace
{

/// Defence a unit gains by defending: a fifth of its own rounded down, and at least one point,
/// plus its DEFENSIVE_STANCE.
struct DefendCase
{
	const char * name;
	const char * creature;
	int stance;
	int defenceGained;
};

}

class DefendActionTest : public BattleTestFixture, public ::testing::WithParamInterface<DefendCase>
{
};

TEST_P(DefendActionTest, raisesDefenceForTheTurn)
{
	const auto & scenario = GetParam();

	startGame();
	startBattle();

	CStack * unit = addStack(BattleSide::ATTACKER, creatureByName(scenario.creature), BattleHex(leftHex), 10);
	ASSERT_NE(unit, nullptr);

	if(scenario.stance != 0)
		unit->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::DEFENSIVE_STANCE, BonusSource::OTHER, scenario.stance, BonusSourceID()));

	const int defenceBefore = unit->getDefense(false);

	ASSERT_TRUE(act(BattleAction::makeDefend(unit)));

	EXPECT_EQ(unit->getDefense(false), defenceBefore + scenario.defenceGained) << scenario.name;
	EXPECT_TRUE(unit->defending) << scenario.name;
	EXPECT_TRUE(unit->hasBonusOfType(BonusType::UNIT_DEFENDING)) << scenario.name;
}

INSTANTIATE_TEST_SUITE_P(Creatures, DefendActionTest, ::testing::Values(
	DefendCase{"angel",           "core:angel",   0, 4},
	// a fifth of 8 is 1.6
	DefendCase{"griffin",         "core:griffin", 0, 1},
	// a fifth of 1 rounds to nothing, so the weakest units get a flat point instead
	DefendCase{"peasant",         "core:peasant", 0, 1},
	DefendCase{"angelWithStance", "core:angel",   3, 7}
),
	[](const ::testing::TestParamInfo<DefendCase> & info) { return info.param.name; });

TEST_F(NoTargetActionTest, retreatEndsBattleWithEnemyAsWinner)
{
	startGame();
	startBattle();
	activateUnitOfAttacker();
	addBattleActivity();

	ASSERT_TRUE(act(BattleAction::makeRetreat(BattleSide::ATTACKER)));

	ASSERT_EQ(server.battleResults.size(), 1u);
	EXPECT_EQ(server.battleResults.front().result, EBattleResult::ESCAPE);
	EXPECT_EQ(server.battleResults.front().winner, BattleSide::DEFENDER);
}

TEST_F(NoTargetActionTest, retreatIsRefusedWhenFleeingIsForbidden)
{
	startGame();
	startBattle();
	activateUnitOfAttacker();

	ASSERT_TRUE(battle()->battleCanFlee(PlayerColor(0)));

	// the penalty Shackles of War put on the opposing hero
	attackerSideHero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::BATTLE_CAN_FLEE, BonusSource::OTHER, -1, BonusSourceID()));
	ASSERT_FALSE(battle()->battleCanFlee(PlayerColor(0)));

	EXPECT_FALSE(act(BattleAction::makeRetreat(BattleSide::ATTACKER)));
	EXPECT_EQ(gameState()->currentBattles.size(), 1u);
}

TEST_F(NoTargetActionTest, surrenderPaysGoldAndEndsBattleWithEnemyAsWinner)
{
	startGame();
	startBattle();
	activateUnitOfAttacker();
	addBattleActivity();

	const auto cost = battle()->battleGetSurrenderCost(PlayerColor(0));
	ASSERT_GT(cost, 0);
	const auto goldBefore = gameState()->getPlayerState(PlayerColor(0))->resources[EGameResID::GOLD];

	ASSERT_TRUE(act(BattleAction::makeSurrender(BattleSide::ATTACKER)));

	EXPECT_EQ(gameState()->getPlayerState(PlayerColor(0))->resources[EGameResID::GOLD], goldBefore - cost);
	ASSERT_EQ(server.battleResults.size(), 1u);
	EXPECT_EQ(server.battleResults.front().result, EBattleResult::SURRENDER);
	EXPECT_EQ(server.battleResults.front().winner, BattleSide::DEFENDER);
}

TEST_F(NoTargetActionTest, surrenderIsRefusedWhenForbidden)
{
	startGame();
	startBattle();
	activateUnitOfAttacker();

	ASSERT_GT(battle()->battleGetSurrenderCost(PlayerColor(0)), 0);

	attackerSideHero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::BATTLE_CAN_SURRENDER, BonusSource::OTHER, -1, BonusSourceID()));
	ASSERT_LT(battle()->battleGetSurrenderCost(PlayerColor(0)), 0);

	const auto goldBefore = gameState()->getPlayerState(PlayerColor(0))->resources[EGameResID::GOLD];

	EXPECT_FALSE(act(BattleAction::makeSurrender(BattleSide::ATTACKER)));
	EXPECT_EQ(gameState()->getPlayerState(PlayerColor(0))->resources[EGameResID::GOLD], goldBefore);
	EXPECT_EQ(gameState()->currentBattles.size(), 1u);
}

TEST_F(NoTargetActionTest, surrenderIsRefusedWithoutEnoughGold)
{
	startGame();
	startBattle();
	activateUnitOfAttacker();

	const auto cost = battle()->battleGetSurrenderCost(PlayerColor(0));
	ASSERT_GT(cost, 0);

	const auto gold = gameState()->getPlayerState(PlayerColor(0))->resources[EGameResID::GOLD];
	gameHandler->giveResource(PlayerColor(0), EGameResID::GOLD, cost - 1 - gold);
	ASSERT_EQ(gameState()->getPlayerState(PlayerColor(0))->resources[EGameResID::GOLD], cost - 1);

	EXPECT_FALSE(act(BattleAction::makeSurrender(BattleSide::ATTACKER)));
	EXPECT_EQ(gameState()->getPlayerState(PlayerColor(0))->resources[EGameResID::GOLD], cost - 1);
	EXPECT_EQ(gameState()->currentBattles.size(), 1u);
}
