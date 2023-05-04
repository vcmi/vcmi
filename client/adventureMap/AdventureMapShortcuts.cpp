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
#include "../PlayerLocalState.h"
#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../lobby/CSavingScreen.h"
#include "../mapView/mapHandler.h"
#include "../windows/CKingdomInterface.h"
#include "../windows/CSpellWindow.h"
#include "../windows/CTradeWindow.h"
#include "../windows/settings/SettingsMainWindow.h"
#include "CAdventureMapInterface.h"
#include "CAdventureOptions.h"

#include "../../CCallback.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CPathfinder.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapping/CMap.h"

AdventureMapShortcuts::AdventureMapShortcuts(CAdventureMapInterface & owner)
	:owner(owner)
{}

std::vector<AdventureMapShortcutState> AdventureMapShortcuts::getShortcuts()
{
	std::vector<AdventureMapShortcutState> result = {
		{ EShortcut::ADVENTURE_KINGDOM_OVERVIEW, optionDefault(),        [this]() { this->showOverview(); } },
		{ EShortcut::ADVENTURE_EXIT_WORLD_VIEW,  optionDefault(),        [this]() { this->worldViewBack(); } },
		{ EShortcut::ADVENTURE_VIEW_WORLD_X1,    optionDefault(),        [this]() { this->worldViewScale1x(); } },
		{ EShortcut::ADVENTURE_VIEW_WORLD_X2,    optionDefault(),        [this]() { this->worldViewScale2x(); } },
		{ EShortcut::ADVENTURE_VIEW_WORLD_X4,    optionDefault(),        [this]() { this->worldViewScale4x(); } },
		{ EShortcut::ADVENTURE_TOGGLE_MAP_LEVEL, optionHasUnderground(), [this]() { this->switchMapLevel(); } },
		{ EShortcut::ADVENTURE_QUEST_LOG,        optionHasQuests(),      [this]() { this->showQuestlog(); } },
		{ EShortcut::ADVENTURE_TOGGLE_SLEEP,     optionHeroSelected(),   [this]() { this->toggleSleepWake(); } },
		{ EShortcut::ADVENTURE_SET_HERO_ASLEEP,  optionHeroSleeping(),   [this]() { this->setHeroSleeping(); } },
		{ EShortcut::ADVENTURE_SET_HERO_AWAKE,   optionHeroSleeping(),   [this]() { this->setHeroAwake(); } },
		{ EShortcut::ADVENTURE_MOVE_HERO,        optionHeroCanMove(),    [this]() { this->moveHeroAlongPath(); } },
		{ EShortcut::ADVENTURE_CAST_SPELL,       optionHeroSelected(),   [this]() { this->showSpellbook(); } },
		{ EShortcut::ADVENTURE_GAME_OPTIONS,     optionDefault(),        [this]() { this->adventureOptions(); } },
		{ EShortcut::GLOBAL_OPTIONS,             optionDefault(),        [this]() { this->systemOptions(); } },
		{ EShortcut::ADVENTURE_NEXT_HERO,        optionHasNextHero(),    [this]() { this->nextHero(); } },
		{ EShortcut::GAME_END_TURN,              optionDefault(),        [this]() { this->endTurn(); } },
		{ EShortcut::ADVENTURE_THIEVES_GUILD,    optionDefault(),        [this]() { this->showThievesGuild(); } },
		{ EShortcut::ADVENTURE_VIEW_SCENARIO,    optionDefault(),        [this]() { this->showScenarioInfo(); } },
		{ EShortcut::GAME_SAVE_GAME,             optionDefault(),        [this]() { this->saveGame(); } },
		{ EShortcut::GAME_LOAD_GAME,             optionDefault(),        [this]() { this->loadGame(); } },
		{ EShortcut::ADVENTURE_DIG_GRAIL,        optionHeroSelected(),   [this]() { this->digGrail(); } },
		{ EShortcut::ADVENTURE_VIEW_PUZZLE,      optionDefault(),        [this]() { this->viewPuzzleMap(); } },
		{ EShortcut::GAME_RESTART_GAME,          optionDefault(),        [this]() { this->restartGame(); } },
		{ EShortcut::ADVENTURE_VISIT_OBJECT,     optionHeroSelected(),   [this]() { this->visitObject(); } },
		{ EShortcut::ADVENTURE_VIEW_SELECTED,    optionDefault(),        [this]() { this->openObject(); } },
		{ EShortcut::GLOBAL_CANCEL,              optionSpellcasting(),   [this]() { this->abortSpellcasting(); } },
		{ EShortcut::GAME_OPEN_MARKETPLACE,      optionDefault(),        [this]() { this->showMarketplace(); } },
		{ EShortcut::ADVENTURE_NEXT_TOWN,        optionDefault(),        [this]() { this->nextTown(); } },
		{ EShortcut::ADVENTURE_NEXT_OBJECT,      optionDefault(),        [this]() { this->nextObject(); } },
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
	GH.pushIntT<CKingdomInterface>();
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
	// with support for future multi-level maps :)
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
	const CGHeroInstance *h = LOCPLINT->localState->getCurrentHero();
	if (!h)
		return;
	bool newSleep = !LOCPLINT->localState->isHeroSleeping(h);

	if (newSleep)
		LOCPLINT->localState->setHeroAsleep(h);
	else
		LOCPLINT->localState->setHeroAwaken(h);

	owner.onHeroChanged(h);

	if (newSleep)
		nextHero();
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
		LOCPLINT->localState->setHeroAsleep(h);
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
	if (!LOCPLINT->localState->getCurrentHero()) //checking necessary values
		return;

	owner.centerOnObject(LOCPLINT->localState->getCurrentHero());

	GH.pushIntT<CSpellWindow>(LOCPLINT->localState->getCurrentHero(), LOCPLINT, false);
}

void AdventureMapShortcuts::adventureOptions()
{
	GH.pushIntT<CAdventureOptions>();
}

void AdventureMapShortcuts::systemOptions()
{
	GH.pushIntT<SettingsMainWindow>();
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
			if(!LOCPLINT->localState->isHeroSleeping(hero) && hero->movement > 0)
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
	CAdventureOptions::showScenarioInfo();
}

void AdventureMapShortcuts::saveGame()
{
	GH.pushIntT<CSavingScreen>();
}

void AdventureMapShortcuts::loadGame()
{
	LOCPLINT->proposeLoadingGame();
}

void AdventureMapShortcuts::digGrail()
{
	const CGHeroInstance *h = LOCPLINT->localState->getCurrentHero();

	if(h && LOCPLINT->makingTurn)
		LOCPLINT->tryDiggging(h);
	return;
}

void AdventureMapShortcuts::viewPuzzleMap()
{
	LOCPLINT->showPuzzleMap();
}

void AdventureMapShortcuts::restartGame()
{
	LOCPLINT->showYesNoDialog(CGI->generaltexth->translate("vcmi.adventureMap.confirmRestartGame"),
		[](){ GH.pushUserEvent(EUserEvent::RESTART_GAME); }, nullptr);
}

void AdventureMapShortcuts::visitObject()
{
	const CGHeroInstance *h = LOCPLINT->localState->getCurrentHero();

	if(h)
		LOCPLINT->cb->moveHero(h,h->pos);
}

void AdventureMapShortcuts::openObject()
{
	const CGHeroInstance *h = LOCPLINT->localState->getCurrentHero();
	const CGTownInstance *t = LOCPLINT->localState->getCurrentTown();
	if(h)
		LOCPLINT->openHeroWindow(h);

	if(t)
		LOCPLINT->openTownWindow(t);

	return;
}

void AdventureMapShortcuts::abortSpellcasting()
{
	owner.hotkeyAbortCastingMode();
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
		GH.pushIntT<CMarketplaceWindow>(townWithMarket);
	else //if not - complain
		LOCPLINT->showInfoDialog(CGI->generaltexth->translate("vcmi.adventureMap.noTownWithMarket"));
}

void AdventureMapShortcuts::nextTown()
{
	//TODO
}

void AdventureMapShortcuts::nextObject()
{
	//TODO
}

void AdventureMapShortcuts::moveHeroDirectional(const Point & direction)
{
	owner.hotkeyMoveHeroDirectional(direction);
}

bool AdventureMapShortcuts::optionHasQuests()
{
	return CGI->mh->getMap()->quests.empty();
}

bool AdventureMapShortcuts::optionHasUnderground()
{
	return LOCPLINT->cb->getMapSize().z > 0;
}

bool AdventureMapShortcuts::optionMapLevelSurface()
{
	return false; //TODO
}

bool AdventureMapShortcuts::optionHeroSleeping()
{
	const CGHeroInstance *hero = LOCPLINT->localState->getCurrentHero();
	return hero && LOCPLINT->localState->isHeroSleeping(hero);
}

bool AdventureMapShortcuts::optionHeroSelected()
{
	return LOCPLINT->localState->getCurrentHero() != nullptr;
}

bool AdventureMapShortcuts::optionHeroCanMove()
{
	const auto * hero = LOCPLINT->localState->getCurrentHero();
	return hero && hero->movement != 0 && LOCPLINT->localState->hasPath(hero);
}

bool AdventureMapShortcuts::optionHasNextHero()
{
	const auto * hero = LOCPLINT->localState->getCurrentHero();
	const auto * nextSuitableHero = LOCPLINT->localState->getNextWanderingHero(hero);

	return nextSuitableHero != nullptr;
}

bool AdventureMapShortcuts::optionSpellcasting()
{
	return true; //TODO
}

bool AdventureMapShortcuts::optionDefault()
{
	return true; //TODO
}
