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
#include "../gui/CursorHandler.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../render/Canvas.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"
#include "../render/IScreenHandler.h"
#include "../PlayerLocalState.h"
#include "../CPlayerInterface.h"

#include "../../lib/mapping/CMap.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/IGameSettings.h"
#include "../../lib/StartInfo.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/pathfinder/CGPathNode.h"
#include "../../lib/pathfinder/TurnInfo.h"
#include "../../lib/spells/adventure/AdventureSpellEffect.h"
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
	pos.w = ENGINE->screenDimensions().x;
	pos.h = ENGINE->screenDimensions().y;

	shortcuts = std::make_shared<AdventureMapShortcuts>(*this);

	widget = std::make_shared<AdventureMapWidget>(shortcuts);
	shortcuts->setState(EAdventureState::MAKING_TURN);
	widget->getMapView()->onViewMapActivated();

	if(GAME->interface()->cb->getStartInfo()->turnTimerInfo.turnTimer != 0)
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

	if (h && h == GAME->interface()->localState->getCurrentHero() && !widget->getInfoBar()->showingComponents())
		widget->getInfoBar()->showSelection();

	widget->updateActiveState();
}

void AdventureMapInterface::onTownChanged(const CGTownInstance * town)
{
	widget->getTownList()->updateElement(town);

	if (town && town == GAME->interface()->localState->getCurrentTown() && !widget->getInfoBar()->showingComponents())
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

	if(GAME->interface())
	{
		GAME->interface()->cingconsole->activate();
		GAME->interface()->cingconsole->pos = this->pos;
	}

	ENGINE->fakeMouseMove(); //to restore the cursor

	// workaround for an edge case:
	// if player unequips Angel Wings / Boots of Levitation of currently active hero
	// game will correctly invalidate paths but current route will not be updated since verifyPath() is not called for current hero
	if (GAME->interface()->makingTurn && GAME->interface()->localState->getCurrentHero())
		GAME->interface()->localState->verifyPath(GAME->interface()->localState->getCurrentHero());
}

void AdventureMapInterface::deactivate()
{
	CIntObject::deactivate();
	ENGINE->cursor().set(Cursor::Map::POINTER);

	if(GAME->interface())
		GAME->interface()->cingconsole->deactivate();
}

void AdventureMapInterface::showAll(Canvas & to)
{
	CIntObject::showAll(to);
	dim(to);
	GAME->interface()->cingconsole->show(to);
}

void AdventureMapInterface::show(Canvas & to)
{
	CIntObject::show(to);
	dim(to);
	GAME->interface()->cingconsole->show(to);
}

void AdventureMapInterface::dim(Canvas & to)
{
	auto const isBigWindow = [&](std::shared_ptr<CIntObject> window) { return window->pos.w >= 800 && window->pos.h >= 600; }; // OH3 fullscreen

	if(settings["adventure"]["hideBackground"].Bool())
		for (auto window : ENGINE->windows().findWindows<CIntObject>())
		{
			if(!std::dynamic_pointer_cast<AdventureMapInterface>(window) && std::dynamic_pointer_cast<CIntObject>(window) && isBigWindow(window))
			{
				to.fillTexture(ENGINE->renderHandler().loadImage(ImagePath::builtin("DiBoxBck"), EImageBlitMode::OPAQUE));
				return;
			}
		}
	for (auto window : ENGINE->windows().findWindows<CIntObject>())
	{
		if (!std::dynamic_pointer_cast<AdventureMapInterface>(window) && !std::dynamic_pointer_cast<RadialMenu>(window) && !window->isPopupWindow() && (settings["adventure"]["backgroundDimSmallWindows"].Bool() || isBigWindow(window) || shortcuts->getState() == EAdventureState::HOTSEAT_WAIT))
		{
			Rect targetRect(0, 0, ENGINE->screenDimensions().x, ENGINE->screenDimensions().y);
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

	Point cursorPosition = ENGINE->getCursorPosition();
	Point scrollDirection;

	if (cursorPosition.x < borderScrollWidth)
		scrollDirection.x = -1;

	if (cursorPosition.x > ENGINE->screenDimensions().x - borderScrollWidth)
		scrollDirection.x = +1;

	if (cursorPosition.y < borderScrollWidth)
		scrollDirection.y = -1;

	if (cursorPosition.y > ENGINE->screenDimensions().y - borderScrollWidth)
		scrollDirection.y = +1;

	Point scrollDelta = scrollDirection * scrollDistance;

	bool cursorInScrollArea = scrollDelta != Point(0,0);
	bool scrollingActive = cursorInScrollArea && shortcuts->optionMapScrollingActive() && !scrollingWasBlocked;
	bool scrollingBlocked = ENGINE->isKeyboardCtrlDown() || !settings["adventure"]["borderScroll"].Bool() || !ENGINE->screenHandler().hasFocus();

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
			ENGINE->cursor().set(Cursor::Map::SCROLL_NORTHEAST);
		if(scrollDelta.y > 0)
			ENGINE->cursor().set(Cursor::Map::SCROLL_SOUTHEAST);
		if(scrollDelta.y == 0)
			ENGINE->cursor().set(Cursor::Map::SCROLL_EAST);
	}
	if(scrollDelta.x < 0)
	{
		if(scrollDelta.y < 0)
			ENGINE->cursor().set(Cursor::Map::SCROLL_NORTHWEST);
		if(scrollDelta.y > 0)
			ENGINE->cursor().set(Cursor::Map::SCROLL_SOUTHWEST);
		if(scrollDelta.y == 0)
			ENGINE->cursor().set(Cursor::Map::SCROLL_WEST);
	}

	if (scrollDelta.x == 0)
	{
		if(scrollDelta.y < 0)
			ENGINE->cursor().set(Cursor::Map::SCROLL_NORTH);
		if(scrollDelta.y > 0)
			ENGINE->cursor().set(Cursor::Map::SCROLL_SOUTH);
		if(scrollDelta.y == 0)
			ENGINE->cursor().set(Cursor::Map::POINTER);
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
	ENGINE->fakeMouseMove();
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

		GAME->interface()->localState->verifyPath(hero);
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

void AdventureMapInterface::onMapTilesChanged(boost::optional<FowTilesType> positions)
{
	if (positions)
		widget->getMinimap()->updateTiles(*positions);
	else
		widget->getMinimap()->update();
}

void AdventureMapInterface::onHotseatWaitStarted(PlayerColor playerID)
{
	backgroundDimLevel = 255;

	widget->getMinimap()->setAIRadar(true);
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
	if(playerID == GAME->interface()->playerID || settings["session"]["spectate"].Bool())
	{
		widget->getMinimap()->setAIRadar(false);
		widget->getInfoBar()->showSelection();
	}

	widget->getHeroList()->updateWidget();
	widget->getTownList()->updateWidget();

	const CGHeroInstance * heroToSelect = nullptr;

	// find first non-sleeping hero
	for (auto hero : GAME->interface()->localState->getWanderingHeroes())
	{
		if (!GAME->interface()->localState->isHeroSleeping(hero))
		{
			heroToSelect = hero;
			break;
		}
	}

	//select first hero if available.
	if (heroToSelect != nullptr)
	{
		GAME->interface()->localState->setSelection(heroToSelect);
	}
	else if (GAME->interface()->localState->getOwnedTowns().size())
	{
		GAME->interface()->localState->setSelection(GAME->interface()->localState->getOwnedTown(0));
	}
	else
	{
		GAME->interface()->localState->setSelection(GAME->interface()->localState->getWanderingHero(0));
	}

	onSelectionChanged(GAME->interface()->localState->getCurrentArmy());

	//show new day animation and sound on infobar, except for 1st day of the game
	if (GAME->interface()->cb->getDate(Date::DAY) != 1)
		widget->getInfoBar()->showDate();

	onHeroChanged(nullptr);
	ENGINE->windows().totalRedraw();
	mapAudio->onPlayerTurnStarted();

	if(settings["session"]["autoSkip"].Bool() && !ENGINE->isKeyboardShiftDown())
	{
		if(auto iw = ENGINE->windows().topWindow<CInfoWindow>())
			iw->close();

		ENGINE->dispatchMainThread([this]()
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
		GAME->interface()->performAutosave();
	}

	GAME->interface()->makingTurn = false;
	GAME->interface()->cb->endTurn();

	mapAudio->onPlayerTurnEnded();

	// Normally, game will receive PlayerStartsTurn call almost instantly with new player ID that will switch UI to waiting mode
	// However, when simturns are active it is possible for such call not to come because another player is still acting
	// So find first player other than ours that is acting at the moment and update UI as if he had started turn
	for (auto player = PlayerColor(0); player < PlayerColor::PLAYER_LIMIT; ++player)
	{
		if (player != GAME->interface()->playerID && GAME->interface()->cb->isPlayerMakingTurn(player))
		{
			onEnemyTurnStarted(player, GAME->interface()->cb->getStartInfo()->playerInfos.at(player).isControlledByHuman());
			break;
		}
	}
}

const CGObjectInstance* AdventureMapInterface::getActiveObject(const int3 &mapPos)
{
	std::vector < const CGObjectInstance * > bobjs = GAME->interface()->cb->getBlockingObjs(mapPos);  //blocking objects at tile

	if (bobjs.empty())
		return nullptr;

	return *boost::range::max_element(bobjs, &CMap::compareObjectBlitOrder);
}

void AdventureMapInterface::onTileLeftClicked(const int3 &targetPosition)
{
	if(!shortcuts->optionMapViewActive())
		return;

	const CGObjectInstance *topBlocking = GAME->interface()->cb->isVisible(targetPosition) ? getActiveObject(targetPosition) : nullptr;

	if(spellBeingCasted)
	{
		assert(shortcuts->optionSpellcasting());

		if(isValidAdventureSpellTarget(targetPosition))
			performSpellcasting(targetPosition);
		return;
	}

	if(!GAME->interface()->cb->isVisible(targetPosition))
		return;

	//check if we can select this object
	bool canSelect = topBlocking && topBlocking->ID == Obj::HERO && topBlocking->tempOwner == GAME->interface()->playerID;
	canSelect |= topBlocking && topBlocking->ID == Obj::TOWN && GAME->interface()->cb->getPlayerRelations(GAME->interface()->playerID, topBlocking->tempOwner) != PlayerRelations::ENEMIES;

	if(GAME->interface()->localState->getCurrentArmy()->ID != Obj::HERO) //hero is not selected (presumably town)
	{
		if(GAME->interface()->localState->getCurrentArmy() == topBlocking) //selected town clicked
			GAME->interface()->openTownWindow(static_cast<const CGTownInstance*>(topBlocking));
		else if(canSelect)
			GAME->interface()->localState->setSelection(static_cast<const CArmedInstance*>(topBlocking));
	}
	else if(const CGHeroInstance * currentHero = GAME->interface()->localState->getCurrentHero()) //hero is selected
	{
		const CGPathNode *pn = GAME->interface()->getPathsInfo(currentHero)->getPathInfo(targetPosition);

		const auto shipyard = dynamic_cast<const IShipyard *>(topBlocking);

		if(currentHero == topBlocking) //clicked selected hero
		{
			GAME->interface()->openHeroWindow(currentHero);
			return;
		}
		else if(canSelect && pn->turns == 255 ) //selectable object at inaccessible tile
		{
			GAME->interface()->localState->setSelection(static_cast<const CArmedInstance*>(topBlocking));
			return;
		}
		else if(shipyard != nullptr && pn->turns == 255 && GAME->interface()->cb->getPlayerRelations(GAME->interface()->playerID, topBlocking->tempOwner) != PlayerRelations::ENEMIES)
		{
			GAME->interface()->showShipyardDialogOrProblemPopup(shipyard);
		}
		else //still here? we need to move hero if we clicked end of already selected path or calculate a new path otherwise
		{
			int3 destinationTile = targetPosition;

			if(topBlocking && topBlocking->isVisitable() && !topBlocking->visitableAt(destinationTile) && settings["gameTweaks"]["simpleObjectSelection"].Bool())
				destinationTile = topBlocking->visitablePos();

			if(!settings["adventure"]["showMovePath"].Bool())
			{
				GAME->interface()->localState->setPath(currentHero, destinationTile);
				onHeroChanged(currentHero);				
			}

			if(GAME->interface()->localState->hasPath(currentHero) &&
			   GAME->interface()->localState->getPath(currentHero).endPos() == destinationTile &&
			   !ENGINE->isKeyboardShiftDown())//we'll be moving
			{
				assert(!GAME->map().hasOngoingAnimations());
				if(!GAME->map().hasOngoingAnimations() && GAME->interface()->localState->getPath(currentHero).nextNode().turns == 0)
					GAME->interface()->moveHero(currentHero, GAME->interface()->localState->getPath(currentHero));
				return;
			}
			else
			{
				if(ENGINE->isKeyboardShiftDown()) //normal click behaviour (as no hero selected)
				{
					if(canSelect)
						GAME->interface()->localState->setSelection(static_cast<const CArmedInstance*>(topBlocking));
				}
				else //remove old path and find a new one if we clicked on accessible tile
				{
					GAME->interface()->localState->setPath(currentHero, destinationTile);
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
	//if the player is not ingame (loser, winner, wrong) we are in a shutdown process
	if (!GAME->interface()->cb || GAME->interface()->cb->getPlayerStatus(GAME->interface()->playerID) != EPlayerStatus::INGAME)
		return;
	//may occur just at the start of game (fake move before full initialization)
	if(!GAME->interface()->localState->getCurrentArmy())
		return;

	bool isTargetPositionVisible = GAME->interface()->cb->isVisible(targetPosition);
	const CGObjectInstance *objAtTile = isTargetPositionVisible ? getActiveObject(targetPosition) : nullptr;

	if(spellBeingCasted)
	{
		const auto * hero = GAME->interface()->localState->getCurrentHero();
		const auto * spellEffect = spellBeingCasted->getAdventureMechanics().getEffectAs<AdventureSpellRangedEffect>(hero);
		spells::detail::ProblemImpl problem;

		if(spellEffect && spellEffect->canBeCastAtImpl(problem, GAME->interface()->cb.get(), hero, targetPosition))
			ENGINE->cursor().set(spellEffect->getCursorForTarget(GAME->interface()->cb.get(), hero, targetPosition));
		else
			ENGINE->cursor().set(Cursor::Map::POINTER);

		return;
	}

	if(!isTargetPositionVisible)
	{
		ENGINE->cursor().set(Cursor::Map::POINTER);
		return;
	}

	auto objRelations = PlayerRelations::ALLIES;

	if(objAtTile)
	{
		objRelations = GAME->interface()->cb->getPlayerRelations(GAME->interface()->playerID, objAtTile->tempOwner);
		std::string text = GAME->interface()->localState->getCurrentHero() ? objAtTile->getHoverText(GAME->interface()->localState->getCurrentHero()) : objAtTile->getHoverText(GAME->interface()->playerID);
		boost::replace_all(text,"\n"," ");
		if (ENGINE->isKeyboardCmdDown())
			text.append(" (" + std::to_string(targetPosition.x) + ", " + std::to_string(targetPosition.y) + ", " + std::to_string(targetPosition.z) + ")");
		ENGINE->statusbar()->write(text);
	}
	else if(isTargetPositionVisible)
	{
		std::string tileTooltipText = GAME->map().getTerrainDescr(targetPosition, false);
		if (ENGINE->isKeyboardCmdDown())
			tileTooltipText.append(" (" + std::to_string(targetPosition.x) + ", " + std::to_string(targetPosition.y) + ", " + std::to_string(targetPosition.z) + ")");
		ENGINE->statusbar()->write(tileTooltipText);
	}

	if(GAME->interface()->localState->getCurrentArmy()->ID == Obj::TOWN || ENGINE->isKeyboardShiftDown())
	{
		if(objAtTile)
		{
			if(objAtTile->ID == Obj::TOWN && objRelations != PlayerRelations::ENEMIES)
				ENGINE->cursor().set(Cursor::Map::TOWN);
			else if(objAtTile->ID == Obj::HERO && objRelations == PlayerRelations::SAME_PLAYER)
				ENGINE->cursor().set(Cursor::Map::HERO);
			else
				ENGINE->cursor().set(Cursor::Map::POINTER);
		}
		else
			ENGINE->cursor().set(Cursor::Map::POINTER);
	}
	else if(const CGHeroInstance * hero = GAME->interface()->localState->getCurrentHero())
	{
		std::array<Cursor::Map, 4> cursorMove      = { Cursor::Map::T1_MOVE,       Cursor::Map::T2_MOVE,       Cursor::Map::T3_MOVE,       Cursor::Map::T4_MOVE,       };
		std::array<Cursor::Map, 4> cursorAttack    = { Cursor::Map::T1_ATTACK,     Cursor::Map::T2_ATTACK,     Cursor::Map::T3_ATTACK,     Cursor::Map::T4_ATTACK,     };
		std::array<Cursor::Map, 4> cursorSail      = { Cursor::Map::T1_SAIL,       Cursor::Map::T2_SAIL,       Cursor::Map::T3_SAIL,       Cursor::Map::T4_SAIL,       };
		std::array<Cursor::Map, 4> cursorDisembark = { Cursor::Map::T1_DISEMBARK,  Cursor::Map::T2_DISEMBARK,  Cursor::Map::T3_DISEMBARK,  Cursor::Map::T4_DISEMBARK,  };
		std::array<Cursor::Map, 4> cursorExchange  = { Cursor::Map::T1_EXCHANGE,   Cursor::Map::T2_EXCHANGE,   Cursor::Map::T3_EXCHANGE,   Cursor::Map::T4_EXCHANGE,   };
		std::array<Cursor::Map, 4> cursorVisit     = { Cursor::Map::T1_VISIT,      Cursor::Map::T2_VISIT,      Cursor::Map::T3_VISIT,      Cursor::Map::T4_VISIT,      };
		std::array<Cursor::Map, 4> cursorSailVisit = { Cursor::Map::T1_SAIL_VISIT, Cursor::Map::T2_SAIL_VISIT, Cursor::Map::T3_SAIL_VISIT, Cursor::Map::T4_SAIL_VISIT, };

		const CGPathNode * pathNode = GAME->interface()->getPathsInfo(hero)->getPathInfo(targetPosition);
		assert(pathNode);

		if((ENGINE->isKeyboardAltDown() || settings["gameTweaks"]["forceMovementInfo"].Bool()) && pathNode->reachable()) //overwrite status bar text with movement info
		{
			showMoveDetailsInStatusbar(*hero, *pathNode);
		}

		if (objAtTile && pathNode->action == EPathNodeAction::UNKNOWN)
		{
			if(objAtTile->ID == Obj::TOWN && objRelations != PlayerRelations::ENEMIES)
			{
				ENGINE-> cursor().set(Cursor::Map::TOWN);
				return;
			}
			else if(objAtTile->ID == Obj::HERO && objRelations == PlayerRelations::SAME_PLAYER)
			{
				ENGINE-> cursor().set(Cursor::Map::HERO);
				return;
			}
			else if (objAtTile->ID == Obj::SHIPYARD && objRelations != PlayerRelations::ENEMIES)
			{
				ENGINE-> cursor().set(Cursor::Map::T1_SAIL);
				return;
			}

			if(objAtTile->isVisitable() && !objAtTile->visitableAt(targetPosition) && settings["gameTweaks"]["simpleObjectSelection"].Bool())
				pathNode = GAME->interface()->getPathsInfo(hero)->getPathInfo(objAtTile->visitablePos());
		}

		int turns = pathNode->turns;
		vstd::amin(turns, 3);
		switch(pathNode->action)
		{
		case EPathNodeAction::NORMAL:
		case EPathNodeAction::TELEPORT_NORMAL:
			if(pathNode->layer == EPathfindingLayer::LAND)
				ENGINE->cursor().set(cursorMove[turns]);
			else
				ENGINE->cursor().set(cursorSail[turns]);
			break;

		case EPathNodeAction::VISIT:
		case EPathNodeAction::BLOCKING_VISIT:
		case EPathNodeAction::TELEPORT_BLOCKING_VISIT:
			if(objAtTile && objAtTile->ID == Obj::HERO)
			{
				if(GAME->interface()->localState->getCurrentArmy()  == objAtTile)
					ENGINE->cursor().set(Cursor::Map::HERO);
				else
					ENGINE->cursor().set(cursorExchange[turns]);
			}
			else if(pathNode->layer == EPathfindingLayer::LAND)
				ENGINE->cursor().set(cursorVisit[turns]);
			else if (pathNode->layer == EPathfindingLayer::SAIL &&
					 objAtTile &&
					 objAtTile->isCoastVisitable() &&
					 pathNode->theNodeBefore &&
					 pathNode->theNodeBefore->layer == EPathfindingLayer::LAND )
			{
				// exception - when visiting shipwreck located on coast from land - show 'horse' cursor, not 'ship' cursor
				ENGINE->cursor().set(cursorVisit[turns]);
			}
			else
				ENGINE->cursor().set(cursorSailVisit[turns]);
			break;

		case EPathNodeAction::BATTLE:
		case EPathNodeAction::TELEPORT_BATTLE:
			ENGINE->cursor().set(cursorAttack[turns]);
			break;

		case EPathNodeAction::EMBARK:
			ENGINE->cursor().set(cursorSail[turns]);
			break;

		case EPathNodeAction::DISEMBARK:
			ENGINE->cursor().set(cursorDisembark[turns]);
			break;

		default:
				ENGINE->cursor().set(Cursor::Map::POINTER);
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

	std::string result = LIBRARY->generaltexth->translate("vcmi.adventureMap", pathNode.turns > 0 ? "moveCostDetails" : "moveCostDetailsNoTurns");

	boost::replace_first(result, "%TURNS", std::to_string(pathNode.turns));
	boost::replace_first(result, "%POINTS", std::to_string(movementPointsLastTurnCost));
	boost::replace_first(result, "%REMAINING", std::to_string(remainingPointsAfterMove));
	boost::replace_first(result, "%TOTAL", std::to_string(totalMovementCost));

	ENGINE->statusbar()->write(result);
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

	if(!GAME->interface()->cb->isVisible(mapPos))
	{
		CRClickPopup::createAndPush(LIBRARY->generaltexth->allTexts[61]); //Uncharted Territory
		return;
	}

	const CGObjectInstance * obj = getActiveObject(mapPos);
	if(!obj)
	{
		// Bare or undiscovered terrain
		const TerrainTile * tile = GAME->interface()->cb->getTile(mapPos);
		if(tile)
		{
			std::string hlp = GAME->map().getTerrainDescr(mapPos, true);
			CRClickPopup::createAndPush(hlp);
		}
		return;
	}

	CRClickPopup::createAndPush(obj, ENGINE->getCursorPosition(), ETextAlignment::CENTER);
}

void AdventureMapInterface::enterCastingMode(const CSpell * sp)
{
	spellBeingCasted = sp;
	GAME->interface()->localState->setCurrentSpell(sp->id);
	setState(EAdventureState::CASTING_SPELL);
}

void AdventureMapInterface::exitCastingMode()
{
	assert(spellBeingCasted);
	spellBeingCasted = nullptr;
	setState(EAdventureState::MAKING_TURN);
	GAME->interface()->localState->setCurrentSpell(SpellID::NONE);
}

void AdventureMapInterface::hotkeyAbortCastingMode()
{
	exitCastingMode();
	GAME->interface()->showInfoDialog(LIBRARY->generaltexth->allTexts[731]); //Spell cancelled
}

void AdventureMapInterface::performSpellcasting(const int3 & dest)
{
	SpellID id = spellBeingCasted->id;
	exitCastingMode();
	GAME->interface()->cb->castSpell(GAME->interface()->localState->getCurrentHero(), id, dest);
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
	int selectedIndex = widget->getTownList()->getSelectedIndex();
	widget->getTownList()->selectNext();

	if(selectedIndex == widget->getTownList()->getSelectedIndex())
		widget->getTownList()->refreshSelected();
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
	pos.w = ENGINE->screenDimensions().x;
	pos.h = ENGINE->screenDimensions().y;

	widget = std::make_shared<AdventureMapWidget>(shortcuts);
	widget->getMapView()->onViewMapActivated();
	widget->setPlayerColor(currentPlayerID);
	widget->updateActiveState();
	widget->getMinimap()->update();
	widget->getInfoBar()->showSelection();

	if (GAME->interface() && GAME->interface()->localState->getCurrentArmy())
		widget->getMapView()->onCenteredObject(GAME->interface()->localState->getCurrentArmy());

	adjustActiveness();

	if (widgetActive)
		activate();
}

bool AdventureMapInterface::isValidAdventureSpellTarget(int3 targetPosition) const
{
	spells::detail::ProblemImpl problem;

	return spellBeingCasted->getAdventureMechanics().canBeCastAt(problem, GAME->interface()->cb.get(), GAME->interface()->localState->getCurrentHero(), targetPosition);
}

void AdventureMapInterface::updateActiveState()
{
	widget->updateActiveState();
}
