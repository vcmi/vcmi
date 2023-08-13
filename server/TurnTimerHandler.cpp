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
#include "../lib/CPlayerState.h"
#include "../lib/gameState/CGameState.h"
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
			gameHandler.applyAndSend(&ttu);
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
			gameHandler.applyAndSend(&ttu);
		}
	}
}

void TurnTimerHandler::onPlayerMakingTurn(PlayerState & state, int waitTime)
{
	const auto * gs = gameHandler.gameState();
	const auto * si = gameHandler.getStartInfo();
	if(!si || !gs)
		return;
	
	if(si->turnTimerInfo.isEnabled() && !gs->curB)
	{
		if(state.turnTimer.turnTimer > 0)
		{
			state.turnTimer.turnTimer -= waitTime;
			
			if(state.status == EPlayerStatus::INGAME //do not send message if player is not active already
			   && state.turnTimer.turnTimer % turnTimePropagateFrequency == 0)
			{
				TurnTimeUpdate ttu;
				ttu.player = state.color;
				ttu.turnTimer = state.turnTimer;
				gameHandler.applyAndSend(&ttu);
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
