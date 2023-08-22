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
		if (mustActBefore(player, which))
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
}

void TurnOrderProcessor::doStartPlayerTurn(PlayerColor which)
{
	//if player runs out of time, he shouldn't get the turn (especially AI)
	//pre-trigger may change anything, should check before each player
	//TODO: is it enough to check only one player?
	gameHandler->checkVictoryLossConditionsForAll();

	assert(gameHandler->getPlayerState(which));
	assert(gameHandler->getPlayerState(which)->status == EPlayerStatus::INGAME);

	gameHandler->onPlayerTurnStarted(which);

	YourTurn yt;
	yt.player = which;
	//Change local daysWithoutCastle counter for local interface message //TODO: needed?
	yt.daysWithoutCastle = gameHandler->getPlayerState(which)->daysWithoutCastle;
	gameHandler->sendAndApply(&yt);
}

void TurnOrderProcessor::doEndPlayerTurn(PlayerColor which)
{
	assert(playerMakingTurn(which));

	actingPlayers.erase(which);
	actedPlayers.insert(which);

	if (!awaitingPlayers.empty())
		tryStartTurnsForPlayers();

	if (actingPlayers.empty())
		doStartNewDay();
}

void TurnOrderProcessor::addPlayer(PlayerColor which)
{
	awaitingPlayers.insert(which);
}

bool TurnOrderProcessor::onPlayerEndsTurn(PlayerColor which)
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

	doEndPlayerTurn(which);
	return true;
}

void TurnOrderProcessor::onGameStarted()
{
	tryStartTurnsForPlayers();

	// this may be game load - send notification to players that they can act
	auto actingPlayersCopy = actingPlayers;
	for (auto player : actingPlayersCopy)
		doStartPlayerTurn(player);
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
