/*
 * GameHandlerTestServer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#pragma once

#include "../../server/IGameServer.h"
#include "../../lib/GameConstants.h"

#include <memory>
#include <optional>

class CGameState;

class GameHandlerTestServer : public IGameServer
{
public:
	explicit GameHandlerTestServer(std::shared_ptr<CGameState> gameState = nullptr);
	GameHandlerTestServer(std::shared_ptr<CGameState> gameState, PlayerColor hostedPlayer);

	void setState(EServerState value) override;
	EServerState getState() const override;
	bool isPlayerHost(const PlayerColor & color) const override;
	bool hasPlayerAt(PlayerColor player, GameConnectionID connectionID) const override;
	bool hasBothPlayersAtSameConnection(PlayerColor left, PlayerColor right) const override;
	void applyPack(CPackForClient & pack) override;
	void sendPack(CPackForClient & pack, GameConnectionID connectionID) override;

private:
	EServerState state = EServerState::LOBBY;
	std::shared_ptr<CGameState> gameState;
	std::optional<PlayerColor> hostedPlayer;
};
