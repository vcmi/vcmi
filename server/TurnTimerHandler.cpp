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
		timerInfo[player].timer = si->turnTimerInfo;
		timerInfo[player].timer.turnTimer = 0;
		timerInfo[player].isEnabled = true;
		timerInfo[player].isBattle = false;
		timerInfo[player].lastUpdate = std::numeric_limits<int>::max();
	}
}

void TurnTimerHandler::setTimerEnabled(PlayerColor player, bool enabled)
{
	std::lock_guard<std::recursive_mutex> guard(mx);
	assert(player.isValidPlayer());
	timerInfo[player].isEnabled = enabled;
}

void TurnTimerHandler::sendTimerUpdate(PlayerColor player)
{
	auto & info = timerInfo[player];
	TurnTimeUpdate ttu;
	ttu.player = player;
	ttu.turnTimer = info.timer;
	gameHandler.sendAndApply(&ttu);
	info.lastUpdate = 0;
}

void TurnTimerHandler::onPlayerGetTurn(PlayerColor player)
{
	std::lock_guard<std::recursive_mutex> guard(mx);
	if(const auto * si = gameHandler.getStartInfo())
	{
		if(si->turnTimerInfo.isEnabled())
		{
			auto & info = timerInfo[player];
			if(si->turnTimerInfo.baseTimer > 0)
				info.timer.baseTimer += info.timer.turnTimer;
			info.timer.turnTimer = si->turnTimerInfo.turnTimer;
			
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
		
		if(gs->curB)
			onBattleLoop(waitTime);
	}
}

bool TurnTimerHandler::timerCountDown(int & timer, int initialTimer, PlayerColor player, int waitTime)
{
	if(timer > 0)
	{
		auto & info = timerInfo[player];
		timer -= waitTime;
		info.lastUpdate += waitTime;
		int frequency = (timer > turnTimePropagateThreshold
						 && initialTimer - timer > turnTimePropagateThreshold)
		? turnTimePropagateFrequency : turnTimePropagateFrequencyCrit;
		
		if(info.lastUpdate >= frequency)
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
	
	auto & info = timerInfo[player];
	const auto * state = gameHandler.getPlayerState(player);
	if(state && state->human && info.isEnabled && !info.isBattle && state->status == EPlayerStatus::INGAME)
	{
		if(!timerCountDown(info.timer.turnTimer, si->turnTimerInfo.turnTimer, player, waitTime))
		{
			if(info.timer.baseTimer > 0)
			{
				info.timer.turnTimer = info.timer.baseTimer;
				info.timer.baseTimer = 0;
				onPlayerMakingTurn(player, 0);
			}
			else if(!gameHandler.queries->topQuery(state->color)) //wait for replies to avoid pending queries
				gameHandler.turnOrder->onPlayerEndsTurn(state->color);
		}
	}
}

bool TurnTimerHandler::isPvpBattle() const
{
	const auto * gs = gameHandler.gameState();
	auto attacker = gs->curB->getSidePlayer(BattleSide::ATTACKER);
	auto defender = gs->curB->getSidePlayer(BattleSide::DEFENDER);
	if(attacker.isValidPlayer() && defender.isValidPlayer())
	{
		const auto * attackerState = gameHandler.getPlayerState(attacker);
		const auto * defenderState = gameHandler.getPlayerState(defender);
		if(attackerState && defenderState && attackerState->human && defenderState->human)
			return true;
	}
	return false;
}

void TurnTimerHandler::onBattleStart()
{
	std::lock_guard<std::recursive_mutex> guard(mx);
	const auto * gs = gameHandler.gameState();
	const auto * si = gameHandler.getStartInfo();
	if(!si || !gs || !gs->curB || !si->turnTimerInfo.isBattleEnabled())
		return;

	auto attacker = gs->curB->getSidePlayer(BattleSide::ATTACKER);
	auto defender = gs->curB->getSidePlayer(BattleSide::DEFENDER);
	
	bool pvpBattle = isPvpBattle();
	
	for(auto i : {attacker, defender})
	{
		if(i.isValidPlayer())
		{
			auto & info = timerInfo[i];
			info.isBattle = true;
			info.timer.battleTimer = (pvpBattle ? si->turnTimerInfo.battleTimer : 0);
			info.timer.creatureTimer = (pvpBattle ? si->turnTimerInfo.creatureTimer : si->turnTimerInfo.battleTimer);
			
			sendTimerUpdate(i);
		}
	}
}

void TurnTimerHandler::onBattleEnd()
{
	std::lock_guard<std::recursive_mutex> guard(mx);
	const auto * gs = gameHandler.gameState();
	const auto * si = gameHandler.getStartInfo();
	if(!si || !gs || !gs->curB || !si->turnTimerInfo.isBattleEnabled())
		return;

	auto attacker = gs->curB->getSidePlayer(BattleSide::ATTACKER);
	auto defender = gs->curB->getSidePlayer(BattleSide::DEFENDER);
	
	bool pvpBattle = isPvpBattle();
	
	for(auto i : {attacker, defender})
	{
		if(i.isValidPlayer() && !pvpBattle)
		{
			auto & info = timerInfo[i];
			info.isBattle = false;
			if(si->turnTimerInfo.baseTimer && info.timer.baseTimer == 0)
				info.timer.baseTimer = info.timer.creatureTimer;
			else if(si->turnTimerInfo.turnTimer && info.timer.turnTimer == 0)
				info.timer.turnTimer = info.timer.creatureTimer;

			sendTimerUpdate(i);
		}
	}
}

void TurnTimerHandler::onBattleNextStack(const CStack & stack)
{
	std::lock_guard<std::recursive_mutex> guard(mx);
	const auto * gs = gameHandler.gameState();
	const auto * si = gameHandler.getStartInfo();
	if(!si || !gs || !gs->curB || !si->turnTimerInfo.isBattleEnabled())
		return;
	
	if(isPvpBattle())
	{
		auto player = stack.getOwner();
		
		auto & info = timerInfo[player];
		if(info.timer.battleTimer == 0)
			info.timer.battleTimer = info.timer.creatureTimer;
		info.timer.creatureTimer = si->turnTimerInfo.creatureTimer;
		
		sendTimerUpdate(player);
	}
}

void TurnTimerHandler::onBattleLoop(int waitTime)
{
	std::lock_guard<std::recursive_mutex> guard(mx);
	const auto * gs = gameHandler.gameState();
	const auto * si = gameHandler.getStartInfo();
	if(!si || !gs || !gs->curB || !si->turnTimerInfo.isBattleEnabled())
		return;
	
	ui8 side = 0;
	const CStack * stack = nullptr;
	bool isTactisPhase = gs->curB->battleTacticDist() > 0;
	
	if(isTactisPhase)
		side = gs->curB->battleGetTacticsSide();
	else
	{
		stack = gs->curB->battleGetStackByID(gs->curB->getActiveStackID());
		if(!stack || !stack->getOwner().isValidPlayer())
			return;
		side = stack->unitSide();
	}
	
	auto player = gs->curB->getSidePlayer(side);
	if(!player.isValidPlayer())
		return;
	
	const auto * state = gameHandler.getPlayerState(player);
	if(!state || state->status != EPlayerStatus::INGAME || !state->human)
		return;
	
	auto & info = timerInfo[player];
	if(info.isEnabled && info.isBattle && !timerCountDown(info.timer.creatureTimer, si->turnTimerInfo.creatureTimer, player, waitTime))
	{
		if(isPvpBattle())
		{
			if(info.timer.battleTimer > 0)
			{
				info.timer.creatureTimer = info.timer.battleTimer;
				timerCountDown(info.timer.creatureTimer, info.timer.battleTimer, player, 0);
				info.timer.battleTimer = 0;
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
				gameHandler.battles->makePlayerBattleAction(player, doNothing);
			}
		}
		else
		{
			if(info.timer.turnTimer > 0)
			{
				info.timer.creatureTimer = info.timer.turnTimer;
				timerCountDown(info.timer.creatureTimer, info.timer.turnTimer, player, 0);
				info.timer.turnTimer = 0;
			}
			else if(info.timer.baseTimer > 0)
			{
				info.timer.creatureTimer = info.timer.baseTimer;
				timerCountDown(info.timer.creatureTimer, info.timer.baseTimer, player, 0);
				info.timer.baseTimer = 0;
			}
			else
			{
				BattleAction retreat;
				retreat.side = side;
				retreat.actionType = EActionType::RETREAT; //harsh punishment
				gameHandler.battles->makePlayerBattleAction(player, retreat);
			}
		}
	}
}
