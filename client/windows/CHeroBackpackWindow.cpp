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

	stretchedBackground = std::make_shared<CFilledTexture>("DIBOXBCK", Rect());

	arts = std::make_shared<CArtifactsOfHeroBackpack>(Point(windowMargin, windowMargin));
	arts->setHero(hero);
	addSet(arts);

	addCloseCallback(std::bind(&CHeroBackpackWindow::close, this));

	quitButton = std::make_shared<CButton>(Point(), "IOKAY32.def", CButton::tooltip(""), [this]() { close(); }, EShortcut::GLOBAL_RETURN);

	stretchedBackground->pos.w = arts->pos.w + 2 * windowMargin;
	stretchedBackground->pos.h = arts->pos.h + quitButton->pos.h + 3 * windowMargin;
	pos.w = stretchedBackground->pos.w;
	pos.h = stretchedBackground->pos.h;
	center();

	quitButton->moveBy(Point(GH.screenDimensions().x / 2 - quitButton->pos.w / 2 - quitButton->pos.x, arts->pos.h + 2 * windowMargin));
}

void CHeroBackpackWindow::showAll(Canvas & to)
{
	CIntObject::showAll(to);
	CMessage::drawBorder(PlayerColor(LOCPLINT->playerID), to.getInternalSurface(), pos.w+28, pos.h+29, pos.x-14, pos.y-15);
}
