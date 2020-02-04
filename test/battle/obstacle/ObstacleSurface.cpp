/*
 * ObstacleSurface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "lib/battle/obstacle/ObstacleSurface.h"

class ObstacleSurfaceTest : public ::testing::Test
{
public:
	ObstacleSurface terrain;

	void isAppropriate(BattlefieldType type, bool expect = true)
	{
		if(expect)
			EXPECT_TRUE(terrain.isAppropriateForSurface(type));
		else
			EXPECT_FALSE(terrain.isAppropriateForSurface(type));
	}
};

TEST_F(ObstacleSurfaceTest, isAppropriateForSurface)
{
	isAppropriate(BattlefieldType::ROCKLANDS, false);
	terrain.battlefieldSurface.push_back(BattlefieldType::SAND_SHORE);
	terrain.battlefieldSurface.push_back(BattlefieldType::SHIP);
	isAppropriate(BattlefieldType::SAND_SHORE);
	isAppropriate(BattlefieldType::SHIP);
	isAppropriate(BattlefieldType::SHIP_TO_SHIP, false);
}

TEST_F(ObstacleSurfaceTest, addingFromStringToSurface)
{
	isAppropriate(BattlefieldType::ROCKLANDS, false);
	terrain.battlefieldSurface.push_back(BattlefieldType::fromString("ship"));
	terrain.battlefieldSurface.push_back(BattlefieldType::fromString("sandShore"));
	isAppropriate(BattlefieldType::SAND_SHORE);
	isAppropriate(BattlefieldType::SHIP);
	isAppropriate(BattlefieldType::SHIP_TO_SHIP, false);
}

