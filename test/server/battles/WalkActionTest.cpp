/*
 * WalkActionTest.cpp, part of VCMI engine
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
#include "../../../lib/battle/actions/BattleActionType.h"
#include "../../../lib/gameState/CGameState.h"

namespace
{
// creatures
constexpr int pikeman = 0; // walks 4 hexes
constexpr int griffin = 4; // flies 6 hexes
}

/// Where a WALK action leaves the unit, what stops it on the way, which destinations the
/// server refuses, and what the client shows for them. Every walk here goes along one row, where the shortest path is a straight line.
class WalkActionTest : public BattleTestFixture
{
public:
	static inline const BattleHex start = BattleHex(3, 5);
	static inline const BattleHex destination = BattleHex(6, 5);
	static inline const BattleHex halfway = BattleHex(5, 5);
};

TEST_F(WalkActionTest, unitReachesDestination)
{
	startGame();
	startBattle();

	CStack * walker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), start, 10);

	ASSERT_TRUE(act(BattleAction::makeMove(walker, destination)));
	EXPECT_EQ(walker->getPosition(), destination);
}

TEST_F(WalkActionTest, destinationOutOfReachIsRefused)
{
	startGame();
	startBattle();

	CStack * walker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), start, 10);

	EXPECT_FALSE(act(BattleAction::makeMove(walker, BattleHex(9, 5))));
	EXPECT_EQ(walker->getPosition(), start);
}

TEST_F(WalkActionTest, occupiedDestinationIsRefused)
{
	startGame();
	startBattle();

	CStack * walker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), start, 10);
	addStack(BattleSide::DEFENDER, CreatureID(pikeman), destination, 10);

	EXPECT_FALSE(act(BattleAction::makeMove(walker, destination)));
	EXPECT_EQ(walker->getPosition(), start);
}

TEST_F(WalkActionTest, quicksandOnTheWayStopsWalkingUnit)
{
	startGame();
	startBattle();

	CStack * walker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), start, 10);
	addQuicksand(halfway);

	// stepping into a trap is a legal move that ends early
	ASSERT_TRUE(act(BattleAction::makeMove(walker, destination)));
	EXPECT_EQ(walker->getPosition(), halfway);
}

TEST_F(WalkActionTest, flyingUnitPassesOverQuicksand)
{
	startGame();
	startBattle();

	CStack * flyer = addStack(BattleSide::ATTACKER, CreatureID(griffin), start, 10);
	addQuicksand(halfway);

	ASSERT_TRUE(act(BattleAction::makeMove(flyer, destination)));
	EXPECT_EQ(flyer->getPosition(), destination);
}

TEST_F(WalkActionTest, tacticsPhaseWalkDoesNotEndTurn)
{
	startGame();
	startBattle();

	CStack * walker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), BattleHex(2, 3), 10);
	battle()->tacticDistance = 4;
	battle()->tacticsSide = BattleSide::ATTACKER;

	ASSERT_TRUE(act(BattleAction::makeMove(walker, BattleHex(4, 3))));
	EXPECT_EQ(walker->getPosition(), BattleHex(4, 3));
	EXPECT_EQ(walker->getTurnStatus(), battle::UnitTurnStatus::READY);
}

TEST_F(WalkActionTest, tacticsPhaseWalkOutsideTacticsZoneIsRefused)
{
	startGame();
	startBattle();

	const CStack * walker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), BattleHex(1, 3), 10);
	battle()->tacticDistance = 2;
	battle()->tacticsSide = BattleSide::ATTACKER;

	// in the reach of a pikeman, but beyond the columns of the tactics zone
	EXPECT_FALSE(act(BattleAction::makeMove(walker, BattleHex(4, 3))));
	EXPECT_EQ(walker->getPosition(), BattleHex(1, 3));
}

TEST_F(WalkActionTest, defenderWalksIntoCastleGate)
{
	startGame();
	startSiege();

	const CStack * walker = addStack(BattleSide::DEFENDER, CreatureID(pikeman), BattleHex(BattleHex::GATE_INNER).cloneInDirection(BattleHex::RIGHT), 10);

	ASSERT_TRUE(act(BattleAction::makeMove(walker, BattleHex(BattleHex::GATE_OUTER))));
	EXPECT_EQ(walker->getPosition(), BattleHex(BattleHex::GATE_OUTER));
}

TEST_F(WalkActionTest, walkPreviewShadesHexInReach)
{
	startGame();
	startBattle();

	const CStack * walker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), start, 10);

	const auto shown = previewOf(walker, EActionType::WALK, destination);
	EXPECT_EQ(shown.cursor, "combatMove");
	EXPECT_EQ(shown.shadedHexes, BattleHexArray({destination}));

	const auto option = optionOf(walker, EActionType::WALK);
	EXPECT_EQ(option.type->build(*battle(), option, {walker, destination}).value().getTarget(battle()).at(0).hexValue, destination);
}

TEST_F(WalkActionTest, flyingUnitShowsFlyCursor)
{
	startGame();
	startBattle();

	const CStack * flyer = addStack(BattleSide::ATTACKER, CreatureID(griffin), start, 10);

	EXPECT_EQ(previewOf(flyer, EActionType::WALK, destination).cursor, "combatFly");
}

TEST_F(WalkActionTest, walkPreviewOutOfReachIsBlocked)
{
	startGame();
	startBattle();

	const CStack * walker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), start, 10);

	const auto shown = previewOf(walker, EActionType::WALK, BattleHex(9, 5));
	EXPECT_EQ(shown.cursor, "combatBlocked");
	EXPECT_TRUE(shown.shadedHexes.empty());
}

TEST_F(WalkActionTest, ownHexIsNoMoveTarget)
{
	startGame();
	startBattle();

	const CStack * walker = addStack(BattleSide::ATTACKER, CreatureID(pikeman), start, 10);

	EXPECT_FALSE(isLegal(optionOf(walker, EActionType::WALK), {walker, start}));
}

TEST_F(WalkActionTest, twoHexUnitMovesWhereItsTailFits)
{
	startGame();
	startBattle();

	const CStack * flyer = addStack(BattleSide::ATTACKER, CreatureID(griffin), start, 10);
	ASSERT_TRUE(flyer->doubleWide());
	addStack(BattleSide::DEFENDER, CreatureID(pikeman), halfway, 10);

	// the head can't stand on the hovered hex, because the tail would stand on the enemy; one hex further both fit
	const BattleHex head = destination.cloneInDirection(BattleHex::RIGHT);
	EXPECT_EQ(previewOf(flyer, EActionType::WALK, destination).shadedHexes, BattleHexArray({head, destination}));
}
