/*
 * TurnOrderProcessor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/GameConstants.h"

class CGameHandler;

enum class PlayerTurnEndReason
{
	CLIENT_REQUEST, // client requested end of turn (e.g. press End Turn button)
	TURN_TIMEOUT, // Player's turn timer has run out
	GAME_END // Player have won or lost the game
};

class TurnOrderProcessor : boost::noncopyable
{
	CGameHandler * gameHandler;

	std::set<PlayerColor> awaitingPlayers;
	std::set<PlayerColor> actingPlayers;
	std::set<PlayerColor> actedPlayers;

	/// Returns true if waiting player can act alongside with currently acting player
	bool canActSimultaneously(PlayerColor active, PlayerColor waiting) const;

	/// Returns true if left player must act before right player
	bool mustActBefore(PlayerColor left, PlayerColor right) const;

	/// Returns true if player is ready to start turn
	bool canStartTurn(PlayerColor which) const;

	/// Starts turn for all players that can start turn
	void tryStartTurnsForPlayers();

	void doStartNewDay();
	void doStartPlayerTurn(PlayerColor which);
	void doEndPlayerTurn(PlayerColor which, PlayerTurnEndReason reason);

public:
	TurnOrderProcessor(CGameHandler * owner);

	/// Add new player to handle (e.g. on game start)
	void addPlayer(PlayerColor which);

	/// NetPack call-in
	bool onPlayerEndsTurn(PlayerColor which, PlayerTurnEndReason reason);

	/// Start game (or resume from save) and send YourTurn pack to player(s)
	void onGameStarted();

	/// Returns true if player turn has not started today
	bool playerAwaitsTurn(PlayerColor which) const;

	/// Returns true if player is currently making his turn
	bool playerMakingTurn(PlayerColor which) const;

	/// Returns true if player has finished his turn and is waiting for new day
	bool playerAwaitsNewDay(PlayerColor which) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & awaitingPlayers;
		h & actingPlayers;
		h & actedPlayers;
	}
};
