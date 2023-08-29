/*
 * TurnTimerWidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "TurnTimerWidget.h"

#include "../CGameInfo.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../battle/BattleInterface.h"
#include "../battle/BattleStacksController.h"

#include "../render/EFont.h"
#include "../render/Graphics.h"
#include "../gui/CGuiHandler.h"
#include "../gui/TextAlignment.h"
#include "../widgets/Images.h"
#include "../widgets/TextControls.h"
#include "../../CCallback.h"
#include "../../lib/CStack.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/filesystem/ResourceID.h"

TurnTimerWidget::DrawRect::DrawRect(const Rect & r, const ColorRGBA & c):
	CIntObject(), rect(r), color(c)
{
}

void TurnTimerWidget::DrawRect::showAll(Canvas & to)
{
	to.drawColor(rect, color);
	
	CIntObject::showAll(to);
}

TurnTimerWidget::TurnTimerWidget():
	InterfaceObjectConfigurable(TIME),
	turnTime(0), lastTurnTime(0), cachedTurnTime(0), lastPlayer(PlayerColor::CANNOT_DETERMINE)
{
	REGISTER_BUILDER("drawRect", &TurnTimerWidget::buildDrawRect);
	
	recActions &= ~DEACTIVATE;
	
	const JsonNode config(ResourceID("config/widgets/turnTimer.json"));
	
	build(config);
	
	std::transform(variables["notificationTime"].Vector().begin(),
				   variables["notificationTime"].Vector().end(),
				   std::inserter(notifications, notifications.begin()),
				   [](const JsonNode & node){ return node.Integer(); });
}

std::shared_ptr<TurnTimerWidget::DrawRect> TurnTimerWidget::buildDrawRect(const JsonNode & config) const
{
	logGlobal->debug("Building widget TurnTimerWidget::DrawRect");
	auto rect = readRect(config["rect"]);
	auto color = readColor(config["color"]);
	return std::make_shared<TurnTimerWidget::DrawRect>(rect, color);
}

void TurnTimerWidget::show(Canvas & to)
{
	showAll(to);
}

void TurnTimerWidget::setTime(PlayerColor player, int time)
{
	int newTime = time / 1000;
	if(player == LOCPLINT->playerID
	   && newTime != turnTime
	   && notifications.count(newTime))
	{
		CCS->soundh->playSound(variables["notificationSound"].String());
	}

	turnTime = newTime;

	if(auto w = widget<CLabel>("timer"))
	{
		std::ostringstream oss;
		oss << turnTime / 60 << ":" << std::setw(2) << std::setfill('0') << turnTime % 60;
		w->setText(oss.str());
		
		if(graphics && LOCPLINT && LOCPLINT->cb
		   && variables["textColorFromPlayerColor"].Bool()
		   && player.isValidPlayer())
		{
			w->setColor(graphics->playerColors[player]);
		}
	}
}

void TurnTimerWidget::tick(uint32_t msPassed)
{
	if(!LOCPLINT || !LOCPLINT->cb)
		return;

	for (PlayerColor p(0); p < PlayerColor::PLAYER_LIMIT; ++p)
	{
		auto player = p;
		if(LOCPLINT->battleInt)
		{
			if(auto * stack = LOCPLINT->battleInt->stacksController->getActiveStack())
				player = stack->getOwner();
		}
		
		if(p != player || !LOCPLINT->cb->isPlayerMakingTurn(player))
			continue;

		auto time = LOCPLINT->cb->getPlayerTurnTime(player);
		if(LOCPLINT->isTimerEnabled())
			cachedTurnTime -= msPassed;
		if(cachedTurnTime < 0) cachedTurnTime = 0; //do not go below zero
		
		if(lastPlayer != player)
		{
			lastPlayer = player;
			lastTurnTime = 0;
		}
		
		auto timeCheckAndUpdate = [&](int time)
		{
			if(time / 1000 != lastTurnTime / 1000)
			{
				//do not update timer on this tick
				lastTurnTime = time;
				cachedTurnTime = time;
			}
			else
				setTime(player, cachedTurnTime);
		};
		
		auto * playerInfo = LOCPLINT->cb->getPlayer(player);
		if(player.isValidPlayer() || (playerInfo && playerInfo->isHuman()))
		{
			if(LOCPLINT->battleInt)
			{
				if(time.isBattleEnabled())
					timeCheckAndUpdate(time.creatureTimer);
			}
			else
			{
				timeCheckAndUpdate(time.turnTimer);
			}
		}
		else
			timeCheckAndUpdate(0);
	}
}
