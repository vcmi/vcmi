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

namespace
{
// creatures
constexpr int pikeman = 0; // walks 4 hexes
constexpr int griffin = 4; // flies 6 hexes
}

/// Where a WALK action leaves the unit, what stops it on the way, and which destinations the
/// server refuses. Every walk here goes along one row, where the shortest path is a straight line.
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
	EXPECT_FALSE(walker->movedThisRound);
}
