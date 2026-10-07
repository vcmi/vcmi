/*
 * GameHandlerTestClient.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "GameHandlerTestClient.h"

#include "../../lib/gameState/CGameState.h"
#include "../../lib/networkPacks/PacksForServer.h"
#include "../../lib/serializer/CMemorySerializer.h"

GameHandlerTestClient::GameHandlerTestClient(const std::shared_ptr<CGameState> & gameState, PlayerColor player)
	: server(gameState, player)
	, gameHandler(server, gameState)
{
	gameState->actingPlayers.insert(player);
}

std::optional<BattleAction> GameHandlerTestClient::makeSurrenderRetreatDecision(
	PlayerColor,
	const BattleID &,
	const BattleStateInfoForRetreat &)
{
	return std::nullopt;
}

int GameHandlerTestClient::sendRequest(const CPackForServer & request, PlayerColor player, bool)
{
	request.player = player;
	request.requestID = ++lastRequestID;
	auto serverRequest = CMemorySerializer::deepCopy(request);
	gameHandler.handleReceivedPack(GameConnectionID::FIRST_CONNECTION, *serverRequest);
	return lastRequestID;
}
