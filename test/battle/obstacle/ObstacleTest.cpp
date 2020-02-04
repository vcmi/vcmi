/*
 * ObstacleTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "lib/battle/obstacle/Obstacle.h"
#include "mock/mock_Obstacle.h"

using namespace ::testing;

TEST(ObstacleTest, checkID)
{
	ASSERT_NE(ObstacleMock().ID.getID(), ObstacleMock().ID.getID());
	ASSERT_NE(ObstacleMock().ID.getID(), ObstacleMock().ID.getID());
	ASSERT_NE(ObstacleMock().ID.getID(), ObstacleMock().ID.getID());
}

TEST(ObstacleTest, blocksTiles)
{
	ObstacleMock mock;

	EXPECT_CALL(mock, getType()).Times(AtLeast(1)).WillRepeatedly(Return(ObstacleType::STATIC));
	EXPECT_TRUE(mock.blocksTiles());

	EXPECT_CALL(mock, getType()).Times(AtLeast(1)).WillRepeatedly(Return(ObstacleType::MOAT));
	EXPECT_FALSE(mock.blocksTiles());
}

TEST(ObstacleTest, stopsMovement)
{
	ObstacleMock mock;

	EXPECT_CALL(mock, getType()).Times(AtLeast(1)).WillRepeatedly(Return(ObstacleType::STATIC));
	EXPECT_FALSE(mock.stopsMovement());

	EXPECT_CALL(mock, getType()).Times(AtLeast(1)).WillRepeatedly(Return(ObstacleType::MOAT));
	EXPECT_TRUE(mock.stopsMovement());

}

TEST(ObstacleTest, setupArea)
{
	ObstacleMock mock;
	ObstacleArea area(std::vector<BattleHex>{4,5,6,7,8,9,10}, 22);
	mock.setArea(area);
	EXPECT_THAT(mock.getArea().getFields(), ContainerEq(area.getFields()));
	EXPECT_EQ(mock.getArea().getPosition(), area.getPosition());
}

TEST(ObstacleTest, setupGraphicsInfo)
{
	ObstacleMock mock;
	ObstacleGraphicsInfo info;
	info.setGraphicsName("FIREWALL.pcx");
	info.setOffsetGraphicsInX(-123);
	info.setOffsetGraphicsInY(55);
	mock.setGraphicsInfo(info);
	EXPECT_EQ(mock.getGraphicsInfo().getGraphicsName(), "FIREWALL.pcx");
	EXPECT_EQ(mock.getGraphicsInfo().getOffsetGraphicsInX(), -123);
	EXPECT_EQ(mock.getGraphicsInfo().getOffsetGraphicsInY(), 55);

}
