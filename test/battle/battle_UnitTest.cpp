/*
 * battle_UnitTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/battle/Unit.h"

TEST(battle_Unit_getSurroundingHexes, oneWide)
{
	BattleHex position(77);

	auto actual = battle::Unit::getSurroundingHexes(position, false, 0);

	EXPECT_EQ(actual, position.neighbouringTiles());
}

TEST(battle_Unit_getSurroundingHexes, oneWideLeftCorner)
{
	BattleHex position(34);

	auto actual = battle::Unit::getSurroundingHexes(position, false, 0);

	EXPECT_EQ(actual, position.neighbouringTiles());
}

TEST(battle_Unit_getSurroundingHexes, oneWideRightCorner)
{
	BattleHex position(117);

	auto actual = battle::Unit::getSurroundingHexes(position, false, 0);

	EXPECT_EQ(actual, position.neighbouringTiles());
}

TEST(battle_Unit_getSurroundingHexes, doubleWideAttacker)
{
	BattleHex position(77);

	auto actual = battle::Unit::getSurroundingHexes(position, true, BattleSide::ATTACKER);

	static const std::vector<BattleHex> expected =
	{
		60,
		61,
		78,
		95,
		94,
		93,
		75,
		59
	};

	EXPECT_EQ(actual, expected);
}

TEST(battle_Unit_getSurroundingHexes, doubleWideLeftCorner)
{
	BattleHex position(52);

	auto actualAtt = battle::Unit::getSurroundingHexes(position, true, BattleSide::ATTACKER);

	static const std::vector<BattleHex> expectedAtt =
	{
		35,
		53,
		69
	};

	EXPECT_EQ(actualAtt, expectedAtt);

	auto actualDef = battle::Unit::getSurroundingHexes(position, true, BattleSide::DEFENDER);

	static const std::vector<BattleHex> expectedDef =
	{
		35,
		36,
		54,
		70,
		69
	};
	EXPECT_EQ(actualDef, expectedDef);
}


TEST(battle_Unit_getSurroundingHexes, doubleWideRightCorner)
{
	BattleHex position(134);

	auto actualAtt = battle::Unit::getSurroundingHexes(position, true, BattleSide::ATTACKER);

	static const std::vector<BattleHex> expectedAtt =
	{
		116,
		117,
		151,
		150,
		149,
		132,
		115
	};

	EXPECT_EQ(actualAtt, expectedAtt);

	auto actualDef = battle::Unit::getSurroundingHexes(position, true, BattleSide::DEFENDER);

	static const std::vector<BattleHex> expectedDef =
	{
		116,
		117,
		151,
		150,
		133
	};

	EXPECT_EQ(actualDef, expectedDef);
}


TEST(battle_Unit_getSurroundingHexes, doubleWideDefender)
{
	BattleHex position(77);

	auto actual = battle::Unit::getSurroundingHexes(position, true, BattleSide::DEFENDER);

	static const std::vector<BattleHex> expected =
	{
		60,
		61,
		62,
		79,
		96,
		95,
		94,
		76,
	};

	EXPECT_EQ(actual, expected);
}
