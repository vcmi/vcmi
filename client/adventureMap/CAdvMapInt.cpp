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
#include "mapHandler.h"

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

#include <SDL_surface.h>
#include <SDL_events.h>

#define ADVOPT (conf.go()->ac)

std::shared_ptr<CAdvMapInt> adventureInt;

static void setScrollingCursor(ui8 direction)
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
	worldViewScale(0.0f), //actual init later in changeMode
	minimap(Rect(ADVOPT.minimapX, ADVOPT.minimapY, ADVOPT.minimapW, ADVOPT.minimapH)),
	statusbar(CGStatusBar::create(ADVOPT.statusbarX,ADVOPT.statusbarY,ADVOPT.statusbarG)),
	heroList(ADVOPT.hlistSize, Point(ADVOPT.hlistX, ADVOPT.hlistY), ADVOPT.hlistAU, ADVOPT.hlistAD),
	townList(ADVOPT.tlistSize, Point(ADVOPT.tlistX, ADVOPT.tlistY), ADVOPT.tlistAU, ADVOPT.tlistAD),
	infoBar(Rect(ADVOPT.infoboxX, ADVOPT.infoboxY, 192, 192)), state(NA),
	spellBeingCasted(nullptr), position(int3(0, 0, 0)), selection(nullptr),
	updateScreen(false), anim(0), animValHitCount(0), heroAnim(0), heroAnimValHitCount(0),
	activeMapPanel(nullptr), duringAITurn(false), scrollingDir(0), scrollingState(false),
	swipeEnabled(settings["general"]["swipe"].Bool()), swipeMovementRequested(false),
	swipeTargetPosition(int3(-1, -1, -1))
{
	pos.x = pos.y = 0;
	pos.w = screen->w;
	pos.h = screen->h;
	strongInterest = true; // handle all mouse move events to prevent dead mouse move space in fullscreen mode
	townList.onSelect = std::bind(&CAdvMapInt::selectionChanged,this);
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

	int panelSpaceBottom = screen->h - resdatabar.pos.h - 4;

	panelMain = std::make_shared<CAdvMapPanel>(nullptr, Point(0, 0));
	// TODO correct drawing position
	panelWorldView = std::make_shared<CAdvMapWorldViewPanel>(worldViewIcons, bgWorldView, Point(heroList.pos.x - 2, 195), panelSpaceBottom, LOCPLINT->playerID);

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
	worldViewBackConfig.x = screen->w - 73;
	worldViewBackConfig.y = 343 + 195;
	worldViewBackConfig.playerColoured = false;
	panelWorldView->addChildToPanel(
		makeButton(288, std::bind(&CAdvMapInt::fworldViewBack,this), worldViewBackConfig, SDLK_ESCAPE), ACTIVATE | DEACTIVATE);

	config::ButtonInfo worldViewPuzzleConfig = config::ButtonInfo();
	worldViewPuzzleConfig.defName = "VWPUZ.DEF";
	worldViewPuzzleConfig.x = screen->w - 188;
	worldViewPuzzleConfig.y = 343 + 195;
	worldViewPuzzleConfig.playerColoured = false;
	panelWorldView->addChildToPanel( // no help text for this one
		std::make_shared<CButton>(Point(worldViewPuzzleConfig.x, worldViewPuzzleConfig.y), worldViewPuzzleConfig.defName, std::pair<std::string, std::string>(),
				std::bind(&CPlayerInterface::showPuzzleMap,LOCPLINT), SDLK_p, worldViewPuzzleConfig.playerColoured), ACTIVATE | DEACTIVATE);

	config::ButtonInfo worldViewScale1xConfig = config::ButtonInfo();
	worldViewScale1xConfig.defName = "VWMAG1.DEF";
	worldViewScale1xConfig.x = screen->w - 191;
	worldViewScale1xConfig.y = 23 + 195;
	worldViewScale1xConfig.playerColoured = false;
	panelWorldView->addChildToPanel( // help text is wrong for this button
		makeButton(291, std::bind(&CAdvMapInt::fworldViewScale1x,this), worldViewScale1xConfig, SDLK_1), ACTIVATE | DEACTIVATE);

	config::ButtonInfo worldViewScale2xConfig = config::ButtonInfo();
	worldViewScale2xConfig.defName = "VWMAG2.DEF";
	worldViewScale2xConfig.x = screen->w - 191 + 63;
	worldViewScale2xConfig.y = 23 + 195;
	worldViewScale2xConfig.playerColoured = false;
	panelWorldView->addChildToPanel( // help text is wrong for this button
		makeButton(291, std::bind(&CAdvMapInt::fworldViewScale2x,this), worldViewScale2xConfig, SDLK_2), ACTIVATE | DEACTIVATE);

	config::ButtonInfo worldViewScale4xConfig = config::ButtonInfo();
	worldViewScale4xConfig.defName = "VWMAG4.DEF";
	worldViewScale4xConfig.x = screen->w - 191 + 126;
	worldViewScale4xConfig.y = 23 + 195;
	worldViewScale4xConfig.playerColoured = false;
	panelWorldView->addChildToPanel( // help text is wrong for this button
		makeButton(291, std::bind(&CAdvMapInt::fworldViewScale4x,this), worldViewScale4xConfig, SDLK_4), ACTIVATE | DEACTIVATE);

	config::ButtonInfo worldViewUndergroundConfig = config::ButtonInfo();
	worldViewUndergroundConfig.defName = "IAM010.DEF";
	worldViewUndergroundConfig.additionalDefs.push_back("IAM003.DEF");
	worldViewUndergroundConfig.x = screen->w - 115;
	worldViewUndergroundConfig.y = 343 + 195;
	worldViewUndergroundConfig.playerColoured = true;
	worldViewUnderground = makeButton(294, std::bind(&CAdvMapInt::fswitchLevel,this), worldViewUndergroundConfig, SDLK_u);
	panelWorldView->addChildColorableButton(worldViewUnderground);

	setPlayer(LOCPLINT->playerID);

	int iconColorMultiplier = player.getNum() * 19;
	int wvLeft = heroList.pos.x - 2; // TODO correct drawing position
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

	changeMode(EAdvMapMode::NORMAL);

	underground->block(!CGI->mh->map->twoLevel);
	questlog->block(!CGI->mh->map->quests.size());
	worldViewUnderground->block(!CGI->mh->map->twoLevel);

	addUsedEvents(MOVE);
}

void CAdvMapInt::fshowOverview()
{
	GH.pushIntT<CKingdomInterface>();
}

void CAdvMapInt::fworldViewBack()
{
	changeMode(EAdvMapMode::NORMAL);
	CGI->mh->discardWorldViewCache();

	auto hero = curHero();
	if (hero)
		centerOn(hero);
}

void CAdvMapInt::fworldViewScale1x()
{
	// TODO set corresponding scale button to "selected" mode
	changeMode(EAdvMapMode::WORLD_VIEW, 0.22f);
}

void CAdvMapInt::fworldViewScale2x()
{
	changeMode(EAdvMapMode::WORLD_VIEW, 0.36f);
}

void CAdvMapInt::fworldViewScale4x()
{
	changeMode(EAdvMapMode::WORLD_VIEW, 0.5f);
}

void CAdvMapInt::fswitchLevel()
{
	// with support for future multi-level maps :)
	int maxLevels = CGI->mh->map->levels();
	if (maxLevels < 2)
		return;

	position.z = (position.z + 1) % maxLevels;

	underground->setIndex(position.z, true);
	underground->redraw();

	worldViewUnderground->setIndex(position.z, true);
	worldViewUnderground->redraw();

	updateScreen = true;
	minimap.setLevel(position.z);

	if (mode == EAdvMapMode::WORLD_VIEW)
		terrain.redraw();
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
}

void CAdvMapInt::fmoveHero()
{
	const CGHeroInstance *h = curHero();
	if (!h || !terrain.currentPath || !CGI->mh->canStartHeroMovement())
		return;

	LOCPLINT->moveHero(h, *terrain.currentPath);
}

void CAdvMapInt::fshowSpellbok()
{
	if (!curHero()) //checking necessary values
		return;

	centerOn(selection);

	GH.pushIntT<CSpellWindow>(curHero(), LOCPLINT, false);
}

void CAdvMapInt::fadventureOPtions()
{
	GH.pushIntT<CAdventureOptions>();
}

void CAdvMapInt::fsystemOptions()
{
	GH.pushIntT<CSystemOptionsWindow>();
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
				auto path = LOCPLINT->getAndVerifyPath(hero);
				if(!path || path->nodes.size() < 2 || !path->nodes[path->nodes.size()-2].turns)
				{
					LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[55], std::bind(&CAdvMapInt::endingTurn, this), nullptr);
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
		hasPath = LOCPLINT->paths[h].nodes.size() ? true : false;

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
			heroList.activate();
			townList.activate();
			infoBar.activate();
		}
		minimap.activate();
		terrain.activate();
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
			heroList.deactivate();
			townList.deactivate();
			infoBar.deactivate();
		}
		minimap.deactivate();
		terrain.deactivate();
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

		heroList.showAll(to);
		townList.showAll(to);
		infoBar.showAll(to);
		break;
	case EAdvMapMode::WORLD_VIEW:

		terrain.showAll(to);
		break;
	}
	activeMapPanel->showAll(to);

	updateScreen = true;
	minimap.showAll(to);
	show(to);


	resdatabar.showAll(to);

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

	++animValHitCount; //for animations

	if(animValHitCount % 2 == 0)
	{
		++heroAnim;
	}
	if(animValHitCount >= 8)
	{
		CGI->mh->updateWater();
		animValHitCount = 0;
		++anim;
		updateScreen = true;
	}

	if(swipeEnabled)
	{
		handleSwipeUpdate();
	}
#if defined(VCMI_ANDROID) || defined(VCMI_IOS) // on mobile, map-moving mode is exclusive (TODO technically it might work with both enabled; to be checked)
	else
#endif
	{
		handleMapScrollingUpdate();
	}

	for(int i = 0; i < 4; i++)
	{
		if(settings["session"]["spectate"].Bool())
			gems[i]->setFrame(PlayerColor(1).getNum());
		else
			gems[i]->setFrame(LOCPLINT->playerID.getNum());
	}
	if(updateScreen)
	{
		int3 betterPos = LOCPLINT->repairScreenPos(position);
		if (betterPos != position)
		{
			logGlobal->warn("Incorrect position for adventure map!");
			position = betterPos;
		}

		terrain.show(to);
		for(int i = 0; i < 4; i++)
			gems[i]->showAll(to);
		updateScreen=false;
		LOCPLINT->cingconsole->show(to);
	}
	else if (terrain.needsAnimUpdate())
	{
		terrain.showAnim(to);
		for(int i = 0; i < 4; i++)
			gems[i]->showAll(to);
	}

	infoBar.show(to);
	statusbar->showAll(to);
}

void CAdvMapInt::handleMapScrollingUpdate()
{
	int scrollSpeed = static_cast<int>(settings["adventure"]["scrollSpeed"].Float());
	//if advmap needs updating AND (no dialog is shown OR ctrl is pressed)
	if((animValHitCount % (4 / scrollSpeed)) == 0
	   && ((GH.topInt().get() == this) || CSDL_Ext::isCtrlKeyDown()))
	{
		if((scrollingDir & LEFT) && (position.x > -CGI->mh->frameW))
			position.x--;

		if((scrollingDir & RIGHT) && (position.x < CGI->mh->map->width - CGI->mh->tilesW + CGI->mh->frameW))
			position.x++;

		if((scrollingDir & UP) && (position.y > -CGI->mh->frameH))
			position.y--;

		if((scrollingDir & DOWN) && (position.y < CGI->mh->map->height - CGI->mh->tilesH + CGI->mh->frameH))
			position.y++;

		if(scrollingDir)
		{
			setScrollingCursor(scrollingDir);
			scrollingState = true;
			updateScreen = true;
			minimap.redraw();
			if(mode == EAdvMapMode::WORLD_VIEW)
				terrain.redraw();
		}
		else if(scrollingState)
		{
			CCS->curh->set(Cursor::Map::POINTER);
			scrollingState = false;
		}
	}
}

void CAdvMapInt::handleSwipeUpdate()
{
	if(swipeMovementRequested)
	{
		auto fixedPos = LOCPLINT->repairScreenPos(swipeTargetPosition);
		position.x = fixedPos.x;
		position.y = fixedPos.y;
		CCS->curh->set(Cursor::Map::POINTER);
		updateScreen = true;
		minimap.redraw();
		swipeMovementRequested = false;
	}
}

void CAdvMapInt::selectionChanged()
{
	const CGTownInstance *to = LOCPLINT->towns[townList.getSelectedIndex()];
	if (selection != to)
		select(to);
}

void CAdvMapInt::centerOn(int3 on, bool fade)
{
	bool switchedLevels = on.z != position.z;

	if (fade)
	{
		terrain.fadeFromCurrentView();
	}

	switch (mode)
	{
	default:
	case EAdvMapMode::NORMAL:
		on.x -= CGI->mh->frameW; // is this intentional? frame size doesn't really have to correspond to camera size...
		on.y -= CGI->mh->frameH;
		break;
	case EAdvMapMode::WORLD_VIEW:
		on.x -= static_cast<si32>(CGI->mh->tilesW / 2 / worldViewScale);
		on.y -= static_cast<si32>(CGI->mh->tilesH / 2 / worldViewScale);
		break;
	}


	on = LOCPLINT->repairScreenPos(on);

	position = on;
	updateScreen=true;
	underground->setIndex(on.z,true); //change underground switch button image
	underground->redraw();
	worldViewUnderground->setIndex(on.z, true);
	worldViewUnderground->redraw();
	if (switchedLevels)
		minimap.setLevel(position.z);
	minimap.redraw();

	if (mode == EAdvMapMode::WORLD_VIEW)
		terrain.redraw();
}

void CAdvMapInt::centerOn(const CGObjectInstance * obj, bool fade)
{
	centerOn(obj->getSightCenter(), fade);
}

void CAdvMapInt::keyPressed(const SDL_KeyboardEvent & key)
{

	if (mode == EAdvMapMode::WORLD_VIEW)
		return;

	ui8 Dir = 0;
	SDL_Keycode k = key.keysym.sym;
	const CGHeroInstance *h = curHero(); //selected hero
	const CGTownInstance *t = curTown(); //selected town

	switch(k)
	{
	case SDLK_g:
		if(key.state != SDL_PRESSED || GH.topInt()->type & BLOCK_ADV_HOTKEYS)
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
	case SDLK_s:
		if(isActive() && key.type == SDL_KEYUP)
			GH.pushIntT<CSavingScreen>();
		return;
	case SDLK_d:
		{
			if(h && isActive() && LOCPLINT->makingTurn && key.state == SDL_PRESSED)
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
		if(isActive() && LOCPLINT->ctrlPressed())
		{
			LOCPLINT->showYesNoDialog(CGI->generaltexth->translate("vcmi.adventureMap.confirmRestartGame"),
				[](){ LOCPLINT->sendCustomEvent(EUserEvent::RESTART_GAME); }, nullptr);
		}
		return;
	case SDLK_SPACE: //space - try to revisit current object with selected hero
		{
			if(!isActive())
				return;
			if(h && key.state == SDL_PRESSED)
			{
				auto unlockPim = vstd::makeUnlockGuard(*CPlayerInterface::pim);
				//TODO!!!!!!! possible freeze, when GS mutex is locked and network thread can't apply package
				//this thread leaves scope and tries to lock pim while holding gs,
				//network thread tries to lock gs (appluy cl) while holding pim
				//this thread should first lock pim, however gs locking/unlocking is done inside cb
				LOCPLINT->cb->moveHero(h,h->pos);
			}
		}
		return;
	case SDLK_RETURN:
		{
			if(!isActive() || !selection || key.state != SDL_PRESSED)
				return;
			if(h)
				LOCPLINT->openHeroWindow(h);
			else if(t)
				LOCPLINT->openTownWindow(t);
			return;
		}
	case SDLK_ESCAPE:
		{
			if(isActive() || GH.topInt().get() != this || !spellBeingCasted || key.state != SDL_PRESSED)
				return;

			leaveCastingMode();
			return;
		}
	case SDLK_t:
		{
			//act on key down if marketplace windows is not already opened
			if(key.state != SDL_PRESSED || GH.topInt()->type & BLOCK_ADV_HOTKEYS)
				return;

			if(LOCPLINT->ctrlPressed()) //CTRL + T => open marketplace
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
				townList.selectNext();
			}
			return;
		}
	default:
		{
			static const int3 directions[] = {  int3(-1, +1, 0), int3(0, +1, 0), int3(+1, +1, 0),
												int3(-1, 0, 0),  int3(0, 0, 0),  int3(+1, 0, 0),
												int3(-1, -1, 0), int3(0, -1, 0), int3(+1, -1, 0) };

			//numpad arrow
			if(CGuiHandler::isArrowKey(k))
				k = CGuiHandler::arrowToNum(k);

			k -= SDLK_KP_1;

			if(k < 0 || k > 8)
				return;

			if (!CGI->mh->canStartHeroMovement())
				return;

			int3 dir = directions[k];

			if(!isActive() || LOCPLINT->ctrlPressed())//ctrl makes arrow move screen, not hero
			{
				Dir = (dir.x<0 ? LEFT  : 0) |
					  (dir.x>0 ? RIGHT : 0) |
					  (dir.y<0 ? UP    : 0) |
					  (dir.y>0 ? DOWN  : 0) ;
				break;
			}

			if(!h || key.state != SDL_PRESSED)
				break;

			if(k == 4)
			{
				centerOn(h);
				return;
			}

			CGPath &path = LOCPLINT->paths[h];
			terrain.currentPath = &path;
			int3 dst = h->visitablePos() + dir;
			if(dst != verifyPos(dst) || !LOCPLINT->cb->getPathsInfo(h)->getPath(path, dst))
			{
				terrain.currentPath = nullptr;
				return;
			}

			if (path.nodes.size() > 2)
				updateMoveHero(h);
			else
			if(!path.nodes[0].turns)
				LOCPLINT->moveHero(h, path);
		}

		return;
	}
	if(Dir && key.state == SDL_PRESSED //arrow is pressed
		&& LOCPLINT->ctrlPressed()
	)
		scrollingDir |= Dir;
	else
		scrollingDir &= ~Dir;
}
void CAdvMapInt::handleRightClick(std::string text, tribool down)
{
	if(down)
	{
		CRClickPopup::createAndPush(text);
	}
}
int3 CAdvMapInt::verifyPos(int3 ver)
{
	if (ver.x<0)
		ver.x=0;
	if (ver.y<0)
		ver.y=0;
	if (ver.z<0)
		ver.z=0;
	if (ver.x>=CGI->mh->sizes.x)
		ver.x=CGI->mh->sizes.x-1;
	if (ver.y>=CGI->mh->sizes.y)
		ver.y=CGI->mh->sizes.y-1;
	if (ver.z>=CGI->mh->sizes.z)
		ver.z=CGI->mh->sizes.z-1;
	return ver;
}

void CAdvMapInt::select(const CArmedInstance *sel, bool centerView)
{
	assert(sel);
	LOCPLINT->setSelection(sel);
	selection = sel;
	if (LOCPLINT->battleInt == nullptr && LOCPLINT->makingTurn)
	{
		auto pos = sel->visitablePos();
		auto tile = LOCPLINT->cb->getTile(pos);
		if(tile)
			CCS->musich->playMusicFromSet("terrain", tile->terType->getJsonKey(), true, false);
	}
	if(centerView)
		centerOn(sel);

	terrain.currentPath = nullptr;
	if(sel->ID==Obj::TOWN)
	{
		auto town = dynamic_cast<const CGTownInstance*>(sel);

		infoBar.showTownSelection(town);
		townList.select(town);
		heroList.select(nullptr);

		updateSleepWake(nullptr);
		updateMoveHero(nullptr);
		updateSpellbook(nullptr);
	}
	else //hero selected
	{
		auto hero = dynamic_cast<const CGHeroInstance*>(sel);

		infoBar.showHeroSelection(hero);
		heroList.select(hero);
		townList.select(nullptr);

		terrain.currentPath = LOCPLINT->getAndVerifyPath(hero);

		updateSleepWake(hero);
		updateMoveHero(hero);
		updateSpellbook(hero);
	}
	townList.redraw();
	heroList.redraw();
}

void CAdvMapInt::mouseMoved( const SDL_MouseMotionEvent & sEvent )
{
#if defined(VCMI_ANDROID) || defined(VCMI_IOS)
	if(swipeEnabled)
		return;
#endif
	// adventure map scrolling with mouse
	// currently disabled in world view mode (as it is in OH3), but should work correctly if mode check is removed
	// don't scroll if there is no window in focus - these events don't seem to correspond to the actual mouse movement
	if(!CSDL_Ext::isCtrlKeyDown() && isActive() && sEvent.windowID != 0 && mode == EAdvMapMode::NORMAL)
	{
		if(sEvent.x<15)
		{
			scrollingDir |= LEFT;
		}
		else
		{
			scrollingDir &= ~LEFT;
		}
		if(sEvent.x>screen->w-15)
		{
			scrollingDir |= RIGHT;
		}
		else
		{
			scrollingDir &= ~RIGHT;
		}
		if(sEvent.y<15)
		{
			scrollingDir |= UP;
		}
		else
		{
			scrollingDir &= ~UP;
		}
		if(sEvent.y>screen->h-15)
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
	player = Player;
	bg->playerColored(player);

	panelMain->setPlayerColor(player);
	panelWorldView->setPlayerColor(player);
	panelWorldView->recolorIcons(player, player.getNum() * 19);
	resdatabar.background->colorize(player);
}

void CAdvMapInt::startTurn()
{
	state = INGAME;
	if(LOCPLINT->cb->getCurrentPlayer() == LOCPLINT->playerID
		|| settings["session"]["spectate"].Bool())
	{
		adjustActiveness(false);
		minimap.setAIRadar(false);
	}
}

void CAdvMapInt::endingTurn()
{
	if(settings["session"]["spectate"].Bool())
		return;

	LOCPLINT->makingTurn = false;
	LOCPLINT->cb->endTurn();
	CCS->soundh->ambientStopAllChannels();
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

void CAdvMapInt::tileLClicked(const int3 &mapPos)
{
	if(mode != EAdvMapMode::NORMAL)
		return;
	if(!LOCPLINT->cb->isVisible(mapPos) || !LOCPLINT->makingTurn)
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
		assert(!terrain.currentPath); //path can be active only when hero is selected
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
			if(terrain.currentPath && terrain.currentPath->endPos() == mapPos)//we'll be moving
			{
				if(CGI->mh->canStartHeroMovement())
					LOCPLINT->moveHero(currentHero, *terrain.currentPath);
				return;
			}
			else //remove old path and find a new one if we clicked on accessible tile
			{
				CGPath &path = LOCPLINT->paths[currentHero];
				CGPath newpath;
				bool gotPath = LOCPLINT->cb->getPathsInfo(currentHero)->getPath(newpath, mapPos); //try getting path, erase if failed
				if(gotPath && newpath.nodes.size())
					path = newpath;

				if(path.nodes.size())
					terrain.currentPath = &path;
				else
					LOCPLINT->eraseCurrentPathOf(currentHero);

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

void CAdvMapInt::tileHovered(const int3 &mapPos)
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
		std::string hlp;
		CGI->mh->getTerrainDescr(mapPos, hlp, false);
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

		if(LOCPLINT->altPressed() && pathNode->reachable()) //overwrite status bar text with movement info
		{
			ShowMoveDetailsInStatusbar(*hero, *pathNode);
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

void CAdvMapInt::ShowMoveDetailsInStatusbar(const CGHeroInstance & hero, const CGPathNode & pathNode)
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

void CAdvMapInt::tileRClicked(const int3 &mapPos)
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
		if (tile)
		{
			std::string hlp;
			CGI->mh->getTerrainDescr(mapPos, hlp, true);
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
	terrain.activate();
	GH.fakeMouseMove();
}

void CAdvMapInt::leaveCastingMode(bool cast, int3 dest)
{
	assert(spellBeingCasted);
	SpellID id = spellBeingCasted->id;
	spellBeingCasted = nullptr;
	terrain.deactivate();
	activate();

	if(cast)
		LOCPLINT->cb->castSpell(curHero(), id, dest);
	else
		LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[731]); //Spell cancelled
}

const CGHeroInstance * CAdvMapInt::curHero() const
{
	if(selection && selection->ID == Obj::HERO)
		return static_cast<const CGHeroInstance *>(selection);
	else
		return nullptr;
}

const CGTownInstance * CAdvMapInt::curTown() const
{
	if(selection && selection->ID == Obj::TOWN)
		return static_cast<const CGTownInstance *>(selection);
	else
		return nullptr;
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
	CCS->musich->playMusicFromSet("enemy-turn", true, false);
	adventureInt->minimap.setAIRadar(true);
	adventureInt->infoBar.startEnemyTurn(LOCPLINT->cb->getCurrentPlayer());
	adventureInt->infoBar.showAll(screen);//force refresh on inactive object
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

void CAdvMapInt::changeMode(EAdvMapMode newMode, float newScale)
{
	if (mode != newMode)
	{
		mode = newMode;

		switch (mode)
		{
		case EAdvMapMode::NORMAL:
			panelMain->activate();
			panelWorldView->deactivate();
			activeMapPanel = panelMain;

			townList.activate();
			heroList.activate();
			infoBar.activate();

			worldViewOptions.clear();

			break;
		case EAdvMapMode::WORLD_VIEW:
			panelMain->deactivate();
			panelWorldView->activate();

			activeMapPanel = panelWorldView;

			townList.deactivate();
			heroList.deactivate();
			infoBar.showSelection(); // to prevent new day animation interfering world view mode
			infoBar.deactivate();

			break;
		}
		worldViewScale = newScale;
		redraw();
	}
	else if (worldViewScale != newScale) // still in world view mode, but the scale changed
	{
		worldViewScale = newScale;
		redraw();
	}
}

CAdvMapInt::WorldViewOptions::WorldViewOptions()
{
	clear();
}

void CAdvMapInt::WorldViewOptions::clear()
{
	showAllTerrain = false;

	iconPositions.clear();
}

void CAdvMapInt::WorldViewOptions::adjustDrawingInfo(MapDrawingInfo& info)
{
	info.showAllTerrain = showAllTerrain;

	info.additionalIcons = &iconPositions;
}

