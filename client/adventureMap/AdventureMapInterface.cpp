/*
 * AdventureMapInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "AdventureMapInterface.h"

#include "AdventureOptions.h"
#include "AdventureState.h"
#include "CInGameConsole.h"
#include "CMinimap.h"
#include "CList.h"
#include "CInfoBar.h"
#include "MapAudioPlayer.h"
#include "AdventureMapWidget.h"
#include "AdventureMapShortcuts.h"

#include "../mapView/mapHandler.h"
#include "../mapView/MapView.h"
#include "../windows/InfoWindows.h"
#include "../CGameInfo.h"
#include "../gui/CursorHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../CMT.h"
#include "../PlayerLocalState.h"
#include "../CPlayerInterface.h"

#include "../../CCallback.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/CPathfinder.h"
#include "../../lib/mapping/CMap.h"

std::shared_ptr<AdventureMapInterface> adventureInt;

AdventureMapInterface::AdventureMapInterface():
	mapAudio(new MapAudioPlayer()),
	spellBeingCasted(nullptr),
	scrollingWasActive(false),
	scrollingWasBlocked(false)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pos.x = pos.y = 0;
	pos.w = GH.screenDimensions().x;
	pos.h = GH.screenDimensions().y;
	strongInterest = true; // handle all mouse move events to prevent dead mouse move space in fullscreen mode

	shortcuts = std::make_shared<AdventureMapShortcuts>(*this);

	widget = std::make_shared<AdventureMapWidget>(shortcuts);
	shortcuts->setState(EAdventureState::MAKING_TURN);
	widget->getMapView()->onViewMapActivated();

	addUsedEvents(KEYBOARD | TIME);
}

void AdventureMapInterface::onMapViewMoved(const Rect & visibleArea, int mapLevel)
{
	shortcuts->onMapViewMoved(visibleArea, mapLevel);
	widget->getMinimap()->onMapViewMoved(visibleArea, mapLevel);
	widget->onMapViewMoved(visibleArea, mapLevel);
}

void AdventureMapInterface::onAudioResumed()
{
	mapAudio->onAudioResumed();
}

void AdventureMapInterface::onAudioPaused()
{
	mapAudio->onAudioPaused();
}

void AdventureMapInterface::onHeroMovementStarted(const CGHeroInstance * hero)
{
	widget->getInfoBar()->popAll();
	widget->getInfoBar()->showSelection();
}

void AdventureMapInterface::onHeroChanged(const CGHeroInstance *h)
{
	widget->getHeroList()->update(h);

	if (h && h == LOCPLINT->localState->getCurrentHero() && !widget->getInfoBar()->showingComponents())
		widget->getInfoBar()->showSelection();

	widget->updateActiveState();
}

void AdventureMapInterface::onTownChanged(const CGTownInstance * town)
{
	widget->getTownList()->update(town);

	if (town && town == LOCPLINT->localState->getCurrentTown() && !widget->getInfoBar()->showingComponents())
		widget->getInfoBar()->showSelection();
}

void AdventureMapInterface::showInfoBoxMessage(const std::vector<Component> & components, std::string message, int timer)
{
	widget->getInfoBar()->pushComponents(components, message, timer);
}

void AdventureMapInterface::activate()
{
	CIntObject::activate();

	adjustActiveness();

	screenBuf = screen;
	
	if(LOCPLINT)
	{
		LOCPLINT->cingconsole->activate();
		LOCPLINT->cingconsole->pos = this->pos;
	}

	GH.fakeMouseMove(); //to restore the cursor
}

void AdventureMapInterface::deactivate()
{
	CIntObject::deactivate();
	CCS->curh->set(Cursor::Map::POINTER);
}

void AdventureMapInterface::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	LOCPLINT->cingconsole->show(to);
}

void AdventureMapInterface::show(SDL_Surface * to)
{
	CIntObject::show(to);
	LOCPLINT->cingconsole->show(to);
}

void AdventureMapInterface::tick(uint32_t msPassed)
{
	handleMapScrollingUpdate(msPassed);

	// we want animations to be active during enemy turn but map itself to be non-interactive
	// so call timer update directly on inactive element
	widget->getMapView()->tick(msPassed);
}

void AdventureMapInterface::handleMapScrollingUpdate(uint32_t timePassed)
{
	/// Width of window border, in pixels, that triggers map scrolling
	static constexpr uint32_t borderScrollWidth = 15;

	uint32_t scrollSpeedPixels = settings["adventure"]["scrollSpeedPixels"].Float();
	uint32_t scrollDistance = scrollSpeedPixels * timePassed / 1000;

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

	bool cursorInScrollArea = scrollDelta != Point(0,0);
	bool scrollingActive = cursorInScrollArea && active && shortcuts->optionSidePanelActive() && !scrollingWasBlocked;
	bool scrollingBlocked = GH.isKeyboardCtrlDown();

	if (!scrollingWasActive && scrollingBlocked)
	{
		scrollingWasBlocked = true;
		return;
	}

	if (!cursorInScrollArea && scrollingWasBlocked)
	{
		scrollingWasBlocked = false;
		return;
	}

	if (scrollingActive)
		widget->getMapView()->onMapScrolled(scrollDelta);

	if (!scrollingActive && !scrollingWasActive)
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

	scrollingWasActive = scrollingActive;
}

void AdventureMapInterface::centerOnTile(int3 on)
{
	widget->getMapView()->onCenteredTile(on);
}

void AdventureMapInterface::centerOnObject(const CGObjectInstance * obj)
{
	widget->getMapView()->onCenteredObject(obj);
}

void AdventureMapInterface::keyPressed(EShortcut key)
{
	if (key == EShortcut::GLOBAL_CANCEL && spellBeingCasted)
		hotkeyAbortCastingMode();

	//fake mouse use to trigger onTileHovered()
	GH.fakeMouseMove();
}

void AdventureMapInterface::onSelectionChanged(const CArmedInstance *sel)
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

	widget->updateActiveState();
	widget->getHeroList()->redraw();
	widget->getTownList()->redraw();
}

void AdventureMapInterface::onMapTilesChanged(boost::optional<std::unordered_set<int3>> positions)
{
	if (positions)
		widget->getMinimap()->updateTiles(*positions);
	else
		widget->getMinimap()->update();
}

void AdventureMapInterface::onHotseatWaitStarted(PlayerColor playerID)
{
	onCurrentPlayerChanged(playerID);
	setState(EAdventureState::HOTSEAT_WAIT);
}

void AdventureMapInterface::onEnemyTurnStarted(PlayerColor playerID)
{
	if(settings["session"]["spectate"].Bool())
		return;

	mapAudio->onEnemyTurnStarted();
	widget->getMinimap()->setAIRadar(true);
	widget->getInfoBar()->startEnemyTurn(LOCPLINT->cb->getCurrentPlayer());
	setState(EAdventureState::ENEMY_TURN);

}

void AdventureMapInterface::setState(EAdventureState state)
{
	shortcuts->setState(state);
	adjustActiveness();
	widget->updateActiveState();
}

void AdventureMapInterface::adjustActiveness()
{
	bool widgetMustBeActive = active && shortcuts->optionSidePanelActive();
	bool mapViewMustBeActive = active && (shortcuts->optionMapViewActive());

	if (widgetMustBeActive && !widget->active)
		widget->activate();

	if (!widgetMustBeActive && widget->active)
		widget->deactivate();

	if (mapViewMustBeActive && !widget->getMapView()->active)
		widget->getMapView()->activate();

	if (!mapViewMustBeActive && widget->getMapView()->active)
		widget->getMapView()->deactivate();
}

void AdventureMapInterface::onCurrentPlayerChanged(PlayerColor playerID)
{
	LOCPLINT->localState->setSelection(nullptr);

	if (playerID == currentPlayerID)
		return;

	currentPlayerID = playerID;
	widget->setPlayer(playerID);
}

void AdventureMapInterface::onPlayerTurnStarted(PlayerColor playerID)
{
	onCurrentPlayerChanged(playerID);

	setState(EAdventureState::MAKING_TURN);
	if(LOCPLINT->cb->getCurrentPlayer() == LOCPLINT->playerID
		|| settings["session"]["spectate"].Bool())
	{
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
		if(auto iw = GH.windows().topWindow<CInfoWindow>())
			iw->close();

		hotkeyEndingTurn();
	}
}

void AdventureMapInterface::hotkeyEndingTurn()
{
	if(settings["session"]["spectate"].Bool())
		return;

	LOCPLINT->makingTurn = false;
	LOCPLINT->cb->endTurn();
	mapAudio->onPlayerTurnEnded();
}

const CGObjectInstance* AdventureMapInterface::getActiveObject(const int3 &mapPos)
{
	std::vector < const CGObjectInstance * > bobjs = LOCPLINT->cb->getBlockingObjs(mapPos);  //blocking objects at tile

	if (bobjs.empty())
		return nullptr;

	return *boost::range::max_element(bobjs, &CMapHandler::compareObjectBlitOrder);
}

void AdventureMapInterface::onTileLeftClicked(const int3 &mapPos)
{
	if(!shortcuts->optionMapViewActive())
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
		assert(shortcuts->optionSpellcasting());

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

void AdventureMapInterface::onTileHovered(const int3 &mapPos)
{
	if(!shortcuts->optionMapViewActive())
		return;

	//may occur just at the start of game (fake move before full intiialization)
	if(!LOCPLINT->localState->getCurrentArmy())
		return;

	if(!LOCPLINT->cb->isVisible(mapPos))
	{
		CCS->curh->set(Cursor::Map::POINTER);
		GH.statusbar()->clear();
		return;
	}
	auto objRelations = PlayerRelations::ALLIES;
	const CGObjectInstance *objAtTile = getActiveObject(mapPos);
	if(objAtTile)
	{
		objRelations = LOCPLINT->cb->getPlayerRelations(LOCPLINT->playerID, objAtTile->tempOwner);
		std::string text = LOCPLINT->localState->getCurrentHero() ? objAtTile->getHoverText(LOCPLINT->localState->getCurrentHero()) : objAtTile->getHoverText(LOCPLINT->playerID);
		boost::replace_all(text,"\n"," ");
		GH.statusbar()->write(text);
	}
	else
	{
		std::string hlp = CGI->mh->getTerrainDescr(mapPos, false);
		GH.statusbar()->write(hlp);
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

void AdventureMapInterface::showMoveDetailsInStatusbar(const CGHeroInstance & hero, const CGPathNode & pathNode)
{
	const int maxMovementPointsAtStartOfLastTurn = pathNode.turns > 0 ? hero.maxMovePoints(pathNode.layer == EPathfindingLayer::LAND) : hero.movement;
	const int movementPointsLastTurnCost = maxMovementPointsAtStartOfLastTurn - pathNode.moveRemains;
	const int remainingPointsAfterMove = pathNode.turns == 0 ? pathNode.moveRemains : 0;

	std::string result = VLC->generaltexth->translate("vcmi.adventureMap", pathNode.turns > 0 ? "moveCostDetails" : "moveCostDetailsNoTurns");

	boost::replace_first(result, "%TURNS", std::to_string(pathNode.turns));
	boost::replace_first(result, "%POINTS", std::to_string(movementPointsLastTurnCost));
	boost::replace_first(result, "%REMAINING", std::to_string(remainingPointsAfterMove));

	GH.statusbar()->write(result);
}

void AdventureMapInterface::onTileRightClicked(const int3 &mapPos)
{
	if(!shortcuts->optionMapViewActive())
		return;

	if(spellBeingCasted)
	{
		hotkeyAbortCastingMode();
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

void AdventureMapInterface::enterCastingMode(const CSpell * sp)
{
	assert(sp->id == SpellID::SCUTTLE_BOAT || sp->id == SpellID::DIMENSION_DOOR);
	spellBeingCasted = sp;
	Settings config = settings.write["session"]["showSpellRange"];
	config->Bool() = true;

	setState(EAdventureState::CASTING_SPELL);
}

void AdventureMapInterface::exitCastingMode()
{
	assert(spellBeingCasted);
	spellBeingCasted = nullptr;
	setState(EAdventureState::MAKING_TURN);

	Settings config = settings.write["session"]["showSpellRange"];
	config->Bool() = false;
}

void AdventureMapInterface::hotkeyAbortCastingMode()
{
	exitCastingMode();
	LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[731]); //Spell cancelled
}

void AdventureMapInterface::performSpellcasting(const int3 & dest)
{
	SpellID id = spellBeingCasted->id;
	exitCastingMode();
	LOCPLINT->cb->castSpell(LOCPLINT->localState->getCurrentHero(), id, dest);
}

Rect AdventureMapInterface::terrainAreaPixels() const
{
	return widget->getMapView()->pos;
}

const IShipyard * AdventureMapInterface::ourInaccessibleShipyard(const CGObjectInstance *obj) const
{
	const IShipyard *ret = IShipyard::castFrom(obj);

	if(!ret ||
		obj->tempOwner != currentPlayerID ||
		(CCS->curh->get<Cursor::Map>() != Cursor::Map::T1_SAIL && CCS->curh->get<Cursor::Map>() != Cursor::Map::POINTER))
		return nullptr;

	return ret;
}

void AdventureMapInterface::hotkeyExitWorldView()
{
	setState(EAdventureState::MAKING_TURN);
	widget->getMapView()->onViewMapActivated();
}

void AdventureMapInterface::openWorldView(int tileSize)
{
	setState(EAdventureState::WORLD_VIEW);
	widget->getMapView()->onViewWorldActivated(tileSize);
}

void AdventureMapInterface::openWorldView()
{
	openWorldView(11);
}

void AdventureMapInterface::openWorldView(const std::vector<ObjectPosInfo>& objectPositions, bool showTerrain)
{
	openWorldView(11);
	widget->getMapView()->onViewSpellActivated(11, objectPositions, showTerrain);
}

void AdventureMapInterface::hotkeyNextTown()
{
	widget->getTownList()->selectNext();
}

void AdventureMapInterface::hotkeySwitchMapLevel()
{
	widget->getMapView()->onMapLevelSwitched();
}

void AdventureMapInterface::onScreenResize()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	widget.reset();
	pos.x = pos.y = 0;
	pos.w = GH.screenDimensions().x;
	pos.h = GH.screenDimensions().y;

	widget = std::make_shared<AdventureMapWidget>(shortcuts);
	widget->getMapView()->onViewMapActivated();
	widget->setPlayer(currentPlayerID);
	widget->updateActiveState();
	widget->getMinimap()->update();
	widget->getInfoBar()->showSelection();

	adjustActiveness();
}
