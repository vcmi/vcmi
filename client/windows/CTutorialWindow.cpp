/*
 * CTutorialWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CTutorialWindow.h"

#include "../eventsSDL/InputHandler.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/ConditionalWait.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/GameLibrary.h"
#include "../CPlayerInterface.h"

#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../widgets/Images.h"
#include "../widgets/Buttons.h"
#include "../widgets/TextControls.h"
#include "../widgets/VideoWidget.h"
#include "../render/Canvas.h"

CTutorialWindow::CTutorialWindow(const TutorialMode & m)
	: CWindowObject(BORDERED, ImagePath::builtin("DIBOXBCK")), mode { m }, page { 0 }
{
	OBJECT_CONSTRUCTION;

	pos = Rect(pos.x, pos.y, 380, 400); //video: 320x240
	background = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), Rect(0, 0, pos.w, pos.h));

	updateShadow();

	center();

	addUsedEvents(LCLICK);

	if(mode == TutorialMode::TOUCH_ADVENTUREMAP) videos = { "RightClick", "MapPanning", "MapZooming", "RadialWheel" };
	else if(mode == TutorialMode::TOUCH_BATTLE) videos = { "BattleDirection", "BattleDirectionAbort", "AbortSpell" };

	labelTitle = std::make_shared<CLabel>(190, 15, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->translate("vcmi.tutorialWindow.title"));
	labelInformation = std::make_shared<CMultiLineLabel>(Rect(5, 40, 370, 60), EFonts::FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, "");
	buttonOk = std::make_shared<CButton>(Point(159, 367), AnimationPath::builtin("IOKAY"), CButton::tooltip(), std::bind(&CTutorialWindow::exit, this), EShortcut::GLOBAL_RETURN); //62x28
	buttonLeft = std::make_shared<CButton>(Point(5, 217), AnimationPath::builtin("HSBTNS3"), CButton::tooltip(), std::bind(&CTutorialWindow::previous, this), EShortcut::MOVE_LEFT); //22x46
	buttonRight = std::make_shared<CButton>(Point(352, 217), AnimationPath::builtin("HSBTNS5"), CButton::tooltip(), std::bind(&CTutorialWindow::next, this), EShortcut::MOVE_RIGHT); //22x46

	setContent();
}

void CTutorialWindow::setContent()
{
	OBJECT_CONSTRUCTION;
	auto video = VideoPath::builtin("tutorial/" + videos[page]);

	videoPlayer = std::make_shared<VideoWidget>(Point(30, 120), video, false);

	buttonLeft->block(page<1);
	buttonRight->block(page>videos.size() - 2);

	labelInformation->setText(LIBRARY->generaltexth->translate("vcmi.tutorialWindow.decription." + videos[page]));
}

void CTutorialWindow::openWindowFirstTime(const TutorialMode & m)
{
	if(ENGINE->input().getCurrentInputMode() == InputMode::TOUCH && !persistentStorage["gui"]["tutorialCompleted" + std::to_string(m)].Bool())
	{
		if(GAME->interface())
			GAME->interface()->showingDialog->setBusy();
		ENGINE->windows().pushWindow(std::make_shared<CTutorialWindow>(m));

		Settings s = persistentStorage.write["gui"]["tutorialCompleted" + std::to_string(m)];
		s->Bool() = true;
	}
}

void CTutorialWindow::exit()
{
	if(GAME->interface())
		GAME->interface()->showingDialog->setFree();

	close();
}

void CTutorialWindow::next()
{
	page++;
	setContent();
	deactivate();
	activate();
}

void CTutorialWindow::previous()
{
	page--;
	setContent();
	deactivate();
	activate();
}
