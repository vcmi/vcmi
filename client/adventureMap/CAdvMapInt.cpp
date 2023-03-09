/*
 * CAdvMapInt.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CAdvMapInt.h"

#include "CAdvMapPanel.h"
#include "CAdventureOptions.h"
#include "CInGameConsole.h"
#include "CMinimap.h"
#include "CResDataBar.h"
#include "CList.h"
#include "CInfoBar.h"
#include "MapAudioPlayer.h"

#include "../mapView/mapHandler.h"
#include "../mapView/MapView.h"
#include "../windows/CKingdomInterface.h"
#include "../windows/CSpellWindow.h"
#include "../windows/CTradeWindow.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"
#include "../CGameInfo.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../lobby/CSavingScreen.h"
#include "../render/CAnimation.h"
#include "../gui/CursorHandler.h"
#include "../render/IImage.h"
#include "../gui/CGuiHandler.h"
#include "../widgets/TextControls.h"
#include "../widgets/Buttons.h"
#include "../windows/settings/SettingsMainWindow.h"
#include "../CMT.h"

#include "../../CCallback.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/CPathfinder.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/UnlockGuard.h"
#include "../../lib/TerrainHandler.h"

#define ADVOPT (conf.go()->ac)

std::shared_ptr<CAdvMapInt> adventureInt;

void CAdvMapInt::setScrollingCursor(ui8 direction) const
{
	if(direction & CAdvMapInt::RIGHT)
	{
		if(direction & CAdvMapInt::UP)
			CCS->curh->set(Cursor::Map::SCROLL_NORTHEAST);
		else if(direction & CAdvMapInt::DOWN)
			CCS->curh->set(Cursor::Map::SCROLL_SOUTHEAST);
		else
			CCS->curh->set(Cursor::Map::SCROLL_EAST);
	}
	else if(direction & CAdvMapInt::LEFT)
	{
		if(direction & CAdvMapInt::UP)
			CCS->curh->set(Cursor::Map::SCROLL_NORTHWEST);
		else if(direction & CAdvMapInt::DOWN)
			CCS->curh->set(Cursor::Map::SCROLL_SOUTHWEST);
		else
			CCS->curh->set(Cursor::Map::SCROLL_WEST);
	}
	else if(direction & CAdvMapInt::UP)
		CCS->curh->set(Cursor::Map::SCROLL_NORTH);
	else if(direction & CAdvMapInt::DOWN)
		CCS->curh->set(Cursor::Map::SCROLL_SOUTH);
}

CAdvMapInt::CAdvMapInt():
	mode(EAdvMapMode::NORMAL),
	minimap(new CMinimap(Rect(ADVOPT.minimapX, ADVOPT.minimapY, ADVOPT.minimapW, ADVOPT.minimapH))),
	statusbar(CGStatusBar::create(ADVOPT.statusbarX,ADVOPT.statusbarY,ADVOPT.statusbarG)),
	heroList(new CHeroList(ADVOPT.hlistSize, Point(ADVOPT.hlistX, ADVOPT.hlistY), ADVOPT.hlistAU, ADVOPT.hlistAD)),
	townList(new CTownList(ADVOPT.tlistSize, Point(ADVOPT.tlistX, ADVOPT.tlistY), ADVOPT.tlistAU, ADVOPT.tlistAD)),
	infoBar(new CInfoBar(Point(ADVOPT.infoboxX, ADVOPT.infoboxY))),
	resdatabar(new CResDataBar),
	mapAudio(new MapAudioPlayer()),
	terrain(new MapView(Point(ADVOPT.advmapX, ADVOPT.advmapY), Point(ADVOPT.advmapW, ADVOPT.advmapH))),
	state(NA),
	spellBeingCasted(nullptr),
	selection(nullptr),
	activeMapPanel(nullptr),
	duringAITurn(false),
	scrollingDir(0),
	scrollingState(false)
{
	pos.x = pos.y = 0;
	pos.w = GH.screenDimensions().x;
	pos.h = GH.screenDimensions().y;
	strongInterest = true; // handle all mouse move events to prevent dead mouse move space in fullscreen mode
	townList->onSelect = std::bind(&CAdvMapInt::selectionChanged,this);
	bg = IImage::createFromFile(ADVOPT.mainGraphic);
	if(!ADVOPT.worldViewGraphic.empty())
	{
		bgWorldView = IImage::createFromFile(ADVOPT.worldViewGraphic);
	}
	else
	{
		bgWorldView = nullptr;
		logGlobal->warn("ADVOPT.worldViewGraphic is empty => bitmap not loaded");
	}
	if (!bgWorldView)
	{
		logGlobal->warn("bgWorldView not defined in resolution config; fallback to VWorld.bmp");
		bgWorldView = IImage::createFromFile("VWorld.bmp");
	}

	worldViewIcons = std::make_shared<CAnimation>("VwSymbol");//todo: customize with ADVOPT
	worldViewIcons->preload();

	for(int g = 0; g < ADVOPT.gemG.size(); ++g)
	{
		gems.push_back(std::make_shared<CAnimImage>(ADVOPT.gemG[g], 0, 0, ADVOPT.gemX[g], ADVOPT.gemY[g]));
	}

	auto makeButton = [&](int textID, std::function<void()> callback, config::ButtonInfo info, int key) -> std::shared_ptr<CButton>
	{
		auto button = std::make_shared<CButton>(Point(info.x, info.y), info.defName, CGI->generaltexth->zelp[textID], callback, key, info.playerColoured);
		for(auto image : info.additionalDefs)
			button->addImage(image);
		return button;
	};

	kingOverview = makeButton(293, std::bind(&CAdvMapInt::fshowOverview,this),     ADVOPT.kingOverview, SDLK_k);
	underground  = makeButton(294, std::bind(&CAdvMapInt::fswitchLevel,this),      ADVOPT.underground,  SDLK_u);
	questlog     = makeButton(295, std::bind(&CAdvMapInt::fshowQuestlog,this),     ADVOPT.questlog,     SDLK_q);
	sleepWake    = makeButton(296, std::bind(&CAdvMapInt::fsleepWake,this),        ADVOPT.sleepWake,    SDLK_w);
	moveHero     = makeButton(297, std::bind(&CAdvMapInt::fmoveHero,this),         ADVOPT.moveHero,     SDLK_m);
	spellbook    = makeButton(298, std::bind(&CAdvMapInt::fshowSpellbok,this),     ADVOPT.spellbook,    SDLK_c);
	advOptions   = makeButton(299, std::bind(&CAdvMapInt::fadventureOPtions,this), ADVOPT.advOptions,   SDLK_a);
	sysOptions   = makeButton(300, std::bind(&CAdvMapInt::fsystemOptions,this),    ADVOPT.sysOptions,   SDLK_o);
	nextHero     = makeButton(301, std::bind(&CAdvMapInt::fnextHero,this),         ADVOPT.nextHero,     SDLK_h);
	endTurn      = makeButton(302, std::bind(&CAdvMapInt::fendTurn,this),          ADVOPT.endTurn,      SDLK_e);

	int panelSpaceBottom = GH.screenDimensions().y - resdatabar->pos.h - 4;

	panelMain = std::make_shared<CAdvMapPanel>(nullptr, Point(0, 0));
	// TODO correct drawing position
	panelWorldView = std::make_shared<CAdvMapWorldViewPanel>(worldViewIcons, bgWorldView, Point(heroList->pos.x - 2, 195), panelSpaceBottom, LOCPLINT->playerID);

	panelMain->addChildColorableButton(kingOverview);
	panelMain->addChildColorableButton(underground);
	panelMain->addChildColorableButton(questlog);
	panelMain->addChildColorableButton(sleepWake);
	panelMain->addChildColorableButton(moveHero);
	panelMain->addChildColorableButton(spellbook);
	panelMain->addChildColorableButton(advOptions);
	panelMain->addChildColorableButton(sysOptions);
	panelMain->addChildColorableButton(nextHero);
	panelMain->addChildColorableButton(endTurn);


	// TODO move configs to resolutions.json, similarly to previous buttons
	config::ButtonInfo worldViewBackConfig = config::ButtonInfo();
	worldViewBackConfig.defName = "IOK6432.DEF";
	worldViewBackConfig.x = GH.screenDimensions().x - 73;
	worldViewBackConfig.y = 343 + 195;
	worldViewBackConfig.playerColoured = false;
	panelWorldView->addChildToPanel(
		makeButton(288, std::bind(&CAdvMapInt::fworldViewBack,this), worldViewBackConfig, SDLK_ESCAPE), ACTIVATE | DEACTIVATE);

	config::ButtonInfo worldViewPuzzleConfig = config::ButtonInfo();
	worldViewPuzzleConfig.defName = "VWPUZ.DEF";
	worldViewPuzzleConfig.x = GH.screenDimensions().x - 188;
	worldViewPuzzleConfig.y = 343 + 195;
	worldViewPuzzleConfig.playerColoured = false;
	panelWorldView->addChildToPanel( // no help text for this one
		std::make_shared<CButton>(Point(worldViewPuzzleConfig.x, worldViewPuzzleConfig.y), worldViewPuzzleConfig.defName, std::pair<std::string, std::string>(),
				std::bind(&CPlayerInterface::showPuzzleMap,LOCPLINT), SDLK_p, worldViewPuzzleConfig.playerColoured), ACTIVATE | DEACTIVATE);

	config::ButtonInfo worldViewScale1xConfig = config::ButtonInfo();
	worldViewScale1xConfig.defName = "VWMAG1.DEF";
	worldViewScale1xConfig.x = GH.screenDimensions().x - 191;
	worldViewScale1xConfig.y = 23 + 195;
	worldViewScale1xConfig.playerColoured = false;
	panelWorldView->addChildToPanel( // help text is wrong for this button
		makeButton(291, std::bind(&CAdvMapInt::fworldViewScale1x,this), worldViewScale1xConfig, SDLK_1), ACTIVATE | DEACTIVATE);

	config::ButtonInfo worldViewScale2xConfig = config::ButtonInfo();
	worldViewScale2xConfig.defName = "VWMAG2.DEF";
	worldViewScale2xConfig.x = GH.screenDimensions().x- 191 + 63;
	worldViewScale2xConfig.y = 23 + 195;
	worldViewScale2xConfig.playerColoured = false;
	panelWorldView->addChildToPanel( // help text is wrong for this button
		makeButton(291, std::bind(&CAdvMapInt::fworldViewScale2x,this), worldViewScale2xConfig, SDLK_2), ACTIVATE | DEACTIVATE);

	config::ButtonInfo worldViewScale4xConfig = config::ButtonInfo();
	worldViewScale4xConfig.defName = "VWMAG4.DEF";
	worldViewScale4xConfig.x = GH.screenDimensions().x- 191 + 126;
	worldViewScale4xConfig.y = 23 + 195;
	worldViewScale4xConfig.playerColoured = false;
	panelWorldView->addChildToPanel( // help text is wrong for this button
		makeButton(291, std::bind(&CAdvMapInt::fworldViewScale4x,this), worldViewScale4xConfig, SDLK_4), ACTIVATE | DEACTIVATE);

	config::ButtonInfo worldViewUndergroundConfig = config::ButtonInfo();
	worldViewUndergroundConfig.defName = "IAM010.DEF";
	worldViewUndergroundConfig.additionalDefs.push_back("IAM003.DEF");
	worldViewUndergroundConfig.x = GH.screenDimensions().x - 115;
	worldViewUndergroundConfig.y = 343 + 195;
	worldViewUndergroundConfig.playerColoured = true;
	worldViewUnderground = makeButton(294, std::bind(&CAdvMapInt::fswitchLevel,this), worldViewUndergroundConfig, SDLK_u);
	panelWorldView->addChildColorableButton(worldViewUnderground);

	setPlayer(LOCPLINT->playerID);

	int iconColorMultiplier = player.getNum() * 19;
	int wvLeft = heroList->pos.x - 2; // TODO correct drawing position
	//int wvTop = 195;
	for (int i = 0; i < 5; ++i)
	{
		panelWorldView->addChildIcon(std::pair<int, Point>(i, Point(5, 58 + i * 20)), iconColorMultiplier);
		panelWorldView->addChildToPanel(std::make_shared<CLabel>(wvLeft + 45, 263 + i * 20, EFonts::FONT_SMALL, ETextAlignment::TOPLEFT,
												Colors::WHITE, CGI->generaltexth->allTexts[612 + i]));
	}
	for (int i = 0; i < 7; ++i)
	{
		panelWorldView->addChildIcon(std::pair<int, Point>(i +  5, Point(5, 182 + i * 20)), iconColorMultiplier);
		panelWorldView->addChildIcon(std::pair<int, Point>(i + 12, Point(160, 182 + i * 20)), iconColorMultiplier);
		panelWorldView->addChildToPanel(std::make_shared<CLabel>(wvLeft + 45, 387 + i * 20, EFonts::FONT_SMALL, ETextAlignment::TOPLEFT,
												Colors::WHITE, CGI->generaltexth->allTexts[619 + i]));
	}
	panelWorldView->addChildToPanel(std::make_shared<CLabel>(wvLeft +   5, 367, EFonts::FONT_SMALL, ETextAlignment::TOPLEFT,
											Colors::WHITE, CGI->generaltexth->allTexts[617]));
	panelWorldView->addChildToPanel(std::make_shared<CLabel>(wvLeft + 45, 367, EFonts::FONT_SMALL, ETextAlignment::TOPLEFT,
											Colors::WHITE, CGI->generaltexth->allTexts[618]));

	activeMapPanel = panelMain;

	exitWorldView();

	underground->block(!CGI->mh->getMap()->twoLevel);
	questlog->block(!CGI->mh->getMap()->quests.size());
	worldViewUnderground->block(!CGI->mh->getMap()->twoLevel);

	addUsedEvents(MOVE);
}

void CAdvMapInt::fshowOverview()
{
	GH.pushIntT<CKingdomInterface>();
}

void CAdvMapInt::fworldViewBack()
{
	exitWorldView();

	auto hero = curHero();
	if (hero)
		centerOnObject(hero);
}

void CAdvMapInt::fworldViewScale1x()
{
	// TODO set corresponding scale button to "selected" mode
	openWorldView(7);
}

void CAdvMapInt::fworldViewScale2x()
{
	openWorldView(11);
}

void CAdvMapInt::fworldViewScale4x()
{
	openWorldView(16);
}

void CAdvMapInt::fswitchLevel()
{
	// with support for future multi-level maps :)
	int maxLevels = CGI->mh->getMap()->levels();
	if (maxLevels < 2)
		return;

	terrain->onMapLevelSwitched();
}

void CAdvMapInt::onMapViewMoved(const Rect & visibleArea, int mapLevel)
{
	underground->setIndex(mapLevel, true);
	underground->redraw();

	worldViewUnderground->setIndex(mapLevel, true);
	worldViewUnderground->redraw();

	minimap->onMapViewMoved(visibleArea, mapLevel);
}

void CAdvMapInt::onAudioResumed()
{
	mapAudio->onAudioResumed();
}

void CAdvMapInt::onAudioPaused()
{
	mapAudio->onAudioPaused();
}

void CAdvMapInt::fshowQuestlog()
{
	LOCPLINT->showQuestLog();
}

void CAdvMapInt::fsleepWake()
{
	const CGHeroInstance *h = curHero();
	if (!h)
		return;
	bool newSleep = !isHeroSleeping(h);
	setHeroSleeping(h, newSleep);
	updateSleepWake(h);
	if (newSleep)
	{
		fnextHero();

		//moveHero.block(true);
		//uncomment to enable original HoMM3 behaviour:
		//move button is disabled for hero going to sleep, even though it's enabled when you reselect him
	}

	// redraw to update the image of sleep/wake button
	panelMain->redraw();
}

void CAdvMapInt::fmoveHero()
{
	const CGHeroInstance *h = curHero();
	if (!h || !LOCPLINT->paths.hasPath(h) || CGI->mh->hasOngoingAnimations())
		return;

	LOCPLINT->moveHero(h, LOCPLINT->paths.getPath(h));
}

void CAdvMapInt::fshowSpellbok()
{
	if (!curHero()) //checking necessary values
		return;

	centerOnObject(selection);

	GH.pushIntT<CSpellWindow>(curHero(), LOCPLINT, false);
}

void CAdvMapInt::fadventureOPtions()
{
	GH.pushIntT<CAdventureOptions>();
}

void CAdvMapInt::fsystemOptions()
{
	GH.pushIntT<SettingsMainWindow>();
}

void CAdvMapInt::fnextHero()
{
	auto hero = dynamic_cast<const CGHeroInstance*>(selection);
	int next = getNextHeroIndex(vstd::find_pos(LOCPLINT->wanderingHeroes, hero));
	if (next < 0)
		return;
	select(LOCPLINT->wanderingHeroes[next], true);
}

void CAdvMapInt::fendTurn()
{
	if(!LOCPLINT->makingTurn)
		return;

	if(settings["adventure"]["heroReminder"].Bool())
	{
		for(auto hero : LOCPLINT->wanderingHeroes)
		{
			if(!isHeroSleeping(hero) && hero->movement > 0)
			{
				// Only show hero reminder if conditions met:
				// - There still movement points
				// - Hero don't have a path or there not points for first step on path
				LOCPLINT->paths.verifyPath(hero);

				if(!LOCPLINT->paths.hasPath(hero))
				{
					LOCPLINT->showYesNoDialog( CGI->generaltexth->allTexts[55], std::bind(&CAdvMapInt::endingTurn, this), nullptr );
					return;
				}

				auto path = LOCPLINT->paths.getPath(hero);
				if (path.nodes.size() < 2 || path.nodes[path.nodes.size() - 2].turns)
				{
					LOCPLINT->showYesNoDialog( CGI->generaltexth->allTexts[55], std::bind(&CAdvMapInt::endingTurn, this), nullptr );
					return;
				}
			}
		}
	}
	endingTurn();
}

void CAdvMapInt::updateSleepWake(const CGHeroInstance *h)
{
	sleepWake->block(!h);
	if (!h)
		return;
	bool state = isHeroSleeping(h);
	sleepWake->setIndex(state ? 1 : 0, true);
	sleepWake->assignedKeys.clear();
	sleepWake->assignedKeys.insert(state ? SDLK_w : SDLK_z);
}

void CAdvMapInt::updateMoveHero(const CGHeroInstance *h, tribool hasPath)
{
	if(!h)
	{
		moveHero->block(true);
		return;
	}
	//default value is for everywhere but CPlayerInterface::moveHero, because paths are not updated from there immediately
	if(boost::logic::indeterminate(hasPath))
		hasPath = LOCPLINT->paths.hasPath(h);

	moveHero->block(!(bool)hasPath || (h->movement == 0));
}

void CAdvMapInt::updateSpellbook(const CGHeroInstance *h)
{
	spellbook->block(!h);
}

int CAdvMapInt::getNextHeroIndex(int startIndex)
{
	if (LOCPLINT->wanderingHeroes.size() == 0)
		return -1;
	if (startIndex < 0)
		startIndex = 0;
	int i = startIndex;
	do
	{
		i++;
		if (i >= LOCPLINT->wanderingHeroes.size())
			i = 0;
	}
	while (((LOCPLINT->wanderingHeroes[i]->movement == 0) || isHeroSleeping(LOCPLINT->wanderingHeroes[i])) && (i != startIndex));

	if ((LOCPLINT->wanderingHeroes[i]->movement != 0) && !isHeroSleeping(LOCPLINT->wanderingHeroes[i]))
		return i;
	else
		return -1;
}

void CAdvMapInt::updateNextHero(const CGHeroInstance *h)
{
	int start = vstd::find_pos(LOCPLINT->wanderingHeroes, h);
	int next = getNextHeroIndex(start);
	if (next < 0)
	{
		nextHero->block(true);
		return;
	}
	const CGHeroInstance *nextH = LOCPLINT->wanderingHeroes[next];
	bool noActiveHeroes = (next == start) && ((nextH->movement == 0) || isHeroSleeping(nextH));
	nextHero->block(noActiveHeroes);
}

void CAdvMapInt::activate()
{
	CIntObject::activate();
	if (!(active & KEYBOARD))
		CIntObject::activate(KEYBOARD);

	screenBuf = screen;
	GH.statusbar = statusbar;
	
	if(LOCPLINT)
	{
		LOCPLINT->cingconsole->activate();
		LOCPLINT->cingconsole->pos = this->pos;
	}
	
	if(!duringAITurn)
	{
		activeMapPanel->activate();
		if (mode == EAdvMapMode::NORMAL)
		{
			heroList->activate();
			townList->activate();
			infoBar->activate();
		}
		minimap->activate();
		terrain->activate();
		statusbar->activate();

		GH.fakeMouseMove(); //to restore the cursor
	}
}

void CAdvMapInt::deactivate()
{
	CIntObject::deactivate();

	if(!duringAITurn)
	{
		scrollingDir = 0;

		CCS->curh->set(Cursor::Map::POINTER);
		activeMapPanel->deactivate();
		if (mode == EAdvMapMode::NORMAL)
		{
			heroList->deactivate();
			townList->deactivate();
			infoBar->deactivate();
		}
		minimap->deactivate();
		terrain->deactivate();
		statusbar->deactivate();
	}
}

void CAdvMapInt::showAll(SDL_Surface * to)
{
	bg->draw(to, 0, 0);

	if(state != INGAME)
		return;

	switch (mode)
	{
	case EAdvMapMode::NORMAL:

		heroList->showAll(to);
		townList->showAll(to);
		infoBar->showAll(to);
		break;
	case EAdvMapMode::WORLD_VIEW:
		break;
	}
	activeMapPanel->showAll(to);

	minimap->showAll(to);
	terrain->showAll(to);
	show(to);

	resdatabar->showAll(to);
	statusbar->show(to);
	LOCPLINT->cingconsole->show(to);
}

bool CAdvMapInt::isHeroSleeping(const CGHeroInstance *hero)
{
	if (!hero)
		return false;

	return vstd::contains(LOCPLINT->sleepingHeroes, hero);
}

void CAdvMapInt::setHeroSleeping(const CGHeroInstance *hero, bool sleep)
{
	if (sleep)
		LOCPLINT->sleepingHeroes.push_back(hero); //FIXME: should we check for existence?
	else
		LOCPLINT->sleepingHeroes -= hero;
	updateNextHero(nullptr);
}

void CAdvMapInt::show(SDL_Surface * to)
{
	if(state != INGAME)
		return;

	handleMapScrollingUpdate();

	for(int i = 0; i < 4; i++)
	{
		if(settings["session"]["spectate"].Bool())
			gems[i]->setFrame(PlayerColor(1).getNum());
		else
			gems[i]->setFrame(LOCPLINT->playerID.getNum());
	}

	minimap->show(to);
	terrain->show(to);

	for(int i = 0; i < 4; i++)
		gems[i]->showAll(to);

	LOCPLINT->cingconsole->show(to);

	infoBar->show(to);
	statusbar->showAll(to);
}

void CAdvMapInt::handleMapScrollingUpdate()
{
	uint32_t timePassed = GH.mainFPSmng->getElapsedMilliseconds();
	double scrollSpeedPixels = settings["adventure"]["scrollSpeedPixels"].Float();
	int32_t scrollDistance = static_cast<int32_t>(scrollSpeedPixels * timePassed / 1000);
	//if advmap needs updating AND (no dialog is shown OR ctrl is pressed)

	if(scrollingDir & LEFT)
		terrain->onMapScrolled(Point(-scrollDistance, 0));

	if(scrollingDir & RIGHT)
		terrain->onMapScrolled(Point(+scrollDistance, 0));

	if(scrollingDir & UP)
		terrain->onMapScrolled(Point(0, -scrollDistance));

	if(scrollingDir & DOWN)
		terrain->onMapScrolled(Point(0, +scrollDistance));

	if(scrollingDir)
	{
		setScrollingCursor(scrollingDir);
		scrollingState = true;
	}
	else if(scrollingState)
	{
		CCS->curh->set(Cursor::Map::POINTER);
		scrollingState = false;
	}
}

void CAdvMapInt::selectionChanged()
{
	const CGTownInstance *to = LOCPLINT->towns[townList->getSelectedIndex()];
	if (selection != to)
		select(to);
}

void CAdvMapInt::centerOnTile(int3 on)
{
	terrain->onCenteredTile(on);
}

void CAdvMapInt::centerOnObject(const CGObjectInstance * obj)
{
	terrain->onCenteredObject(obj);
}

void CAdvMapInt::keyReleased(const SDL_Keycode &key)
{
	if (mode != EAdvMapMode::NORMAL)
		return;

	switch (key)
	{
		case SDLK_s:
			if(isActive())
				GH.pushIntT<CSavingScreen>();
			return;
		default:
		{
			auto direction = keyToMoveDirection(key);

			if (!direction)
				return;

			ui8 Dir = (direction->x<0 ? LEFT  : 0) |
				  (direction->x>0 ? RIGHT : 0) |
				  (direction->y<0 ? UP    : 0) |
				  (direction->y>0 ? DOWN  : 0) ;

			scrollingDir &= ~Dir;
		}
	}
}

void CAdvMapInt::keyPressed(const SDL_Keycode & key)
{
	if (mode != EAdvMapMode::NORMAL)
		return;

	const CGHeroInstance *h = curHero(); //selected hero
	const CGTownInstance *t = curTown(); //selected town

	switch(key)
	{
	case SDLK_g:
		if(GH.topInt()->type & BLOCK_ADV_HOTKEYS)
			return;

		{
			//find first town with tavern
			auto itr = range::find_if(LOCPLINT->towns, [](const CGTownInstance * town)
			{
				return town->hasBuilt(BuildingID::TAVERN);
			});

			if(itr != LOCPLINT->towns.end())
				LOCPLINT->showThievesGuildWindow(*itr);
			else
				LOCPLINT->showInfoDialog(CGI->generaltexth->translate("vcmi.adventureMap.noTownWithTavern"));
		}
		return;
	case SDLK_i:
		if(isActive())
			CAdventureOptions::showScenarioInfo();
		return;
	case SDLK_l:
		if(isActive())
			LOCPLINT->proposeLoadingGame();
		return;
	case SDLK_d:
		{
			if(h && isActive() && LOCPLINT->makingTurn)
				LOCPLINT->tryDiggging(h);
			return;
		}
	case SDLK_p:
		if(isActive())
			LOCPLINT->showPuzzleMap();
		return;
	case SDLK_v:
		if(isActive())
			LOCPLINT->viewWorldMap();
		return;
	case SDLK_r:
		if(isActive() && GH.isKeyboardCtrlDown())
		{
			LOCPLINT->showYesNoDialog(CGI->generaltexth->translate("vcmi.adventureMap.confirmRestartGame"),
				[](){ GH.pushUserEvent(EUserEvent::RESTART_GAME); }, nullptr);
		}
		return;
	case SDLK_SPACE: //space - try to revisit current object with selected hero
		{
			if(!isActive())
				return;
			if(h)
			{
				LOCPLINT->cb->moveHero(h,h->pos);
			}
		}
		return;
	case SDLK_RETURN:
		{
			if(!isActive() || !selection)
				return;
			if(h)
				LOCPLINT->openHeroWindow(h);
			else if(t)
				LOCPLINT->openTownWindow(t);
			return;
		}
	case SDLK_ESCAPE:
		{
			//FIXME: this case is never executed since AdvMapInt is disabled while in spellcasting mode
			if(!isActive() || GH.topInt().get() != this || !spellBeingCasted)
				return;

			leaveCastingMode();
			return;
		}
	case SDLK_t:
		{
			//act on key down if marketplace windows is not already opened
			if(GH.topInt()->type & BLOCK_ADV_HOTKEYS)
				return;

			if(GH.isKeyboardCtrlDown()) //CTRL + T => open marketplace
			{
				//check if we have any marketplace
				const CGTownInstance *townWithMarket = nullptr;
				for(const CGTownInstance *t : LOCPLINT->cb->getTownsInfo())
				{
					if(t->hasBuilt(BuildingID::MARKETPLACE))
					{
						townWithMarket = t;
						break;
					}
				}

				if(townWithMarket) //if any town has marketplace, open window
					GH.pushIntT<CMarketplaceWindow>(townWithMarket);
				else //if not - complain
					LOCPLINT->showInfoDialog(CGI->generaltexth->translate("vcmi.adventureMap.noTownWithMarket"));
			}
			else if(isActive()) //no ctrl, advmapint is on the top => switch to town
			{
				townList->selectNext();
			}
			return;
		}
	default:
		{
			auto direction = keyToMoveDirection(key);

			if (!direction)
				return;

			ui8 Dir = (direction->x<0 ? LEFT  : 0) |
				  (direction->x>0 ? RIGHT : 0) |
				  (direction->y<0 ? UP    : 0) |
				  (direction->y>0 ? DOWN  : 0) ;



			//ctrl makes arrow move screen, not hero
			if(GH.isKeyboardCtrlDown())
			{
				scrollingDir |= Dir;
				return;
			}

			if(!h || !isActive())
				return;

			if (CGI->mh->hasOngoingAnimations())
				return;

			if(*direction == Point(0,0))
			{
				centerOnObject(h);
				return;
			}

			int3 dst = h->visitablePos() + int3(direction->x, direction->y, 0);

			if (!CGI->mh->isInMap((dst)))
				return;

			if ( !LOCPLINT->paths.setPath(h, dst))
				return;

			const CGPath & path = LOCPLINT->paths.getPath(h);

			if (path.nodes.size() > 2)
				updateMoveHero(h);
			else
			if(!path.nodes[0].turns)
				LOCPLINT->moveHero(h, path);
		}

		return;
	}
}

boost::optional<Point> CAdvMapInt::keyToMoveDirection(const SDL_Keycode & key)
{
	switch (key) {
		case SDLK_DOWN:  return Point( 0, +1);
		case SDLK_LEFT:  return Point(-1,  0);
		case SDLK_RIGHT: return Point(+1,  0);
		case SDLK_UP:    return Point( 0, -1);

		case SDLK_KP_1: return Point(-1, +1);
		case SDLK_KP_2: return Point( 0, +1);
		case SDLK_KP_3: return Point(+1, +1);
		case SDLK_KP_4: return Point(-1,  0);
		case SDLK_KP_5: return Point( 0,  0);
		case SDLK_KP_6: return Point(+1,  0);
		case SDLK_KP_7: return Point(-1, -1);
		case SDLK_KP_8: return Point( 0, -1);
		case SDLK_KP_9: return Point(+1, -1);
	}
	return boost::none;
}

void CAdvMapInt::select(const CArmedInstance *sel, bool centerView)
{
	assert(sel);
	if(selection != sel)
		infoBar->popAll();
	selection = sel;
	mapAudio->onSelectionChanged(sel);
	if(centerView)
		centerOnObject(sel);

	if(sel->ID==Obj::TOWN)
	{
		auto town = dynamic_cast<const CGTownInstance*>(sel);

		infoBar->showTownSelection(town);
		townList->select(town);
		heroList->select(nullptr);

		updateSleepWake(nullptr);
		updateMoveHero(nullptr);
		updateSpellbook(nullptr);
	}
	else //hero selected
	{
		auto hero = dynamic_cast<const CGHeroInstance*>(sel);

		infoBar->showHeroSelection(hero);
		heroList->select(hero);
		townList->select(nullptr);

		LOCPLINT->paths.verifyPath(hero);

		updateSleepWake(hero);
		updateMoveHero(hero);
		updateSpellbook(hero);
	}
	townList->redraw();
	heroList->redraw();
}

void CAdvMapInt::mouseMoved( const Point & cursorPosition )
{
	// adventure map scrolling with mouse
	// currently disabled in world view mode (as it is in OH3), but should work correctly if mode check is removed
	if(!GH.isKeyboardCtrlDown() && isActive() && mode == EAdvMapMode::NORMAL)
	{
		if(cursorPosition.x<15)
		{
			scrollingDir |= LEFT;
		}
		else
		{
			scrollingDir &= ~LEFT;
		}
		if(cursorPosition.x > GH.screenDimensions().x - 15)
		{
			scrollingDir |= RIGHT;
		}
		else
		{
			scrollingDir &= ~RIGHT;
		}
		if(cursorPosition.y<15)
		{
			scrollingDir |= UP;
		}
		else
		{
			scrollingDir &= ~UP;
		}
		if(cursorPosition.y > GH.screenDimensions().y - 15)
		{
			scrollingDir |= DOWN;
		}
		else
		{
			scrollingDir &= ~DOWN;
		}
	}
}

bool CAdvMapInt::isActive()
{
	return active & ~CIntObject::KEYBOARD;
}

void CAdvMapInt::startHotSeatWait(PlayerColor Player)
{
	state = WAITING;
}

void CAdvMapInt::setPlayer(PlayerColor Player)
{
	if (Player == player)
		return;

	player = Player;
	bg->playerColored(player);

	panelMain->setPlayerColor(player);
	panelWorldView->setPlayerColor(player);
	panelWorldView->recolorIcons(player, player.getNum() * 19);
	resdatabar->colorize(player);
}

void CAdvMapInt::startTurn()
{
	state = INGAME;
	if(LOCPLINT->cb->getCurrentPlayer() == LOCPLINT->playerID
		|| settings["session"]["spectate"].Bool())
	{
		adjustActiveness(false);
		minimap->setAIRadar(false);
	}
}

void CAdvMapInt::initializeNewTurn()
{
	heroList->update();
	townList->update();
	mapAudio->onPlayerTurnStarted();

	const CGHeroInstance * heroToSelect = nullptr;

	// find first non-sleeping hero
	for (auto hero : LOCPLINT->wanderingHeroes)
	{
		if (boost::range::find(LOCPLINT->sleepingHeroes, hero) == LOCPLINT->sleepingHeroes.end())
		{
			heroToSelect = hero;
			break;
		}
	}

	bool centerView = !settings["session"]["autoSkip"].Bool();

	//select first hero if available.
	if (heroToSelect != nullptr)
	{
		select(heroToSelect, centerView);
	}
	else if (LOCPLINT->towns.size())
		select(LOCPLINT->towns.front(), centerView);
	else
		select(LOCPLINT->wanderingHeroes.front());

	//show new day animation and sound on infobar
	infoBar->showDate();

	updateNextHero(nullptr);
	showAll(screen);

	if(settings["session"]["autoSkip"].Bool() && !GH.isKeyboardShiftDown())
	{
		if(CInfoWindow *iw = dynamic_cast<CInfoWindow *>(GH.topInt().get()))
			iw->close();

		endingTurn();
	}
}

void CAdvMapInt::endingTurn()
{
	if(settings["session"]["spectate"].Bool())
		return;

	LOCPLINT->makingTurn = false;
	LOCPLINT->cb->endTurn();
	mapAudio->onPlayerTurnEnded();
}

const CGObjectInstance* CAdvMapInt::getActiveObject(const int3 &mapPos)
{
	std::vector < const CGObjectInstance * > bobjs = LOCPLINT->cb->getBlockingObjs(mapPos);  //blocking objects at tile

	if (bobjs.empty())
		return nullptr;

	return *boost::range::max_element(bobjs, &CMapHandler::compareObjectBlitOrder);
/*
	if (bobjs.back()->ID == Obj::HERO)
		return bobjs.back();
	else
		return bobjs.front();*/
}

void CAdvMapInt::onTileLeftClicked(const int3 &mapPos)
{
	if(mode != EAdvMapMode::NORMAL)
		return;

	//FIXME: this line breaks H3 behavior for Dimension Door
	if(!LOCPLINT->cb->isVisible(mapPos))
		return;
	if(!LOCPLINT->makingTurn)
		return;

	const TerrainTile *tile = LOCPLINT->cb->getTile(mapPos);

	const CGObjectInstance *topBlocking = getActiveObject(mapPos);

	int3 selPos = selection->getSightCenter();
	if(spellBeingCasted && isInScreenRange(selPos, mapPos))
	{
		const TerrainTile *heroTile = LOCPLINT->cb->getTile(selPos);

		switch(spellBeingCasted->id)
		{
		case SpellID::SCUTTLE_BOAT: //Scuttle Boat
			if(topBlocking && topBlocking->ID == Obj::BOAT)
				leaveCastingMode(true, mapPos);
			break;
		case SpellID::DIMENSION_DOOR:
			if(!tile || tile->isClear(heroTile))
				leaveCastingMode(true, mapPos);
			break;
		}
		return;
	}
	//check if we can select this object
	bool canSelect = topBlocking && topBlocking->ID == Obj::HERO && topBlocking->tempOwner == LOCPLINT->playerID;
	canSelect |= topBlocking && topBlocking->ID == Obj::TOWN && LOCPLINT->cb->getPlayerRelations(LOCPLINT->playerID, topBlocking->tempOwner);

	bool isHero = false;
	if(selection->ID != Obj::HERO) //hero is not selected (presumably town)
	{
		if(selection == topBlocking) //selected town clicked
			LOCPLINT->openTownWindow(static_cast<const CGTownInstance*>(topBlocking));
		else if(canSelect)
			select(static_cast<const CArmedInstance*>(topBlocking), false);
	}
	else if(const CGHeroInstance * currentHero = curHero()) //hero is selected
	{
		isHero = true;

		const CGPathNode *pn = LOCPLINT->cb->getPathsInfo(currentHero)->getPathInfo(mapPos);
		if(currentHero == topBlocking) //clicked selected hero
		{
			LOCPLINT->openHeroWindow(currentHero);
			return;
		}
		else if(canSelect && pn->turns == 255 ) //selectable object at inaccessible tile
		{
			select(static_cast<const CArmedInstance*>(topBlocking), false);
			return;
		}
		else //still here? we need to move hero if we clicked end of already selected path or calculate a new path otherwise
		{
			if(LOCPLINT->paths.hasPath(currentHero) &&
			   LOCPLINT->paths.getPath(currentHero).endPos() == mapPos)//we'll be moving
			{
				if(!CGI->mh->hasOngoingAnimations())
					LOCPLINT->moveHero(currentHero, LOCPLINT->paths.getPath(currentHero));
				return;
			}
			else //remove old path and find a new one if we clicked on accessible tile
			{
				LOCPLINT->paths.setPath(currentHero, mapPos);
				updateMoveHero(currentHero);
			}
		}
	} //end of hero is selected "case"
	else
	{
		throw std::runtime_error("Nothing is selected...");
	}

	const auto shipyard = ourInaccessibleShipyard(topBlocking);
	if(isHero && shipyard != nullptr)
	{
		LOCPLINT->showShipyardDialogOrProblemPopup(shipyard);
	}
}

void CAdvMapInt::onTileHovered(const int3 &mapPos)
{
	if(mode != EAdvMapMode::NORMAL //disable in world view
		|| !selection) //may occur just at the start of game (fake move before full intiialization)
		return;
	if(!LOCPLINT->cb->isVisible(mapPos))
	{
		CCS->curh->set(Cursor::Map::POINTER);
		statusbar->clear();
		return;
	}
	auto objRelations = PlayerRelations::ALLIES;
	const CGObjectInstance *objAtTile = getActiveObject(mapPos);
	if(objAtTile)
	{
		objRelations = LOCPLINT->cb->getPlayerRelations(LOCPLINT->playerID, objAtTile->tempOwner);
		std::string text = curHero() ? objAtTile->getHoverText(curHero()) : objAtTile->getHoverText(LOCPLINT->playerID);
		boost::replace_all(text,"\n"," ");
		statusbar->write(text);
	}
	else
	{
		std::string hlp = CGI->mh->getTerrainDescr(mapPos, false);
		statusbar->write(hlp);
	}

	if(spellBeingCasted)
	{
		switch(spellBeingCasted->id)
		{
		case SpellID::SCUTTLE_BOAT:
			if(objAtTile && objAtTile->ID == Obj::BOAT)
				CCS->curh->set(Cursor::Map::SCUTTLE_BOAT);
			else
				CCS->curh->set(Cursor::Map::POINTER);
			return;
		case SpellID::DIMENSION_DOOR:
			{
				const TerrainTile * t = LOCPLINT->cb->getTile(mapPos, false);
				int3 hpos = selection->getSightCenter();
				if((!t || t->isClear(LOCPLINT->cb->getTile(hpos))) && isInScreenRange(hpos, mapPos))
					CCS->curh->set(Cursor::Map::TELEPORT);
				else
					CCS->curh->set(Cursor::Map::POINTER);
				return;
			}
		}
	}

	if(selection->ID == Obj::TOWN)
	{
		if(objAtTile)
		{
			if(objAtTile->ID == Obj::TOWN && objRelations != PlayerRelations::ENEMIES)
				CCS->curh->set(Cursor::Map::TOWN);
			else if(objAtTile->ID == Obj::HERO && objRelations == PlayerRelations::SAME_PLAYER)
				CCS->curh->set(Cursor::Map::HERO);
			else
				CCS->curh->set(Cursor::Map::POINTER);
		}
		else
			CCS->curh->set(Cursor::Map::POINTER);
	}
	else if(const CGHeroInstance * hero = curHero())
	{
		std::array<Cursor::Map, 4> cursorMove      = { Cursor::Map::T1_MOVE,       Cursor::Map::T2_MOVE,       Cursor::Map::T3_MOVE,       Cursor::Map::T4_MOVE,       };
		std::array<Cursor::Map, 4> cursorAttack    = { Cursor::Map::T1_ATTACK,     Cursor::Map::T2_ATTACK,     Cursor::Map::T3_ATTACK,     Cursor::Map::T4_ATTACK,     };
		std::array<Cursor::Map, 4> cursorSail      = { Cursor::Map::T1_SAIL,       Cursor::Map::T2_SAIL,       Cursor::Map::T3_SAIL,       Cursor::Map::T4_SAIL,       };
		std::array<Cursor::Map, 4> cursorDisembark = { Cursor::Map::T1_DISEMBARK,  Cursor::Map::T2_DISEMBARK,  Cursor::Map::T3_DISEMBARK,  Cursor::Map::T4_DISEMBARK,  };
		std::array<Cursor::Map, 4> cursorExchange  = { Cursor::Map::T1_EXCHANGE,   Cursor::Map::T2_EXCHANGE,   Cursor::Map::T3_EXCHANGE,   Cursor::Map::T4_EXCHANGE,   };
		std::array<Cursor::Map, 4> cursorVisit     = { Cursor::Map::T1_VISIT,      Cursor::Map::T2_VISIT,      Cursor::Map::T3_VISIT,      Cursor::Map::T4_VISIT,      };
		std::array<Cursor::Map, 4> cursorSailVisit = { Cursor::Map::T1_SAIL_VISIT, Cursor::Map::T2_SAIL_VISIT, Cursor::Map::T3_SAIL_VISIT, Cursor::Map::T4_SAIL_VISIT, };

		const CGPathNode * pathNode = LOCPLINT->cb->getPathsInfo(hero)->getPathInfo(mapPos);
		assert(pathNode);

		if((GH.isKeyboardAltDown() || settings["gameTweaks"]["forceMovementInfo"].Bool()) && pathNode->reachable()) //overwrite status bar text with movement info
		{
			showMoveDetailsInStatusbar(*hero, *pathNode);
		}

		int turns = pathNode->turns;
		vstd::amin(turns, 3);
		switch(pathNode->action)
		{
		case CGPathNode::NORMAL:
		case CGPathNode::TELEPORT_NORMAL:
			if(pathNode->layer == EPathfindingLayer::LAND)
				CCS->curh->set(cursorMove[turns]);
			else
				CCS->curh->set(cursorSailVisit[turns]);
			break;

		case CGPathNode::VISIT:
		case CGPathNode::BLOCKING_VISIT:
		case CGPathNode::TELEPORT_BLOCKING_VISIT:
			if(objAtTile && objAtTile->ID == Obj::HERO)
			{
				if(selection == objAtTile)
					CCS->curh->set(Cursor::Map::HERO);
				else
					CCS->curh->set(cursorExchange[turns]);
			}
			else if(pathNode->layer == EPathfindingLayer::LAND)
				CCS->curh->set(cursorVisit[turns]);
			else
				CCS->curh->set(cursorSailVisit[turns]);
			break;

		case CGPathNode::BATTLE:
		case CGPathNode::TELEPORT_BATTLE:
			CCS->curh->set(cursorAttack[turns]);
			break;

		case CGPathNode::EMBARK:
			CCS->curh->set(cursorSail[turns]);
			break;

		case CGPathNode::DISEMBARK:
			CCS->curh->set(cursorDisembark[turns]);
			break;

		default:
			if(objAtTile && objRelations != PlayerRelations::ENEMIES)
			{
				if(objAtTile->ID == Obj::TOWN)
					CCS->curh->set(Cursor::Map::TOWN);
				else if(objAtTile->ID == Obj::HERO && objRelations == PlayerRelations::SAME_PLAYER)
					CCS->curh->set(Cursor::Map::HERO);
				else
					CCS->curh->set(Cursor::Map::POINTER);
			}
			else
				CCS->curh->set(Cursor::Map::POINTER);
			break;
		}
	}

	if(ourInaccessibleShipyard(objAtTile))
	{
		CCS->curh->set(Cursor::Map::T1_SAIL);
	}
}

void CAdvMapInt::showMoveDetailsInStatusbar(const CGHeroInstance & hero, const CGPathNode & pathNode)
{
	const int maxMovementPointsAtStartOfLastTurn = pathNode.turns > 0 ? hero.maxMovePoints(pathNode.layer == EPathfindingLayer::LAND) : hero.movement;
	const int movementPointsLastTurnCost = maxMovementPointsAtStartOfLastTurn - pathNode.moveRemains;
	const int remainingPointsAfterMove = pathNode.turns == 0 ? pathNode.moveRemains : 0;

	std::string result = VLC->generaltexth->translate("vcmi.adventureMap", pathNode.turns > 0 ? "moveCostDetails" : "moveCostDetailsNoTurns");

	boost::replace_first(result, "%TURNS", std::to_string(pathNode.turns));
	boost::replace_first(result, "%POINTS", std::to_string(movementPointsLastTurnCost));
	boost::replace_first(result, "%REMAINING", std::to_string(remainingPointsAfterMove));

	statusbar->write(result);
}

void CAdvMapInt::onTileRightClicked(const int3 &mapPos)
{
	if(mode != EAdvMapMode::NORMAL)
		return;
	if(spellBeingCasted)
	{
		leaveCastingMode();
		return;
	}
	if(!LOCPLINT->cb->isVisible(mapPos))
	{
		CRClickPopup::createAndPush(VLC->generaltexth->allTexts[61]); //Uncharted Territory
		return;
	}

	const CGObjectInstance * obj = getActiveObject(mapPos);
	if(!obj)
	{
		// Bare or undiscovered terrain
		const TerrainTile * tile = LOCPLINT->cb->getTile(mapPos);
		if(tile)
		{
			std::string hlp = CGI->mh->getTerrainDescr(mapPos, true);
			CRClickPopup::createAndPush(hlp);
		}
		return;
	}

	CRClickPopup::createAndPush(obj, GH.getCursorPosition(), ETextAlignment::CENTER);
}

void CAdvMapInt::enterCastingMode(const CSpell * sp)
{
	assert(sp->id == SpellID::SCUTTLE_BOAT  ||  sp->id == SpellID::DIMENSION_DOOR);
	spellBeingCasted = sp;

	deactivate();
	terrain->activate();
	GH.fakeMouseMove();
}

void CAdvMapInt::leaveCastingMode(bool cast, int3 dest)
{
	assert(spellBeingCasted);
	SpellID id = spellBeingCasted->id;
	spellBeingCasted = nullptr;
	terrain->deactivate();
	activate();

	if(cast)
		LOCPLINT->cb->castSpell(curHero(), id, dest);
	else
		LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[731]); //Spell cancelled
}

const CGHeroInstance * CAdvMapInt::curHero() const
{
	if(selection && selection->ID == Obj::HERO)
		return dynamic_cast<const CGHeroInstance *>(selection);
	else
		return nullptr;
}

const CGTownInstance * CAdvMapInt::curTown() const
{
	if(selection && selection->ID == Obj::TOWN)
		return dynamic_cast<const CGTownInstance *>(selection);
	else
		return nullptr;
}

const CArmedInstance * CAdvMapInt::curArmy() const
{
	if (selection)
		return dynamic_cast<const CArmedInstance *>(selection);
	else
		return nullptr;
}

Rect CAdvMapInt::terrainAreaPixels() const
{
	return terrain->pos;
}

const IShipyard * CAdvMapInt::ourInaccessibleShipyard(const CGObjectInstance *obj) const
{
	const IShipyard *ret = IShipyard::castFrom(obj);

	if(!ret ||
		obj->tempOwner != player ||
		(CCS->curh->get<Cursor::Map>() != Cursor::Map::T1_SAIL && CCS->curh->get<Cursor::Map>() != Cursor::Map::POINTER))
		return nullptr;

	return ret;
}

void CAdvMapInt::aiTurnStarted()
{
	if(settings["session"]["spectate"].Bool())
		return;

	adjustActiveness(true);
	mapAudio->onEnemyTurnStarted();
	adventureInt->minimap->setAIRadar(true);
	adventureInt->infoBar->startEnemyTurn(LOCPLINT->cb->getCurrentPlayer());
	adventureInt->infoBar->showAll(screen);//force refresh on inactive object
}

void CAdvMapInt::adjustActiveness(bool aiTurnStart)
{
	bool wasActive = isActive();

	if(wasActive)
		deactivate();
	adventureInt->duringAITurn = aiTurnStart;
	if(wasActive)
		activate();
}

void CAdvMapInt::quickCombatLock()
{
	if(!duringAITurn)
		deactivate();
}

void CAdvMapInt::quickCombatUnlock()
{
	if(!duringAITurn)
		activate();
}

void CAdvMapInt::exitWorldView()
{
	mode = EAdvMapMode::NORMAL;

	panelMain->activate();
	panelWorldView->deactivate();
	activeMapPanel = panelMain;

	townList->activate();
	heroList->activate();
	infoBar->activate();

	redraw();
	terrain->onViewMapActivated();
}

void CAdvMapInt::openWorldView(int tileSize)
{
	mode = EAdvMapMode::WORLD_VIEW;
	panelMain->deactivate();
	panelWorldView->activate();

	activeMapPanel = panelWorldView;

	townList->deactivate();
	heroList->deactivate();
	infoBar->showSelection(); // to prevent new day animation interfering world view mode
	infoBar->deactivate();

	redraw();
	terrain->onViewWorldActivated(tileSize);
}

void CAdvMapInt::openWorldView()
{
	openWorldView(11);
}

void CAdvMapInt::openWorldView(const std::vector<ObjectPosInfo>& objectPositions, bool showTerrain)
{
	openWorldView(11);
	terrain->onViewSpellActivated(11, objectPositions, showTerrain);
}
