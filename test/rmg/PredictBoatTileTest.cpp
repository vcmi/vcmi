/*
 * PredictBoatTileTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/int3.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/rmg/Functions.h"

#include <functional>
#include <set>
#include <vector>

namespace
{

const int3 origin{10, 10, 0};
const int3 invalid{-1, -1, -1};

std::vector<int3> shipyardOffsets()
{
	CGShipyard shipyard(nullptr);
	const IBoatGenerator & generator = shipyard;
	std::vector<int3> offsets;
	generator.getOutOffsets(offsets);
	return offsets;
}

std::function<bool(const int3 &)> waterSet(std::initializer_list<int3> tiles)
{
	std::set<int3> set(tiles);
	return [set](const int3 & tile)
	{
		return set.count(tile) != 0;
	};
}

int3 at(int dx, int dy)
{
	return origin + int3(dx, dy, 0);
}

}

TEST(PredictBoatTile, WaterAtAAndLReturnsA)
{
	EXPECT_EQ(predictBoatTile(shipyardOffsets(), origin, waterSet({at(-2, 0), at(0, -1)})), at(-2, 0));
}

TEST(PredictBoatTile, WaterOnlyInTopRowReturnsH)
{
	EXPECT_EQ(predictBoatTile(shipyardOffsets(), origin, waterSet({
		at(-2, -1), at(2, -1), at(-1, -1), at(1, -1), at(0, -1)
	})), at(-2, -1));
}

TEST(PredictBoatTile, WaterAtBAndBottomRowReturnsB)
{
	EXPECT_EQ(predictBoatTile(shipyardOffsets(), origin, waterSet({
		at(2, 0),
		at(-2, 1), at(2, 1), at(-1, 1), at(1, 1), at(0, 1)
	})), at(2, 0));
}

TEST(PredictBoatTile, NoWaterReturnsInvalid)
{
	EXPECT_EQ(predictBoatTile(shipyardOffsets(), origin, waterSet({})), invalid);
}
