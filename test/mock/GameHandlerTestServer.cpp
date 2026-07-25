/*
 * GameHandlerTestServer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "GameHandlerTestServer.h"

#include "../../lib/gameState/CGameState.h"

GameHandlerTestServer::GameHandlerTestServer(std::shared_ptr<CGameState> gameState)
	: gameState(std::move(gameState))
{
}

GameHandlerTestServer::GameHandlerTestServer(std::shared_ptr<CGameState> gameState, PlayerColor hostedPlayer)
	: state(EServerState::GAMEPLAY)
	, gameState(std::move(gameState))
	, hostedPlayer(hostedPlayer)
{
}

void GameHandlerTestServer::setState(EServerState value)
{
	state = value;
}

EServerState GameHandlerTestServer::getState() const
{
	return state;
}

bool GameHandlerTestServer::isPlayerHost(const PlayerColor & color) const
{
	return hostedPlayer == color;
}

bool GameHandlerTestServer::hasPlayerAt(PlayerColor player, GameConnectionID connectionID) const
{
	return hostedPlayer == player && connectionID == GameConnectionID::FIRST_CONNECTION;
}

bool GameHandlerTestServer::hasBothPlayersAtSameConnection(PlayerColor, PlayerColor) const
{
	return false;
}

void GameHandlerTestServer::applyPack(CPackForClient & pack)
{
	if(gameState)
		gameState->apply(pack);
}

void GameHandlerTestServer::sendPack(CPackForClient &, GameConnectionID)
{
	// Test state changes are applied through applyPack.
}
