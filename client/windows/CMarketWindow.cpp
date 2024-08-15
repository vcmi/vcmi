/*
 * CMarketWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CMarketWindow.h"

#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"

#include "../widgets/Buttons.h"
#include "../widgets/TextControls.h"
#include "../widgets/markets/CAltarArtifacts.h"
#include "../widgets/markets/CAltarCreatures.h"
#include "../widgets/markets/CArtifactsBuying.h"
#include "../widgets/markets/CArtifactsSelling.h"
#include "../widgets/markets/CFreelancerGuild.h"
#include "../widgets/markets/CMarketResources.h"
#include "../widgets/markets/CTransferResources.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"

#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/CGMarket.h"
#include "../../lib/mapObjects/CGHeroInstance.h"

CMarketWindow::CMarketWindow(const IMarket * market, const CGHeroInstance * hero, const std::function<void()> & onWindowClosed, EMarketMode mode)
	: CWindowObject(PLAYER_COLORED)
	, windowClosedCallback(onWindowClosed)
{
	assert(mode == EMarketMode::RESOURCE_RESOURCE || mode == EMarketMode::RESOURCE_PLAYER || mode == EMarketMode::CREATURE_RESOURCE ||
		mode == EMarketMode::RESOURCE_ARTIFACT || mode == EMarketMode::ARTIFACT_RESOURCE || mode == EMarketMode::ARTIFACT_EXP ||
		mode == EMarketMode::CREATURE_EXP);
	
	OBJECT_CONSTRUCTION;

	if(mode == EMarketMode::RESOURCE_RESOURCE)
		createMarketResources(market, hero);
	else if(mode == EMarketMode::RESOURCE_PLAYER)
		createTransferResources(market, hero);
	else if(mode == EMarketMode::CREATURE_RESOURCE)
		createFreelancersGuild(market, hero);
	else if(mode == EMarketMode::RESOURCE_ARTIFACT)
		createArtifactsBuying(market, hero);
	else if(mode == EMarketMode::ARTIFACT_RESOURCE)
		createArtifactsSelling(market, hero);
	else if(mode == EMarketMode::ARTIFACT_EXP)
		createAltarArtifacts(market, hero);
	else if(mode == EMarketMode::CREATURE_EXP)
		createAltarCreatures(market, hero);

	statusbar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));
}

void CMarketWindow::updateArtifacts()
{
	update();
}

void CMarketWindow::updateGarrisons()
{
	update();
}

void CMarketWindow::updateResources()
{
	update();
}

void CMarketWindow::updateExperience()
{
	update();
}

void CMarketWindow::update()
{
	CWindowWithArtifacts::update();
	assert(marketWidget);
	marketWidget->update();
}

void CMarketWindow::close()
{
	if(windowClosedCallback)
		windowClosedCallback();

	CWindowObject::close();
}

bool CMarketWindow::holdsGarrison(const CArmedInstance * army)
{
	assert(marketWidget);
	return marketWidget->hero == army;
}

void CMarketWindow::createChangeModeButtons(EMarketMode currentMode, const IMarket * market, const CGHeroInstance * hero)
{
	auto isButtonVisible = [currentMode, market, hero](EMarketMode modeButton) -> bool
	{
		if(currentMode == modeButton)
			return false;

		if(!market->allowsTrade(modeButton))
			return false;

		if(modeButton == EMarketMode::RESOURCE_RESOURCE || modeButton == EMarketMode::RESOURCE_PLAYER)
		{
			if(const auto town = dynamic_cast<const CGTownInstance*>(market))
				return town->getOwner() == LOCPLINT->playerID;
			else
				return true;
		}
		else
		{
			return hero != nullptr;
		}
	};

	changeModeButtons.clear();
	auto buttonPos = Point(18, 520);

	auto addButton = [this, &buttonPos](const AnimationPath & picPath, const std::pair<std::string, std::string> & buttonHelpContainer,
		const std::function<void()> & pressButtonFunctor, EShortcut shortcut)
	{
		changeModeButtons.emplace_back(std::make_shared<CButton>(buttonPos, picPath, buttonHelpContainer, pressButtonFunctor, shortcut));
		buttonPos -= Point(0, buttonHeightWithMargin);
	};

	if(isButtonVisible(EMarketMode::RESOURCE_PLAYER))
		addButton(AnimationPath::builtin("TPMRKBU1.DEF"), CGI->generaltexth->zelp[612], std::bind(&CMarketWindow::createTransferResources, this, market, hero), EShortcut::MARKET_RESOURCE_PLAYER);
	if(isButtonVisible(EMarketMode::ARTIFACT_RESOURCE))
		addButton(AnimationPath::builtin("TPMRKBU3.DEF"), CGI->generaltexth->zelp[613], std::bind(&CMarketWindow::createArtifactsSelling, this, market, hero), EShortcut::MARKET_ARTIFACT_RESOURCE);
	if(isButtonVisible(EMarketMode::RESOURCE_ARTIFACT))
		addButton(AnimationPath::builtin("TPMRKBU2.DEF"), CGI->generaltexth->zelp[598], std::bind(&CMarketWindow::createArtifactsBuying, this, market, hero), EShortcut::MARKET_RESOURCE_ARTIFACT);

	buttonPos = Point(516, 520 - buttonHeightWithMargin);
	if(isButtonVisible(EMarketMode::CREATURE_RESOURCE))
		addButton(AnimationPath::builtin("TPMRKBU4.DEF"), CGI->generaltexth->zelp[599], std::bind(&CMarketWindow::createFreelancersGuild, this, market, hero), EShortcut::MARKET_CREATURE_RESOURCE);
	if(isButtonVisible(EMarketMode::RESOURCE_RESOURCE))
		addButton(AnimationPath::builtin("TPMRKBU5.DEF"), CGI->generaltexth->zelp[605], std::bind(&CMarketWindow::createMarketResources, this, market, hero), EShortcut::MARKET_RESOURCE_RESOURCE);
	
	buttonPos = Point(516, 421);
	if(isButtonVisible(EMarketMode::CREATURE_EXP))
	{
		addButton(AnimationPath::builtin("ALTSACC.DEF"), CGI->generaltexth->zelp[572], std::bind(&CMarketWindow::createAltarCreatures, this, market, hero), EShortcut::MARKET_CREATURE_EXPERIENCE);
		if(marketWidget->hero->getAlignment() == EAlignment::GOOD)
			changeModeButtons.back()->block(true);
	}
	if(isButtonVisible(EMarketMode::ARTIFACT_EXP))
	{
		addButton(AnimationPath::builtin("ALTART.DEF"), CGI->generaltexth->zelp[580], std::bind(&CMarketWindow::createAltarArtifacts, this, market, hero), EShortcut::MARKET_ARTIFACT_EXPERIENCE);
		if(marketWidget->hero->getAlignment() == EAlignment::EVIL)
			changeModeButtons.back()->block(true);
	}
}

void CMarketWindow::initWidgetInternals(const EMarketMode mode, const std::pair<std::string, std::string> & quitButtonHelpContainer)
{
	background->center();
	pos = background->pos;
	marketWidget->setRedrawParent(true);
	marketWidget->moveTo(pos.topLeft());

	createChangeModeButtons(mode, marketWidget->market, marketWidget->hero);
	quitButton = std::make_shared<CButton>(quitButtonPos, AnimationPath::builtin("IOK6432.DEF"),
		quitButtonHelpContainer, [this](){close();}, EShortcut::GLOBAL_RETURN);
	redraw();
}

void CMarketWindow::createArtifactsBuying(const IMarket * market, const CGHeroInstance * hero)
{
	OBJECT_CONSTRUCTION;

	background = createBg(ImagePath::builtin("TPMRKABS.bmp"), PLAYER_COLORED);
	marketWidget = std::make_shared<CArtifactsBuying>(market, hero);
	initWidgetInternals(EMarketMode::RESOURCE_ARTIFACT, CGI->generaltexth->zelp[600]);
}

void CMarketWindow::createArtifactsSelling(const IMarket * market, const CGHeroInstance * hero)
{
	OBJECT_CONSTRUCTION;

	background = createBg(ImagePath::builtin("TPMRKASS.bmp"), PLAYER_COLORED);
	// Create image that copies part of background containing slot MISC_1 into position of slot MISC_5
	artSlotBack = std::make_shared<CPicture>(background->getSurface(), Rect(20, 187, 47, 47), 0, 0);
	artSlotBack->moveTo(pos.topLeft() + Point(18, 339));
	auto artsSellingMarket = std::make_shared<CArtifactsSelling>(market, hero);
	artSets.clear();
	const auto heroArts = artsSellingMarket->getAOHset();
	heroArts->showPopupCallback = [this, heroArts](CArtPlace & artPlace, const Point & cursorPosition){showArifactInfo(*heroArts, artPlace, cursorPosition);};
	addSet(heroArts);
	marketWidget = artsSellingMarket;
	initWidgetInternals(EMarketMode::ARTIFACT_RESOURCE, CGI->generaltexth->zelp[600]);	
}

void CMarketWindow::createMarketResources(const IMarket * market, const CGHeroInstance * hero)
{
	OBJECT_CONSTRUCTION;

	background = createBg(ImagePath::builtin("TPMRKRES.bmp"), PLAYER_COLORED);
	marketWidget = std::make_shared<CMarketResources>(market, hero);
	initWidgetInternals(EMarketMode::RESOURCE_RESOURCE, CGI->generaltexth->zelp[600]);
}

void CMarketWindow::createFreelancersGuild(const IMarket * market, const CGHeroInstance * hero)
{
	OBJECT_CONSTRUCTION;

	background = createBg(ImagePath::builtin("TPMRKCRS.bmp"), PLAYER_COLORED);
	marketWidget = std::make_shared<CFreelancerGuild>(market, hero);
	initWidgetInternals(EMarketMode::CREATURE_RESOURCE, CGI->generaltexth->zelp[600]);
}

void CMarketWindow::createTransferResources(const IMarket * market, const CGHeroInstance * hero)
{
	OBJECT_CONSTRUCTION;

	background = createBg(ImagePath::builtin("TPMRKPTS.bmp"), PLAYER_COLORED);
	marketWidget = std::make_shared<CTransferResources>(market, hero);
	initWidgetInternals(EMarketMode::RESOURCE_PLAYER, CGI->generaltexth->zelp[600]);
}

void CMarketWindow::createAltarArtifacts(const IMarket * market, const CGHeroInstance * hero)
{
	OBJECT_CONSTRUCTION;

	background = createBg(ImagePath::builtin("ALTRART2.bmp"), PLAYER_COLORED);
	auto altarArtifacts = std::make_shared<CAltarArtifacts>(market, hero);
	marketWidget = altarArtifacts;
	artSets.clear();
	const auto heroArts = altarArtifacts->getAOHset();
	heroArts->clickPressedCallback = [this, heroArts](const CArtPlace & artPlace, const Point & cursorPosition)
	{
		clickPressedOnArtPlace(heroArts->getHero(), artPlace.slot, true, true, false);
	};
	heroArts->showPopupCallback = [this, heroArts](CArtPlace & artPlace, const Point & cursorPosition)
	{
		showArtifactAssembling(*heroArts, artPlace, cursorPosition);
	};
	heroArts->gestureCallback = [this, heroArts](const CArtPlace & artPlace, const Point & cursorPosition)
	{
		showQuickBackpackWindow(heroArts->getHero(), artPlace.slot, cursorPosition);
	};
	addSet(heroArts);
	initWidgetInternals(EMarketMode::ARTIFACT_EXP, CGI->generaltexth->zelp[568]);
	updateExperience();
	quitButton->addCallback([altarArtifacts](){altarArtifacts->putBackArtifacts();});
}

void CMarketWindow::createAltarCreatures(const IMarket * market, const CGHeroInstance * hero)
{
	OBJECT_CONSTRUCTION;

	background = createBg(ImagePath::builtin("ALTARMON.bmp"), PLAYER_COLORED);
	marketWidget = std::make_shared<CAltarCreatures>(market, hero);
	initWidgetInternals(EMarketMode::CREATURE_EXP, CGI->generaltexth->zelp[568]);
	updateExperience();
}
