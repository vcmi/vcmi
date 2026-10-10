/*
 * ExploreNeighbourTileTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "AI/Nullkiller2/AIGateway.h"
#include "AI/Nullkiller2/Goals/ExploreNeighbourTile.h"

#include "mock/TinyH3MBuilder.h"
#include "nullkiller2/NullkillerTest.h"

#include "lib/callback/CCallback.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapping/CMap.h"

namespace
{
const PlayerColor PLAYER = PlayerColor(0);
const PlayerColor ENEMY = PlayerColor(1);

NK2AI::NeighbourExplorationCandidate makeAcceptedCandidate()
{
	NK2AI::NeighbourExplorationCandidate candidate;
	candidate.sameDay = true;
	candidate.accessible = true;
	candidate.safe = true;
	candidate.tilesDiscovered = 3;
	candidate.movementCost = 0.5f;
	return candidate;
}

TinyH3M::TinyH3MBuilder makeNeighbourMap()
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.name("NK2Neighbour")
		.playerActive(PLAYER)
		.playerActive(ENEMY)
		.hero({5, 5, 0}, HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID(27), 1}})
		.randomTown({9, 5, 0}, ENEMY);

	return builder;
}

class Nullkiller2_Goals_ExploreNeighbourTileMap : public NullkillerTest
{
};
}

TEST(Nullkiller2_Goals_ExploreNeighbourTile, acceptsSafeSameDayDiscovery)
{
	const auto evaluation = NK2AI::evaluateNeighbourExplorationCandidate(makeAcceptedCandidate());

	EXPECT_TRUE(evaluation.accepted);
	EXPECT_FLOAT_EQ(evaluation.value, 18.0f);
}

TEST(Nullkiller2_Goals_ExploreNeighbourTile, discountsNormalMovementCost)
{
	auto cheaperCandidate = makeAcceptedCandidate();
	auto expensiveCandidate = makeAcceptedCandidate();
	expensiveCandidate.movementCost = 1.5f;

	EXPECT_GT(
		NK2AI::evaluateNeighbourExplorationCandidate(cheaperCandidate).value,
		NK2AI::evaluateNeighbourExplorationCandidate(expensiveCandidate).value)
		<< "equal discovery should prefer the neighbour that leaves more movement";
}

TEST(Nullkiller2_Goals_ExploreNeighbourTile, clampsTinyMovementCost)
{
	auto candidate = makeAcceptedCandidate();
	candidate.movementCost = 0.05f;

	const auto evaluation = NK2AI::evaluateNeighbourExplorationCandidate(candidate);

	EXPECT_TRUE(evaluation.accepted);
	EXPECT_FLOAT_EQ(evaluation.value, 90.0f);
}

TEST(Nullkiller2_Goals_ExploreNeighbourTile, rejectsNextDayMove)
{
	auto candidate = makeAcceptedCandidate();
	candidate.sameDay = false;

	EXPECT_FALSE(NK2AI::evaluateNeighbourExplorationCandidate(candidate).accepted);
}

TEST(Nullkiller2_Goals_ExploreNeighbourTile, rejectsUnsafeDestination)
{
	auto candidate = makeAcceptedCandidate();
	candidate.safe = false;

	EXPECT_FALSE(NK2AI::evaluateNeighbourExplorationCandidate(candidate).accepted);
}

TEST(Nullkiller2_Goals_ExploreNeighbourTile, rejectsMoveWithNoDiscovery)
{
	auto candidate = makeAcceptedCandidate();
	candidate.tilesDiscovered = 0;

	EXPECT_FALSE(NK2AI::evaluateNeighbourExplorationCandidate(candidate).accepted);
}

TEST_F(Nullkiller2_Goals_ExploreNeighbourTileMap, findsNoTargetOnRevealedMap)
{
	startWithMap(makeNeighbourMap());

	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	hero->setMovementPoints(2000);

	revealMap(PLAYER);

	const auto callback = makeCallback(PLAYER);
	const auto gateway = makeGateway(callback);
	const auto target = NK2AI::Goals::ExploreNeighbourTile::findTarget(
		hero,
		gateway->nullkiller.get());

	EXPECT_FALSE(target.has_value());
}

TEST_F(Nullkiller2_Goals_ExploreNeighbourTileMap, reportsUnreachableLivePathAsFailure)
{
	startWithMap(makeNeighbourMap());

	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	hero->setMovementPoints(2000);

	const int3 unreachableTile(6, 5, 0);
	map()->getTile(unreachableTile).terrainType = TerrainId(ETerrainId::ROCK);
	revealMap(PLAYER);

	const auto callback = makeCallback(PLAYER);
	const auto gateway = makeGateway(callback);

	EXPECT_EQ(
		gateway->moveHeroToTile(unreachableTile, NK2AI::HeroPtr(hero, callback.get())),
		NK2AI::HeroMovementResult::BLOCKED);
}
