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
