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
#include "TurnTimerWidget.h"
#include "AdventureMapWidget.h"
#include "AdventureMapShortcuts.h"

#include "../mapView/mapHandler.h"
#include "../mapView/MapView.h"
#include "../windows/InfoWindows.h"
#include "../widgets/RadialMenu.h"
#include "../CGameInfo.h"
#include "../gui/CursorHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../render/Canvas.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"
#include "../render/IScreenHandler.h"
#include "../CMT.h"
#include "../PlayerLocalState.h"
#include "../CPlayerInterface.h"

#include "../../CCallback.h"
#include "../../lib/IGameSettings.h"
#include "../../lib/StartInfo.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapping/CMapDefines.h"
#include "../../lib/pathfinder/CGPathNode.h"
#include "../../lib/pathfinder/TurnInfo.h"
#include "../../lib/spells/ISpellMechanics.h"
#include "../../lib/spells/Problem.h"

std::shared_ptr<AdventureMapInterface> adventureInt;

AdventureMapInterface::AdventureMapInterface():
	mapAudio(new MapAudioPlayer()),
	spellBeingCasted(nullptr),
	scrollingWasActive(false),
	scrollingWasBlocked(false),
	backgroundDimLevel(settings["adventure"]["backgroundDimLevel"].Integer())
{
	OBJECT_CONSTRUCTION;
	pos.x = pos.y = 0;
	pos.w = GH.screenDimensions().x;
	pos.h = GH.screenDimensions().y;

	shortcuts = std::make_shared<AdventureMapShortcuts>(*this);

	widget = std::make_shared<AdventureMapWidget>(shortcuts);
	shortcuts->setState(EAdventureState::MAKING_TURN);
	widget->getMapView()->onViewMapActivated();

	if(LOCPLINT->cb->getStartInfo()->turnTimerInfo.turnTimer != 0)
		watches = std::make_shared<TurnTimerWidget>(Point(24, 24));
	
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
	if (shortcuts->optionMapViewActive())
	{
		widget->getInfoBar()->popAll();
		widget->getInfoBar()->showSelection();
	}
}

void AdventureMapInterface::onHeroChanged(const CGHeroInstance *h)
{
	widget->getHeroList()->updateElement(h);

	if (h && h == LOCPLINT->localState->getCurrentHero() && !widget->getInfoBar()->showingComponents())
		widget->getInfoBar()->showSelection();

	widget->updateActiveState();
}

void AdventureMapInterface::onTownChanged(const CGTownInstance * town)
{
	widget->getTownList()->updateElement(town);

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

	// workaround for an edge case:
	// if player unequips Angel Wings / Boots of Levitation of currently active hero
	// game will correctly invalidate paths but current route will not be updated since verifyPath() is not called for current hero
	if (LOCPLINT->makingTurn && LOCPLINT->localState->getCurrentHero())
		LOCPLINT->localState->verifyPath(LOCPLINT->localState->getCurrentHero());
}

void AdventureMapInterface::deactivate()
{
	CIntObject::deactivate();
	CCS->curh->set(Cursor::Map::POINTER);

	if(LOCPLINT)
		LOCPLINT->cingconsole->deactivate();
}

void AdventureMapInterface::showAll(Canvas & to)
{
	CIntObject::showAll(to);
	dim(to);
	LOCPLINT->cingconsole->show(to);
}

void AdventureMapInterface::show(Canvas & to)
{
	CIntObject::show(to);
	dim(to);
	LOCPLINT->cingconsole->show(to);
}

void AdventureMapInterface::dim(Canvas & to)
{
	auto const isBigWindow = [&](std::shared_ptr<CIntObject> window) { return window->pos.w >= 800 && window->pos.h >= 600; }; // OH3 fullscreen

	if(settings["adventure"]["hideBackground"].Bool())
		for (auto window : GH.windows().findWindows<CIntObject>())
		{
			if(!std::dynamic_pointer_cast<AdventureMapInterface>(window) && std::dynamic_pointer_cast<CIntObject>(window) && isBigWindow(window))
			{
				to.fillTexture(GH.renderHandler().loadImage(ImagePath::builtin("DiBoxBck"), EImageBlitMode::OPAQUE));
				return;
			}
		}
	for (auto window : GH.windows().findWindows<CIntObject>())
	{
		if (!std::dynamic_pointer_cast<AdventureMapInterface>(window) && !std::dynamic_pointer_cast<RadialMenu>(window) && !window->isPopupWindow() && (settings["adventure"]["backgroundDimSmallWindows"].Bool() || isBigWindow(window) || shortcuts->getState() == EAdventureState::HOTSEAT_WAIT))
		{
			Rect targetRect(0, 0, GH.screenDimensions().x, GH.screenDimensions().y);
			ColorRGBA colorToFill(0, 0, 0, std::clamp<int>(backgroundDimLevel, 0, 255));
			if(backgroundDimLevel > 0)
				to.drawColorBlended(targetRect, colorToFill);
			return;
		}
	}
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
	static constexpr int32_t borderScrollWidth = 15;

	int32_t scrollSpeedPixels = settings["adventure"]["scrollSpeedPixels"].Float();
	int32_t scrollDistance = scrollSpeedPixels * timePassed / 1000;

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
	bool scrollingActive = cursorInScrollArea && shortcuts->optionMapScrollingActive() && !scrollingWasBlocked;
	bool scrollingBlocked = GH.isKeyboardCtrlDown() || !settings["adventure"]["borderScroll"].Bool() || !GH.screenHandler().hasFocus();

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
		widget->getTownList()->updateWidget();
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

void AdventureMapInterface::onTownOrderChanged()
{
	widget->getTownList()->updateWidget();
}

void AdventureMapInterface::onHeroOrderChanged()
{
	widget->getHeroList()->updateWidget();
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
	backgroundDimLevel = 255;

	onCurrentPlayerChanged(playerID);
	setState(EAdventureState::HOTSEAT_WAIT);
}

void AdventureMapInterface::onEnemyTurnStarted(PlayerColor playerID, bool isHuman)
{
	if(settings["session"]["spectate"].Bool())
		return;

	mapAudio->onEnemyTurnStarted();
	widget->getMinimap()->setAIRadar(!isHuman);
	widget->getInfoBar()->startEnemyTurn(playerID);
	setState(isHuman ? EAdventureState::MAKING_TURN : EAdventureState::AI_PLAYER_TURN);
}

void AdventureMapInterface::setState(EAdventureState state)
{
	shortcuts->setState(state);
	adjustActiveness();
	widget->updateActiveState();
}

void AdventureMapInterface::adjustActiveness()
{
	bool widgetMustBeActive = isActive() && shortcuts->optionSidePanelActive();
	bool mapViewMustBeActive = isActive() && (shortcuts->optionMapViewActive());

	widget->setInputEnabled(widgetMustBeActive);
	widget->getMapView()->setInputEnabled(mapViewMustBeActive);
}

void AdventureMapInterface::onCurrentPlayerChanged(PlayerColor playerID)
{
	if (playerID == currentPlayerID)
		return;

	currentPlayerID = playerID;
	widget->setPlayerColor(playerID);
}

void AdventureMapInterface::onPlayerTurnStarted(PlayerColor playerID)
{
	backgroundDimLevel = settings["adventure"]["backgroundDimLevel"].Integer();

	onCurrentPlayerChanged(playerID);

	setState(EAdventureState::MAKING_TURN);
	if(playerID == LOCPLINT->playerID || settings["session"]["spectate"].Bool())
	{
		widget->getMinimap()->setAIRadar(false);
		widget->getInfoBar()->showSelection();
	}

	widget->getHeroList()->updateWidget();
	widget->getTownList()->updateWidget();

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

	onSelectionChanged(LOCPLINT->localState->getCurrentArmy());

	//show new day animation and sound on infobar, except for 1st day of the game
	if (LOCPLINT->cb->getDate(Date::DAY) != 1)
		widget->getInfoBar()->showDate();

	onHeroChanged(nullptr);
	Canvas canvas = Canvas::createFromSurface(screen, CanvasScalingPolicy::AUTO);
	showAll(canvas);
	mapAudio->onPlayerTurnStarted();

	if(settings["session"]["autoSkip"].Bool() && !GH.isKeyboardShiftDown())
	{
		if(auto iw = GH.windows().topWindow<CInfoWindow>())
			iw->close();

		GH.dispatchMainThread([this]()
		{
			hotkeyEndingTurn();
		});
	}
}

void AdventureMapInterface::hotkeyEndingTurn()
{
	if(settings["session"]["spectate"].Bool())
		return;

	if(!settings["general"]["startTurnAutosave"].Bool())
	{
		LOCPLINT->performAutosave();
	}

	LOCPLINT->makingTurn = false;
	LOCPLINT->cb->endTurn();

	mapAudio->onPlayerTurnEnded();

	// Normally, game will receive PlayerStartsTurn call almost instantly with new player ID that will switch UI to waiting mode
	// However, when simturns are active it is possible for such call not to come because another player is still acting
	// So find first player other than ours that is acting at the moment and update UI as if he had started turn
	for (auto player = PlayerColor(0); player < PlayerColor::PLAYER_LIMIT; ++player)
	{
		if (player != LOCPLINT->playerID && LOCPLINT->cb->isPlayerMakingTurn(player))
		{
			onEnemyTurnStarted(player, LOCPLINT->cb->getStartInfo()->playerInfos.at(player).isControlledByHuman());
			break;
		}
	}
}

const CGObjectInstance* AdventureMapInterface::getActiveObject(const int3 &mapPos)
{
	std::vector < const CGObjectInstance * > bobjs = LOCPLINT->cb->getBlockingObjs(mapPos);  //blocking objects at tile

	if (bobjs.empty())
		return nullptr;

	return *boost::range::max_element(bobjs, &CMapHandler::compareObjectBlitOrder);
}

void AdventureMapInterface::onTileLeftClicked(const int3 &targetPosition)
{
	if(!shortcuts->optionMapViewActive())
		return;

	const CGObjectInstance *topBlocking = LOCPLINT->cb->isVisible(targetPosition) ? getActiveObject(targetPosition) : nullptr;

	if(spellBeingCasted)
	{
		assert(shortcuts->optionSpellcasting());
		assert(spellBeingCasted->id == SpellID::SCUTTLE_BOAT || spellBeingCasted->id == SpellID::DIMENSION_DOOR);

		if(isValidAdventureSpellTarget(targetPosition))
			performSpellcasting(targetPosition);
		return;
	}

	if(!LOCPLINT->cb->isVisible(targetPosition))
		return;

	//check if we can select this object
	bool canSelect = topBlocking && topBlocking->ID == Obj::HERO && topBlocking->tempOwner == LOCPLINT->playerID;
	canSelect |= topBlocking && topBlocking->ID == Obj::TOWN && LOCPLINT->cb->getPlayerRelations(LOCPLINT->playerID, topBlocking->tempOwner) != PlayerRelations::ENEMIES;

	if(LOCPLINT->localState->getCurrentArmy()->ID != Obj::HERO) //hero is not selected (presumably town)
	{
		if(LOCPLINT->localState->getCurrentArmy() == topBlocking) //selected town clicked
			LOCPLINT->openTownWindow(static_cast<const CGTownInstance*>(topBlocking));
		else if(canSelect)
			LOCPLINT->localState->setSelection(static_cast<const CArmedInstance*>(topBlocking));
	}
	else if(const CGHeroInstance * currentHero = LOCPLINT->localState->getCurrentHero()) //hero is selected
	{
		const CGPathNode *pn = LOCPLINT->getPathsInfo(currentHero)->getPathInfo(targetPosition);

		const auto shipyard = dynamic_cast<const IShipyard *>(topBlocking);

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
		else if(shipyard != nullptr && pn->turns == 255 && LOCPLINT->cb->getPlayerRelations(LOCPLINT->playerID, topBlocking->tempOwner) != PlayerRelations::ENEMIES)
		{
			LOCPLINT->showShipyardDialogOrProblemPopup(shipyard);
		}
		else //still here? we need to move hero if we clicked end of already selected path or calculate a new path otherwise
		{
			int3 destinationTile = targetPosition;

			if(topBlocking && topBlocking->isVisitable() && !topBlocking->visitableAt(destinationTile) && settings["gameTweaks"]["simpleObjectSelection"].Bool())
				destinationTile = topBlocking->visitablePos();

			if(LOCPLINT->localState->hasPath(currentHero) &&
			   LOCPLINT->localState->getPath(currentHero).endPos() == destinationTile &&
			   !GH.isKeyboardShiftDown())//we'll be moving
			{
				assert(!CGI->mh->hasOngoingAnimations());
				if(!CGI->mh->hasOngoingAnimations() && LOCPLINT->localState->getPath(currentHero).nextNode().turns == 0)
					LOCPLINT->moveHero(currentHero, LOCPLINT->localState->getPath(currentHero));
				return;
			}
			else
			{
				if(GH.isKeyboardShiftDown()) //normal click behaviour (as no hero selected)
				{
					if(canSelect)
						LOCPLINT->localState->setSelection(static_cast<const CArmedInstance*>(topBlocking));
				}
				else //remove old path and find a new one if we clicked on accessible tile
				{
					LOCPLINT->localState->setPath(currentHero, destinationTile);
					onHeroChanged(currentHero);
				}
			}
		}
	} //end of hero is selected "case"
	else
	{
		throw std::runtime_error("Nothing is selected...");
	}
}

void AdventureMapInterface::onTileHovered(const int3 &targetPosition)
{
	if(!shortcuts->optionMapViewActive())
		return;

	//may occur just at the start of game (fake move before full initialization)
	if(!LOCPLINT->localState->getCurrentArmy())
		return;

	bool isTargetPositionVisible = LOCPLINT->cb->isVisible(targetPosition);
	const CGObjectInstance *objAtTile = isTargetPositionVisible ? getActiveObject(targetPosition) : nullptr;

	if(spellBeingCasted)
	{
		switch(spellBeingCasted->id)
		{
		case SpellID::SCUTTLE_BOAT:
			if(isValidAdventureSpellTarget(targetPosition))
				CCS->curh->set(Cursor::Map::SCUTTLE_BOAT);
			else
				CCS->curh->set(Cursor::Map::POINTER);
			return;

		case SpellID::DIMENSION_DOOR:
			if(isValidAdventureSpellTarget(targetPosition))
			{
				if(LOCPLINT->cb->getSettings().getBoolean(EGameSettings::DIMENSION_DOOR_TRIGGERS_GUARDS) && LOCPLINT->cb->isTileGuardedUnchecked(targetPosition))
					CCS->curh->set(Cursor::Map::T1_ATTACK);
				else
					CCS->curh->set(Cursor::Map::TELEPORT);
				return;
			}
			else
				CCS->curh->set(Cursor::Map::POINTER);
			return;
		default:
			CCS->curh->set(Cursor::Map::POINTER);
			return;
		}
	}

	if(!isTargetPositionVisible)
	{
		CCS->curh->set(Cursor::Map::POINTER);
		return;
	}

	auto objRelations = PlayerRelations::ALLIES;

	if(objAtTile)
	{
		objRelations = LOCPLINT->cb->getPlayerRelations(LOCPLINT->playerID, objAtTile->tempOwner);
		std::string text = LOCPLINT->localState->getCurrentHero() ? objAtTile->getHoverText(LOCPLINT->localState->getCurrentHero()) : objAtTile->getHoverText(LOCPLINT->playerID);
		boost::replace_all(text,"\n"," ");
		if (GH.isKeyboardCmdDown())
			text.append(" (" + std::to_string(targetPosition.x) + ", " + std::to_string(targetPosition.y) + ")");
		GH.statusbar()->write(text);
	}
	else if(isTargetPositionVisible)
	{
		std::string tileTooltipText = CGI->mh->getTerrainDescr(targetPosition, false);
		if (GH.isKeyboardCmdDown())
			tileTooltipText.append(" (" + std::to_string(targetPosition.x) + ", " + std::to_string(targetPosition.y) + ")");
		GH.statusbar()->write(tileTooltipText);
	}

	if(LOCPLINT->localState->getCurrentArmy()->ID == Obj::TOWN || GH.isKeyboardShiftDown())
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

		const CGPathNode * pathNode = LOCPLINT->getPathsInfo(hero)->getPathInfo(targetPosition);
		assert(pathNode);

		if((GH.isKeyboardAltDown() || settings["gameTweaks"]["forceMovementInfo"].Bool()) && pathNode->reachable()) //overwrite status bar text with movement info
		{
			showMoveDetailsInStatusbar(*hero, *pathNode);
		}

		if (objAtTile && pathNode->action == EPathNodeAction::UNKNOWN)
		{
			if(objAtTile->ID == Obj::TOWN && objRelations != PlayerRelations::ENEMIES)
			{
				CCS->curh->set(Cursor::Map::TOWN);
				return;
			}
			else if(objAtTile->ID == Obj::HERO && objRelations == PlayerRelations::SAME_PLAYER)
			{
				CCS->curh->set(Cursor::Map::HERO);
				return;
			}
			else if (objAtTile->ID == Obj::SHIPYARD && objRelations != PlayerRelations::ENEMIES)
			{
				CCS->curh->set(Cursor::Map::T1_SAIL);
				return;
			}

			if(objAtTile->isVisitable() && !objAtTile->visitableAt(targetPosition) && settings["gameTweaks"]["simpleObjectSelection"].Bool())
				pathNode = LOCPLINT->getPathsInfo(hero)->getPathInfo(objAtTile->visitablePos());
		}

		int turns = pathNode->turns;
		vstd::amin(turns, 3);
		switch(pathNode->action)
		{
		case EPathNodeAction::NORMAL:
		case EPathNodeAction::TELEPORT_NORMAL:
			if(pathNode->layer == EPathfindingLayer::LAND)
				CCS->curh->set(cursorMove[turns]);
			else
				CCS->curh->set(cursorSail[turns]);
			break;

		case EPathNodeAction::VISIT:
		case EPathNodeAction::BLOCKING_VISIT:
		case EPathNodeAction::TELEPORT_BLOCKING_VISIT:
			if(objAtTile && objAtTile->ID == Obj::HERO)
			{
				if(LOCPLINT->localState->getCurrentArmy()  == objAtTile)
					CCS->curh->set(Cursor::Map::HERO);
				else
					CCS->curh->set(cursorExchange[turns]);
			}
			else if(pathNode->layer == EPathfindingLayer::LAND)
				CCS->curh->set(cursorVisit[turns]);
			else if (pathNode->layer == EPathfindingLayer::SAIL &&
					 objAtTile &&
					 objAtTile->isCoastVisitable() &&
					 pathNode->theNodeBefore &&
					 pathNode->theNodeBefore->layer == EPathfindingLayer::LAND )
			{
				// exception - when visiting shipwreck located on coast from land - show 'horse' cursor, not 'ship' cursor
				CCS->curh->set(cursorVisit[turns]);
			}
			else
				CCS->curh->set(cursorSailVisit[turns]);
			break;

		case EPathNodeAction::BATTLE:
		case EPathNodeAction::TELEPORT_BATTLE:
			CCS->curh->set(cursorAttack[turns]);
			break;

		case EPathNodeAction::EMBARK:
			CCS->curh->set(cursorSail[turns]);
			break;

		case EPathNodeAction::DISEMBARK:
			CCS->curh->set(cursorDisembark[turns]);
			break;

		default:
				CCS->curh->set(Cursor::Map::POINTER);
			break;
		}
	}
}

void AdventureMapInterface::showMoveDetailsInStatusbar(const CGHeroInstance & hero, const CGPathNode & pathNode)
{
	const int maxMovementPointsAtStartOfLastTurn = pathNode.turns > 0 ? hero.movementPointsLimit(pathNode.layer == EPathfindingLayer::LAND) : hero.movementPointsRemaining();
	const int movementPointsLastTurnCost = maxMovementPointsAtStartOfLastTurn - pathNode.moveRemains;
	const int remainingPointsAfterMove = pathNode.moveRemains;

	int totalMovementCost = hero.movementPointsRemaining();
	for (int i = 1; i <= pathNode.turns; ++i)
	{
		auto turnInfo = hero.getTurnInfo(i);
		if (pathNode.layer == EPathfindingLayer::SAIL)
			totalMovementCost += turnInfo->getMovePointsLimitWater();
		else
			totalMovementCost += turnInfo->getMovePointsLimitLand();
	}

	totalMovementCost -= pathNode.moveRemains;

	std::string result = VLC->generaltexth->translate("vcmi.adventureMap", pathNode.turns > 0 ? "moveCostDetails" : "moveCostDetailsNoTurns");

	boost::replace_first(result, "%TURNS", std::to_string(pathNode.turns));
	boost::replace_first(result, "%POINTS", std::to_string(movementPointsLastTurnCost));
	boost::replace_first(result, "%REMAINING", std::to_string(remainingPointsAfterMove));
	boost::replace_first(result, "%TOTAL", std::to_string(totalMovementCost));

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

void AdventureMapInterface::hotkeyZoom(int delta, bool useDeadZone)
{
	widget->getMapView()->onMapZoomLevelChanged(delta, useDeadZone);
}

void AdventureMapInterface::onScreenResize()
{
	OBJECT_CONSTRUCTION;

	// remember our activation state and reactive after reconstruction
	// since othervice activate() calls for created elements will bypass virtual dispatch
	// and will call directly CIntObject::activate() instead of dispatching virtual function call
	bool widgetActive = isActive();

	if (widgetActive)
		deactivate();

	widget.reset();
	pos.x = pos.y = 0;
	pos.w = GH.screenDimensions().x;
	pos.h = GH.screenDimensions().y;

	widget = std::make_shared<AdventureMapWidget>(shortcuts);
	widget->getMapView()->onViewMapActivated();
	widget->setPlayerColor(currentPlayerID);
	widget->updateActiveState();
	widget->getMinimap()->update();
	widget->getInfoBar()->showSelection();

	if (LOCPLINT && LOCPLINT->localState->getCurrentArmy())
		widget->getMapView()->onCenteredObject(LOCPLINT->localState->getCurrentArmy());

	adjustActiveness();

	if (widgetActive)
		activate();
}

bool AdventureMapInterface::isValidAdventureSpellTarget(int3 targetPosition) const
{
	spells::detail::ProblemImpl problem;

	return spellBeingCasted->getAdventureMechanics().canBeCastAt(problem, LOCPLINT->cb.get(), LOCPLINT->localState->getCurrentHero(), targetPosition);
}
