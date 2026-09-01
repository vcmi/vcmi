/*
 * ReplayAbortOverlay.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ReplayAbortOverlay.h"

#include "../gui/Shortcut.h"
#include "../render/Colors.h"
#include "../widgets/Buttons.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/Images.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/filesystem/ResourcePath.h"
#include "../../lib/texts/MetaString.h"
#include "../GameInstance.h"

ReplayAbortOverlay::ReplayAbortOverlay(std::function<void()> onAbort, std::function<void(bool)> onPause)
	: onAbort(std::move(onAbort))
	, onPause(std::move(onPause))
{
	OBJECT_CONSTRUCTION;

	pos.x = 10;
	pos.y = 10;
	pos.w = 177;
	pos.h = 84;

	background = std::make_shared<CFilledTexture>(ImagePath::builtin("DiBoxBck"), Rect(0, 0, pos.w, pos.h));
	border = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, pos.w, pos.h), ColorRGBA(0, 0, 0, 0), ColorRGBA(128, 100, 75), 1);

	abortButton = std::make_shared<CButton>(
		Point(6, 6),
		AnimationPath::builtin("settingsWindow/button80"),
		std::make_pair("", MetaString::createFromTextID("vcmi.replay.abort.help").toString(&GAME->translator())),
		[this](){ abortReplay(); });
	abortButton->setTextOverlay(MetaString::createFromTextID("vcmi.replay.abort").toString(&GAME->translator()), FONT_MEDIUM, Colors::YELLOW);

	pauseButton = std::make_shared<CButton>(
		Point(91, 6),
		AnimationPath::builtin("settingsWindow/button80"),
		std::make_pair("", MetaString::createFromTextID("vcmi.replay.pause.help").toString(&GAME->translator())),
		[this](){ togglePause(); });
	updatePauseLabel();

	// the replay drives the very same animations as normal play, so its speed is set by
	// overriding the regular settings - the originals are put back in the destructor
	originalHeroMoveTime = settings["adventure"]["heroMoveTime"];
	originalEnemyMoveTime = settings["adventure"]["enemyMoveTime"];
	originalBattleSpeed = settings["battle"]["speedFactor"];

	const int currentMoveTime = static_cast<int>(originalHeroMoveTime.Float());
	auto closest = std::min_element(heroMoveTimes.begin(), heroMoveTimes.end(),
		[currentMoveTime](int left, int right)
		{
			return std::abs(left - currentMoveTime) < std::abs(right - currentMoveTime);
		});
	const int currentIndex = static_cast<int>(std::distance(heroMoveTimes.begin(), closest));

	speedSlider = std::make_shared<CSlider>(Point(6, 46), pos.w - 12, [this](int index){ setSpeed(index); },
		1, static_cast<int>(heroMoveTimes.size()), currentIndex, Orientation::HORIZONTAL, CSlider::BROWN);

	speedLabel = std::make_shared<CLabel>(pos.w / 2, 72, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE,
		MetaString::createFromTextID("vcmi.replay.speed").toString(&GAME->translator()));

	addUsedEvents(KEYBOARD);
}

ReplayAbortOverlay::~ReplayAbortOverlay()
{
	Settings heroMoveTime = settings.write["adventure"]["heroMoveTime"];
	heroMoveTime->Float() = originalHeroMoveTime.Float();

	Settings enemyMoveTime = settings.write["adventure"]["enemyMoveTime"];
	enemyMoveTime->Float() = originalEnemyMoveTime.Float();

	Settings battleSpeed = settings.write["battle"]["speedFactor"];
	battleSpeed->Float() = originalBattleSpeed.Float();
}

void ReplayAbortOverlay::setSpeed(int index)
{
	if(index < 0 || index >= static_cast<int>(heroMoveTimes.size()))
		return;

	Settings heroMoveTime = settings.write["adventure"]["heroMoveTime"];
	heroMoveTime->Float() = heroMoveTimes[index];

	Settings enemyMoveTime = settings.write["adventure"]["enemyMoveTime"];
	enemyMoveTime->Float() = heroMoveTimes[index];

	Settings battleSpeed = settings.write["battle"]["speedFactor"];
	battleSpeed->Float() = battleSpeeds[index];
}

void ReplayAbortOverlay::updatePauseLabel()
{
	const std::string text = MetaString::createFromTextID(paused ? "vcmi.replay.resume" : "vcmi.replay.pause").toString(&GAME->translator());
	pauseButton->setTextOverlay(text, FONT_MEDIUM, Colors::YELLOW);
}

void ReplayAbortOverlay::togglePause()
{
	paused = !paused;
	updatePauseLabel();

	if(onPause)
		onPause(paused);

	redraw();
}

void ReplayAbortOverlay::abortReplay()
{
	if(onAbort)
		onAbort();
}

bool ReplayAbortOverlay::captureThisKey(EShortcut key)
{
	// while a replay is on screen escape belongs to this button and to nothing else
	return key == EShortcut::GLOBAL_CANCEL;
}

void ReplayAbortOverlay::keyPressed(EShortcut key)
{
	if(key == EShortcut::GLOBAL_CANCEL)
		abortReplay();
}
