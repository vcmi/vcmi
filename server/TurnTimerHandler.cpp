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
#include "../lib/battle/BattleInfo.h"
#include "../lib/gameState/CGameState.h"
#include "../lib/CPlayerState.h"
#include "../lib/CStack.h"
#include "../lib/StartInfo.h"

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
			TurnTimeUpdate ttu;
			ttu.player = state.color;
			ttu.turnTimer = si->turnTimerInfo;
			ttu.turnTimer.turnTimer = 0;
			gameHandler.sendAndApply(&ttu);
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
		else if(!gameHandler.queries.topQuery(state.color)) //wait for replies to avoid pending queries
			gameHandler.states.players.at(state.color).makingTurn = false; //force end turn
	}
}

void TurnTimerHandler::onBattleStart(PlayerState & state)
{
	if(const auto * si = gameHandler.getStartInfo())
	{
		if(si->turnTimerInfo.isBattleEnabled())
		{
			TurnTimeUpdate ttu;
			ttu.player = state.color;
			ttu.turnTimer = state.turnTimer;
			ttu.turnTimer.battleTimer = si->turnTimerInfo.battleTimer;
			ttu.turnTimer.creatureTimer = si->turnTimerInfo.creatureTimer;
			gameHandler.sendAndApply(&ttu);
		}
	}
}

void TurnTimerHandler::onBattleNextStack(PlayerState & state)
{
	if(const auto * si = gameHandler.getStartInfo())
	{
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
}

void TurnTimerHandler::onBattleLoop(PlayerState & state, int waitTime)
{
	const auto * gs = gameHandler.gameState();
	const auto * si = gameHandler.getStartInfo();
	if(!si || !gs || !gs->curB)
		return;
	
	if(state.human && si->turnTimerInfo.isBattleEnabled())
	{
		if(state.turnTimer.creatureTimer > 0)
		{
			state.turnTimer.creatureTimer -= waitTime;
			int frequency = (state.turnTimer.creatureTimer > turnTimePropagateThreshold ? turnTimePropagateFrequency : turnTimePropagateFrequencyCrit);
			
			if(state.status == EPlayerStatus::INGAME //do not send message if player is not active already
			   && state.turnTimer.creatureTimer % frequency == 0)
			{
				TurnTimeUpdate ttu;
				ttu.player = state.color;
				ttu.turnTimer = state.turnTimer;
				gameHandler.sendAndApply(&ttu);
			}
		}
		else if(state.turnTimer.battleTimer > 0)
		{
			state.turnTimer.creatureTimer = state.turnTimer.battleTimer;
			state.turnTimer.battleTimer = 0;
			onBattleLoop(state, waitTime);
		}
		else if(auto * stack = const_cast<BattleInfo *>(gs->curB.get())->getStack(gs->curB->getActiveStackID()))
		{
			BattleAction doNothing;
			doNothing.actionType = EActionType::DEFEND;
			doNothing.side = stack->unitSide();
			doNothing.stackNumber = stack->unitId();
			gameHandler.makeAutomaticAction(stack, doNothing);
		}
	}
}
