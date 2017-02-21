/*
 * BattleField.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include <boost/test/unit_test.hpp>
#include "../lib/BattleHex.h"

BOOST_AUTO_TEST_SUITE(BattlefieldHex_Suite)

BOOST_AUTO_TEST_CASE(getNeighbouringTiles)
{
	BattleHex mainHex;
	std::vector<BattleHex> neighbouringTiles;
	mainHex.setXY(16,0);
	neighbouringTiles = mainHex.neighbouringTiles();
	BOOST_TEST(neighbouringTiles.size()==1);
	mainHex.setXY(0,0);
	neighbouringTiles = mainHex.neighbouringTiles();
	BOOST_TEST(neighbouringTiles.size()==2);
	mainHex.setXY(15,2);
	neighbouringTiles = mainHex.neighbouringTiles();
	BOOST_TEST(neighbouringTiles.size()==3);
	mainHex.setXY(2,0);
	neighbouringTiles = mainHex.neighbouringTiles();
	BOOST_TEST(neighbouringTiles.size()==4);
	mainHex.setXY(1,2);
	neighbouringTiles = mainHex.neighbouringTiles();
	BOOST_TEST(neighbouringTiles.size()==5);
	mainHex.setXY(8,5);
	neighbouringTiles = mainHex.neighbouringTiles();
	BOOST_TEST(neighbouringTiles.size()==6);

	BOOST_REQUIRE(neighbouringTiles.size()==6 && mainHex==93);
	BOOST_TEST(neighbouringTiles.at(0)==75);
	BOOST_TEST(neighbouringTiles.at(1)==94);
	BOOST_TEST(neighbouringTiles.at(2)==110);
	BOOST_TEST(neighbouringTiles.at(3)==109);
	BOOST_TEST(neighbouringTiles.at(4)==92);
	BOOST_TEST(neighbouringTiles.at(5)==76);
}

BOOST_AUTO_TEST_CASE(getDistance)
{
	BattleHex firstHex(0,0), secondHex(16,0);
	BOOST_TEST((int)firstHex.getDistance(firstHex,secondHex)==16);
	firstHex=0, secondHex=170;
	BOOST_TEST((int)firstHex.getDistance(firstHex,secondHex)==10);
	firstHex=16, secondHex=181;
	BOOST_TEST((int)firstHex.getDistance(firstHex,secondHex)==10);
	firstHex=186, secondHex=70;
	BOOST_TEST((int)firstHex.getDistance(firstHex,secondHex)==17);
	firstHex=166, secondHex=39;
	BOOST_TEST((int)firstHex.getDistance(firstHex,secondHex)==11);
	firstHex=25, secondHex=103;
	BOOST_TEST((int)firstHex.getDistance(firstHex,secondHex)==9);
	firstHex=18, secondHex=71;
	BOOST_TEST((int)firstHex.getDistance(firstHex,secondHex)==4);
}

BOOST_AUTO_TEST_CASE(mutualPositions)
{
	BattleHex firstHex(0,0), secondHex(16,0);
	firstHex=86, secondHex=68;
	BOOST_TEST((int)firstHex.mutualPosition(firstHex,secondHex)==0);
	secondHex=69;
	BOOST_TEST((int)firstHex.mutualPosition(firstHex,secondHex)==1);
	secondHex=87;
	BOOST_TEST((int)firstHex.mutualPosition(firstHex,secondHex)==2);
	secondHex=103;
	BOOST_TEST((int)firstHex.mutualPosition(firstHex,secondHex)==3);
	secondHex=102;
	BOOST_TEST((int)firstHex.mutualPosition(firstHex,secondHex)==4);
	secondHex=85;
	BOOST_TEST((int)firstHex.mutualPosition(firstHex,secondHex)==5);
	secondHex=46;
	BOOST_TEST((int)firstHex.mutualPosition(firstHex,secondHex)==-1);
}

BOOST_AUTO_TEST_CASE(getClosestTile)
{
	BattleHex mainHex(0);
	std::set<BattleHex> possibilities;
	possibilities.insert(3);
	possibilities.insert(170);
	possibilities.insert(100);
	possibilities.insert(119);
	possibilities.insert(186);

	BOOST_TEST(mainHex.getClosestTile(true,mainHex,possibilities)==3);
	mainHex = 139;
	BOOST_TEST(mainHex.getClosestTile(false,mainHex,possibilities)==119);
	mainHex = 16;
	BOOST_TEST(mainHex.getClosestTile(false,mainHex,possibilities)==100);
	mainHex = 166;
	BOOST_TEST(mainHex.getClosestTile(true,mainHex,possibilities)==186);
	mainHex = 76;
	BOOST_TEST(mainHex.getClosestTile(false,mainHex,possibilities)==3);
	BOOST_TEST(mainHex.getClosestTile(true,mainHex,possibilities)==100);
}

BOOST_AUTO_TEST_CASE(moveEDir)
{
	BattleHex mainHex(20);
	mainHex.moveInDir(BattleHex::EDir::BOTTOM_RIGHT);
	BOOST_TEST(mainHex==37);
	mainHex.moveInDir(BattleHex::EDir::RIGHT);
	BOOST_TEST(mainHex==38);
	mainHex.moveInDir(BattleHex::EDir::TOP_RIGHT);
	BOOST_TEST(mainHex==22);
	mainHex.moveInDir(BattleHex::EDir::TOP_LEFT);
	BOOST_TEST(mainHex==4);
	mainHex.moveInDir(BattleHex::EDir::LEFT);
	BOOST_TEST(mainHex==3);
	mainHex.moveInDir(BattleHex::EDir::BOTTOM_LEFT);
	BOOST_TEST(mainHex==20);
}

BOOST_AUTO_TEST_SUITE_END()
