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

CHeroBackpackWindow::CHeroBackpackWindow()
	: CWindowObject((EOptions)0)
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	stretchedBackground = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), Rect(0, 0, 0, 0));
}

CHeroBackpackWindow::CHeroBackpackWindow(const CGHeroInstance * hero)
	: CHeroBackpackWindow()
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	arts = std::make_shared<CArtifactsOfHeroBackpack>(Point(windowMargin, windowMargin));
	addSetAndCallbacks(arts);
	arts->setHero(hero);
	addCloseCallback(std::bind(&CHeroBackpackWindow::close, this));

	init();
	center();
}

void CHeroBackpackWindow::init()
{
	quitButton = std::make_shared<CButton>(Point(), AnimationPath::builtin("IOKAY32.def"), CButton::tooltip(""), [this]() { close(); }, EShortcut::GLOBAL_RETURN);
	pos.w = stretchedBackground->pos.w = arts->pos.w + 2 * windowMargin;
	pos.h = stretchedBackground->pos.h = arts->pos.h + quitButton->pos.h + 3 * windowMargin;
	quitButton->moveTo(Point(pos.x + pos.w / 2 - quitButton->pos.w / 2, pos.y + arts->pos.h + 2 * windowMargin));
}

void CHeroBackpackWindow::showAll(Canvas & to)
{
	CIntObject::showAll(to);
	CMessage::drawBorder(PlayerColor(LOCPLINT->playerID), to.getInternalSurface(), pos.w+28, pos.h+29, pos.x-14, pos.y-15);
}

CHeroQuickBackpackWindow::CHeroQuickBackpackWindow(const CGHeroInstance * hero, ArtifactPosition targetSlot)
	: CHeroBackpackWindow()
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	auto artsQuickBp = std::make_shared<CArtifactsOfHeroQuickBackpack>(Point(windowMargin, windowMargin), targetSlot);
	addSetAndCallbacks(static_cast<std::weak_ptr<CArtifactsOfHeroQuickBackpack>>(artsQuickBp));
	arts = artsQuickBp;
	arts->setHero(hero);
	addCloseCallback(std::bind(&CHeroQuickBackpackWindow::close, this));

	init();
}

void CHeroQuickBackpackWindow::showAll(Canvas & to)
{
	if(arts->getSlotsNum() == 0)
	{
		close();
		return;
	}

	CHeroBackpackWindow::showAll(to);
}
