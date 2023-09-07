/*
 * CHeroBackpackWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CHeroBackpackWindow.h"

#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"

#include "../widgets/Buttons.h"
#include "../widgets/Images.h"
#include "CMessage.h"
#include "render/Canvas.h"
#include "CPlayerInterface.h"

CHeroBackpackWindow::CHeroBackpackWindow(const CGHeroInstance * hero)
	: CWindowObject((EOptions)0)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	stretchedBackground = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), Rect(0, 0, 410, 425));
	pos.w = stretchedBackground->pos.w;
	pos.h = stretchedBackground->pos.h;
	center();


	arts = std::make_shared<CArtifactsOfHeroBackpack>(/*Point(-100, -170)*/Point(10, 10));
	arts->setHero(hero);
	addSet(arts);

	addCloseCallback(std::bind(&CHeroBackpackWindow::close, this));

	quitButton = std::make_shared<CButton>(Point(173, 385), AnimationPath::builtin("IOKAY32.def"), CButton::tooltip(""), [this]() { close(); }, EShortcut::GLOBAL_RETURN);
}

void CHeroBackpackWindow::showAll(Canvas &to)
{
	CIntObject::showAll(to);
	CMessage::drawBorder(PlayerColor(LOCPLINT->playerID), to.getInternalSurface(), pos.w+28, pos.h+29, pos.x-14, pos.y-15);
}
