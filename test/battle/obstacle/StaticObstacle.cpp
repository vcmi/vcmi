/*
 * StaticObstacle.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "lib/battle/obstacle/StaticObstacle.h"

TEST(StaticObstacleTest, getType)
{
	StaticObstacle obstacle;
	ASSERT_EQ(obstacle.getType(), ObstacleType::STATIC);
}
