/*
 * BattlefieldTypeTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "lib/GameConstants.h"

TEST(BattlefieldTypeTest, toString){
	BattlefieldType type;
	type = BattlefieldType::CURSED_GROUND;
	EXPECT_EQ(type.toString(), "cursedGround");
	type = BattlefieldType::SAND_SHORE;
	EXPECT_EQ(type.toString(), "sandShore");
	type = BattlefieldType::NONE;
	EXPECT_EQ(type.toString(), "none");
	type = BattlefieldType::SHIP;
	EXPECT_EQ(type.toString(), "ship");
	type = BattlefieldType::FAVORABLE_WINDS;
	EXPECT_EQ(type.toString(), "favorableWinds");
}

TEST(BattlefieldTypeTest, fromString){
	EXPECT_EQ(BattlefieldType::fromString("none"), BattlefieldType::NONE);
	EXPECT_EQ(BattlefieldType::fromString("cursedGround"), BattlefieldType::CURSED_GROUND);
	EXPECT_EQ(BattlefieldType::fromString("dirtBirches"), BattlefieldType::DIRT_BIRCHES);
	EXPECT_EQ(BattlefieldType::fromString("dirtPines"), BattlefieldType::DIRT_PINES);
	EXPECT_EQ(BattlefieldType::fromString("grassHills"), BattlefieldType::GRASS_HILLS);
	EXPECT_EQ(BattlefieldType::fromString("grassPines"), BattlefieldType::GRASS_PINES);
	EXPECT_EQ(BattlefieldType::fromString("rocklands"), BattlefieldType::ROCKLANDS);
	EXPECT_EQ(BattlefieldType::fromString("holyGround"), BattlefieldType::HOLY_GROUND);
	EXPECT_EQ(BattlefieldType::fromString("evilFog"), BattlefieldType::EVIL_FOG);
	EXPECT_EQ(BattlefieldType::fromString("favorableWinds"), BattlefieldType::FAVORABLE_WINDS);
	EXPECT_EQ(BattlefieldType::fromString("rough"), BattlefieldType::ROUGH);
	EXPECT_EQ(BattlefieldType::fromString("ship"), BattlefieldType::SHIP);
	EXPECT_EQ(BattlefieldType::fromString("shipToShip"), BattlefieldType::SHIP_TO_SHIP);
}
