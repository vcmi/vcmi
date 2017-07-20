/*
 * BattleHexTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "../lib/battle/BattleHex.h"

TEST(BattleHexTest, getNeighbouringTiles)
{
	BattleHex mainHex;
	std::vector<BattleHex> neighbouringTiles;
	mainHex.setXY(16,0);
	neighbouringTiles = mainHex.neighbouringTiles();
	EXPECT_EQ(neighbouringTiles.size(), 1);
	mainHex.setXY(0,0);
	neighbouringTiles = mainHex.neighbouringTiles();
	EXPECT_EQ(neighbouringTiles.size(), 2);
	mainHex.setXY(15,2);
	neighbouringTiles = mainHex.neighbouringTiles();
	EXPECT_EQ(neighbouringTiles.size(), 3);
	mainHex.setXY(2,0);
	neighbouringTiles = mainHex.neighbouringTiles();
	EXPECT_EQ(neighbouringTiles.size(), 4);
	mainHex.setXY(1,2);
	neighbouringTiles = mainHex.neighbouringTiles();
	EXPECT_EQ(neighbouringTiles.size(), 5);
	mainHex.setXY(8,5);
	neighbouringTiles = mainHex.neighbouringTiles();
	EXPECT_EQ(neighbouringTiles.size(), 6);

	ASSERT_TRUE(neighbouringTiles.size()==6 && mainHex==93);
	EXPECT_EQ(neighbouringTiles.at(0), 75);
	EXPECT_EQ(neighbouringTiles.at(1), 76);
	EXPECT_EQ(neighbouringTiles.at(2), 94);
	EXPECT_EQ(neighbouringTiles.at(3), 110);
	EXPECT_EQ(neighbouringTiles.at(4), 109);
	EXPECT_EQ(neighbouringTiles.at(5), 92);
}

TEST(BattleHexTest, getDistance)
{
	BattleHex firstHex(0,0), secondHex(16,0);
	EXPECT_EQ((int)firstHex.getDistance(firstHex,secondHex), 16);
	firstHex=0, secondHex=170;
	EXPECT_EQ((int)firstHex.getDistance(firstHex,secondHex), 10);
	firstHex=16, secondHex=181;
	EXPECT_EQ((int)firstHex.getDistance(firstHex,secondHex), 10);
	firstHex=186, secondHex=70;
	EXPECT_EQ((int)firstHex.getDistance(firstHex,secondHex), 17);
	firstHex=166, secondHex=39;
	EXPECT_EQ((int)firstHex.getDistance(firstHex,secondHex), 11);
	firstHex=25, secondHex=103;
	EXPECT_EQ((int)firstHex.getDistance(firstHex,secondHex), 9);
	firstHex=18, secondHex=71;
	EXPECT_EQ((int)firstHex.getDistance(firstHex,secondHex), 4);
}

TEST(BattleHexTest, mutualPositions)
{
	BattleHex firstHex(0,0), secondHex(16,0);
	firstHex=86, secondHex=68;
	EXPECT_EQ((int)firstHex.mutualPosition(firstHex,secondHex), BattleHex::EDir::TOP_LEFT);
	secondHex=69;
	EXPECT_EQ((int)firstHex.mutualPosition(firstHex,secondHex), BattleHex::EDir::TOP_RIGHT);
	secondHex=87;
	EXPECT_EQ((int)firstHex.mutualPosition(firstHex,secondHex), BattleHex::EDir::RIGHT);
	secondHex=103;
	EXPECT_EQ((int)firstHex.mutualPosition(firstHex,secondHex), BattleHex::EDir::BOTTOM_RIGHT);
	secondHex=102;
	EXPECT_EQ((int)firstHex.mutualPosition(firstHex,secondHex), BattleHex::EDir::BOTTOM_LEFT);
	secondHex=85;
	EXPECT_EQ((int)firstHex.mutualPosition(firstHex,secondHex), BattleHex::EDir::LEFT);
	secondHex=46;
	EXPECT_EQ((int)firstHex.mutualPosition(firstHex,secondHex), -1);
}

TEST(BattleHexTest, getClosestTile)
{
	BattleHex mainHex(0);
	std::set<BattleHex> possibilities;
	possibilities.insert(3);
	possibilities.insert(170);
	possibilities.insert(100);
	possibilities.insert(119);
	possibilities.insert(186);

	EXPECT_EQ(mainHex.getClosestTile(0,mainHex,possibilities), 3);
	mainHex = 139;
	EXPECT_EQ(mainHex.getClosestTile(1,mainHex,possibilities), 119);
	mainHex = 16;
	EXPECT_EQ(mainHex.getClosestTile(1,mainHex,possibilities), 100);
	mainHex = 166;
	EXPECT_EQ(mainHex.getClosestTile(0,mainHex,possibilities), 186);
	mainHex = 76;
	EXPECT_EQ(mainHex.getClosestTile(1,mainHex,possibilities), 3);
	EXPECT_EQ(mainHex.getClosestTile(0,mainHex,possibilities), 100);
}

TEST(BattleHexTest, moveEDir)
{
	BattleHex mainHex(20);
	mainHex.moveInDirection(BattleHex::EDir::BOTTOM_RIGHT);
	EXPECT_EQ(mainHex, 37);
	mainHex.moveInDirection(BattleHex::EDir::RIGHT);
	EXPECT_EQ(mainHex, 38);
	mainHex.moveInDirection(BattleHex::EDir::TOP_RIGHT);
	EXPECT_EQ(mainHex, 22);
	mainHex.moveInDirection(BattleHex::EDir::TOP_LEFT);
	EXPECT_EQ(mainHex, 4);
	mainHex.moveInDirection(BattleHex::EDir::LEFT);
	EXPECT_EQ(mainHex, 3);
	mainHex.moveInDirection(BattleHex::EDir::BOTTOM_LEFT);
	EXPECT_EQ(mainHex, 20);
}
