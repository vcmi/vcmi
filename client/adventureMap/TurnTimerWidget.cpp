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

#include "../CPlayerInterface.h"
#include "../battle/BattleInterface.h"
#include "../battle/BattleStacksController.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../media/ISoundPlayer.h"
#include "../render/Graphics.h"
#include "../widgets/Images.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
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
	, isBattleMode(player.isValidPlayer())
{
	OBJECT_CONSTRUCTION;

	pos += position;
	pos.w = 0;
	pos.h = 0;
	recActions &= ~DEACTIVATE;
	const auto & timers = GAME->interface()->cb->getStartInfo()->turnTimerInfo;

	backgroundTexture = std::make_shared<CFilledTexture>(ImagePath::builtin("DiBoxBck"), pos); // 1 px smaller on all sides

	if (isBattleMode)
		backgroundBorder = std::make_shared<TransparentFilledRectangle>(pos, ColorRGBA(0, 0, 0, 128), Colors::BRIGHT_YELLOW);
	else
		backgroundBorder = std::make_shared<TransparentFilledRectangle>(pos, ColorRGBA(0, 0, 0, 128), Colors::BLACK);

	if (isBattleMode)
	{
		pos.w = 76;

		pos.h += 20;
		playerLabelsMain[player] = std::make_shared<CLabel>(pos.w / 2, pos.h - 10, FONT_BIG, ETextAlignment::CENTER, graphics->playerColors[player.getNum()], "");

		if (timers.battleTimer != 0)
		{
			pos.h += 20;
			playerLabelsBattle[player] = std::make_shared<CLabel>(pos.w / 2, pos.h - 10, FONT_BIG, ETextAlignment::CENTER, graphics->playerColors[player.getNum()], "");
		}

		if (!timers.accumulatingUnitTimer && timers.unitTimer != 0)
		{
			pos.h += 20;
			playerLabelsUnit[player] = std::make_shared<CLabel>(pos.w / 2, pos.h - 10, FONT_BIG, ETextAlignment::CENTER, graphics->playerColors[player.getNum()], "");
		}

		updateTextLabel(player, GAME->interface()->cb->getPlayerTurnTime(player));
	}
	else
	{
		if (!timers.accumulatingTurnTimer && timers.baseTimer != 0)
			pos.w = 120;
		else
			pos.w = 60;

		for(PlayerColor player(0); player < PlayerColor::PLAYER_LIMIT; ++player)
		{
			if (GAME->interface()->cb->getStartInfo()->playerInfos.count(player) == 0)
				continue;

			if (!GAME->interface()->cb->getStartInfo()->playerInfos.at(player).isControlledByHuman())
				continue;

			pos.h += 20;
			playerLabelsMain[player] = std::make_shared<CLabel>(pos.w / 2, pos.h - 10, FONT_BIG, ETextAlignment::CENTER, graphics->playerColors[player.getNum()], "");

			updateTextLabel(player, GAME->interface()->cb->getPlayerTurnTime(player));
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
	if(player != GAME->interface()->playerID)
		return;

	int newTimeSeconds = timeMs / 1000;

	if (newTimeSeconds != lastSoundCheckSeconds && notificationThresholds.count(newTimeSeconds))
		ENGINE->sound().playSound(AudioPath::builtin("WE5"));

	lastSoundCheckSeconds = newTimeSeconds;
}

static std::string msToString(int timeMs)
{
	int timeSeconds = timeMs / 1000;
	std::ostringstream oss;
	oss << timeSeconds / 60 << ":" << std::setw(2) << std::setfill('0') << timeSeconds % 60;
	return oss.str();
}

void TurnTimerWidget::updateTextLabel(PlayerColor player, const TurnTimerInfo & timer)
{
	const auto & timerSettings = GAME->interface()->cb->getStartInfo()->turnTimerInfo;
	auto mainLabel = playerLabelsMain[player];

	if (isBattleMode)
	{
		mainLabel->setText(msToString(timer.baseTimer + timer.turnTimer));

		if (timerSettings.battleTimer != 0)
		{
			auto battleLabel = playerLabelsBattle[player];
			if (timer.battleTimer != 0)
			{
				if (timerSettings.accumulatingUnitTimer)
					battleLabel->setText("+" + msToString(timer.battleTimer + timer.unitTimer));
				else
					battleLabel->setText("+" + msToString(timer.battleTimer));
			}
			else
				battleLabel->setText("");
		}

		if (!timerSettings.accumulatingUnitTimer && timerSettings.unitTimer != 0)
		{
			auto unitLabel = playerLabelsUnit[player];
			if (timer.unitTimer != 0)
				unitLabel->setText("+" + msToString(timer.unitTimer));
			else
				unitLabel->setText("");
		}
	}
	else
	{
		if (!timerSettings.accumulatingTurnTimer && timerSettings.baseTimer != 0)
			mainLabel->setText(msToString(timer.baseTimer) + "+" + msToString(timer.turnTimer));
		else
			mainLabel->setText(msToString(timer.baseTimer + timer.turnTimer));
	}
}

void TurnTimerWidget::updateTimer(PlayerColor player, uint32_t msPassed)
{
	const auto & gamestateTimer = GAME->interface()->cb->getPlayerTurnTime(player);
	updateNotifications(player, gamestateTimer.valueMs());
	updateTextLabel(player, gamestateTimer);
}

void TurnTimerWidget::tick(uint32_t msPassed)
{
	for(const auto & player : playerLabelsMain)
	{
		if (GAME->interface()->battleInt)
		{
			const auto & battle = GAME->interface()->battleInt->getBattle();

			bool isDefender = battle->sideToPlayer(BattleSide::DEFENDER) == player.first;
			bool isAttacker = battle->sideToPlayer(BattleSide::ATTACKER) == player.first;
			bool isMakingUnitTurn = battle->battleActiveUnit() && battle->battleActiveUnit()->unitOwner() == player.first;
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
