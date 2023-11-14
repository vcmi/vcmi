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
#include "../CPlayerInterface.h"

#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../widgets/Images.h"
#include "../widgets/Buttons.h"
#include "../widgets/TextControls.h"

CTutorialWindow::CTutorialWindow(const TutorialMode & m)
	: CWindowObject(BORDERED, ImagePath::builtin("DIBOXBCK")), mode { m }
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	pos = Rect(pos.x, pos.y, 540, 400); //video: 480x320
    background = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), Rect(0, 0, pos.w, pos.h));

	updateShadow();

	center();

    addUsedEvents(LCLICK);

    buttonOk = std::make_shared<CButton>(Point(239, 367), AnimationPath::builtin("IOKAY"), CButton::tooltip(), std::bind(&CTutorialWindow::close, this), EShortcut::GLOBAL_ACCEPT); //62x28
    buttonLeft = std::make_shared<CButton>(Point(5, 177), AnimationPath::builtin("HSBTNS3"), CButton::tooltip(), std::bind(&CTutorialWindow::previous, this), EShortcut::MOVE_LEFT); //22x46
    buttonRight = std::make_shared<CButton>(Point(513, 177), AnimationPath::builtin("HSBTNS5"), CButton::tooltip(), std::bind(&CTutorialWindow::next, this), EShortcut::MOVE_RIGHT); //22x46
    buttonRight->block(true);

    labelTitle = std::make_shared<CLabel>(270, 15, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, "Touchscreen Intro");
    labelInformation = std::make_shared<CMultiLineLabel>(Rect(5, 50, 530, 60), EFonts::FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, "Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua.");
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

void CTutorialWindow::close()
{
    if(LOCPLINT)
		LOCPLINT->showingDialog->setn(false);

    WindowBase::close();
}

void CTutorialWindow::next()
{

}

void CTutorialWindow::previous()
{
    
}