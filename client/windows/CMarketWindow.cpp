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
#include "../widgets/markets/CArtifactsBuying.h"
#include "../widgets/markets/CFreelancerGuild.h"
#include "../widgets/markets/CMarketResources.h"
#include "../widgets/markets/CTransferResources.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"

#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/CGMarket.h"
#include "../../lib/mapObjects/CGHeroInstance.h"

CMarketWindow::CMarketWindow(const IMarket * market, const CGHeroInstance * hero, const std::function<void()> & onWindowClosed, EMarketMode mode)
	: CStatusbarWindow(PLAYER_COLORED)
	, hero(hero)
	, windowClosedCallback(onWindowClosed)
{
	assert(mode == EMarketMode::RESOURCE_RESOURCE || mode == EMarketMode::RESOURCE_PLAYER || mode == EMarketMode::CREATURE_RESOURCE ||
		mode == EMarketMode::RESOURCE_ARTIFACT || mode == EMarketMode::ARTIFACT_RESOURCE || mode == EMarketMode::ARTIFACT_EXP ||
		mode == EMarketMode::CREATURE_EXP);
	
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

	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);
	statusbar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));
}

void CMarketWindow::artifactsChanged()
{
	market->artifactsChanged(false);
}

void CMarketWindow::updateGarrisons()
{
	CAltarWindow::updateGarrisons();

	if(guild)
		guild->updateSlots();
}

void CMarketWindow::resourceChanged()
{
	if(resRes)
		resRes->updateSlots();
	if(trRes)
		trRes->updateSlots();
}

void CMarketWindow::close()
{
	if(windowClosedCallback)
		windowClosedCallback();

	CWindowObject::close();
}

const CGHeroInstance * CMarketWindow::getHero() const
{
	return hero;
}

void CMarketWindow::createChangeModeButtons(EMarketMode currentMode, const IMarket * market, const CGHeroInstance * hero)
{
	auto isButton = [this, currentMode, market, hero](EMarketMode modeButton) -> bool
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

	auto buttonPosY = 520;
	changeModeButtons.clear();

	if(isButton(EMarketMode::RESOURCE_PLAYER))
		changeModeButtons.emplace_back(std::make_shared<CButton>(Point(18, buttonPosY), AnimationPath::builtin("TPMRKBU1.DEF"),
			CGI->generaltexth->zelp[612], std::bind(&CMarketWindow::createTransferResources, this, market, hero)));

	buttonPosY -= buttonHeightWithMargin;
	if(isButton(EMarketMode::ARTIFACT_RESOURCE))
	{
		changeModeButtons.emplace_back(std::make_shared<CButton>(Point(18, buttonPosY), AnimationPath::builtin("TPMRKBU3.DEF"),
			CGI->generaltexth->zelp[613], std::bind(&CMarketWindow::createArtifactsSelling, this, market, hero)));
		buttonPosY -= buttonHeightWithMargin;
	}
	if(isButton(EMarketMode::CREATURE_RESOURCE))
		changeModeButtons.emplace_back(std::make_shared<CButton>(Point(516, buttonPosY), AnimationPath::builtin("TPMRKBU4.DEF"),
			CGI->generaltexth->zelp[599], std::bind(&CMarketWindow::createFreelancersGuild, this, market, hero)));
	if(isButton(EMarketMode::RESOURCE_RESOURCE))
		changeModeButtons.emplace_back(std::make_shared<CButton>(Point(516, buttonPosY), AnimationPath::builtin("TPMRKBU5.DEF"),
			CGI->generaltexth->zelp[605], std::bind(&CMarketWindow::createMarketResources, this, market, hero)));
	if(isButton(EMarketMode::RESOURCE_ARTIFACT))
		changeModeButtons.emplace_back(std::make_shared<CButton>(Point(18, buttonPosY), AnimationPath::builtin("TPMRKBU2.DEF"),
			CGI->generaltexth->zelp[598], std::bind(&CMarketWindow::createArtifactsBuying, this, market, hero)));
	if(isButton(EMarketMode::CREATURE_EXP))
	{
		changeModeButtons.emplace_back(std::make_shared<CButton>(Point(516, 421), AnimationPath::builtin("ALTSACC.DEF"),
			CGI->generaltexth->zelp[572], std::bind(&CMarketWindow::createAltarCreatures, this, market, hero)));
		if(altar->hero->getAlignment() == EAlignment::GOOD)
			changeModeButtons.back()->block(true);
	}
	if(isButton(EMarketMode::ARTIFACT_EXP))
	{
		changeModeButtons.emplace_back(std::make_shared<CButton>(Point(516, 421), AnimationPath::builtin("ALTART.DEF"),
			CGI->generaltexth->zelp[580], std::bind(&CMarketWindow::createAltarArtifacts, this, market, hero)));
		if(altar->hero->getAlignment() == EAlignment::EVIL)
			changeModeButtons.back()->block(true);
	}
}

void CMarketWindow::createInternals(EMarketMode mode, const IMarket * market, const CGHeroInstance * hero)
{
	background->center();
	pos = background->pos;
	this->market->setRedrawParent(true);
	createChangeModeButtons(mode, market, hero);
	quitButton = std::make_shared<CButton>(quitButtonPos, AnimationPath::builtin("IOK6432.DEF"),
		CGI->generaltexth->zelp[600], [this]() {close(); }, EShortcut::GLOBAL_RETURN);
	redraw();
}

void CMarketWindow::createArtifactsBuying(const IMarket * market, const CGHeroInstance * hero)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	background = createBg(ImagePath::builtin("TPMRKABS.bmp"), PLAYER_COLORED);
	//this->market = std::make_shared<CMarketplaceWindow>(market, hero, []() {}, EMarketMode::RESOURCE_ARTIFACT);
	//createInternals(EMarketMode::RESOURCE_ARTIFACT, market, hero);

	artsBuy = std::make_shared<CArtifactsBuying>(market, hero);

	background->center();
	pos = background->pos;
	artsBuy->setRedrawParent(true);
	artsBuy->moveTo(pos.topLeft());

	createChangeModeButtons(EMarketMode::RESOURCE_ARTIFACT, market, hero);
	quitButton = std::make_shared<CButton>(quitButtonPos, AnimationPath::builtin("IOK6432.DEF"),
		CGI->generaltexth->zelp[600], [this]() {close(); }, EShortcut::GLOBAL_RETURN);
	redraw();
}

void CMarketWindow::createArtifactsSelling(const IMarket * market, const CGHeroInstance * hero)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	background = createBg(ImagePath::builtin("TPMRKASS.bmp"), PLAYER_COLORED);
	this->market = std::make_shared<CMarketplaceWindow>(market, hero, []() {}, EMarketMode::ARTIFACT_RESOURCE);
	createInternals(EMarketMode::ARTIFACT_RESOURCE, market, hero);
}

void CMarketWindow::createMarketResources(const IMarket * market, const CGHeroInstance * hero)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	background = createBg(ImagePath::builtin("TPMRKRES.bmp"), PLAYER_COLORED);
	resRes = std::make_shared<CMarketResources>(market, hero);

	background->center();
	pos = background->pos;
	resRes->setRedrawParent(true);
	resRes->moveTo(pos.topLeft());

	createChangeModeButtons(EMarketMode::RESOURCE_RESOURCE, market, hero);
	quitButton = std::make_shared<CButton>(quitButtonPos, AnimationPath::builtin("IOK6432.DEF"),
		CGI->generaltexth->zelp[600], [this]() {close(); }, EShortcut::GLOBAL_RETURN);
	redraw();
}

void CMarketWindow::createFreelancersGuild(const IMarket * market, const CGHeroInstance * hero)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	background = createBg(ImagePath::builtin("TPMRKCRS.bmp"), PLAYER_COLORED);
	guild = std::make_shared<CFreelancerGuild>(market, hero);
	
	background->center();
	pos = background->pos;
	guild->setRedrawParent(true);
	guild->moveTo(pos.topLeft());

	createChangeModeButtons(EMarketMode::CREATURE_RESOURCE, market, hero);
	quitButton = std::make_shared<CButton>(quitButtonPos, AnimationPath::builtin("IOK6432.DEF"),
		CGI->generaltexth->zelp[600], [this]() {close(); }, EShortcut::GLOBAL_RETURN);
	redraw();
}

void CMarketWindow::createTransferResources(const IMarket * market, const CGHeroInstance * hero)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	background = createBg(ImagePath::builtin("TPMRKPTS.bmp"), PLAYER_COLORED);
	trRes = std::make_shared<CTransferResources>(market, hero);

	background->center();
	pos = background->pos;
	trRes->setRedrawParent(true);
	trRes->moveTo(pos.topLeft());

	createChangeModeButtons(EMarketMode::RESOURCE_PLAYER, market, hero);
	quitButton = std::make_shared<CButton>(quitButtonPos, AnimationPath::builtin("IOK6432.DEF"),
		CGI->generaltexth->zelp[600], [this]() {close(); }, EShortcut::GLOBAL_RETURN);
	redraw();
}

void CMarketWindow::createAltarArtifacts(const IMarket * market, const CGHeroInstance * hero)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	background = createBg(ImagePath::builtin("ALTRART2.bmp"), PLAYER_COLORED);
	auto altarArtifacts = std::make_shared<CAltarArtifacts>(market, hero);
	altar = altarArtifacts;
	artSets.clear();
	addSetAndCallbacks(altarArtifacts->getAOHset()); altarArtifacts->putBackArtifacts();

	background->center();
	pos = background->pos;
	altar->setRedrawParent(true);
	createChangeModeButtons(EMarketMode::ARTIFACT_EXP, market, hero);
	altar->moveTo(pos.topLeft());

	quitButton = std::make_shared<CButton>(quitButtonPos, AnimationPath::builtin("IOK6432.DEF"),
		CGI->generaltexth->zelp[568], [this, altarArtifacts]()
		{
			altarArtifacts->putBackArtifacts();
			CMarketWindow::close();
		}, EShortcut::GLOBAL_RETURN);

	updateExpToLevel();
	redraw();
}

void CMarketWindow::createAltarCreatures(const IMarket * market, const CGHeroInstance * hero)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);

	background = createBg(ImagePath::builtin("ALTARMON.bmp"), PLAYER_COLORED);
	altar = std::make_shared<CAltarCreatures>(market, hero);

	background->center();
	pos = background->pos;
	altar->setRedrawParent(true);
	createChangeModeButtons(EMarketMode::CREATURE_EXP, market, hero);
	altar->moveTo(pos.topLeft());

	quitButton = std::make_shared<CButton>(quitButtonPos, AnimationPath::builtin("IOK6432.DEF"),
		CGI->generaltexth->zelp[568], std::bind(&CMarketWindow::close, this), EShortcut::GLOBAL_RETURN);

	updateExpToLevel();
	redraw();
}
