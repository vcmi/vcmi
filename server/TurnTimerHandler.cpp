/*
 * TurnTimerHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "TurnTimerHandler.h"
#include "CGameHandler.h"
#include "battles/BattleProcessor.h"
#include "queries/QueriesProcessor.h"
#include "processors/TurnOrderProcessor.h"
#include "../lib/battle/BattleInfo.h"
#include "../lib/gameState/CGameState.h"
#include "../lib/networkPacks/PacksForClient.h"
#include "../lib/networkPacks/PacksForClientBattle.h"
#include "../lib/CPlayerState.h"
#include "../lib/CStack.h"
#include "../lib/StartInfo.h"

TurnTimerHandler::TurnTimerHandler(CGameHandler & gh):
	gameHandler(gh)
{
	
}

void TurnTimerHandler::onGameplayStart(PlayerColor player)
{
	if(const auto * si = gameHandler.getStartInfo())
	{
		timers[player] = si->turnTimerInfo;
		timers[player].turnTimer = 0;
		timers[player].battleTimer = 0;
		timers[player].unitTimer = 0;
		timers[player].isActive = true;
		timers[player].isBattle = false;
		lastUpdate[player] = std::numeric_limits<int>::max();
		endTurnAllowed[player] = true;
	}
}

void TurnTimerHandler::setTimerEnabled(PlayerColor player, bool enabled)
{
	assert(player.isValidPlayer());
	timers[player].isActive = enabled;
	sendTimerUpdate(player);
}

void TurnTimerHandler::setEndTurnAllowed(PlayerColor player, bool enabled)
{
	assert(player.isValidPlayer());
	endTurnAllowed[player] = enabled;
}

void TurnTimerHandler::sendTimerUpdate(PlayerColor player)
{
	TurnTimeUpdate ttu;
	ttu.player = player;
	ttu.turnTimer = timers[player];
	gameHandler.sendAndApply(ttu);
	lastUpdate[player] = 0;
}

void TurnTimerHandler::onPlayerGetTurn(PlayerColor player)
{
	if(const auto * si = gameHandler.getStartInfo())
	{
		if(si->turnTimerInfo.isEnabled())
		{
			endTurnAllowed[player] = true;
			auto & timer = timers[player];
			if(si->turnTimerInfo.accumulatingTurnTimer)
				timer.baseTimer += timer.turnTimer;
			timer.turnTimer = si->turnTimerInfo.turnTimer;
			
			sendTimerUpdate(player);
		}
	}
}

void TurnTimerHandler::prolongTimers(int durationMs)
{
	for (auto & timer : timers)
		timer.second.baseTimer += durationMs;
}

void TurnTimerHandler::update(int waitTimeMs)
{
	if(!gameHandler.getStartInfo()->turnTimerInfo.isEnabled())
		return;

	for(PlayerColor player(0); player < PlayerColor::PLAYER_LIMIT; ++player)
		if(gameHandler.gameState().isPlayerMakingTurn(player))
			onPlayerMakingTurn(player, waitTimeMs);

	// create copy for iterations - battle might end during onBattleLoop call
	std::vector<BattleID> ongoingBattles;

	for (auto & battle : gameHandler.gameState().currentBattles)
		ongoingBattles.push_back(battle->battleID);

	for (auto & battleID : ongoingBattles)
		onBattleLoop(battleID, waitTimeMs);
}

bool TurnTimerHandler::timerCountDown(int & timer, int initialTimer, PlayerColor player, int waitTime)
{
	if(timer > 0)
	{
		timer -= waitTime;
		lastUpdate[player] += waitTime;
		
		if(lastUpdate[player] >= turnTimePropagateFrequency)
			sendTimerUpdate(player);

		return true;
	}
	return false;
}

void TurnTimerHandler::onPlayerMakingTurn(PlayerColor player, int waitTime)
{
	const auto * si = gameHandler.getStartInfo();
	if(!si || !si->turnTimerInfo.isEnabled())
		return;
	
	auto & timer = timers[player];
	const auto * state = gameHandler.getPlayerState(player);
	if(state && state->human && timer.isActive && !timer.isBattle && state->status == EPlayerStatus::INGAME)
	{
		// turn timers are only used if turn timer is non-zero
		if (si->turnTimerInfo.turnTimer == 0)
			return;

		if(timerCountDown(timer.turnTimer, si->turnTimerInfo.turnTimer, player, waitTime))
			return;

		if(timerCountDown(timer.baseTimer, si->turnTimerInfo.baseTimer, player, waitTime))
			return;

		if(endTurnAllowed[state->color] && !gameHandler.queries->topQuery(state->color)) //wait for replies to avoid pending queries
			gameHandler.turnOrder->onPlayerEndsTurn(state->color);
	}
}

bool TurnTimerHandler::isPvpBattle(const BattleID & battleID) const
{
	const auto & gs = gameHandler.gameState();
	auto attacker = gs.getBattle(battleID)->getSidePlayer(BattleSide::ATTACKER);
	auto defender = gs.getBattle(battleID)->getSidePlayer(BattleSide::DEFENDER);
	if(attacker.isValidPlayer() && defender.isValidPlayer())
	{
		const auto * attackerState = gameHandler.getPlayerState(attacker);
		const auto * defenderState = gameHandler.getPlayerState(defender);
		if(attackerState && defenderState && attackerState->human && defenderState->human)
			return true;
	}
	return false;
}

void TurnTimerHandler::onBattleStart(const BattleID & battleID)
{
	const auto & gs = gameHandler.gameState();
	const auto * si = gameHandler.getStartInfo();
	if(!si)
		return;

	auto attacker = gs.getBattle(battleID)->getSidePlayer(BattleSide::ATTACKER);
	auto defender = gs.getBattle(battleID)->getSidePlayer(BattleSide::DEFENDER);
	
	bool pvpBattle = isPvpBattle(battleID);
	
	for(auto i : {attacker, defender})
	{
		if(i.isValidPlayer())
		{
			auto & timer = timers[i];
			timer.isBattle = true;
			timer.isActive = si->turnTimerInfo.isBattleEnabled();
			timer.battleTimer = si->turnTimerInfo.battleTimer;
			timer.unitTimer = (pvpBattle ? si->turnTimerInfo.unitTimer : 0);
			
			sendTimerUpdate(i);
		}
	}
}

void TurnTimerHandler::onBattleEnd(const BattleID & battleID)
{
	const auto & gs = gameHandler.gameState();
	const auto * si = gameHandler.getStartInfo();
	if(!si)
	{
		assert(0);
		return;
	}

	if (!si->turnTimerInfo.isBattleEnabled())
		return;
	
	auto attacker = gs.getBattle(battleID)->getSidePlayer(BattleSide::ATTACKER);
	auto defender = gs.getBattle(battleID)->getSidePlayer(BattleSide::DEFENDER);
	
	for(auto i : {attacker, defender})
	{
		if(i.isValidPlayer())
		{
			auto & timer = timers[i];
			timer.isBattle = false;
			timer.isActive = true;
			sendTimerUpdate(i);
		}
	}
}

void TurnTimerHandler::onBattleNextStack(const BattleID & battleID, const CStack & stack)
{
	const auto & gs = gameHandler.gameState();
	const auto * si = gameHandler.getStartInfo();
	if(!si || !gs.getBattle(battleID))
	{
		assert(0);
		return;
	}
	
	if (!si->turnTimerInfo.isBattleEnabled())
		return;
	
	if(isPvpBattle(battleID))
	{
		auto player = stack.getOwner();
		
		auto & timer = timers[player];
		if(timer.accumulatingUnitTimer)
			timer.battleTimer += timer.unitTimer;
		timer.unitTimer = si->turnTimerInfo.unitTimer;
		
		sendTimerUpdate(player);
	}
}

void TurnTimerHandler::onBattleLoop(const BattleID & battleID, int waitTime)
{
	const auto & gs = gameHandler.gameState();
	const auto * si = gameHandler.getStartInfo();
	if(!si)
	{
		assert(0);
		return;
	}
	
	if (!si->turnTimerInfo.isBattleEnabled())
		return;

	BattleSide side = BattleSide::NONE;
	const CStack * stack = nullptr;
	bool isTactisPhase = gs.getBattle(battleID)->battleTacticDist() > 0;
	
	if(isTactisPhase)
		side = gs.getBattle(battleID)->battleGetTacticsSide();
	else
	{
		stack = gs.getBattle(battleID)->battleGetStackByID(gs.getBattle(battleID)->getActiveStackID());
		if(!stack || !stack->getOwner().isValidPlayer())
			return;
		side = stack->unitSide();
	}
	
	auto player = gs.getBattle(battleID)->getSidePlayer(side);
	if(!player.isValidPlayer())
		return;
	
	const auto * state = gameHandler.getPlayerState(player);
	assert(state && state->status == EPlayerStatus::INGAME);
	if(!state || state->status != EPlayerStatus::INGAME || !state->human)
		return;
	
	auto & timer = timers[player];
	if(timer.isActive && timer.isBattle)
	{
		// in pvp battles, timers are only used if unit timer is non-zero
		if(isPvpBattle(battleID) && si->turnTimerInfo.unitTimer == 0)
			return;

		if (timerCountDown(timer.unitTimer, si->turnTimerInfo.unitTimer, player, waitTime))
			return;

		if (timerCountDown(timer.battleTimer, si->turnTimerInfo.battleTimer, player, waitTime))
			return;

		if (timerCountDown(timer.turnTimer, si->turnTimerInfo.turnTimer, player, waitTime))
			return;

		if (timerCountDown(timer.baseTimer, si->turnTimerInfo.baseTimer, player, waitTime))
			return;

		if(isPvpBattle(battleID))
		{
			BattleAction doNothing;
			doNothing.side = side;
			if(isTactisPhase)
				doNothing.actionType = EActionType::END_TACTIC_PHASE;
			else
			{
				doNothing.actionType = EActionType::DEFEND;
				doNothing.stackNumber = stack->unitId();
			}
			gameHandler.battles->makePlayerBattleAction(battleID, player, doNothing);
		}
		else
		{
			// battle vs neutrals - no-op, let battle run till the end
			// once battle is over player turn will be over due to running out of timer on adventure map
		}
	}
}
