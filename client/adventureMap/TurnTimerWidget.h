/*
 * TurnTimerWidget.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../gui/CIntObject.h"
#include "../render/Colors.h"
#include "../../lib/TurnTimerInfo.h"

class CAnimImage;
class CLabel;
class CFilledTexture;
class TransparentFilledRectangle;

VCMI_LIB_NAMESPACE_BEGIN
class PlayerColor;
VCMI_LIB_NAMESPACE_END

class TurnTimerWidget : public CIntObject
{
	int lastSoundCheckSeconds;
	bool isBattleMode;

	const std::set<int> notificationThresholds = {1, 2, 3, 4, 5, 10, 20, 30};

	std::map<PlayerColor, std::shared_ptr<CLabel>> playerLabelsMain;
	std::map<PlayerColor, std::shared_ptr<CLabel>> playerLabelsBattle;
	std::map<PlayerColor, std::shared_ptr<CLabel>> playerLabelsUnit;
	std::shared_ptr<CFilledTexture> backgroundTexture;
	std::shared_ptr<TransparentFilledRectangle> backgroundBorder;

	void updateTimer(PlayerColor player, uint32_t msPassed);

	void show(Canvas & to) override;
	void tick(uint32_t msPassed) override;
	
	void updateNotifications(PlayerColor player, int timeMs);
	void updateTextLabel(PlayerColor player, const TurnTimerInfo & timer);

public:
	/// Activates adventure map mode in which widget will display timer for all players
	TurnTimerWidget(const Point & position);

	/// Activates battle mode in which timer displays only timer of specific player
	TurnTimerWidget(const Point & position, PlayerColor player);
};
