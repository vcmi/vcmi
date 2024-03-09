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
#include "../../lib/CondSh.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../CPlayerInterface.h"
#include "../CGameInfo.h"
#include "../CVideoHandler.h"

#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../widgets/Images.h"
#include "../widgets/Buttons.h"
#include "../widgets/TextControls.h"
#include "../render/Canvas.h"

CTutorialWindow::CTutorialWindow(const TutorialMode & m)
	: CWindowObject(BORDERED, ImagePath::builtin("DIBOXBCK")), mode { m }, page { 0 }
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	pos = Rect(pos.x, pos.y, 380, 400); //video: 320x240
	background = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), Rect(0, 0, pos.w, pos.h));

	updateShadow();

	center();

	addUsedEvents(LCLICK);

	if(mode == TutorialMode::TOUCH_ADVENTUREMAP) videos = { "RightClick", "MapPanning", "MapZooming", "RadialWheel" };
	else if(mode == TutorialMode::TOUCH_BATTLE) videos = { "BattleDirection", "BattleDirectionAbort", "AbortSpell" };

	labelTitle = std::make_shared<CLabel>(190, 15, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->translate("vcmi.tutorialWindow.title"));
	labelInformation = std::make_shared<CMultiLineLabel>(Rect(5, 40, 370, 60), EFonts::FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, "");
	buttonOk = std::make_shared<CButton>(Point(159, 367), AnimationPath::builtin("IOKAY"), CButton::tooltip(), std::bind(&CTutorialWindow::exit, this), EShortcut::GLOBAL_ACCEPT); //62x28
	buttonLeft = std::make_shared<CButton>(Point(5, 217), AnimationPath::builtin("HSBTNS3"), CButton::tooltip(), std::bind(&CTutorialWindow::previous, this), EShortcut::MOVE_LEFT); //22x46
	buttonRight = std::make_shared<CButton>(Point(352, 217), AnimationPath::builtin("HSBTNS5"), CButton::tooltip(), std::bind(&CTutorialWindow::next, this), EShortcut::MOVE_RIGHT); //22x46

	setContent();
}

void CTutorialWindow::setContent()
{
	video = "tutorial/" + videos[page];

	buttonLeft->block(page<1);
	buttonRight->block(page>videos.size() - 2);

	labelInformation->setText(CGI->generaltexth->translate("vcmi.tutorialWindow.decription." + videos[page]));
}

void CTutorialWindow::openWindowFirstTime(const TutorialMode & m)
{
	if(GH.input().hasTouchInputDevice() && !persistentStorage["gui"]["tutorialCompleted" + std::to_string(m)].Bool())
	{
		if(LOCPLINT)
			LOCPLINT->showingDialog->set(true);
		GH.windows().pushWindow(std::make_shared<CTutorialWindow>(m));

		Settings s = persistentStorage.write["gui"]["tutorialCompleted" + std::to_string(m)];
		s->Bool() = true;
	}
}

void CTutorialWindow::exit()
{
	if(LOCPLINT)
		LOCPLINT->showingDialog->setn(false);

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

void CTutorialWindow::show(Canvas & to)
{
	CCS->videoh->update(pos.x + 30, pos.y + 120, to.getInternalSurface(), true, false,
	[&]()
	{
		CCS->videoh->close();
		CCS->videoh->open(VideoPath::builtin(video));
	});

	CIntObject::show(to);
}

void CTutorialWindow::activate()
{
	CCS->videoh->open(VideoPath::builtin(video));
	CIntObject::activate();
}

void CTutorialWindow::deactivate()
{
	CCS->videoh->close();
}
