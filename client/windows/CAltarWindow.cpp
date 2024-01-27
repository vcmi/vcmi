/*
 * CAltarWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CAltarWindow.h"

#include "../gui/CGuiHandler.h"
#include "../render/Canvas.h"
#include "../gui/Shortcut.h"
#include "../widgets/Buttons.h"
#include "../widgets/TextControls.h"

#include "../CGameInfo.h"

#include "../lib/networkPacks/ArtifactLocation.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"

CAltarWindow::CAltarWindow(const IMarket * market, const CGHeroInstance * hero, const std::function<void()> & onWindowClosed, EMarketMode mode)
	: CWindowObject(PLAYER_COLORED, ImagePath::builtin(mode == EMarketMode::CREATURE_EXP ? "ALTARMON.bmp" : "ALTRART2.bmp"))
	, hero(hero)
	, windowClosedCallback(onWindowClosed)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	assert(mode == EMarketMode::ARTIFACT_EXP || mode == EMarketMode::CREATURE_EXP);
	if(mode == EMarketMode::ARTIFACT_EXP)
		createAltarArtifacts(market, hero);
	else if(mode == EMarketMode::CREATURE_EXP)
		createAltarCreatures(market, hero);

	updateExpToLevel();
	statusBar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));
}

void CAltarWindow::updateExpToLevel()
{
	altar->expToLevel->setText(std::to_string(CGI->heroh->reqExp(CGI->heroh->level(altar->hero->exp) + 1) - altar->hero->exp));
}

void CAltarWindow::updateGarrisons()
{
	if(auto altarCreatures = std::static_pointer_cast<CAltarCreatures>(altar))
		altarCreatures->updateSlots();
}

bool CAltarWindow::holdsGarrison(const CArmedInstance * army)
{
	return hero == army;
}

const CGHeroInstance * CAltarWindow::getHero() const
{
	return hero;
}

void CAltarWindow::close()
{
	if(windowClosedCallback)
		windowClosedCallback();

	CWindowObject::close();
}

void CAltarWindow::createAltarArtifacts(const IMarket * market, const CGHeroInstance * hero)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	background = createBg(ImagePath::builtin("ALTRART2.bmp"), PLAYER_COLORED);

	auto altarArtifacts = std::make_shared<CAltarArtifacts>(market, hero);
	altar = altarArtifacts;
	artSets.clear();
	addSetAndCallbacks(altarArtifacts->getAOHset()); altarArtifacts->putBackArtifacts();

	changeModeButton = std::make_shared<CButton>(Point(516, 421), AnimationPath::builtin("ALTSACC.DEF"),
		CGI->generaltexth->zelp[572], std::bind(&CAltarWindow::createAltarCreatures, this, market, hero));
	if(altar->hero->getAlignment() == EAlignment::GOOD)
		changeModeButton->block(true);
	quitButton = std::make_shared<CButton>(Point(516, 520), AnimationPath::builtin("IOK6432.DEF"),
		CGI->generaltexth->zelp[568], [this, altarArtifacts]()
		{
			altarArtifacts->putBackArtifacts();
			CAltarWindow::close();
		}, EShortcut::GLOBAL_RETURN);
	altar->setRedrawParent(true);
	redraw();
}

void CAltarWindow::createAltarCreatures(const IMarket * market, const CGHeroInstance * hero)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	background = createBg(ImagePath::builtin("ALTARMON.bmp"), PLAYER_COLORED);

	altar = std::make_shared<CAltarCreatures>(market, hero);

	changeModeButton = std::make_shared<CButton>(Point(516, 421), AnimationPath::builtin("ALTART.DEF"),
		CGI->generaltexth->zelp[580], std::bind(&CAltarWindow::createAltarArtifacts, this, market, hero));
	if(altar->hero->getAlignment() == EAlignment::EVIL)
		changeModeButton->block(true);
	quitButton = std::make_shared<CButton>(Point(516, 520), AnimationPath::builtin("IOK6432.DEF"),
		CGI->generaltexth->zelp[568], std::bind(&CAltarWindow::close, this), EShortcut::GLOBAL_RETURN);
	altar->setRedrawParent(true);
	redraw();
}

void CAltarWindow::artifactMoved(const ArtifactLocation & srcLoc, const ArtifactLocation & destLoc, bool withRedraw)
{
	if(!getState().has_value())
		return;

	if(auto altarArtifacts = std::static_pointer_cast<CAltarArtifacts>(altar))
	{
		if(srcLoc.artHolder == altarArtifacts->getObjId() || destLoc.artHolder == altarArtifacts->getObjId())
			altarArtifacts->updateSlots();

		if(const auto pickedArt = getPickedArtifact())
			altarArtifacts->setSelectedArtifact(pickedArt);
		else
			altarArtifacts->setSelectedArtifact(nullptr);
	}
	CWindowWithArtifacts::artifactMoved(srcLoc, destLoc, withRedraw);
}

void CAltarWindow::showAll(Canvas & to)
{
	// This func is temporary workaround for compliance with CTradeWindow
	CWindowObject::showAll(to);

	if(altar->hRight)
	{
		altar->hRight->showAllAt(altar->pos.topLeft() + Point(396, 423), "", to);
	}
	if(altar->hLeft)
	{
		altar->hLeft->showAllAt(altar->pos.topLeft() + Point(150, 423), "", to);
	}
}
