/*
 * CAdventureMapInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CAdventureMapInterface.h"

#include "CAdventureOptions.h"
#include "CInGameConsole.h"
#include "CMinimap.h"
#include "CList.h"
#include "CInfoBar.h"
#include "MapAudioPlayer.h"
#include "CAdventureMapWidget.h"

#include "../mapView/mapHandler.h"
#include "../mapView/MapView.h"
#include "../windows/CKingdomInterface.h"
#include "../windows/CSpellWindow.h"
#include "../windows/CTradeWindow.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"
#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../lobby/CSavingScreen.h"
#include "../render/CAnimation.h"
#include "../gui/CursorHandler.h"
#include "../render/IImage.h"
#include "../renderSDL/SDL_Extensions.h"
#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../widgets/TextControls.h"
#include "../widgets/Buttons.h"
#include "../windows/settings/SettingsMainWindow.h"
#include "../CMT.h"
#include "../PlayerLocalState.h"

#include "../../CCallback.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/CPathfinder.h"
#include "../../lib/mapping/CMap.h"

std::shared_ptr<CAdventureMapInterface> adventureInt;

CAdventureMapInterface::CAdventureMapInterface():
	mapAudio(new MapAudioPlayer()),
	spellBeingCasted(nullptr),
	scrollingCursorSet(false)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pos.x = pos.y = 0;
	pos.w = GH.screenDimensions().x;
	pos.h = GH.screenDimensions().y;
	strongInterest = true; // handle all mouse move events to prevent dead mouse move space in fullscreen mode

	widget = std::make_shared<CAdventureMapWidget>();
	exitWorldView();

	widget->setOptionHasQuests(!CGI->mh->getMap()->quests.empty());
	widget->setOptionHasUnderground(CGI->mh->getMap()->twoLevel);
}

void CAdventureMapInterface::fshowOverview()
{
	GH.pushIntT<CKingdomInterface>();
}

void CAdventureMapInterface::fworldViewBack()
{
	exitWorldView();

	auto hero = LOCPLINT->localState->getCurrentHero();
	if (hero)
		centerOnObject(hero);
}

void CAdventureMapInterface::fworldViewScale1x()
{
	// TODO set corresponding scale button to "selected" mode
	openWorldView(7);
}

void CAdventureMapInterface::fworldViewScale2x()
{
	openWorldView(11);
}

void CAdventureMapInterface::fworldViewScale4x()
{
	openWorldView(16);
}

void CAdventureMapInterface::fswitchLevel()
{
	// with support for future multi-level maps :)
	int maxLevels = CGI->mh->getMap()->levels();
	if (maxLevels < 2)
		return;

	widget->getMapView()->onMapLevelSwitched();
}

void CAdventureMapInterface::onMapViewMoved(const Rect & visibleArea, int mapLevel)
{
	widget->setOptionUndergroundLevel(mapLevel > 0);
	widget->getMinimap()->onMapViewMoved(visibleArea, mapLevel);
}

void CAdventureMapInterface::onAudioResumed()
{
	mapAudio->onAudioResumed();
}

void CAdventureMapInterface::onAudioPaused()
{
	mapAudio->onAudioPaused();
}

void CAdventureMapInterface::fshowQuestlog()
{
	LOCPLINT->showQuestLog();
}

void CAdventureMapInterface::fsleepWake()
{
	const CGHeroInstance *h = LOCPLINT->localState->getCurrentHero();
	if (!h)
		return;
	bool newSleep = !LOCPLINT->localState->isHeroSleeping(h);

	if (newSleep)
		LOCPLINT->localState->setHeroAsleep(h);
	else
		LOCPLINT->localState->setHeroAwaken(h);

	onHeroChanged(h);

	if (newSleep)
		fnextHero();
}

void CAdventureMapInterface::fmoveHero()
{
	const CGHeroInstance *h = LOCPLINT->localState->getCurrentHero();
	if (!h || !LOCPLINT->localState->hasPath(h) || CGI->mh->hasOngoingAnimations())
		return;

	LOCPLINT->moveHero(h, LOCPLINT->localState->getPath(h));
}

void CAdventureMapInterface::fshowSpellbok()
{
	if (!LOCPLINT->localState->getCurrentHero()) //checking necessary values
		return;

	centerOnObject(LOCPLINT->localState->getCurrentHero());

	GH.pushIntT<CSpellWindow>(LOCPLINT->localState->getCurrentHero(), LOCPLINT, false);
}

void CAdventureMapInterface::fadventureOPtions()
{
	GH.pushIntT<CAdventureOptions>();
}

void CAdventureMapInterface::fsystemOptions()
{
	GH.pushIntT<SettingsMainWindow>();
}

void CAdventureMapInterface::fnextHero()
{
	const auto * currHero = LOCPLINT->localState->getCurrentHero();
	const auto * nextHero = LOCPLINT->localState->getNextWanderingHero(currHero);

	if (nextHero)
	{
		LOCPLINT->localState->setSelection(nextHero);
		centerOnObject(nextHero);
	}
}

void CAdventureMapInterface::fendTurn()
{
	if(!LOCPLINT->makingTurn)
		return;

	if(settings["adventure"]["heroReminder"].Bool())
	{
		for(auto hero : LOCPLINT->localState->getWanderingHeroes())
		{
			if(!LOCPLINT->localState->isHeroSleeping(hero) && hero->movement > 0)
			{
				// Only show hero reminder if conditions met:
				// - There still movement points
				// - Hero don't have a path or there not points for first step on path
				LOCPLINT->localState->verifyPath(hero);

				if(!LOCPLINT->localState->hasPath(hero))
				{
					LOCPLINT->showYesNoDialog( CGI->generaltexth->allTexts[55], std::bind(&CAdventureMapInterface::endingTurn, this), nullptr );
					return;
				}

				auto path = LOCPLINT->localState->getPath(hero);
				if (path.nodes.size() < 2 || path.nodes[path.nodes.size() - 2].turns)
				{
					LOCPLINT->showYesNoDialog( CGI->generaltexth->allTexts[55], std::bind(&CAdventureMapInterface::endingTurn, this), nullptr );
					return;
				}
			}
		}
	}
	endingTurn();
}

void CAdventureMapInterface::updateButtons()
{
	const auto * hero = LOCPLINT->localState->getCurrentHero();
	const auto * nextSuitableHero = LOCPLINT->localState->getNextWanderingHero(hero);

	widget->setOptionHeroSelected(hero != nullptr);
	widget->setOptionHeroCanMove(hero && LOCPLINT->localState->hasPath(hero) && hero->movement != 0);
	widget->setOptionHasNextHero(nextSuitableHero != nullptr);
	widget->setOptionHeroSleeping(hero && LOCPLINT->localState->isHeroSleeping(hero));
}

void CAdventureMapInterface::onHeroMovementStarted(const CGHeroInstance * hero)
{
	widget->getInfoBar()->popAll();
	widget->getInfoBar()->showSelection();
}

void CAdventureMapInterface::onHeroChanged(const CGHeroInstance *h)
{
	widget->getHeroList()->update(h);

	if (h && h == LOCPLINT->localState->getCurrentHero() && !widget->getInfoBar()->showingComponents())
		widget->getInfoBar()->showSelection();

	updateButtons();
}

void CAdventureMapInterface::onTownChanged(const CGTownInstance * town)
{
	widget->getTownList()->update(town);

	if (town && town == LOCPLINT->localState->getCurrentTown() && !widget->getInfoBar()->showingComponents())
		widget->getInfoBar()->showSelection();
}

void CAdventureMapInterface::showInfoBoxMessage(const std::vector<Component> & components, std::string message, int timer)
{
	widget->getInfoBar()->pushComponents(components, message, timer);
}

void CAdventureMapInterface::activate()
{
	CIntObject::activate();

	screenBuf = screen;
	
	if(LOCPLINT)
	{
		LOCPLINT->cingconsole->activate();
		LOCPLINT->cingconsole->pos = this->pos;
	}

	GH.fakeMouseMove(); //to restore the cursor
}

void CAdventureMapInterface::deactivate()
{
	CIntObject::deactivate();
	CCS->curh->set(Cursor::Map::POINTER);
}

void CAdventureMapInterface::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	LOCPLINT->cingconsole->show(to);
}

void CAdventureMapInterface::show(SDL_Surface * to)
{
	handleMapScrollingUpdate();

	CIntObject::show(to);
	LOCPLINT->cingconsole->show(to);
}

void CAdventureMapInterface::handleMapScrollingUpdate()
{
	/// Width of window border, in pixels, that triggers map scrolling
	static constexpr uint32_t borderScrollWidth = 15;

	uint32_t timePassed = GH.mainFPSmng->getElapsedMilliseconds();
	uint32_t scrollSpeedPixels = settings["adventure"]["scrollSpeedPixels"].Float();
	uint32_t scrollDistance = scrollSpeedPixels * timePassed / 1000;

	bool scrollingActive = !GH.isKeyboardCtrlDown() && isActive() && widget->getState() == EGameState::MAKING_TURN;

	Point cursorPosition = GH.getCursorPosition();
	Point scrollDirection;

	if (cursorPosition.x < borderScrollWidth)
		scrollDirection.x = -1;

	if (cursorPosition.x > GH.screenDimensions().x - borderScrollWidth)
		scrollDirection.x = +1;

	if (cursorPosition.y < borderScrollWidth)
		scrollDirection.y = -1;

	if (cursorPosition.y > GH.screenDimensions().y - borderScrollWidth)
		scrollDirection.y = +1;

	Point scrollDelta = scrollDirection * scrollDistance;

	if (scrollingActive && scrollDelta != Point(0,0))
		widget->getMapView()->onMapScrolled(scrollDelta);

	if (scrollDelta == Point(0,0) && !scrollingCursorSet)
		return;

	if(scrollDelta.x > 0)
	{
		if(scrollDelta.y < 0)
			CCS->curh->set(Cursor::Map::SCROLL_NORTHEAST);
		if(scrollDelta.y > 0)
			CCS->curh->set(Cursor::Map::SCROLL_SOUTHEAST);
		if(scrollDelta.y == 0)
			CCS->curh->set(Cursor::Map::SCROLL_EAST);
	}
	if(scrollDelta.x < 0)
	{
		if(scrollDelta.y < 0)
			CCS->curh->set(Cursor::Map::SCROLL_NORTHWEST);
		if(scrollDelta.y > 0)
			CCS->curh->set(Cursor::Map::SCROLL_SOUTHWEST);
		if(scrollDelta.y == 0)
			CCS->curh->set(Cursor::Map::SCROLL_WEST);
	}

	if (scrollDelta.x == 0)
	{
		if(scrollDelta.y < 0)
			CCS->curh->set(Cursor::Map::SCROLL_NORTH);
		if(scrollDelta.y > 0)
			CCS->curh->set(Cursor::Map::SCROLL_SOUTH);
		if(scrollDelta.y == 0)
			CCS->curh->set(Cursor::Map::POINTER);
	}

	scrollingCursorSet = scrollDelta != Point(0,0);
}

void CAdventureMapInterface::centerOnTile(int3 on)
{
	widget->getMapView()->onCenteredTile(on);
}

void CAdventureMapInterface::centerOnObject(const CGObjectInstance * obj)
{
	widget->getMapView()->onCenteredObject(obj);
}

void CAdventureMapInterface::keyPressed(EShortcut key)
{
	if (widget->getState() != EGameState::MAKING_TURN)
		return;

	//fake mouse use to trigger onTileHovered()
	GH.fakeMouseMove();

	const CGHeroInstance *h = LOCPLINT->localState->getCurrentHero(); //selected hero
	const CGTownInstance *t = LOCPLINT->localState->getCurrentTown(); //selected town

	switch(key)
	{
	case EShortcut::ADVENTURE_THIEVES_GUILD:
		if(GH.topInt()->type & BLOCK_ADV_HOTKEYS)
			return;

		{
			//find first town with tavern
			auto itr = range::find_if(LOCPLINT->localState->getOwnedTowns(), [](const CGTownInstance * town)
			{
				return town->hasBuilt(BuildingID::TAVERN);
			});

			if(itr != LOCPLINT->localState->getOwnedTowns().end())
				LOCPLINT->showThievesGuildWindow(*itr);
			else
				LOCPLINT->showInfoDialog(CGI->generaltexth->translate("vcmi.adventureMap.noTownWithTavern"));
		}
		return;
	case EShortcut::ADVENTURE_VIEW_SCENARIO:
		if(isActive())
			CAdventureOptions::showScenarioInfo();
		return;
	case EShortcut::GAME_SAVE_GAME:
		if(isActive())
			GH.pushIntT<CSavingScreen>();
		return;
	case EShortcut::GAME_LOAD_GAME:
		if(isActive())
			LOCPLINT->proposeLoadingGame();
		return;
	case EShortcut::ADVENTURE_DIG_GRAIL:
		{
			if(h && isActive() && LOCPLINT->makingTurn)
				LOCPLINT->tryDiggging(h);
			return;
		}
	case EShortcut::ADVENTURE_VIEW_PUZZLE:
		if(isActive())
			LOCPLINT->showPuzzleMap();
		return;
	case EShortcut::ADVENTURE_VIEW_WORLD:
		if(isActive())
			LOCPLINT->viewWorldMap();
		return;
	case EShortcut::GAME_RESTART_GAME:
		if(isActive() && GH.isKeyboardCtrlDown())
		{
			LOCPLINT->showYesNoDialog(CGI->generaltexth->translate("vcmi.adventureMap.confirmRestartGame"),
				[](){ GH.pushUserEvent(EUserEvent::RESTART_GAME); }, nullptr);
		}
		return;
	case EShortcut::ADVENTURE_VISIT_OBJECT: //space - try to revisit current object with selected hero
		{
			if(!isActive())
				return;
			if(h)
			{
				LOCPLINT->cb->moveHero(h,h->pos);
			}
		}
		return;
	case EShortcut::ADVENTURE_VIEW_SELECTED:
		{
			if(!isActive() || !LOCPLINT->localState->getCurrentArmy())
				return;
			if(h)
				LOCPLINT->openHeroWindow(h);
			else if(t)
				LOCPLINT->openTownWindow(t);
			return;
		}
	case EShortcut::GLOBAL_CANCEL:
		{
			//FIXME: this case is never executed since AdvMapInt is disabled while in spellcasting mode
			if(!isActive() || GH.topInt().get() != this || !spellBeingCasted)
				return;

			abortCastingMode();
			return;
		}
	case EShortcut::GAME_OPEN_MARKETPLACE:
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
	case EShortcut::ADVENTURE_NEXT_TOWN:
			if(isActive() && !GH.isKeyboardCtrlDown()) //no ctrl, advmapint is on the top => switch to town
			{
				widget->getTownList()->selectNext();
			}
			return;
		}
	case EShortcut::ADVENTURE_MOVE_HERO_SW: return hotkeyMoveHeroDirectional({-1, +1});
	case EShortcut::ADVENTURE_MOVE_HERO_SS: return hotkeyMoveHeroDirectional({ 0, +1});
	case EShortcut::ADVENTURE_MOVE_HERO_SE: return hotkeyMoveHeroDirectional({+1, +1});
	case EShortcut::ADVENTURE_MOVE_HERO_WW: return hotkeyMoveHeroDirectional({-1,  0});
	case EShortcut::ADVENTURE_MOVE_HERO_EE: return hotkeyMoveHeroDirectional({+1,  0});
	case EShortcut::ADVENTURE_MOVE_HERO_NW: return hotkeyMoveHeroDirectional({-1, -1});
	case EShortcut::ADVENTURE_MOVE_HERO_NN: return hotkeyMoveHeroDirectional({ 0, -1});
	case EShortcut::ADVENTURE_MOVE_HERO_NE: return hotkeyMoveHeroDirectional({+1, -1});
	}
}

void CAdventureMapInterface::hotkeyMoveHeroDirectional(Point direction)
{
	const CGHeroInstance *h = LOCPLINT->localState->getCurrentHero(); //selected hero

	if(!h || !isActive())
		return;

	if (CGI->mh->hasOngoingAnimations())
		return;

	if(direction == Point(0,0))
	{
		centerOnObject(h);
		return;
	}

	int3 dst = h->visitablePos() + int3(direction.x, direction.y, 0);

	if (!CGI->mh->isInMap((dst)))
		return;

	if ( !LOCPLINT->localState->setPath(h, dst))
		return;

	const CGPath & path = LOCPLINT->localState->getPath(h);

	if (path.nodes.size() > 2)
		onHeroChanged(h);
	else
		if(!path.nodes[0].turns)
			LOCPLINT->moveHero(h, path);
}

void CAdventureMapInterface::onSelectionChanged(const CArmedInstance *sel)
{
	assert(sel);

	widget->getInfoBar()->popAll();
	mapAudio->onSelectionChanged(sel);
	bool centerView = !settings["session"]["autoSkip"].Bool();

	if (centerView)
		centerOnObject(sel);

	if(sel->ID==Obj::TOWN)
	{
		auto town = dynamic_cast<const CGTownInstance*>(sel);

		widget->getInfoBar()->showTownSelection(town);
		widget->getTownList()->select(town);
		widget->getHeroList()->select(nullptr);
		onHeroChanged(nullptr);
	}
	else //hero selected
	{
		auto hero = dynamic_cast<const CGHeroInstance*>(sel);

		widget->getInfoBar()->showHeroSelection(hero);
		widget->getHeroList()->select(hero);
		widget->getTownList()->select(nullptr);

		LOCPLINT->localState->verifyPath(hero);
		onHeroChanged(hero);
	}
	updateButtons();
	widget->getHeroList()->redraw();
	widget->getTownList()->redraw();
}

bool CAdventureMapInterface::isActive()
{
	return active & ~CIntObject::KEYBOARD;
}

void CAdventureMapInterface::onMapTilesChanged(boost::optional<std::unordered_set<int3>> positions)
{
	if (positions)
		widget->getMinimap()->updateTiles(*positions);
	else
		widget->getMinimap()->update();
}

void CAdventureMapInterface::onHotseatWaitStarted(PlayerColor playerID)
{
	onCurrentPlayerChanged(playerID);
	widget->setState(EGameState::HOTSEAT_WAIT);
}

void CAdventureMapInterface::onEnemyTurnStarted(PlayerColor playerID)
{
	if(settings["session"]["spectate"].Bool())
		return;

	adjustActiveness(true);
	mapAudio->onEnemyTurnStarted();
	widget->getMinimap()->setAIRadar(true);
	widget->getInfoBar()->startEnemyTurn(LOCPLINT->cb->getCurrentPlayer());
	widget->getMinimap()->showAll(screen);//force refresh on inactive object
	widget->getInfoBar()->showAll(screen);//force refresh on inactive object
}

void CAdventureMapInterface::adjustActiveness(bool aiTurnStart)
{
	bool wasActive = isActive();

	if(wasActive)
		deactivate();

	if (aiTurnStart)
		widget->setState(EGameState::ENEMY_TURN);
	else
		widget->setState(EGameState::MAKING_TURN);

	if(wasActive)
		activate();
}

void CAdventureMapInterface::onCurrentPlayerChanged(PlayerColor playerID)
{
	LOCPLINT->localState->setSelection(nullptr);

	if (playerID == currentPlayerID)
		return;

	currentPlayerID = playerID;
	widget->setPlayer(playerID);
}

void CAdventureMapInterface::onPlayerTurnStarted(PlayerColor playerID)
{
	onCurrentPlayerChanged(playerID);

	widget->setState(EGameState::MAKING_TURN);
	if(LOCPLINT->cb->getCurrentPlayer() == LOCPLINT->playerID
		|| settings["session"]["spectate"].Bool())
	{
		adjustActiveness(false);
		widget->getMinimap()->setAIRadar(false);
		widget->getInfoBar()->showSelection();
	}

	widget->getHeroList()->update();
	widget->getTownList()->update();

	const CGHeroInstance * heroToSelect = nullptr;

	// find first non-sleeping hero
	for (auto hero : LOCPLINT->localState->getWanderingHeroes())
	{
		if (!LOCPLINT->localState->isHeroSleeping(hero))
		{
			heroToSelect = hero;
			break;
		}
	}

	//select first hero if available.
	if (heroToSelect != nullptr)
	{
		LOCPLINT->localState->setSelection(heroToSelect);
	}
	else if (LOCPLINT->localState->getOwnedTowns().size())
	{
		LOCPLINT->localState->setSelection(LOCPLINT->localState->getOwnedTown(0));
	}
	else
	{
		LOCPLINT->localState->setSelection(LOCPLINT->localState->getWanderingHero(0));
	}

	//show new day animation and sound on infobar
	widget->getInfoBar()->showDate();

	onHeroChanged(nullptr);
	showAll(screen);
	mapAudio->onPlayerTurnStarted();

	if(settings["session"]["autoSkip"].Bool() && !GH.isKeyboardShiftDown())
	{
		if(CInfoWindow *iw = dynamic_cast<CInfoWindow *>(GH.topInt().get()))
			iw->close();

		endingTurn();
	}
}

void CAdventureMapInterface::endingTurn()
{
	if(settings["session"]["spectate"].Bool())
		return;

	LOCPLINT->makingTurn = false;
	LOCPLINT->cb->endTurn();
	mapAudio->onPlayerTurnEnded();
}

const CGObjectInstance* CAdventureMapInterface::getActiveObject(const int3 &mapPos)
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

void CAdventureMapInterface::onTileLeftClicked(const int3 &mapPos)
{
	if(widget->getState() == EGameState::MAKING_TURN)
		return;

	//FIXME: this line breaks H3 behavior for Dimension Door
	if(!LOCPLINT->cb->isVisible(mapPos))
		return;
	if(!LOCPLINT->makingTurn)
		return;

	const TerrainTile *tile = LOCPLINT->cb->getTile(mapPos);

	const CGObjectInstance *topBlocking = getActiveObject(mapPos);

	int3 selPos = LOCPLINT->localState->getCurrentArmy()->getSightCenter();
	if(spellBeingCasted)
	{
		if (!isInScreenRange(selPos, mapPos))
			return;

		const TerrainTile *heroTile = LOCPLINT->cb->getTile(selPos);

		switch(spellBeingCasted->id)
		{
		case SpellID::SCUTTLE_BOAT: //Scuttle Boat
			if(topBlocking && topBlocking->ID == Obj::BOAT)
				performSpellcasting(mapPos);
			break;
		case SpellID::DIMENSION_DOOR:
			if(!tile || tile->isClear(heroTile))
				performSpellcasting(mapPos);
			break;
		}
		return;
	}
	//check if we can select this object
	bool canSelect = topBlocking && topBlocking->ID == Obj::HERO && topBlocking->tempOwner == LOCPLINT->playerID;
	canSelect |= topBlocking && topBlocking->ID == Obj::TOWN && LOCPLINT->cb->getPlayerRelations(LOCPLINT->playerID, topBlocking->tempOwner);

	bool isHero = false;
	if(LOCPLINT->localState->getCurrentArmy()->ID != Obj::HERO) //hero is not selected (presumably town)
	{
		if(LOCPLINT->localState->getCurrentArmy() == topBlocking) //selected town clicked
			LOCPLINT->openTownWindow(static_cast<const CGTownInstance*>(topBlocking));
		else if(canSelect)
			LOCPLINT->localState->setSelection(static_cast<const CArmedInstance*>(topBlocking));
	}
	else if(const CGHeroInstance * currentHero = LOCPLINT->localState->getCurrentHero()) //hero is selected
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
			LOCPLINT->localState->setSelection(static_cast<const CArmedInstance*>(topBlocking));
			return;
		}
		else //still here? we need to move hero if we clicked end of already selected path or calculate a new path otherwise
		{
			if(LOCPLINT->localState->hasPath(currentHero) &&
			   LOCPLINT->localState->getPath(currentHero).endPos() == mapPos)//we'll be moving
			{
				if(!CGI->mh->hasOngoingAnimations())
					LOCPLINT->moveHero(currentHero, LOCPLINT->localState->getPath(currentHero));
				return;
			}
			else //remove old path and find a new one if we clicked on accessible tile
			{
				LOCPLINT->localState->setPath(currentHero, mapPos);
				onHeroChanged(currentHero);
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

void CAdventureMapInterface::onTileHovered(const int3 &mapPos)
{
	if(widget->getState() != EGameState::MAKING_TURN)
		return;

	//may occur just at the start of game (fake move before full intiialization)
	if(!LOCPLINT->localState->getCurrentArmy())
		return;

	if(!LOCPLINT->cb->isVisible(mapPos))
	{
		CCS->curh->set(Cursor::Map::POINTER);
		GH.statusbar->clear();
		return;
	}
	auto objRelations = PlayerRelations::ALLIES;
	const CGObjectInstance *objAtTile = getActiveObject(mapPos);
	if(objAtTile)
	{
		objRelations = LOCPLINT->cb->getPlayerRelations(LOCPLINT->playerID, objAtTile->tempOwner);
		std::string text = LOCPLINT->localState->getCurrentHero() ? objAtTile->getHoverText(LOCPLINT->localState->getCurrentHero()) : objAtTile->getHoverText(LOCPLINT->playerID);
		boost::replace_all(text,"\n"," ");
		GH.statusbar->write(text);
	}
	else
	{
		std::string hlp = CGI->mh->getTerrainDescr(mapPos, false);
		GH.statusbar->write(hlp);
	}

	if(spellBeingCasted)
	{
		switch(spellBeingCasted->id)
		{
		case SpellID::SCUTTLE_BOAT:
			{
			int3 hpos = LOCPLINT->localState->getCurrentArmy()->getSightCenter();

			if(objAtTile && objAtTile->ID == Obj::BOAT && isInScreenRange(hpos, mapPos))
				CCS->curh->set(Cursor::Map::SCUTTLE_BOAT);
			else
				CCS->curh->set(Cursor::Map::POINTER);
			return;
			}
		case SpellID::DIMENSION_DOOR:
			{
				const TerrainTile * t = LOCPLINT->cb->getTile(mapPos, false);
				int3 hpos = LOCPLINT->localState->getCurrentArmy()->getSightCenter();
				if((!t || t->isClear(LOCPLINT->cb->getTile(hpos))) && isInScreenRange(hpos, mapPos))
					CCS->curh->set(Cursor::Map::TELEPORT);
				else
					CCS->curh->set(Cursor::Map::POINTER);
				return;
			}
		}
	}

	if(LOCPLINT->localState->getCurrentArmy()->ID == Obj::TOWN)
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
	else if(const CGHeroInstance * hero = LOCPLINT->localState->getCurrentHero())
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
				if(LOCPLINT->localState->getCurrentArmy()  == objAtTile)
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

void CAdventureMapInterface::showMoveDetailsInStatusbar(const CGHeroInstance & hero, const CGPathNode & pathNode)
{
	const int maxMovementPointsAtStartOfLastTurn = pathNode.turns > 0 ? hero.maxMovePoints(pathNode.layer == EPathfindingLayer::LAND) : hero.movement;
	const int movementPointsLastTurnCost = maxMovementPointsAtStartOfLastTurn - pathNode.moveRemains;
	const int remainingPointsAfterMove = pathNode.turns == 0 ? pathNode.moveRemains : 0;

	std::string result = VLC->generaltexth->translate("vcmi.adventureMap", pathNode.turns > 0 ? "moveCostDetails" : "moveCostDetailsNoTurns");

	boost::replace_first(result, "%TURNS", std::to_string(pathNode.turns));
	boost::replace_first(result, "%POINTS", std::to_string(movementPointsLastTurnCost));
	boost::replace_first(result, "%REMAINING", std::to_string(remainingPointsAfterMove));

	GH.statusbar->write(result);
}

void CAdventureMapInterface::onTileRightClicked(const int3 &mapPos)
{
	if(widget->getState() != EGameState::MAKING_TURN)
		return;

	if(spellBeingCasted)
	{
		abortCastingMode();
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

void CAdventureMapInterface::enterCastingMode(const CSpell * sp)
{
	assert(sp->id == SpellID::SCUTTLE_BOAT || sp->id == SpellID::DIMENSION_DOOR);
	spellBeingCasted = sp;
	Settings config = settings.write["session"]["showSpellRange"];
	config->Bool() = true;

	widget->setState(EGameState::CASTING_SPELL);
}

void CAdventureMapInterface::exitCastingMode()
{
	assert(spellBeingCasted);
	spellBeingCasted = nullptr;
	widget->setState(EGameState::MAKING_TURN);

	Settings config = settings.write["session"]["showSpellRange"];
	config->Bool() = false;
}

void CAdventureMapInterface::abortCastingMode()
{
	exitCastingMode();
	LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[731]); //Spell cancelled
}

void CAdventureMapInterface::performSpellcasting(const int3 & dest)
{
	SpellID id = spellBeingCasted->id;
	exitCastingMode();
	LOCPLINT->cb->castSpell(LOCPLINT->localState->getCurrentHero(), id, dest);
}

Rect CAdventureMapInterface::terrainAreaPixels() const
{
	return widget->getMapView()->pos;
}

const IShipyard * CAdventureMapInterface::ourInaccessibleShipyard(const CGObjectInstance *obj) const
{
	const IShipyard *ret = IShipyard::castFrom(obj);

	if(!ret ||
		obj->tempOwner != currentPlayerID ||
		(CCS->curh->get<Cursor::Map>() != Cursor::Map::T1_SAIL && CCS->curh->get<Cursor::Map>() != Cursor::Map::POINTER))
		return nullptr;

	return ret;
}

void CAdventureMapInterface::exitWorldView()
{
	widget->setState(EGameState::MAKING_TURN);
	widget->getMapView()->onViewMapActivated();
}

void CAdventureMapInterface::openWorldView(int tileSize)
{
	widget->setState(EGameState::WORLD_VIEW);
	widget->getMapView()->onViewWorldActivated(tileSize);
}

void CAdventureMapInterface::openWorldView()
{
	openWorldView(11);
}

void CAdventureMapInterface::openWorldView(const std::vector<ObjectPosInfo>& objectPositions, bool showTerrain)
{
	openWorldView(11);
	widget->getMapView()->onViewSpellActivated(11, objectPositions, showTerrain);
}
