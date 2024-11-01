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
#include "PlayerMessageProcessor.h"

#include "../queries/QueriesProcessor.h"
#include "../queries/MapQueries.h"
#include "../CGameHandler.h"
#include "../CVCMIServer.h"

#include "../../lib/CPlayerState.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapObjects/CGObjectInstance.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/pathfinder/CPathfinder.h"
#include "../../lib/pathfinder/PathfinderOptions.h"

TurnOrderProcessor::TurnOrderProcessor(CGameHandler * owner):
	gameHandler(owner)
{

}

int TurnOrderProcessor::simturnsTurnsMaxLimit() const
{
	if (simturnsMaxDurationDays)
		return *simturnsMaxDurationDays;
	return gameHandler->getStartInfo()->simturnsInfo.optionalTurns;
}

int TurnOrderProcessor::simturnsTurnsMinLimit() const
{
	if (simturnsMinDurationDays)
		return *simturnsMinDurationDays;
	return gameHandler->getStartInfo()->simturnsInfo.requiredTurns;
}

std::vector<TurnOrderProcessor::PlayerPair> TurnOrderProcessor::computeContactStatus() const
{
	std::vector<PlayerPair> result;

	assert(actedPlayers.empty());
	assert(actingPlayers.empty());

	for (auto left : awaitingPlayers)
	{
		for(auto right : awaitingPlayers)
		{
			if (left == right)
				continue;

			if (computeCanActSimultaneously(left, right))
				result.push_back({left, right});
		}
	}
	return result;
}

void TurnOrderProcessor::updateAndNotifyContactStatus()
{
	auto newBlockedContacts = computeContactStatus();

	if (newBlockedContacts.empty())
	{
		// Simturns between all players have ended - send single global notification
		if (!blockedContacts.empty())
			gameHandler->playerMessages->broadcastSystemMessage("Simultaneous turns have ended");
	}
	else
	{
		// Simturns between some players have ended - notify each pair
		for (auto const & contact : blockedContacts)
		{
			if (vstd::contains(newBlockedContacts, contact))
				continue;

			MetaString message;
			message.appendRawString("Simultaneous turns between players %s and %s have ended"); // FIXME: we should send MetaString itself and localize it on client side
			message.replaceName(contact.a);
			message.replaceName(contact.b);

			gameHandler->playerMessages->broadcastSystemMessage(message.toString());
		}
	}

	blockedContacts = newBlockedContacts;
}

bool TurnOrderProcessor::playersInContact(PlayerColor left, PlayerColor right) const
{
	// TODO: refactor, cleanup and optimize

	boost::multi_array<bool, 3> leftReachability;
	boost::multi_array<bool, 3> rightReachability;

	int3 mapSize = gameHandler->getMapSize();

	leftReachability.resize(boost::extents[mapSize.z][mapSize.x][mapSize.y]);
	rightReachability.resize(boost::extents[mapSize.z][mapSize.x][mapSize.y]);

	const auto * leftInfo = gameHandler->getPlayerState(left, false);
	const auto * rightInfo = gameHandler->getPlayerState(right, false);

	for (auto obj : gameHandler->gameState()->map->objects)
	{
		if (obj && obj->isVisitable())
		{
			int3 pos = obj->visitablePos();
			if (obj->tempOwner == left)
				leftReachability[pos.z][pos.x][pos.y] = true;
			if (obj->tempOwner == right)
				rightReachability[pos.z][pos.x][pos.y] = true;
		}
	}

	for(const auto & hero : leftInfo->getHeroes())
	{
		CPathsInfo out(mapSize, hero);
		auto config = std::make_shared<SingleHeroPathfinderConfig>(out, gameHandler->gameState(), hero);
		config->options.ignoreGuards = true;
		config->options.turnLimit = 1;
		CPathfinder pathfinder(gameHandler->gameState(), config);
		pathfinder.calculatePaths();

		for (int z = 0; z < mapSize.z; ++z)
			for (int y = 0; y < mapSize.y; ++y)
				for (int x = 0; x < mapSize.x; ++x)
					if (out.getNode({x,y,z})->reachable())
						leftReachability[z][x][y] = true;
	}

	for(const auto & hero : rightInfo->getHeroes())
	{
		CPathsInfo out(mapSize, hero);
		auto config = std::make_shared<SingleHeroPathfinderConfig>(out, gameHandler->gameState(), hero);
		config->options.ignoreGuards = true;
		config->options.turnLimit = 1;
		CPathfinder pathfinder(gameHandler->gameState(), config);
		pathfinder.calculatePaths();

		for (int z = 0; z < mapSize.z; ++z)
			for (int y = 0; y < mapSize.y; ++y)
				for (int x = 0; x < mapSize.x; ++x)
					if (out.getNode({x,y,z})->reachable())
						rightReachability[z][x][y] = true;
	}

	for (int z = 0; z < mapSize.z; ++z)
		for (int y = 0; y < mapSize.y; ++y)
			for (int x = 0; x < mapSize.x; ++x)
				if (leftReachability[z][x][y] && rightReachability[z][x][y])
					return true;

	return false;
}

bool TurnOrderProcessor::isContactAllowed(PlayerColor active, PlayerColor waiting) const
{
	assert(active != waiting);
	return !vstd::contains(blockedContacts, PlayerPair{active, waiting});
}

bool TurnOrderProcessor::computeCanActSimultaneously(PlayerColor active, PlayerColor waiting) const
{
	const auto * activeInfo = gameHandler->getPlayerState(active, false);
	const auto * waitingInfo = gameHandler->getPlayerState(waiting, false);

	assert(active != waiting);
	assert(activeInfo);
	assert(waitingInfo);

	if (activeInfo->human != waitingInfo->human)
	{
		// only one AI and one human can play simultaneously from single connection
		if (!gameHandler->getStartInfo()->simturnsInfo.allowHumanWithAI)
			return false;
	}
	else
	{
		// two AI or two humans in hotseat can't play at the same time
		if (gameHandler->hasBothPlayersAtSameConnection(active, waiting))
			return false;
	}

	if (gameHandler->getDate(Date::DAY) < simturnsTurnsMinLimit())
		return true;

	if (gameHandler->getDate(Date::DAY) > simturnsTurnsMaxLimit())
		return false;

	if (gameHandler->getStartInfo()->simturnsInfo.ignoreAlliedContacts && activeInfo->team == waitingInfo->team)
		return true;

	if (playersInContact(active, waiting))
		return false;

	return true;
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

	return false;
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
		if (player != which && isContactAllowed(player, which))
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
	{
		gameHandler->gameLobby()->setState(EServerState::SHUTDOWN);
		return;
	}

	std::swap(actedPlayers, awaitingPlayers);

	gameHandler->onNewTurn();
	updateAndNotifyContactStatus();
	tryStartTurnsForPlayers();
}

void TurnOrderProcessor::doStartPlayerTurn(PlayerColor which)
{
	assert(gameHandler->getPlayerState(which));
	assert(gameHandler->getPlayerState(which)->status == EPlayerStatus::INGAME);

	// Only if player is actually starting his turn (and not loading from save)
	if (!actingPlayers.count(which))
		gameHandler->onPlayerTurnStarted(which);

	actingPlayers.insert(which);
	awaitingPlayers.erase(which);

	auto turnQuery = std::make_shared<TimerPauseQuery>(gameHandler, which);
	gameHandler->queries->addQuery(turnQuery);

	PlayerStartsTurn pst;
	pst.player = which;
	pst.queryID = turnQuery->queryID;
	gameHandler->sendAndApply(pst);

	assert(!actingPlayers.empty());
}

void TurnOrderProcessor::doEndPlayerTurn(PlayerColor which)
{
	assert(isPlayerMakingTurn(which));
	assert(gameHandler->getPlayerStatus(which) == EPlayerStatus::INGAME);

	actingPlayers.erase(which);
	actedPlayers.insert(which);

	PlayerEndsTurn pet;
	pet.player = which;
	gameHandler->sendAndApply(pet);

	if (!awaitingPlayers.empty())
		tryStartTurnsForPlayers();

	if (actingPlayers.empty())
		doStartNewDay();

	assert(!actingPlayers.empty());
}

void TurnOrderProcessor::addPlayer(PlayerColor which)
{
	awaitingPlayers.insert(which);
}

void TurnOrderProcessor::onPlayerEndsGame(PlayerColor which)
{
	awaitingPlayers.erase(which);
	actingPlayers.erase(which);
	actedPlayers.erase(which);

	if (!awaitingPlayers.empty())
		tryStartTurnsForPlayers();

	if (actingPlayers.empty())
		doStartNewDay();
}

bool TurnOrderProcessor::onPlayerEndsTurn(PlayerColor which)
{
	if (!isPlayerMakingTurn(which))
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

	gameHandler->onPlayerTurnEnded(which);

	// it is possible that player have lost - e.g. spent 7 days without town
	// in this case - don't call doEndPlayerTurn - turn transfer was already handled by onPlayerEndsGame
	if(gameHandler->getPlayerStatus(which) == EPlayerStatus::INGAME)
		doEndPlayerTurn(which);

	return true;
}

void TurnOrderProcessor::onGameStarted()
{
	if (actingPlayers.empty())
		blockedContacts = computeContactStatus();

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

bool TurnOrderProcessor::isPlayerAwaitsTurn(PlayerColor which) const
{
	return vstd::contains(awaitingPlayers, which);
}

bool TurnOrderProcessor::isPlayerMakingTurn(PlayerColor which) const
{
	return vstd::contains(actingPlayers, which);
}

bool TurnOrderProcessor::isPlayerAwaitsNewDay(PlayerColor which) const
{
	return vstd::contains(actedPlayers, which);
}

void TurnOrderProcessor::setMinSimturnsDuration(int days)
{
	simturnsMinDurationDays = gameHandler->getDate(Date::DAY) + days;
}

void TurnOrderProcessor::setMaxSimturnsDuration(int days)
{
	simturnsMaxDurationDays = gameHandler->getDate(Date::DAY) + days;
}
