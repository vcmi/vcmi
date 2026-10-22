/*
 * PlayerFactionsTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../mock/TinyMapGameTest.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/entities/building/CBuilding.h"

/// Uses RoE map, whose format has no Conflux - same as factions added by mods or newer HotA versions
class PlayerFactionsTest : public TinyMapGameTest
{
protected:
	const PlayerColor player = PlayerColor(0);

	Services * gameServices() override { return LIBRARY; }

	const std::set<FactionID> & loadFactions(uint32_t mask)
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::ROE);
		builder.size(36, false).playerActive(player).playerFactions(player, mask);
		startWithMap(std::move(builder));
		return map()->players.at(player.getNum()).allowedFactions;
	}
};

TEST_F(PlayerFactionsTest, mostFactionsAllowedBansOnlyMissingOnes)
{
	const auto & factions = loadFactions(0xff & ~(1 << FactionID::CASTLE.getNum()));

	EXPECT_FALSE(factions.count(FactionID::CASTLE));
	EXPECT_TRUE(factions.count(FactionID::RAMPART));
	EXPECT_TRUE(factions.count(FactionID::CONFLUX)) << "faction unknown to map format must stay available";
}

TEST_F(PlayerFactionsTest, fewFactionsAllowedAreKeptAsIs)
{
	const auto & factions = loadFactions((1 << FactionID::CASTLE.getNum()) | (1 << FactionID::RAMPART.getNum()));

	EXPECT_EQ(factions, std::set<FactionID>({FactionID::CASTLE, FactionID::RAMPART}));
}

TEST_F(PlayerFactionsTest, randomTownNotAlignedWithPlayerAvoidsHisFaction)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::HOTA);
	builder.hotaVersion(3)
		.size(36, false)
		.playerActive(player)
		.playerFactions(player, 1 << FactionID::CASTLE.getNum())
		.randomTown({10, 10, 0}, PlayerColor::NEUTRAL, PlayerColor::PLAYER_LIMIT_I + player.getNum());
	startWithMap(std::move(builder));

	auto towns = findAll<CGTownInstance>();
	ASSERT_EQ(towns.size(), 1);
	EXPECT_EQ(towns.front()->alignmentToPlayer, player);
	EXPECT_TRUE(towns.front()->alignmentExcludesPlayer);
	EXPECT_NE(towns.front()->getFactionID(), FactionID::CASTLE);
}

TEST_F(PlayerFactionsTest, specialBuildingsOfTownFactionAreApplied)
{
	// Castle: raw 19 (Brotherhood of the Sword) built, raw 20 (Stables) banned; Rampart: raw 18 (Mystic Pond) built
	const std::vector<uint8_t> specialBuildings = {0, 1, 2, 0, 1, 0, 0, 0};

	TinyH3M::TinyH3MBuilder builder(EMapFormat::HOTA);
	builder.hotaVersion(5)
		.size(36, false)
		.playerActive(player)
		.playerFactions(player, 1 << FactionID::CASTLE.getNum())
		.randomTown({10, 10, 0}, player, 0xff, specialBuildings)
		.town({20, 10, 0}, FactionID::CASTLE, PlayerColor::NEUTRAL, specialBuildings);
	startWithMap(std::move(builder));

	auto towns = findAll<CGTownInstance>();
	ASSERT_EQ(towns.size(), 2);
	for(const auto * town : towns)
	{
		ASSERT_EQ(town->getFactionID(), FactionID::CASTLE);
		EXPECT_TRUE(town->hasBuilt(BuildingID::SPECIAL_3));
		EXPECT_TRUE(town->forbiddenBuildings.count(BuildingID::SPECIAL_2));
		EXPECT_FALSE(town->hasBuilt(BuildingID::SPECIAL_1)) << "building of another faction must not be applied";
	}
}
