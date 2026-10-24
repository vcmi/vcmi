/*
 * NeutralTownEventsTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "CGameHandler.h"

#include "mock/GameHandlerTestServer.h"
#include "mock/TinyMapGameTest.h"

#include "lib/GameLibrary.h"
#include "lib/gameState/CGameState.h"
#include "lib/mapObjects/CGTownInstance.h"
#include "lib/mapping/CCastleEvent.h"

class NeutralTownEventsTest : public TinyMapGameTest
{
protected:
	Services * gameServices() override { return LIBRARY; }

	/// Starts a map with a neutral town that has an event building a mage guild on the next day
	CGTownInstance * startWithTownEvent(bool neutralAffected)
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder.size(36, false)
			.playerActive(PlayerColor(0))
			.town({10, 10, 0}, FactionID::CASTLE, PlayerColor::NEUTRAL);
		startWithMap(std::move(builder));

		auto * town = findAll<CGTownInstance>().front();
		CCastleEvent event;
		event.players.insert(PlayerColor(0));
		event.firstOccurrence = gameState()->day;
		event.buildings.insert(BuildingID::MAGES_GUILD_1);
		event.neutralAffected = neutralAffected;
		town->events.push_back(event);
		return town;
	}
};

TEST_F(NeutralTownEventsTest, eventAffectingNeutralTownsOccursOnNewDay)
{
	auto * town = startWithTownEvent(true);
	GameHandlerTestServer server(gameState());
	CGameHandler gameHandler(server, gameState());

	gameHandler.onNewTurn();
	EXPECT_TRUE(town->hasBuilt(BuildingID::MAGES_GUILD_1));
}

TEST_F(NeutralTownEventsTest, eventOfPlayersDoesNotAffectNeutralTown)
{
	auto * town = startWithTownEvent(false);
	GameHandlerTestServer server(gameState());
	CGameHandler gameHandler(server, gameState());

	gameHandler.onNewTurn();
	EXPECT_FALSE(town->hasBuilt(BuildingID::MAGES_GUILD_1));
}
