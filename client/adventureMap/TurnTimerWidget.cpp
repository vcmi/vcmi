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
#include "../gui/CGuiHandler.h"
#include "../render/Graphics.h"
#include "../widgets/Images.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/TextControls.h"

#include "../../CCallback.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/CStack.h"
#include "../../lib/StartInfo.h"

TurnTimerWidget::TurnTimerWidget(const Point & position)
	: TurnTimerWidget(position, PlayerColor::NEUTRAL)
{}

TurnTimerWidget::TurnTimerWidget(const Point & position, PlayerColor player)
	: CIntObject(TIME)
	, lastSoundCheckSeconds(0)
{
	pos += position;
	pos.w = 50;
	pos.h = 0;
	recActions &= ~DEACTIVATE;
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	backgroundTexture = std::make_shared<CFilledTexture>(ImagePath::builtin("DiBoxBck"), pos); // 1 px smaller on all sides
	backgroundBorder = std::make_shared<TransparentFilledRectangle>(pos, ColorRGBA(0, 0, 0, 128), Colors::METALLIC_GOLD);

	if (player.isValidPlayer())
	{
		pos.h += 16;
		playerLabels[player] = std::make_shared<CLabel>(pos.w / 2, pos.h - 8, FONT_BIG, ETextAlignment::CENTER, graphics->playerColors[player], "");
	}
	else
	{
		for(PlayerColor player(0); player < PlayerColor::PLAYER_LIMIT; ++player)
		{
			if (LOCPLINT->cb->getStartInfo()->playerInfos.count(player) == 0)
				continue;

			if (!LOCPLINT->cb->getStartInfo()->playerInfos.at(player).isControlledByHuman())
				continue;

			pos.h += 16;
			playerLabels[player] = std::make_shared<CLabel>(pos.w / 2, pos.h - 8, FONT_BIG, ETextAlignment::CENTER, graphics->playerColors[player], "");
		}
	}

	backgroundTexture->pos = Rect::createAround(pos, -1);
	backgroundBorder->pos = pos;
}

void TurnTimerWidget::show(Canvas & to)
{
	showAll(to);
}

void TurnTimerWidget::updateNotifications(PlayerColor player, int timeMs)
{
	int newTimeSeconds = timeMs / 1000;
	if(player == LOCPLINT->playerID
	   && newTimeSeconds != lastSoundCheckSeconds
	   && notificationThresholds.count(newTimeSeconds))
	{
		CCS->soundh->playSound(AudioPath::builtin("WE5"));
	}
	lastSoundCheckSeconds = newTimeSeconds;
}

void TurnTimerWidget::updateTextLabel(PlayerColor player, int timeMs)
{
	auto label = playerLabels[player];

	std::ostringstream oss;
	oss << lastSoundCheckSeconds / 60 << ":" << std::setw(2) << std::setfill('0') << lastSoundCheckSeconds % 60;
	label->setText(oss.str());
	label->setColor(graphics->playerColors[player]);
}

void TurnTimerWidget::updateTimer(PlayerColor player, uint32_t msPassed)
{
	const auto & gamestateTimer = LOCPLINT->cb->getPlayerTurnTime(player);

	if (!(lastUpdateTimers[player] == gamestateTimer))
	{
		lastUpdateTimers[player] = gamestateTimer;
		countingDownTimers[player] = gamestateTimer;
	}

	auto & countingDownTimer = countingDownTimers[player];

	if(countingDownTimer.isActive && LOCPLINT->cb->isPlayerMakingTurn(player))
		countingDownTimer.substractTimer(msPassed);
	
	updateNotifications(player, countingDownTimer.valueMs());
	updateTextLabel(player, countingDownTimer.valueMs());
}

void TurnTimerWidget::tick(uint32_t msPassed)
{
	for(const auto & player : playerLabels)
	{
		if (LOCPLINT->battleInt)
		{
			const auto & battle = LOCPLINT->battleInt->getBattle();

			bool isDefender = battle->sideToPlayer(BattleSide::DEFENDER) == player.first;
			bool isAttacker = battle->sideToPlayer(BattleSide::ATTACKER) == player.first;
			bool isMakingUnitTurn = battle->battleActiveUnit()->unitOwner() == player.first;
			bool isEngagedInBattle = isDefender || isAttacker;

			// Due to way our network message queue works during battle animation
			// client actually does not receives updates from server as to which timer is active when game has battle animations playing
			// so during battle skip updating timer unless game is waiting for player to select action
			if (isEngagedInBattle && !isMakingUnitTurn)
				continue;
		}

		updateTimer(player.first, msPassed);
	}
}
