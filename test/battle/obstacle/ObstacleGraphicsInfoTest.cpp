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
#include "lib/battle/obstacle/ObstacleGraphicsInfo.h"

using namespace ::testing;

TEST(ObstacleGraphicsInfoTest, setupParameters)
{
	ObstacleGraphicsInfo graphicsInfo;
	graphicsInfo.setGraphicsName("EXPLOSION.pcx");
	graphicsInfo.setOffsetGraphicsInX(0);
	graphicsInfo.setOffsetGraphicsInY(25);
	EXPECT_EQ(graphicsInfo.getGraphicsName(), "EXPLOSION.pcx");
	EXPECT_EQ(graphicsInfo.getOffsetGraphicsInX(), 0);
	EXPECT_EQ(graphicsInfo.getOffsetGraphicsInY(), 25);
}
