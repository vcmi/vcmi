/*
 * MoatObstacleTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "lib/battle/obstacle/MoatObstacle.h"

using namespace ::testing;

TEST(MoatObstacleTest, getType)
{
	MoatObstacle moat;
	ASSERT_EQ(moat.getType(), ObstacleType::MOAT);
}

TEST(MoatObstacleTest, setupDamage)
{
	MoatObstacle moat;
	moat.setDamage(122);
	ASSERT_EQ(moat.getDamage(), 122);
}
