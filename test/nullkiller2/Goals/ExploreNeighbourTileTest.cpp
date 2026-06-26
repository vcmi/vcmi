/*
 * ExploreNeighbourTileTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Goals/ExploreNeighbourTile.h"

TEST(Nullkiller2_Goals_ExploreNeighbourTile, scoreDiscountsNormalMovementCost)
{
	constexpr int tilesDiscovered = 6;

	EXPECT_GT(
		NK2AI::Goals::ExploreNeighbourTile::evaluateTileScore(tilesDiscovered, 0.5f),
		NK2AI::Goals::ExploreNeighbourTile::evaluateTileScore(tilesDiscovered, 1.5f))
		<< "equal discovery should prefer the neighbour that leaves more movement";

	EXPECT_FLOAT_EQ(
		NK2AI::Goals::ExploreNeighbourTile::evaluateTileScore(tilesDiscovered, 0.01f),
		NK2AI::Goals::ExploreNeighbourTile::evaluateTileScore(tilesDiscovered, 0.1f))
		<< "tiny movement costs should still be clamped";
}
