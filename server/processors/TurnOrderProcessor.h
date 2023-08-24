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
	void doEndPlayerTurn(PlayerColor which);

	bool playerAwaitsTurn(PlayerColor which) const;
	bool playerMakingTurn(PlayerColor which) const;
	bool playerAwaitsNewDay(PlayerColor which) const;

public:
	TurnOrderProcessor(CGameHandler * owner);

	/// Add new player to handle (e.g. on game start)
	void addPlayer(PlayerColor which);

	/// NetPack call-in
	bool onPlayerEndsTurn(PlayerColor which);

	/// Ends player turn and removes this player from turn order
	void onPlayerEndsGame(PlayerColor which);

	/// Start game (or resume from save) and send YourTurn pack to player(s)
	void onGameStarted();

	template<typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & awaitingPlayers;
		h & actingPlayers;
		h & actedPlayers;
	}
};
