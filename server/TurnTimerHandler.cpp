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
#include "../lib/CPlayerState.h"
#include "../lib/CStack.h"
#include "../lib/StartInfo.h"
#include "../lib/NetPacks.h"

TurnTimerHandler::TurnTimerHandler(CGameHandler & gh):
	gameHandler(gh)
{
	
}

void TurnTimerHandler::onGameplayStart(PlayerColor player)
{
	std::lock_guard<std::recursive_mutex> guard(mx);
	if(const auto * si = gameHandler.getStartInfo())
	{
		timers[player] = si->turnTimerInfo;
		timers[player].turnTimer = 0;
		timers[player].isActive = true;
		timers[player].isBattle = false;
		lastUpdate[player] = std::numeric_limits<int>::max();
		endTurnAllowed[player] = true;
	}
}

void TurnTimerHandler::setTimerEnabled(PlayerColor player, bool enabled)
{
	std::lock_guard<std::recursive_mutex> guard(mx);
	assert(player.isValidPlayer());
	timers[player].isActive = enabled;
	sendTimerUpdate(player);
}

void TurnTimerHandler::setEndTurnAllowed(PlayerColor player, bool enabled)
{
	std::lock_guard<std::recursive_mutex> guard(mx);
	assert(player.isValidPlayer());
	endTurnAllowed[player] = enabled;
}

void TurnTimerHandler::sendTimerUpdate(PlayerColor player)
{
	TurnTimeUpdate ttu;
	ttu.player = player;
	ttu.turnTimer = timers[player];
	gameHandler.sendAndApply(&ttu);
	lastUpdate[player] = 0;
}

void TurnTimerHandler::onPlayerGetTurn(PlayerColor player)
{
	std::lock_guard<std::recursive_mutex> guard(mx);
	if(const auto * si = gameHandler.getStartInfo())
	{
		if(si->turnTimerInfo.isEnabled())
		{
			endTurnAllowed[player] = true;
			auto & timer = timers[player];
			if(si->turnTimerInfo.baseTimer > 0)
				timer.baseTimer += timer.turnTimer;
			timer.turnTimer = si->turnTimerInfo.turnTimer;
			
			sendTimerUpdate(player);
		}
	}
}

void TurnTimerHandler::update(int waitTime)
{
	std::lock_guard<std::recursive_mutex> guard(mx);
	if(const auto * gs = gameHandler.gameState())
	{
		for(PlayerColor player(0); player < PlayerColor::PLAYER_LIMIT; ++player)
			if(gs->isPlayerMakingTurn(player))
				onPlayerMakingTurn(player, waitTime);
		
		for (auto & battle : gs->currentBattles)
			onBattleLoop(battle->battleID, waitTime);
	}
}

bool TurnTimerHandler::timerCountDown(int & timer, int initialTimer, PlayerColor player, int waitTime)
{
	if(timer > 0)
	{
		timer -= waitTime;
		lastUpdate[player] += waitTime;
		int frequency = (timer > turnTimePropagateThreshold
						 && initialTimer - timer > turnTimePropagateThreshold)
		? turnTimePropagateFrequency : turnTimePropagateFrequencyCrit;
		
		if(lastUpdate[player] >= frequency)
			sendTimerUpdate(player);

		return true;
	}
	return false;
}

void TurnTimerHandler::onPlayerMakingTurn(PlayerColor player, int waitTime)
{
	std::lock_guard<std::recursive_mutex> guard(mx);
	const auto * gs = gameHandler.gameState();
	const auto * si = gameHandler.getStartInfo();
	if(!si || !gs || !si->turnTimerInfo.isEnabled())
		return;
	
	auto & timer = timers[player];
	const auto * state = gameHandler.getPlayerState(player);
	if(state && state->human && timer.isActive && !timer.isBattle && state->status == EPlayerStatus::INGAME)
	{
		if(!timerCountDown(timer.turnTimer, si->turnTimerInfo.turnTimer, player, waitTime))
		{
			if(timer.baseTimer > 0)
			{
				timer.turnTimer = timer.baseTimer;
				timer.baseTimer = 0;
				onPlayerMakingTurn(player, 0);
			}
			else if(endTurnAllowed[state->color] && !gameHandler.queries->topQuery(state->color)) //wait for replies to avoid pending queries
				gameHandler.turnOrder->onPlayerEndsTurn(state->color);
		}
	}
}

bool TurnTimerHandler::isPvpBattle(const BattleID & battleID) const
{
	const auto * gs = gameHandler.gameState();
	auto attacker = gs->getBattle(battleID)->getSidePlayer(BattleSide::ATTACKER);
	auto defender = gs->getBattle(battleID)->getSidePlayer(BattleSide::DEFENDER);
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
	std::lock_guard<std::recursive_mutex> guard(mx);
	const auto * gs = gameHandler.gameState();
	const auto * si = gameHandler.getStartInfo();
	if(!si || !gs)
		return;

	auto attacker = gs->getBattle(battleID)->getSidePlayer(BattleSide::ATTACKER);
	auto defender = gs->getBattle(battleID)->getSidePlayer(BattleSide::DEFENDER);
	
	bool pvpBattle = isPvpBattle(battleID);
	
	for(auto i : {attacker, defender})
	{
		if(i.isValidPlayer())
		{
			auto & timer = timers[i];
			timer.isBattle = true;
			timer.isActive = si->turnTimerInfo.isBattleEnabled();
			timer.battleTimer = (pvpBattle ? si->turnTimerInfo.battleTimer : 0);
			timer.creatureTimer = (pvpBattle ? si->turnTimerInfo.creatureTimer : si->turnTimerInfo.battleTimer);
			
			sendTimerUpdate(i);
		}
	}
}

void TurnTimerHandler::onBattleEnd(const BattleID & battleID)
{
	std::lock_guard<std::recursive_mutex> guard(mx);
	const auto * gs = gameHandler.gameState();
	const auto * si = gameHandler.getStartInfo();
	if(!si || !gs)
	{
		assert(0);
		return;
	}

	if (!si->turnTimerInfo.isBattleEnabled())
		return;
	
	auto attacker = gs->getBattle(battleID)->getSidePlayer(BattleSide::ATTACKER);
	auto defender = gs->getBattle(battleID)->getSidePlayer(BattleSide::DEFENDER);
	
	bool pvpBattle = isPvpBattle(battleID);
	
	for(auto i : {attacker, defender})
	{
		if(i.isValidPlayer())
		{
			auto & timer = timers[i];
			timer.isBattle = false;
			timer.isActive = true;
			if(!pvpBattle)
			{
				if(si->turnTimerInfo.baseTimer && timer.baseTimer == 0)
					timer.baseTimer = timer.creatureTimer;
				else if(si->turnTimerInfo.turnTimer && timer.turnTimer == 0)
					timer.turnTimer = timer.creatureTimer;
			}

			sendTimerUpdate(i);
		}
	}
}

void TurnTimerHandler::onBattleNextStack(const BattleID & battleID, const CStack & stack)
{
	std::lock_guard<std::recursive_mutex> guard(mx);
	const auto * gs = gameHandler.gameState();
	const auto * si = gameHandler.getStartInfo();
	if(!si || !gs || !gs->getBattle(battleID))
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
		if(timer.battleTimer == 0)
			timer.battleTimer = timer.creatureTimer;
		timer.creatureTimer = si->turnTimerInfo.creatureTimer;
		
		sendTimerUpdate(player);
	}
}

void TurnTimerHandler::onBattleLoop(const BattleID & battleID, int waitTime)
{
	std::lock_guard<std::recursive_mutex> guard(mx);
	const auto * gs = gameHandler.gameState();
	const auto * si = gameHandler.getStartInfo();
	if(!si || !gs || !si->turnTimerInfo.isBattleEnabled())
	{
		assert(0);
		return;
	}
	
	ui8 side = 0;
	const CStack * stack = nullptr;
	bool isTactisPhase = gs->getBattle(battleID)->battleTacticDist() > 0;
	
	if(isTactisPhase)
		side = gs->getBattle(battleID)->battleGetTacticsSide();
	else
	{
		stack = gs->getBattle(battleID)->battleGetStackByID(gs->getBattle(battleID)->getActiveStackID());
		if(!stack || !stack->getOwner().isValidPlayer())
			return;
		side = stack->unitSide();
	}
	
	auto player = gs->getBattle(battleID)->getSidePlayer(side);
	if(!player.isValidPlayer())
		return;
	
	const auto * state = gameHandler.getPlayerState(player);
	if(!state || state->status != EPlayerStatus::INGAME || !state->human)
		return;
	
	auto & timer = timers[player];
	if(timer.isActive && timer.isBattle && !timerCountDown(timer.creatureTimer, si->turnTimerInfo.creatureTimer, player, waitTime))
	{
		if(isPvpBattle(battleID))
		{
			if(timer.battleTimer > 0)
			{
				timer.creatureTimer = timer.battleTimer;
				timerCountDown(timer.creatureTimer, timer.battleTimer, player, 0);
				timer.battleTimer = 0;
			}
			else
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
		}
		else
		{
			if(timer.turnTimer > 0)
			{
				timer.creatureTimer = timer.turnTimer;
				timerCountDown(timer.creatureTimer, timer.turnTimer, player, 0);
				timer.turnTimer = 0;
			}
			else if(timer.baseTimer > 0)
			{
				timer.creatureTimer = timer.baseTimer;
				timerCountDown(timer.creatureTimer, timer.baseTimer, player, 0);
				timer.baseTimer = 0;
			}
			else
			{
				BattleAction retreat;
				retreat.side = side;
				retreat.actionType = EActionType::RETREAT; //harsh punishment
				gameHandler.battles->makePlayerBattleAction(battleID, player, retreat);
			}
		}
	}
}
