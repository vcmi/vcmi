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
#include "../gui/WindowHandler.h"
#include "../widgets/Images.h"

CTutorialWindow::CTutorialWindow(const TutorialMode & m)
	: CWindowObject(BORDERED, ImagePath::builtin("DIBOXBCK")), mode { m }
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	pos = Rect(pos.x, pos.y, 480, 320);
    background = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), Rect(0, 0, pos.w, pos.h));

	updateShadow();

	center();

    addUsedEvents(LCLICK);
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

void CTutorialWindow::clickPressed(const Point & cursorPosition)
{
    close();
    
    if(LOCPLINT)
		LOCPLINT->showingDialog->setn(false);
}