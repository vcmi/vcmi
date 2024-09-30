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

#include "../../CCallback.h"

#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/networkPacks/ArtifactLocation.h"

CHeroBackpackWindow::CHeroBackpackWindow(const CGHeroInstance * hero, const std::vector<CArtifactsOfHeroPtr> & artsSets)
	: CWindowWithArtifacts(&artsSets)
{
	OBJECT_CONSTRUCTION;

	stretchedBackground = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), Rect(0, 0, 0, 0));
	arts = std::make_shared<CArtifactsOfHeroBackpack>();
	arts->moveBy(Point(windowMargin, windowMargin));
	arts->clickPressedCallback = [this](const CArtPlace & artPlace, const Point & cursorPosition)
	{
		clickPressedOnArtPlace(arts->getHero(), artPlace.slot, true, false, true);
	};
	arts->showPopupCallback = [this](CArtPlace & artPlace, const Point & cursorPosition)
	{
		showArtifactAssembling(*arts, artPlace, cursorPosition);
	};
	addSet(arts);
	arts->setHero(hero);
	
	buttons.emplace_back(std::make_unique<CButton>(Point(), AnimationPath::builtin("ALTFILL.DEF"),
		CButton::tooltipLocalized("vcmi.heroWindow.sortBackpackByCost"),
		[hero]() { LOCPLINT->cb->sortBackpackArtifactsByCost(hero->id); }));
	buttons.emplace_back(std::make_unique<CButton>(Point(), AnimationPath::builtin("ALTFILL.DEF"),
		CButton::tooltipLocalized("vcmi.heroWindow.sortBackpackBySlot"),
		[hero]() { LOCPLINT->cb->sortBackpackArtifactsBySlot(hero->id); }));
	buttons.emplace_back(std::make_unique<CButton>(Point(), AnimationPath::builtin("ALTFILL.DEF"),
		CButton::tooltipLocalized("vcmi.heroWindow.sortBackpackByClass"),
		[hero]() { LOCPLINT->cb->sortBackpackArtifactsByClass(hero->id); }));

	pos.w = stretchedBackground->pos.w = arts->pos.w + 2 * windowMargin;
	pos.h = stretchedBackground->pos.h = arts->pos.h + buttons.back()->pos.h + 3 * windowMargin;
	
	auto buttonPos = Point(pos.x + windowMargin, pos.y + arts->pos.h + 2 * windowMargin);
	for(const auto & button : buttons)
	{
		button->moveTo(buttonPos);
		buttonPos += Point(button->pos.w + 10, 0);
	}

	statusbar = CGStatusBar::create(0, pos.h, ImagePath::builtin("ADROLLVR.bmp"), pos.w);
	pos.h += statusbar->pos.h;
	addUsedEvents(LCLICK);
	center();
}

void CHeroBackpackWindow::notFocusedClick()
{
	close();
}

void CHeroBackpackWindow::showAll(Canvas & to)
{
	CIntObject::showAll(to);
	CMessage::drawBorder(PlayerColor(LOCPLINT->playerID), to, pos.w+28, pos.h+29, pos.x-14, pos.y-15);
}

CHeroQuickBackpackWindow::CHeroQuickBackpackWindow(const CGHeroInstance * hero, ArtifactPosition targetSlot)
{
	OBJECT_CONSTRUCTION;

	stretchedBackground = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), Rect(0, 0, 0, 0));
	arts = std::make_shared<CArtifactsOfHeroQuickBackpack>(targetSlot);
	arts->moveBy(Point(windowMargin, windowMargin));
	arts->clickPressedCallback = [this](const CArtPlace & artPlace, const Point & cursorPosition)
	{
		if(const auto curHero = arts->getHero())
			swapArtifactAndClose(*arts, artPlace.slot, ArtifactLocation(curHero->id, arts->getFilterSlot()));
	};
	arts->showPopupCallback = [this](CArtPlace & artPlace, const Point & cursorPosition)
	{
		showArifactInfo(*arts, artPlace, cursorPosition);
	};
	addSet(arts);
	arts->setHero(hero);
	addUsedEvents(GESTURE);
	addUsedEvents(LCLICK);
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

void CHeroQuickBackpackWindow::notFocusedClick()
{
	close();
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
