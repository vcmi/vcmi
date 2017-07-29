/*
 * ObstacleArea.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "lib/battle/obstacle/ObstacleArea.h"
#include "lib/battle/BattleHex.h"

class ObstacleAreaTest : public ::testing::Test
{
public:
	ObstacleArea area;
};

TEST_F(ObstacleAreaTest, getInitialArea)
{
	EXPECT_EQ(area.getPosition(), 0);
	EXPECT_EQ(area.getWidth(), 0);
	EXPECT_EQ(area.getHeight(), 0);
	EXPECT_TRUE(area.getFields().empty());
}

TEST_F(ObstacleAreaTest, getFields)
{
	area.addField(BattleHex(12));
	EXPECT_EQ(area.getFields().size(), 1);
	area.addField(BattleHex(12));
	EXPECT_EQ(area.getFields().size(), 1);
	EXPECT_EQ(area.getFields().at(0), BattleHex(12));
}

TEST_F(ObstacleAreaTest, moveAreaToField)
{
	std::vector<BattleHex> fields{103, 104, 122, 138, 137, 120};

	area.setArea(fields);
	area.setPosition(121);
	area.moveAreaToField(44);

	EXPECT_EQ(area.getPosition(), 44);
	EXPECT_EQ(area.getFields().size(), 6);
	EXPECT_EQ(area.getFields().at(0), 27);
	EXPECT_EQ(area.getFields().at(1), 28);
	EXPECT_EQ(area.getFields().at(2), 45);
	EXPECT_EQ(area.getFields().at(3), 62);
	EXPECT_EQ(area.getFields().at(4), 61);
	EXPECT_EQ(area.getFields().at(5), 43);

	area.moveAreaToField(61);

	EXPECT_EQ(area.getFields().at(0), 43);
	EXPECT_EQ(area.getFields().at(1), 44);
	EXPECT_EQ(area.getFields().at(2), 62);
	EXPECT_EQ(area.getFields().at(3), 78);
	EXPECT_EQ(area.getFields().at(4), 77);
	EXPECT_EQ(area.getFields().at(5), 60);

	area.moveAreaToField(19);

	EXPECT_EQ(area.getFields().at(0), 1);
	EXPECT_EQ(area.getFields().at(1), 2);
	EXPECT_EQ(area.getFields().at(2), 20);
	EXPECT_EQ(area.getFields().at(3), 36);
	EXPECT_EQ(area.getFields().at(4), 35);
	EXPECT_EQ(area.getFields().at(5), 18);

	fields.clear();
	fields = {2, 19, 35, 52};

	area.setArea(fields);
	area.setPosition(130);
	area.moveAreaToField(146);

	EXPECT_EQ(area.getFields().at(0), 19);
	EXPECT_EQ(area.getFields().at(1), 35);
	EXPECT_EQ(area.getFields().at(2), 52);
	EXPECT_EQ(area.getFields().at(3), 68);

	fields.clear();
	fields = {74, 58, 40, 24};

	area.setArea(fields);
	area.setPosition(74);
	area.moveAreaToField(127);

	EXPECT_EQ(area.getFields().at(0), 127);
	EXPECT_EQ(area.getFields().at(1), 110);
	EXPECT_EQ(area.getFields().at(2), 93);
	EXPECT_EQ(area.getFields().at(3), 76);
}


TEST_F(ObstacleAreaTest, getWidth)
{
	std::vector<BattleHex> fields = {1,2};
	area.setArea(fields);
	EXPECT_EQ(area.getWidth(), 2);
	fields = {0,1,2,3};
	area.setArea(fields);
	EXPECT_EQ(area.getWidth(), 4);
	fields = {0, 1, -13, -14, -15, -16};
	area.setArea(fields);
	EXPECT_EQ(area.getWidth(), 5);
	fields = {-15, -16};
	area.setArea(fields);
	EXPECT_EQ(area.getWidth(), 2);
	fields = {-16};
	area.setArea(fields);
	EXPECT_EQ(area.getWidth(), 1);
	fields = {-15, -16, -32, -33, -48, -49};
	area.setArea(fields);
	EXPECT_EQ(area.getWidth(), 3);
	fields = {1, -16, -33};
	area.setArea(fields);
	EXPECT_EQ(area.getWidth(), 1);
	fields = {3, -13, -14, -15, -33, -49, -66};
	area.setArea(fields);
	EXPECT_EQ(area.getWidth(), 4);
	fields = {-10, -11, -12, -13, -14, -15, -16};
	area.setArea(fields);
	EXPECT_EQ(area.getWidth(), 7);
	fields = {1, 16};
	area.setArea(fields);
	EXPECT_EQ(area.getWidth(), 16);
	fields = {0, 1, -13, -14};
	area.setArea(fields);
	EXPECT_EQ(area.getWidth(), 5);
}

TEST_F(ObstacleAreaTest, getHeight)
{
	std::vector<BattleHex> fields = {0, 1, 5, 6};
	area.setArea(fields);
	EXPECT_EQ(area.getHeight(), 1);
	fields = {-15, -16, -32, -33, -48, -49};
	area.setArea(fields);
	EXPECT_EQ(area.getHeight(), 3);
	fields = {1, 2, 3, -13, -14, -15, -16};
	area.setArea(fields);
	EXPECT_EQ(area.getHeight(), 2);
	fields = {1, -1, -17};
	area.setArea(fields);
	EXPECT_EQ(area.getHeight(), 2);
	fields = {1, 170};
	area.setArea(fields);
	EXPECT_EQ(area.getHeight(), 11);
	fields = {-170, 1, 170};
	area.setArea(fields);
	EXPECT_EQ(area.getHeight(), 21);
	fields = {-171, 1, 170};
	area.setArea(fields);
	EXPECT_EQ(area.getHeight(), 22);
}

TEST_F(ObstacleAreaTest, getPosition)
{
	area.setPosition(12);
	EXPECT_EQ(area.getPosition(), 12);
	area.setPosition(33);
	EXPECT_EQ(area.getPosition(), 33);
	area.setPosition(5);
	EXPECT_EQ(area.getPosition(), 5);
}
