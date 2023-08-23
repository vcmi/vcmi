/*
 * TurnOrderProcessor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "TurnOrderProcessor.h"

#include "../queries/QueriesProcessor.h"
#include "../CGameHandler.h"
#include "../CVCMIServer.h"

#include "../../lib/CPlayerState.h"
#include "../../lib/NetPacks.h"

TurnOrderProcessor::TurnOrderProcessor(CGameHandler * owner):
	gameHandler(owner)
{

}

bool TurnOrderProcessor::canActSimultaneously(PlayerColor active, PlayerColor waiting) const
{
	return false;
}

bool TurnOrderProcessor::mustActBefore(PlayerColor left, PlayerColor right) const
{
	const auto * leftInfo = gameHandler->getPlayerState(left, false);
	const auto * rightInfo = gameHandler->getPlayerState(right, false);

	assert(left != right);
	assert(leftInfo && rightInfo);

	if (!leftInfo)
		return false;
	if (!rightInfo)
		return true;

	if (leftInfo->isHuman() && !rightInfo->isHuman())
		return true;

	if (!leftInfo->isHuman() && rightInfo->isHuman())
		return false;

	return left < right;
}

bool TurnOrderProcessor::canStartTurn(PlayerColor which) const
{
	for (auto player : awaitingPlayers)
	{
		if (player != which && mustActBefore(player, which))
			return false;
	}

	for (auto player : actingPlayers)
	{
		if (!canActSimultaneously(player, which))
			return false;
	}

	return true;
}

void TurnOrderProcessor::doStartNewDay()
{
	assert(awaitingPlayers.empty());
	assert(actingPlayers.empty());

	bool activePlayer = false;
	for (auto player : actedPlayers)
	{
		if (gameHandler->getPlayerState(player)->status == EPlayerStatus::INGAME)
			activePlayer = true;
	}

	if(!activePlayer)
		gameHandler->gameLobby()->setState(EServerState::GAMEPLAY_ENDED);

	std::swap(actedPlayers, awaitingPlayers);

	gameHandler->onNewTurn();
	tryStartTurnsForPlayers();
}

void TurnOrderProcessor::doStartPlayerTurn(PlayerColor which)
{
	assert(gameHandler->getPlayerState(which));
	assert(gameHandler->getPlayerState(which)->status == EPlayerStatus::INGAME);

	//Note: on game load, "actingPlayer" might already contain list of players
	actingPlayers.insert(which);
	awaitingPlayers.erase(which);
	gameHandler->onPlayerTurnStarted(which);

	YourTurn yt;
	yt.player = which;
	//Change local daysWithoutCastle counter for local interface message //TODO: needed?
	yt.daysWithoutCastle = gameHandler->getPlayerState(which)->daysWithoutCastle;
	gameHandler->sendAndApply(&yt);

	assert(actingPlayers.size() == 1); // No simturns yet :(
	assert(gameHandler->getCurrentPlayer() == *actingPlayers.begin());
}

void TurnOrderProcessor::doEndPlayerTurn(PlayerColor which, PlayerTurnEndReason reason)
{
	assert(playerMakingTurn(which));

	actingPlayers.erase(which);
	if (reason != PlayerTurnEndReason::GAME_END)
		actedPlayers.insert(which);

	if (!awaitingPlayers.empty())
		tryStartTurnsForPlayers();

	if (actingPlayers.empty())
		doStartNewDay();

	assert(!actingPlayers.empty());
	assert(actingPlayers.size() == 1); // No simturns yet :(
	assert(gameHandler->getCurrentPlayer() == *actingPlayers.begin());
}

void TurnOrderProcessor::addPlayer(PlayerColor which)
{
	awaitingPlayers.insert(which);
}

bool TurnOrderProcessor::onPlayerEndsTurn(PlayerColor which, PlayerTurnEndReason reason)
{
	if (!playerMakingTurn(which))
	{
		gameHandler->complain("Can not end turn for player that is not acting!");
		return false;
	}

	if(gameHandler->getPlayerStatus(which) != EPlayerStatus::INGAME)
	{
		gameHandler->complain("Can not end turn for player that is not in game!");
		return false;
	}

	if(gameHandler->queries->topQuery(which) != nullptr)
	{
		gameHandler->complain("Cannot end turn before resolving queries!");
		return false;
	}

	if (reason != PlayerTurnEndReason::GAME_END)
		gameHandler->onPlayerTurnEnded(which);

	doEndPlayerTurn(which, reason);

	return true;
}

void TurnOrderProcessor::onGameStarted()
{
	// this may be game load - send notification to players that they can act
	auto actingPlayersCopy = actingPlayers;
	for (auto player : actingPlayersCopy)
		doStartPlayerTurn(player);

	tryStartTurnsForPlayers();
}

void TurnOrderProcessor::tryStartTurnsForPlayers()
{
	auto awaitingPlayersCopy = awaitingPlayers;
	for (auto player : awaitingPlayersCopy)
	{
		if (canStartTurn(player))
			doStartPlayerTurn(player);
	}
}

bool TurnOrderProcessor::playerAwaitsTurn(PlayerColor which) const
{
	return vstd::contains(awaitingPlayers, which);
}

bool TurnOrderProcessor::playerMakingTurn(PlayerColor which) const
{
	return vstd::contains(actingPlayers, which);
}

bool TurnOrderProcessor::playerAwaitsNewDay(PlayerColor which) const
{
	return vstd::contains(actedPlayers, which);
}
