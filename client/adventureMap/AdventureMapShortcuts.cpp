/*
 * AdventureMapShortcuts.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "AdventureMapShortcuts.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../CServerHandler.h"
#include "../PlayerLocalState.h"
#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../lobby/CSavingScreen.h"
#include "../mapView/mapHandler.h"
#include "../windows/CKingdomInterface.h"
#include "../windows/CSpellWindow.h"
#include "../windows/CTradeWindow.h"
#include "../windows/settings/SettingsMainWindow.h"
#include "AdventureMapInterface.h"
#include "AdventureOptions.h"
#include "AdventureState.h"

#include "../../CCallback.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/pathfinder/CGPathNode.h"

AdventureMapShortcuts::AdventureMapShortcuts(AdventureMapInterface & owner)
	: owner(owner)
	, state(EAdventureState::NOT_INITIALIZED)
	, mapLevel(0)
{}

void AdventureMapShortcuts::setState(EAdventureState newState)
{
	state = newState;
}

void AdventureMapShortcuts::onMapViewMoved(const Rect & visibleArea, int newMapLevel)
{
	mapLevel = newMapLevel;
}

std::vector<AdventureMapShortcutState> AdventureMapShortcuts::getShortcuts()
{
	std::vector<AdventureMapShortcutState> result = {
		{ EShortcut::ADVENTURE_KINGDOM_OVERVIEW, optionInMapView(),      [this]() { this->showOverview(); } },
		{ EShortcut::ADVENTURE_EXIT_WORLD_VIEW,  optionInWorldView(),    [this]() { this->worldViewBack(); } },
		{ EShortcut::ADVENTURE_VIEW_WORLD,       optionInMapView(),      [this]() { this->worldViewScale1x(); } },
		{ EShortcut::ADVENTURE_VIEW_WORLD_X1,    optionInWorldView(),    [this]() { this->worldViewScale1x(); } },
		{ EShortcut::ADVENTURE_VIEW_WORLD_X2,    optionInWorldView(),    [this]() { this->worldViewScale2x(); } },
		{ EShortcut::ADVENTURE_VIEW_WORLD_X4,    optionInWorldView(),    [this]() { this->worldViewScale4x(); } },
		{ EShortcut::ADVENTURE_TOGGLE_MAP_LEVEL, optionCanToggleLevel(), [this]() { this->switchMapLevel(); } },
		{ EShortcut::ADVENTURE_QUEST_LOG,        optionCanViewQuests(),  [this]() { this->showQuestlog(); } },
		{ EShortcut::ADVENTURE_TOGGLE_SLEEP,     optionHeroSelected(),   [this]() { this->toggleSleepWake(); } },
		{ EShortcut::ADVENTURE_SET_HERO_ASLEEP,  optionHeroAwake(),      [this]() { this->setHeroSleeping(); } },
		{ EShortcut::ADVENTURE_SET_HERO_AWAKE,   optionHeroSleeping(),   [this]() { this->setHeroAwake(); } },
		{ EShortcut::ADVENTURE_MOVE_HERO,        optionHeroCanMove(),    [this]() { this->moveHeroAlongPath(); } },
		{ EShortcut::ADVENTURE_CAST_SPELL,       optionHeroSelected(),   [this]() { this->showSpellbook(); } },
		{ EShortcut::ADVENTURE_GAME_OPTIONS,     optionInMapView(),      [this]() { this->adventureOptions(); } },
		{ EShortcut::GLOBAL_OPTIONS,             optionInMapView(),      [this]() { this->systemOptions(); } },
		{ EShortcut::ADVENTURE_NEXT_HERO,        optionHasNextHero(),    [this]() { this->nextHero(); } },
		{ EShortcut::GAME_END_TURN,              optionInMapView(),      [this]() { this->endTurn(); } },
		{ EShortcut::ADVENTURE_THIEVES_GUILD,    optionInMapView(),      [this]() { this->showThievesGuild(); } },
		{ EShortcut::ADVENTURE_VIEW_SCENARIO,    optionInMapView(),      [this]() { this->showScenarioInfo(); } },
		{ EShortcut::GAME_SAVE_GAME,             optionInMapView(),      [this]() { this->saveGame(); } },
		{ EShortcut::GAME_LOAD_GAME,             optionInMapView(),      [this]() { this->loadGame(); } },
		{ EShortcut::ADVENTURE_DIG_GRAIL,        optionHeroSelected(),   [this]() { this->digGrail(); } },
		{ EShortcut::ADVENTURE_VIEW_PUZZLE,      optionSidePanelActive(),[this]() { this->viewPuzzleMap(); } },
		{ EShortcut::GAME_RESTART_GAME,          optionInMapView(),      [this]() { this->restartGame(); } },
		{ EShortcut::ADVENTURE_VISIT_OBJECT,     optionHeroSelected(),   [this]() { this->visitObject(); } },
		{ EShortcut::ADVENTURE_VIEW_SELECTED,    optionInMapView(),      [this]() { this->openObject(); } },
		{ EShortcut::GAME_OPEN_MARKETPLACE,      optionInMapView(),      [this]() { this->showMarketplace(); } },
		{ EShortcut::ADVENTURE_ZOOM_IN,          optionSidePanelActive(),[this]() { this->zoom(+1); } },
		{ EShortcut::ADVENTURE_ZOOM_OUT,         optionSidePanelActive(),[this]() { this->zoom(-1); } },
		{ EShortcut::ADVENTURE_ZOOM_RESET,       optionSidePanelActive(),[this]() { this->zoom( 0); } },
		{ EShortcut::ADVENTURE_NEXT_TOWN,        optionInMapView(),      [this]() { this->nextTown(); } },
		{ EShortcut::ADVENTURE_NEXT_OBJECT,      optionInMapView(),      [this]() { this->nextObject(); } },
		{ EShortcut::ADVENTURE_MOVE_HERO_SW,     optionHeroSelected(),   [this]() { this->moveHeroDirectional({-1, +1}); } },
		{ EShortcut::ADVENTURE_MOVE_HERO_SS,     optionHeroSelected(),   [this]() { this->moveHeroDirectional({ 0, +1}); } },
		{ EShortcut::ADVENTURE_MOVE_HERO_SE,     optionHeroSelected(),   [this]() { this->moveHeroDirectional({+1, +1}); } },
		{ EShortcut::ADVENTURE_MOVE_HERO_WW,     optionHeroSelected(),   [this]() { this->moveHeroDirectional({-1,  0}); } },
		{ EShortcut::ADVENTURE_MOVE_HERO_EE,     optionHeroSelected(),   [this]() { this->moveHeroDirectional({+1,  0}); } },
		{ EShortcut::ADVENTURE_MOVE_HERO_NW,     optionHeroSelected(),   [this]() { this->moveHeroDirectional({-1, -1}); } },
		{ EShortcut::ADVENTURE_MOVE_HERO_NN,     optionHeroSelected(),   [this]() { this->moveHeroDirectional({ 0, -1}); } },
		{ EShortcut::ADVENTURE_MOVE_HERO_NE,     optionHeroSelected(),   [this]() { this->moveHeroDirectional({+1, -1}); } }
	};
	return result;
}

void AdventureMapShortcuts::showOverview()
{
	GH.windows().createAndPushWindow<CKingdomInterface>();
}

void AdventureMapShortcuts::worldViewBack()
{
	owner.hotkeyExitWorldView();

	auto hero = LOCPLINT->localState->getCurrentHero();
	if (hero)
		owner.centerOnObject(hero);
}

void AdventureMapShortcuts::worldViewScale1x()
{
	// TODO set corresponding scale button to "selected" mode
	owner.openWorldView(7);
}

void AdventureMapShortcuts::worldViewScale2x()
{
	owner.openWorldView(11);
}

void AdventureMapShortcuts::worldViewScale4x()
{
	owner.openWorldView(16);
}

void AdventureMapShortcuts::switchMapLevel()
{
	int maxLevels = LOCPLINT->cb->getMapSize().z;
	if (maxLevels < 2)
		return;

	owner.hotkeySwitchMapLevel();
}

void AdventureMapShortcuts::showQuestlog()
{
	LOCPLINT->showQuestLog();
}

void AdventureMapShortcuts::toggleSleepWake()
{
	if (!optionHeroSelected())
		return;

	if (optionHeroAwake())
		setHeroSleeping();
	else
		setHeroAwake();
}

void AdventureMapShortcuts::setHeroSleeping()
{
	const CGHeroInstance *h = LOCPLINT->localState->getCurrentHero();
	if (h)
	{
		LOCPLINT->localState->setHeroAsleep(h);
		owner.onHeroChanged(h);
		nextHero();
	}
}

void AdventureMapShortcuts::setHeroAwake()
{
	const CGHeroInstance *h = LOCPLINT->localState->getCurrentHero();
	if (h)
	{
		LOCPLINT->localState->setHeroAwaken(h);
		owner.onHeroChanged(h);
	}
}

void AdventureMapShortcuts::moveHeroAlongPath()
{
	const CGHeroInstance *h = LOCPLINT->localState->getCurrentHero();
	if (!h || !LOCPLINT->localState->hasPath(h))
		return;

	LOCPLINT->moveHero(h, LOCPLINT->localState->getPath(h));
}

void AdventureMapShortcuts::showSpellbook()
{
	if (!LOCPLINT->localState->getCurrentHero())
		return;

	owner.centerOnObject(LOCPLINT->localState->getCurrentHero());

	GH.windows().createAndPushWindow<CSpellWindow>(LOCPLINT->localState->getCurrentHero(), LOCPLINT, false);
}

void AdventureMapShortcuts::adventureOptions()
{
	GH.windows().createAndPushWindow<AdventureOptions>();
}

void AdventureMapShortcuts::systemOptions()
{
	GH.windows().createAndPushWindow<SettingsMainWindow>();
}

void AdventureMapShortcuts::nextHero()
{
	const auto * currHero = LOCPLINT->localState->getCurrentHero();
	const auto * nextHero = LOCPLINT->localState->getNextWanderingHero(currHero);

	if (nextHero)
	{
		LOCPLINT->localState->setSelection(nextHero);
		owner.centerOnObject(nextHero);
	}
}

void AdventureMapShortcuts::endTurn()
{
	if(!LOCPLINT->makingTurn)
		return;

	if(settings["adventure"]["heroReminder"].Bool())
	{
		for(auto hero : LOCPLINT->localState->getWanderingHeroes())
		{
			if(!LOCPLINT->localState->isHeroSleeping(hero) && hero->movementPointsRemaining() > 0)
			{
				// Only show hero reminder if conditions met:
				// - There still movement points
				// - Hero don't have a path or there not points for first step on path
				LOCPLINT->localState->verifyPath(hero);

				if(!LOCPLINT->localState->hasPath(hero))
				{
					LOCPLINT->showYesNoDialog( CGI->generaltexth->allTexts[55], [this](){ owner.hotkeyEndingTurn(); }, nullptr);
					return;
				}

				auto path = LOCPLINT->localState->getPath(hero);
				if (path.nodes.size() < 2 || path.nodes[path.nodes.size() - 2].turns)
				{
					LOCPLINT->showYesNoDialog( CGI->generaltexth->allTexts[55], [this](){ owner.hotkeyEndingTurn(); }, nullptr);
					return;
				}
			}
		}
	}
	owner.hotkeyEndingTurn();
}

void AdventureMapShortcuts::showThievesGuild()
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

void AdventureMapShortcuts::showScenarioInfo()
{
	AdventureOptions::showScenarioInfo();
}

void AdventureMapShortcuts::saveGame()
{
	GH.windows().createAndPushWindow<CSavingScreen>();
}

void AdventureMapShortcuts::loadGame()
{
	LOCPLINT->proposeLoadingGame();
}

void AdventureMapShortcuts::digGrail()
{
	const CGHeroInstance *h = LOCPLINT->localState->getCurrentHero();

	if(h && LOCPLINT->makingTurn)
		LOCPLINT->tryDigging(h);
}

void AdventureMapShortcuts::viewPuzzleMap()
{
	LOCPLINT->showPuzzleMap();
}

void AdventureMapShortcuts::restartGame()
{
	LOCPLINT->showYesNoDialog(
		CGI->generaltexth->translate("vcmi.adventureMap.confirmRestartGame"),
		[]()
		{
			GH.dispatchMainThread(
				[]()
				{
					CSH->sendRestartGame();
				}
			);
		},
		nullptr
	);
}

void AdventureMapShortcuts::visitObject()
{
	const CGHeroInstance *h = LOCPLINT->localState->getCurrentHero();

	if(h)
		LOCPLINT->cb->moveHero(h, h->pos);
}

void AdventureMapShortcuts::openObject()
{
	const CGHeroInstance *h = LOCPLINT->localState->getCurrentHero();
	const CGTownInstance *t = LOCPLINT->localState->getCurrentTown();
	if(h)
		LOCPLINT->openHeroWindow(h);

	if(t)
		LOCPLINT->openTownWindow(t);
}

void AdventureMapShortcuts::showMarketplace()
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
		GH.windows().createAndPushWindow<CMarketplaceWindow>(townWithMarket);
	else //if not - complain
		LOCPLINT->showInfoDialog(CGI->generaltexth->translate("vcmi.adventureMap.noTownWithMarket"));
}

void AdventureMapShortcuts::nextTown()
{
	owner.hotkeyNextTown();
}

void AdventureMapShortcuts::zoom( int distance)
{
	owner.hotkeyZoom(distance);
}

void AdventureMapShortcuts::nextObject()
{
	const CGHeroInstance *h = LOCPLINT->localState->getCurrentHero();
	const CGTownInstance *t = LOCPLINT->localState->getCurrentTown();
	if(h)
		nextHero();

	if(t)
		nextTown();
}

void AdventureMapShortcuts::moveHeroDirectional(const Point & direction)
{
	const CGHeroInstance *h = LOCPLINT->localState->getCurrentHero(); //selected hero

	if(!h)
		return;

	if (CGI->mh->hasOngoingAnimations())
		return;

	int3 dst = h->visitablePos() + int3(direction.x, direction.y, 0);

	if (!CGI->mh->isInMap((dst)))
		return;

	if ( !LOCPLINT->localState->setPath(h, dst))
		return;

	const CGPath & path = LOCPLINT->localState->getPath(h);

	if (path.nodes.size() > 2)
		owner.onHeroChanged(h);
	else
		if(path.nodes[0].turns == 0)
			LOCPLINT->moveHero(h, path);
}

bool AdventureMapShortcuts::optionCanViewQuests()
{
	return optionInMapView() && CGI->mh->getMap()->quests.empty();
}

bool AdventureMapShortcuts::optionCanToggleLevel()
{
	return optionInMapView() && LOCPLINT->cb->getMapSize().z > 1;
}

bool AdventureMapShortcuts::optionMapLevelSurface()
{
	return mapLevel == 0;
}

bool AdventureMapShortcuts::optionHeroSleeping()
{
	const CGHeroInstance *hero = LOCPLINT->localState->getCurrentHero();
	return optionInMapView() && hero && LOCPLINT->localState->isHeroSleeping(hero);
}

bool AdventureMapShortcuts::optionHeroAwake()
{
	const CGHeroInstance *hero = LOCPLINT->localState->getCurrentHero();
	return optionInMapView() && hero && !LOCPLINT->localState->isHeroSleeping(hero);
}

bool AdventureMapShortcuts::optionHeroSelected()
{
	return optionInMapView() && LOCPLINT->localState->getCurrentHero() != nullptr;
}

bool AdventureMapShortcuts::optionHeroCanMove()
{
	const auto * hero = LOCPLINT->localState->getCurrentHero();
	return optionInMapView() && hero && hero->movementPointsRemaining() != 0 && LOCPLINT->localState->hasPath(hero);
}

bool AdventureMapShortcuts::optionHasNextHero()
{
	const auto * hero = LOCPLINT->localState->getCurrentHero();
	const auto * nextSuitableHero = LOCPLINT->localState->getNextWanderingHero(hero);

	return optionInMapView() && nextSuitableHero != nullptr;
}

bool AdventureMapShortcuts::optionSpellcasting()
{
	return state == EAdventureState::CASTING_SPELL;
}

bool AdventureMapShortcuts::optionInMapView()
{
	return state == EAdventureState::MAKING_TURN;
}

bool AdventureMapShortcuts::optionInWorldView()
{
	return state == EAdventureState::WORLD_VIEW;
}

bool AdventureMapShortcuts::optionSidePanelActive()
{
	return state == EAdventureState::MAKING_TURN || state == EAdventureState::WORLD_VIEW;
}

bool AdventureMapShortcuts::optionMapScrollingActive()
{
	return state == EAdventureState::MAKING_TURN || state == EAdventureState::WORLD_VIEW || (state == EAdventureState::OTHER_HUMAN_PLAYER_TURN);
}

bool AdventureMapShortcuts::optionMapViewActive()
{
	return state == EAdventureState::MAKING_TURN || state == EAdventureState::WORLD_VIEW || state == EAdventureState::CASTING_SPELL
		|| (state == EAdventureState::OTHER_HUMAN_PLAYER_TURN);
}
