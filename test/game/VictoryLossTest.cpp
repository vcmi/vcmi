/*
 * VictoryLossTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include <gtest/gtest.h>

#include "../../lib/CPlayerState.h"
#include "../../lib/gameState/CGameState.h"

namespace
{

PlayerState & addPlayer(CGameState & gameState, PlayerColor color, TeamID team, bool human)
{
	auto & player = gameState.players.try_emplace(color, &gameState).first->second;
	player.color = color;
	player.team = team;
	player.human = human;
	player.status = EPlayerStatus::INGAME;
	return player;
}

}

TEST(VictoryLossTest, standardWinPrefersHumanOverAiAlly)
{
	CGameState gameState;
	const TeamID survivingTeam(0);
	const PlayerColor aiPlayer(1);
	const PlayerColor humanPlayer(3);

	addPlayer(gameState, aiPlayer, survivingTeam, false);
	addPlayer(gameState, humanPlayer, survivingTeam, true);

	EXPECT_EQ(gameState.checkForStandardWin(), humanPlayer);
}
