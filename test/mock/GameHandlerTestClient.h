/*
 * GameHandlerTestClient.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#pragma once

#include "GameHandlerTestServer.h"

#include "../../server/CGameHandler.h"
#include "../../lib/callback/IClient.h"

/// Client that delivers requests of one acting player directly to an in-process CGameHandler
class GameHandlerTestClient : public IClient
{
public:
	GameHandlerTestClient(const std::shared_ptr<CGameState> & gameState, PlayerColor player);

	std::optional<BattleAction> makeSurrenderRetreatDecision(
		PlayerColor,
		const BattleID &,
		const BattleStateInfoForRetreat &) override;

	int sendRequest(const CPackForServer & request, PlayerColor player, bool) override;

private:
	GameHandlerTestServer server;
	CGameHandler gameHandler;
	int lastRequestID = 0;
};
