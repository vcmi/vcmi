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
#include "../widgets/TextControls.h"
#include "CMessage.h"
#include "render/Canvas.h"
#include "CPlayerInterface.h"

CHeroBackpackWindow::CHeroBackpackWindow(const CGHeroInstance * hero)
	: CStatusbarWindow(0)
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	stretchedBackground = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), Rect(0, 0, 0, 0));
	arts = std::make_shared<CArtifactsOfHeroBackpack>();
	arts->moveBy(Point(windowMargin, windowMargin));
	addSetAndCallbacks(arts);
	arts->setHero(hero);
	addCloseCallback(std::bind(&CHeroBackpackWindow::close, this));
	quitButton = std::make_shared<CButton>(Point(), AnimationPath::builtin("IOKAY32.def"), CButton::tooltip(""),
		[this]() { WindowBase::close(); }, EShortcut::GLOBAL_RETURN);
	pos.w = stretchedBackground->pos.w = arts->pos.w + 2 * windowMargin;
	pos.h = stretchedBackground->pos.h = arts->pos.h + quitButton->pos.h + 3 * windowMargin;
	quitButton->moveTo(Point(pos.x + pos.w / 2 - quitButton->pos.w / 2, pos.y + arts->pos.h + 2 * windowMargin));
	statusbar = CGStatusBar::create(0, pos.h, ImagePath::builtin("ADROLLVR.bmp"), pos.w);
	pos.h += statusbar->pos.h;

	center();
}

void CHeroBackpackWindow::showAll(Canvas & to)
{
	CIntObject::showAll(to);
	CMessage::drawBorder(PlayerColor(LOCPLINT->playerID), to, pos.w+28, pos.h+29, pos.x-14, pos.y-15);
}

CHeroQuickBackpackWindow::CHeroQuickBackpackWindow(const CGHeroInstance * hero, ArtifactPosition targetSlot)
	: CWindowObject(0)
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	stretchedBackground = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), Rect(0, 0, 0, 0));
	arts = std::make_shared<CArtifactsOfHeroQuickBackpack>(targetSlot);
	arts->moveBy(Point(windowMargin, windowMargin));
	addSetAndCallbacks(static_cast<std::weak_ptr<CArtifactsOfHeroQuickBackpack>>(arts));
	arts->setHero(hero);
	addCloseCallback(std::bind(&CHeroQuickBackpackWindow::close, this));
	addUsedEvents(GESTURE);
	pos.w = stretchedBackground->pos.w = arts->pos.w + 2 * windowMargin;
	pos.h = stretchedBackground->pos.h = arts->pos.h + windowMargin;
}

void CHeroQuickBackpackWindow::gesture(bool on, const Point & initialPosition, const Point & finalPosition)
{
	if(on)
		return;

	arts->swapSelected();
	close();
}

void CHeroQuickBackpackWindow::gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance)
{
	arts->selectSlotAt(currentPosition);
	redraw();
}

void CHeroQuickBackpackWindow::showAll(Canvas & to)
{
	if(arts->getSlotsNum() == 0)
	{
		// Dirty solution for closing that window
		close();
		return;
	}
	CMessage::drawBorder(PlayerColor(LOCPLINT->playerID), to, pos.w + 28, pos.h + 29, pos.x - 14, pos.y - 15);
	CIntObject::showAll(to);
}
