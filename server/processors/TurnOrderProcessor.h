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

	struct PlayerPair
	{
		PlayerColor a;
		PlayerColor b;

		bool operator == (const PlayerPair & other) const
		{
			return (a == other.a && b == other.b) || (a == other.b && b == other.a);
		}

		template<typename Handler>
		void serialize(Handler & h)
		{
			h & a;
			h & b;
		}
	};

	std::vector<PlayerPair> blockedContacts;

	std::set<PlayerColor> awaitingPlayers;
	std::set<PlayerColor> actingPlayers;
	std::set<PlayerColor> actedPlayers;

	std::optional<int> simturnsMinDurationDays;
	std::optional<int> simturnsMaxDurationDays;

	/// Returns date on which simturns must end unconditionally
	int simturnsTurnsMaxLimit() const;

	/// Returns date until which simturns must play unconditionally
	int simturnsTurnsMinLimit() const;

	/// Returns true if players are close enough to each other for their heroes to meet on this turn
	bool playersInContact(PlayerColor left, PlayerColor right) const;

	/// Returns true if waiting player can act alongside with currently acting player
	bool computeCanActSimultaneously(PlayerColor active, PlayerColor waiting) const;

	/// Returns true if left player must act before right player
	bool mustActBefore(PlayerColor left, PlayerColor right) const;

	/// Returns true if player is ready to start turn
	bool canStartTurn(PlayerColor which) const;

	/// Starts turn for all players that can start turn
	void tryStartTurnsForPlayers();

	void updateAndNotifyContactStatus();

	std::vector<PlayerPair> computeContactStatus() const;

	void doStartNewDay();
	void doStartPlayerTurn(PlayerColor which);
	void doEndPlayerTurn(PlayerColor which);

	bool isPlayerAwaitsTurn(PlayerColor which) const;
	bool isPlayerAwaitsNewDay(PlayerColor which) const;

public:
	TurnOrderProcessor(CGameHandler * owner);

	bool isContactAllowed(PlayerColor left, PlayerColor right) const;
	bool isPlayerMakingTurn(PlayerColor which) const;

	/// Add new player to handle (e.g. on game start)
	void addPlayer(PlayerColor which);

	/// NetPack call-in
	bool onPlayerEndsTurn(PlayerColor which);

	/// Ends player turn and removes this player from turn order
	void removePlayer(PlayerColor which);

	/// Start turns for next players if possible
	void resumeTurnOrder();

	/// Start game (or resume from save) and send PlayerStartsTurn pack to player(s)
	void onGameStarted();

	/// Permanently override duration of contactless simultaneous turns
	void setMinSimturnsDuration(int days);

	/// Permanently override duration of simultaneous turns with contact detection
	void setMaxSimturnsDuration(int days);

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & blockedContacts;
		h & awaitingPlayers;
		h & actingPlayers;
		h & actedPlayers;
		h & simturnsMinDurationDays;
		h & simturnsMaxDurationDays;
	}
};
