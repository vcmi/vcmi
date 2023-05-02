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

#include "../windows/CKingdomInterface.h"
#include "../windows/CSpellWindow.h"
#include "../windows/CTradeWindow.h"
#include "../lobby/CSavingScreen.h"

#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../CPlayerInterface.h"
#include "../PlayerLocalState.h"
#include "../CGameInfo.h"
#include "CAdventureMapInterface.h"
#include "CAdventureOptions.h"
#include "../windows/settings/SettingsMainWindow.h"

#include "../../CCallback.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/CPathfinder.h"

AdventureMapShortcuts::AdventureMapShortcuts(CAdventureMapInterface & owner)
	:owner(owner)
{}

std::map<EShortcut, std::function<void()>> AdventureMapShortcuts::getFunctors()
{
	std::map<EShortcut, std::function<void()>> result = {
		{ EShortcut::ADVENTURE_KINGDOM_OVERVIEW, [this]() { this->showOverview(); } },
		{ EShortcut::NONE, [this]() { this->worldViewBack(); } },
		{ EShortcut::NONE, [this]() { this->worldViewScale1x(); } },
		{ EShortcut::NONE, [this]() { this->worldViewScale2x(); } },
		{ EShortcut::NONE, [this]() { this->worldViewScale4x(); } },
		{ EShortcut::ADVENTURE_TOGGLE_MAP_LEVEL, [this]() { this->switchMapLevel(); } },
		{ EShortcut::ADVENTURE_QUEST_LOG, [this]() { this->showQuestlog(); } },
		{ EShortcut::ADVENTURE_TOGGLE_SLEEP, [this]() { this->toggleSleepWake(); } },
		{ EShortcut::ADVENTURE_SET_HERO_ASLEEP, [this]() { this->setHeroSleeping(); } },
		{ EShortcut::ADVENTURE_SET_HERO_AWAKE, [this]() { this->setHeroAwake(); } },
		{ EShortcut::ADVENTURE_MOVE_HERO, [this]() { this->moveHeroAlongPath(); } },
		{ EShortcut::ADVENTURE_CAST_SPELL, [this]() { this->showSpellbook(); } },
		{ EShortcut::ADVENTURE_GAME_OPTIONS, [this]() { this->adventureOptions(); } },
		{ EShortcut::GLOBAL_OPTIONS, [this]() { this->systemOptions(); } },
		{ EShortcut::ADVENTURE_NEXT_HERO, [this]() { this->nextHero(); } },
		{ EShortcut::GAME_END_TURN, [this]() { this->endTurn(); } },
		{ EShortcut::ADVENTURE_THIEVES_GUILD, [this]() { this->showThievesGuild(); } },
		{ EShortcut::ADVENTURE_VIEW_SCENARIO, [this]() { this->showScenarioInfo(); } },
		{ EShortcut::GAME_SAVE_GAME, [this]() { this->saveGame(); } },
		{ EShortcut::GAME_LOAD_GAME, [this]() { this->loadGame(); } },
		{ EShortcut::ADVENTURE_DIG_GRAIL, [this]() { this->digGrail(); } },
		{ EShortcut::ADVENTURE_VIEW_PUZZLE, [this]() { this->viewPuzzleMap(); } },
		{ EShortcut::ADVENTURE_VIEW_WORLD, [this]() { this->viewWorldMap(); } },
		{ EShortcut::GAME_RESTART_GAME, [this]() { this->restartGame(); } },
		{ EShortcut::ADVENTURE_VISIT_OBJECT, [this]() { this->visitObject(); } },
		{ EShortcut::ADVENTURE_VIEW_SELECTED, [this]() { this->openObject(); } },
		{ EShortcut::GLOBAL_CANCEL, [this]() { this->abortSpellcasting(); } },
		{ EShortcut::GAME_OPEN_MARKETPLACE, [this]() { this->showMarketplace(); } },
		{ EShortcut::ADVENTURE_NEXT_TOWN, [this]() { this->nextTown(); } },
//		{ EShortcut::ADVENTURE_NEXT_OBJECT, [this]() { this->nextObject(); } },
		{ EShortcut::ADVENTURE_MOVE_HERO_SW, [this]() { this->moveHeroDirectional({-1, +1}); } },
		{ EShortcut::ADVENTURE_MOVE_HERO_SS, [this]() { this->moveHeroDirectional({ 0, +1}); } },
		{ EShortcut::ADVENTURE_MOVE_HERO_SE, [this]() { this->moveHeroDirectional({+1, +1}); } },
		{ EShortcut::ADVENTURE_MOVE_HERO_WW, [this]() { this->moveHeroDirectional({-1,  0}); } },
		{ EShortcut::ADVENTURE_MOVE_HERO_EE, [this]() { this->moveHeroDirectional({+1,  0}); } },
		{ EShortcut::ADVENTURE_MOVE_HERO_NW, [this]() { this->moveHeroDirectional({-1, -1}); } },
		{ EShortcut::ADVENTURE_MOVE_HERO_NN, [this]() { this->moveHeroDirectional({ 0, -1}); } },
		{ EShortcut::ADVENTURE_MOVE_HERO_NE, [this]() { this->moveHeroDirectional({+1, -1}); } },
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

void AdventureMapShortcuts::viewWorldMap()
{
	LOCPLINT->viewWorldMap();
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
	owner.hotkeyEndingTurn();
}

void AdventureMapShortcuts::moveHeroDirectional(const Point & direction)
{
	owner.hotkeyMoveHeroDirectional(direction);
}
