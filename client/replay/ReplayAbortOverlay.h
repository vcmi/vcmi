/*
 * ReplayAbortOverlay.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

#include "../../lib/json/JsonNode.h"

class CButton;
class CFilledTexture;
class CLabel;
class CSlider;
class TransparentFilledRectangle;

/// Controls drawn in the top left corner on top of everything else - including an ongoing combat -
/// that let the player steer a running replay: abort it, pause it, or change how fast it is shown.
/// The speed is applied to the regular animation settings and restored once the replay is over.
class ReplayAbortOverlay final : public CIntObject
{
	/// animation settings this overlay temporarily overrides, slowest first
	static constexpr std::array<int, 6> heroMoveTimes = {200, 150, 100, 50, 25, 0};
	static constexpr std::array<int, 6> battleSpeeds = {1, 2, 3, 6, 9, 18};

	std::shared_ptr<CFilledTexture> background;
	std::shared_ptr<TransparentFilledRectangle> border;
	std::shared_ptr<CButton> abortButton;
	std::shared_ptr<CButton> pauseButton;
	std::shared_ptr<CSlider> speedSlider;
	std::shared_ptr<CLabel> speedLabel;

	std::function<void()> onAbort;
	std::function<void(bool)> onPause;

	JsonNode originalHeroMoveTime;
	JsonNode originalEnemyMoveTime;
	JsonNode originalBattleSpeed;

	bool paused = false;

	void abortReplay();
	void togglePause();
	void setSpeed(int index);
	void updatePauseLabel();

public:
	ReplayAbortOverlay(std::function<void()> onAbort, std::function<void(bool)> onPause);
	~ReplayAbortOverlay();

	bool captureThisKey(EShortcut key) override;
	void keyPressed(EShortcut key) override;
};
