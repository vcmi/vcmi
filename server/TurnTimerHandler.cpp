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

void TurnTimerHandler::onGameplayStart(PlayerState & state)
{
	if(const auto * si = gameHandler.getStartInfo())
	{
		if(si->turnTimerInfo.isEnabled())
		{
			state.turnTimer = si->turnTimerInfo;
			state.turnTimer.turnTimer = 0;
		}
	}
}

void TurnTimerHandler::onPlayerGetTurn(PlayerState & state)
{
	if(const auto * si = gameHandler.getStartInfo())
	{
		if(si->turnTimerInfo.isEnabled())
		{
			state.turnTimer.baseTimer += state.turnTimer.turnTimer;
			state.turnTimer.turnTimer = si->turnTimerInfo.turnTimer;
			
			TurnTimeUpdate ttu;
			ttu.player = state.color;
			ttu.turnTimer = state.turnTimer;
			gameHandler.sendAndApply(&ttu);
		}
	}
}

void TurnTimerHandler::onPlayerMakingTurn(PlayerState & state, int waitTime)
{
	const auto * gs = gameHandler.gameState();
	const auto * si = gameHandler.getStartInfo();
	if(!si || !gs)
		return;
	
	if(state.human && si->turnTimerInfo.isEnabled() && !gs->curB)
	{
		if(state.turnTimer.turnTimer > 0)
		{
			state.turnTimer.turnTimer -= waitTime;
			int frequency = (state.turnTimer.creatureTimer > turnTimePropagateThreshold ? turnTimePropagateFrequency : turnTimePropagateFrequencyCrit);
			
			if(state.status == EPlayerStatus::INGAME //do not send message if player is not active already
			   && state.turnTimer.turnTimer % frequency == 0)
			{
				TurnTimeUpdate ttu;
				ttu.player = state.color;
				ttu.turnTimer = state.turnTimer;
				gameHandler.sendAndApply(&ttu);
			}
		}
		else if(state.turnTimer.baseTimer > 0)
		{
			state.turnTimer.turnTimer = state.turnTimer.baseTimer;
			state.turnTimer.baseTimer = 0;
			onPlayerMakingTurn(state, waitTime);
		}
		else if(!gameHandler.queries->topQuery(state.color)) //wait for replies to avoid pending queries
			gameHandler.turnOrder->onPlayerEndsTurn(state.color);
	}
}

void TurnTimerHandler::onBattleStart()
{
	const auto * gs = gameHandler.gameState();
	const auto * si = gameHandler.getStartInfo();
	if(!si || !gs || !gs->curB || !si->turnTimerInfo.isBattleEnabled())
		return;
	
	auto attacker = gs->curB->getSidePlayer(BattleSide::ATTACKER);
	auto defender = gs->curB->getSidePlayer(BattleSide::DEFENDER);
	
	for(auto i : {attacker, defender})
	{
		if(i.isValidPlayer())
		{
			const auto & state = gs->players.at(i);
			TurnTimeUpdate ttu;
			ttu.player = state.color;
			ttu.turnTimer = state.turnTimer;
			ttu.turnTimer.battleTimer = si->turnTimerInfo.battleTimer;
			ttu.turnTimer.creatureTimer = si->turnTimerInfo.creatureTimer;
			gameHandler.sendAndApply(&ttu);
		}
	}
}

void TurnTimerHandler::onBattleNextStack(const CStack & stack)
{
	const auto * gs = gameHandler.gameState();
	const auto * si = gameHandler.getStartInfo();
	if(!si || !gs || !gs->curB)
		return;
	
	if(!stack.getOwner().isValidPlayer())
		return;
	
	const auto & state = gs->players.at(stack.getOwner());
	
	if(si->turnTimerInfo.isBattleEnabled())
	{
		TurnTimeUpdate ttu;
		ttu.player = state.color;
		ttu.turnTimer = state.turnTimer;
		if(state.turnTimer.battleTimer < si->turnTimerInfo.battleTimer)
			ttu.turnTimer.battleTimer = ttu.turnTimer.creatureTimer;
		ttu.turnTimer.creatureTimer = si->turnTimerInfo.creatureTimer;
		gameHandler.sendAndApply(&ttu);
	}
}

void TurnTimerHandler::onBattleLoop(int waitTime)
{
	const auto * gs = gameHandler.gameState();
	const auto * si = gameHandler.getStartInfo();
	if(!si || !gs || !gs->curB)
		return;
	
	const auto * stack = gs->curB.get()->battleGetStackByID(gs->curB->getActiveStackID());
	if(!stack || !stack->getOwner().isValidPlayer())
		return;
	
	auto & state = gs->players.at(gs->curB->getSidePlayer(stack->unitSide()));
	
	auto turnTimerUpdateApplier = [&](const TurnTimerInfo & tTimer)
	{
		TurnTimerInfo turnTimerUpdate = tTimer;
		if(tTimer.creatureTimer > 0)
		{
			turnTimerUpdate.creatureTimer -= waitTime;
			int frequency = (turnTimerUpdate.creatureTimer > turnTimePropagateThreshold ? turnTimePropagateFrequency : turnTimePropagateFrequencyCrit);
			
			if(state.status == EPlayerStatus::INGAME //do not send message if player is not active already
			   && turnTimerUpdate.creatureTimer % frequency == 0)
			{
				TurnTimeUpdate ttu;
				ttu.player = state.color;
				ttu.turnTimer = turnTimerUpdate;
				gameHandler.sendAndApply(&ttu);
			}
			return true;
		}
		return false;
	};
	
	if(state.human && si->turnTimerInfo.isBattleEnabled())
	{
		TurnTimerInfo turnTimer = state.turnTimer;
		if(!turnTimerUpdateApplier(turnTimer))
		{
			if(turnTimer.battleTimer > 0)
			{
				turnTimer.creatureTimer = turnTimer.battleTimer;
				turnTimer.battleTimer = 0;
				turnTimerUpdateApplier(turnTimer);
			}
			else
			{
				BattleAction doNothing;
				doNothing.actionType = EActionType::DEFEND;
				doNothing.side = stack->unitSide();
				doNothing.stackNumber = stack->unitId();
				gameHandler.battles->makePlayerBattleAction(state.color, doNothing);
			}
		}
	}
}
