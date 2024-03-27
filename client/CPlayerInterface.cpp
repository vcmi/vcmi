/*
 * CPlayerInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CPlayerInterface.h"

#include <vcmi/Artifact.h>

#include "CGameInfo.h"
#include "CMT.h"
#include "CMusicHandler.h"
#include "CServerHandler.h"
#include "HeroMovementController.h"
#include "PlayerLocalState.h"

#include "adventureMap/AdventureMapInterface.h"
#include "adventureMap/CInGameConsole.h"
#include "adventureMap/CList.h"

#include "battle/BattleEffectsController.h"
#include "battle/BattleFieldController.h"
#include "battle/BattleInterface.h"
#include "battle/BattleInterfaceClasses.h"
#include "battle/BattleWindow.h"

#include "eventsSDL/InputHandler.h"
#include "eventsSDL/NotificationHandler.h"

#include "gui/CGuiHandler.h"
#include "gui/CursorHandler.h"
#include "gui/WindowHandler.h"

#include "mainmenu/CMainMenu.h"
#include "mainmenu/CHighScoreScreen.h"

#include "mapView/mapHandler.h"

#include "render/CAnimation.h"
#include "render/IImage.h"
#include "render/IRenderHandler.h"

#include "widgets/Buttons.h"
#include "widgets/CComponent.h"
#include "widgets/CGarrisonInt.h"

#include "windows/CAltarWindow.h"
#include "windows/CCastleInterface.h"
#include "windows/CCreatureWindow.h"
#include "windows/CHeroWindow.h"
#include "windows/CKingdomInterface.h"
#include "windows/CPuzzleWindow.h"
#include "windows/CQuestLog.h"
#include "windows/CSpellWindow.h"
#include "windows/CTradeWindow.h"
#include "windows/CTutorialWindow.h"
#include "windows/GUIClasses.h"
#include "windows/InfoWindows.h"

#include "../CCallback.h"

#include "../lib/CArtHandler.h"
#include "../lib/CConfigHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CPlayerState.h"
#include "../lib/CStack.h"
#include "../lib/CStopWatch.h"
#include "../lib/CThreadHelper.h"
#include "../lib/CTownHandler.h"
#include "../lib/CondSh.h"
#include "../lib/GameConstants.h"
#include "../lib/RoadHandler.h"
#include "../lib/StartInfo.h"
#include "../lib/TerrainHandler.h"
#include "../lib/TextOperations.h"
#include "../lib/UnlockGuard.h"
#include "../lib/VCMIDirs.h"

#include "../lib/bonuses/Limiters.h"
#include "../lib/bonuses/Propagators.h"
#include "../lib/bonuses/Updaters.h"

#include "../lib/gameState/CGameState.h"

#include "../lib/mapObjects/CGTownInstance.h"
#include "../lib/mapObjects/MiscObjects.h"
#include "../lib/mapObjects/ObjectTemplate.h"

#include "../lib/mapping/CMapHeader.h"

#include "../lib/networkPacks/PacksForClient.h"
#include "../lib/networkPacks/PacksForClientBattle.h"
#include "../lib/networkPacks/PacksForServer.h"

#include "../lib/pathfinder/CGPathNode.h"

#include "../lib/serializer/BinaryDeserializer.h"
#include "../lib/serializer/BinarySerializer.h"
#include "../lib/serializer/CTypeList.h"

#include "../lib/spells/CSpellHandler.h"

// The macro below is used to mark functions that are called by client when game state changes.
// They all assume that interface mutex is locked.
#define EVENT_HANDLER_CALLED_BY_CLIENT

#define BATTLE_EVENT_POSSIBLE_RETURN	if (LOCPLINT != this) return; if (isAutoFightOn && !battleInt) return

CPlayerInterface * LOCPLINT;

std::shared_ptr<BattleInterface> CPlayerInterface::battleInt;

struct HeroObjectRetriever
{
	const CGHeroInstance * operator()(const ConstTransitivePtr<CGHeroInstance> &h) const
	{
		return h;
	}
	const CGHeroInstance * operator()(const ConstTransitivePtr<CStackInstance> &s) const
	{
		return nullptr;
	}
};

CPlayerInterface::CPlayerInterface(PlayerColor Player):
	localState(std::make_unique<PlayerLocalState>(*this)),
	movementController(std::make_unique<HeroMovementController>())
{
	logGlobal->trace("\tHuman player interface for player %s being constructed", Player.toString());
	GH.defActionsDef = 0;
	LOCPLINT = this;
	playerID=Player;
	human=true;
	battleInt = nullptr;
	castleInt = nullptr;
	makingTurn = false;
	showingDialog = new CondSh<bool>(false);
	cingconsole = new CInGameConsole();
	firstCall = 1; //if loading will be overwritten in serialize
	autosaveCount = 0;
	isAutoFightOn = false;
	isAutoFightEndBattle = false;
	ignoreEvents = false;
	numOfMovedArts = 0;
}

CPlayerInterface::~CPlayerInterface()
{
	logGlobal->trace("\tHuman player interface for player %s being destructed", playerID.toString());
	delete showingDialog;
	delete cingconsole;
	if (LOCPLINT == this)
		LOCPLINT = nullptr;
}
void CPlayerInterface::initGameInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CCallback> CB)
{
	cb = CB;
	env = ENV;

	CCS->musich->loadTerrainMusicThemes();
	initializeHeroTownList();

	adventureInt.reset(new AdventureMapInterface());
}

void CPlayerInterface::playerEndsTurn(PlayerColor player)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (player == playerID)
	{
		makingTurn = false;

		// remove all active dialogs that do not expect query answer
		for (;;)
		{
			auto adventureWindow = GH.windows().topWindow<AdventureMapInterface>();
			auto infoWindow = GH.windows().topWindow<CInfoWindow>();

			if(adventureWindow != nullptr)
				break;

			if(infoWindow && infoWindow->ID != QueryID::NONE)
				break;

			if (infoWindow)
				infoWindow->close();
			else
				GH.windows().popWindows(1);
		}

		// remove all pending dialogs that do not expect query answer
		vstd::erase_if(dialogs, [](const std::shared_ptr<CInfoWindow> & window){
			return window->ID == QueryID::NONE;
		});
	}
}

void CPlayerInterface::playerStartsTurn(PlayerColor player)
{
	if(GH.windows().findWindows<AdventureMapInterface>().empty())
	{
		// after map load - remove all active windows and replace them with adventure map
		GH.windows().clear();
		GH.windows().pushWindow(adventureInt);
	}

	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (player != playerID && LOCPLINT == this)
	{
		waitWhileDialog();

		bool isHuman = cb->getStartInfo()->playerInfos.count(player) && cb->getStartInfo()->playerInfos.at(player).isControlledByHuman();

		if (makingTurn == false)
			adventureInt->onEnemyTurnStarted(player, isHuman);
	}
}

void CPlayerInterface::performAutosave()
{
	int frequency = static_cast<int>(settings["general"]["saveFrequency"].Integer());
	if(frequency > 0 && cb->getDate() % frequency == 0)
	{
		bool usePrefix = settings["general"]["useSavePrefix"].Bool();
		std::string prefix = std::string();

		if(usePrefix)
		{
			prefix = settings["general"]["savePrefix"].String();
			if(prefix.empty())
			{
				std::string name = cb->getMapHeader()->name.toString();
				int txtlen = TextOperations::getUnicodeCharactersCount(name);

				TextOperations::trimRightUnicode(name, std::max(0, txtlen - 15));
				std::string forbiddenChars("\\/:?\"<>| ");
				std::replace_if(name.begin(), name.end(), [&](char c) { return std::string::npos != forbiddenChars.find(c); }, '_' );

				prefix = name + "_" + cb->getStartInfo()->startTimeIso8601 + "/";
			}
		}

		autosaveCount++;

		int autosaveCountLimit = settings["general"]["autosaveCountLimit"].Integer();
		if(autosaveCountLimit > 0)
		{
			cb->save("Saves/Autosave/" + prefix + std::to_string(autosaveCount));
			autosaveCount %= autosaveCountLimit;
		}
		else
		{
			std::string stringifiedDate = std::to_string(cb->getDate(Date::MONTH))
					+ std::to_string(cb->getDate(Date::WEEK))
					+ std::to_string(cb->getDate(Date::DAY_OF_WEEK));

			cb->save("Saves/Autosave/" + prefix + stringifiedDate);
		}
	}
}

void CPlayerInterface::gamePause(bool pause)
{
	cb->gamePause(pause);
}

void CPlayerInterface::yourTurn(QueryID queryID)
{
	CTutorialWindow::openWindowFirstTime(TutorialMode::TOUCH_ADVENTUREMAP);

	EVENT_HANDLER_CALLED_BY_CLIENT;
	{
		LOCPLINT = this;
		GH.curInt = this;

		NotificationHandler::notify("Your turn");
		if(settings["general"]["startTurnAutosave"].Bool())
		{
			performAutosave();
		}

		if (CSH->howManyPlayerInterfaces() > 1) //hot seat message
		{
			adventureInt->onHotseatWaitStarted(playerID);

			makingTurn = true;
			std::string msg = CGI->generaltexth->allTexts[13];
			boost::replace_first(msg, "%s", cb->getStartInfo()->playerInfos.find(playerID)->second.name);
			std::vector<std::shared_ptr<CComponent>> cmp;
			cmp.push_back(std::make_shared<CComponent>(ComponentType::FLAG, playerID));
			showInfoDialog(msg, cmp);
		}
		else
		{
			makingTurn = true;
			adventureInt->onPlayerTurnStarted(playerID);
		}
	}
	acceptTurn(queryID);
}

void CPlayerInterface::acceptTurn(QueryID queryID)
{
	if (settings["session"]["autoSkip"].Bool())
	{
		while(auto iw = GH.windows().topWindow<CInfoWindow>())
			iw->close();
	}

	if(CSH->howManyPlayerInterfaces() > 1)
	{
		waitWhileDialog(); // wait for player to accept turn in hot-seat mode

		adventureInt->onPlayerTurnStarted(playerID);
	}

	// warn player if he has no town
	if (cb->howManyTowns() == 0)
	{
		auto playerColor = *cb->getPlayerID();

		std::vector<Component> components;
		components.emplace_back(ComponentType::FLAG, playerColor);
		MetaString text;

		const auto & optDaysWithoutCastle = cb->getPlayerState(playerColor)->daysWithoutCastle;

		if(optDaysWithoutCastle)
		{
			auto daysWithoutCastle = optDaysWithoutCastle.value();
			if (daysWithoutCastle < 6)
			{
				text.appendLocalString(EMetaText::ARRAY_TXT,128); //%s, you only have %d days left to capture a town or you will be banished from this land.
				text.replaceName(playerColor);
				text.replaceNumber(7 - daysWithoutCastle);
			}
			else if (daysWithoutCastle == 6)
			{
				text.appendLocalString(EMetaText::ARRAY_TXT,129); //%s, this is your last day to capture a town or you will be banished from this land.
				text.replaceName(playerColor);
			}

			showInfoDialogAndWait(components, text);
		}
		else
			logGlobal->warn("Player has no towns, but daysWithoutCastle is not set");
	}
	
	cb->selectionMade(0, queryID);
	movementController->onPlayerTurnStarted();
}

void CPlayerInterface::heroMoved(const TryMoveHero & details, bool verbose)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();
	if(LOCPLINT != this)
		return;

	//FIXME: read once and store
	if(settings["session"]["spectate"].Bool() && settings["session"]["spectate-ignore-hero"].Bool())
		return;

	const CGHeroInstance * hero = cb->getHero(details.id); //object representing this hero

	if (!hero)
		return;

	movementController->onTryMoveHero(hero, details);
}

void CPlayerInterface::heroKilled(const CGHeroInstance* hero)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	LOG_TRACE_PARAMS(logGlobal, "Hero %s killed handler for player %s", hero->getNameTranslated() % playerID);

	// if hero is not in town garrison
	if (vstd::contains(localState->getWanderingHeroes(), hero))
		localState->removeWanderingHero(hero);

	adventureInt->onHeroChanged(hero);
	localState->erasePath(hero);
}

void CPlayerInterface::heroVisit(const CGHeroInstance * visitor, const CGObjectInstance * visitedObj, bool start)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if(start && visitedObj)
	{
		if(visitedObj->getVisitSound())
			CCS->soundh->playSound(visitedObj->getVisitSound().value());
	}
}

void CPlayerInterface::heroCreated(const CGHeroInstance * hero)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	localState->addWanderingHero(hero);
	adventureInt->onHeroChanged(hero);
}
void CPlayerInterface::openTownWindow(const CGTownInstance * town)
{
	if(castleInt)
		castleInt->close();
	castleInt = nullptr;

	auto newCastleInt = std::make_shared<CCastleInterface>(town);

	GH.windows().pushWindow(newCastleInt);
}

void CPlayerInterface::heroPrimarySkillChanged(const CGHeroInstance * hero, PrimarySkill which, si64 val)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (which == PrimarySkill::EXPERIENCE)
	{
		for (auto ctw : GH.windows().findWindows<CAltarWindow>())
			ctw->updateExpToLevel();
	}
	else
		adventureInt->onHeroChanged(hero);
}

void CPlayerInterface::heroSecondarySkillChanged(const CGHeroInstance * hero, int which, int val)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	for (auto cuw : GH.windows().findWindows<CUniversityWindow>())
		cuw->redraw();
}

void CPlayerInterface::heroManaPointsChanged(const CGHeroInstance * hero)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->onHeroChanged(hero);
	if (makingTurn && hero->tempOwner == playerID)
		adventureInt->onHeroChanged(hero);
}
void CPlayerInterface::heroMovePointsChanged(const CGHeroInstance * hero)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (makingTurn && hero->tempOwner == playerID)
		adventureInt->onHeroChanged(hero);
}
void CPlayerInterface::receivedResource()
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	for (auto mw : GH.windows().findWindows<CMarketplaceWindow>())
		mw->resourceChanged();

	GH.windows().totalRedraw();
}

void CPlayerInterface::heroGotLevel(const CGHeroInstance *hero, PrimarySkill pskill, std::vector<SecondarySkill>& skills, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();
	CCS->soundh->playSound(soundBase::heroNewLevel);
	GH.windows().createAndPushWindow<CLevelWindow>(hero, pskill, skills, [=](ui32 selection)
	{
		cb->selectionMade(selection, queryID);
	});
}

void CPlayerInterface::commanderGotLevel (const CCommanderInstance * commander, std::vector<ui32> skills, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();
	CCS->soundh->playSound(soundBase::heroNewLevel);
	GH.windows().createAndPushWindow<CStackWindow>(commander, skills, [=](ui32 selection)
	{
		cb->selectionMade(selection, queryID);
	});
}

void CPlayerInterface::heroInGarrisonChange(const CGTownInstance *town)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	if(town->garrisonHero) //wandering hero moved to the garrison
	{
		// This method also gets called on hero recruitment -> garrisoned hero is already in garrison
		if(town->garrisonHero->tempOwner == playerID && vstd::contains(localState->getWanderingHeroes(), town->garrisonHero))
			localState->removeWanderingHero(town->garrisonHero);
	}

	if(town->visitingHero) //hero leaves garrison
	{
		// This method also gets called on hero recruitment -> wandering heroes already contains new hero
		if(town->visitingHero->tempOwner == playerID && !vstd::contains(localState->getWanderingHeroes(), town->visitingHero))
			localState->addWanderingHero(town->visitingHero);
	}
	adventureInt->onHeroChanged(nullptr);
	adventureInt->onTownChanged(town);

	for (auto cgh : GH.windows().findWindows<IGarrisonHolder>())
		if (cgh->holdsGarrison(town))
			cgh->updateGarrisons();

	for (auto ki : GH.windows().findWindows<CKingdomInterface>())
		ki->townChanged(town);

	// Perform totalRedraw to update hero list on adventure map, if any dialogs are open
	GH.windows().totalRedraw();
}

void CPlayerInterface::heroVisitsTown(const CGHeroInstance* hero, const CGTownInstance * town)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (hero->tempOwner != playerID )
		return;

	waitWhileDialog();
	openTownWindow(town);
}

void CPlayerInterface::garrisonsChanged(ObjectInstanceID id1, ObjectInstanceID id2)
{
	std::vector<const CArmedInstance *> instances;

	if(auto obj = dynamic_cast<const CArmedInstance *>(cb->getObjInstance(id1)))
		instances.push_back(obj);


	if(id2 != ObjectInstanceID() && id2 != id1)
	{
		if(auto obj = dynamic_cast<const CArmedInstance *>(cb->getObjInstance(id2)))
			instances.push_back(obj);
	}

	garrisonsChanged(instances);
}

void CPlayerInterface::garrisonsChanged(std::vector<const CArmedInstance *> objs)
{
	for (auto object : objs)
	{
		auto * hero = dynamic_cast<const CGHeroInstance*>(object);
		auto * town = dynamic_cast<const CGTownInstance*>(object);

		if (town)
			adventureInt->onTownChanged(town);

		if (hero)
		{
			adventureInt->onHeroChanged(hero);
			if(hero->inTownGarrison && hero->visitedTown != town)
				adventureInt->onTownChanged(hero->visitedTown);
		}
	}

	for (auto cgh : GH.windows().findWindows<IGarrisonHolder>())
		if (cgh->holdsGarrisons(objs))
			cgh->updateGarrisons();

	GH.windows().totalRedraw();
}

void CPlayerInterface::buildChanged(const CGTownInstance *town, BuildingID buildingID, int what) //what: 1 - built, 2 - demolished
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->onTownChanged(town);

	if (castleInt)
	{
		castleInt->townlist->updateElement(town);

		if (castleInt->town == town)
		{
			switch(what)
			{
			case 1:
				CCS->soundh->playSound(soundBase::newBuilding);
				castleInt->addBuilding(buildingID);
				break;
			case 2:
				castleInt->removeBuilding(buildingID);
				break;
			}
		}

		// Perform totalRedraw in order to force redraw of updated town list icon from adventure map
		GH.windows().totalRedraw();
	}

	for (auto cgh : GH.windows().findWindows<ITownHolder>())
		cgh->buildChanged();
}

void CPlayerInterface::battleStartBefore(const BattleID & battleID, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2)
{
	movementController->onBattleStarted();

	//Don't wait for dialogs when we are non-active hot-seat player
	if (LOCPLINT == this)
		waitForAllDialogs();
}

void CPlayerInterface::battleStart(const BattleID & battleID, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side, bool replayAllowed)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	bool useQuickCombat = settings["adventure"]["quickCombat"].Bool();
	bool forceQuickCombat = settings["adventure"]["forceQuickCombat"].Bool();

	if ((replayAllowed && useQuickCombat) || forceQuickCombat)
	{
		autofightingAI = CDynLibHandler::getNewBattleAI(settings["server"]["friendlyAI"].String());

		AutocombatPreferences autocombatPreferences = AutocombatPreferences();
		autocombatPreferences.enableSpellsUsage = settings["battle"]["enableAutocombatSpells"].Bool();

		autofightingAI->initBattleInterface(env, cb, autocombatPreferences);
		autofightingAI->battleStart(battleID, army1, army2, tile, hero1, hero2, side, false);
		isAutoFightOn = true;
		cb->registerBattleInterface(autofightingAI);
	}

	//Don't wait for dialogs when we are non-active hot-seat player
	if (LOCPLINT == this)
		waitForAllDialogs();

	BATTLE_EVENT_POSSIBLE_RETURN;
}

void CPlayerInterface::battleUnitsChanged(const BattleID & battleID, const std::vector<UnitChanges> & units)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	for(auto & info : units)
	{
		switch(info.operation)
		{
		case UnitChanges::EOperation::RESET_STATE:
			{
				const CStack * stack = cb->getBattle(battleID)->battleGetStackByID(info.id );

				if(!stack)
				{
					logGlobal->error("Invalid unit ID %d", info.id);
					continue;
				}
				battleInt->stackReset(stack);
			}
			break;
		case UnitChanges::EOperation::REMOVE:
			battleInt->stackRemoved(info.id);
			break;
		case UnitChanges::EOperation::ADD:
			{
				const CStack * unit = cb->getBattle(battleID)->battleGetStackByID(info.id);
				if(!unit)
				{
					logGlobal->error("Invalid unit ID %d", info.id);
					continue;
				}
				battleInt->stackAdded(unit);
			}
			break;
		default:
			logGlobal->error("Unknown unit operation %d", (int)info.operation);
			break;
		}
	}
}

void CPlayerInterface::battleObstaclesChanged(const BattleID & battleID, const std::vector<ObstacleChanges> & obstacles)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	std::vector<std::shared_ptr<const CObstacleInstance>> newObstacles;
	std::vector<ObstacleChanges> removedObstacles;

	for(auto & change : obstacles)
	{
		if(change.operation == BattleChanges::EOperation::ADD)
		{
			auto instance = cb->getBattle(battleID)->battleGetObstacleByID(change.id);
			if(instance)
				newObstacles.push_back(instance);
			else
				logNetwork->error("Invalid obstacle instance %d", change.id);
		}
		if(change.operation == BattleChanges::EOperation::REMOVE)
			removedObstacles.push_back(change); //Obstacles are already removed, so, show animation based on json struct
	}

	if (!newObstacles.empty())
		battleInt->obstaclePlaced(newObstacles);

	if (!removedObstacles.empty())
		battleInt->obstacleRemoved(removedObstacles);

	battleInt->fieldController->redrawBackgroundWithHexes();
}

void CPlayerInterface::battleCatapultAttacked(const BattleID & battleID, const CatapultAttack & ca)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->stackIsCatapulting(ca);
}

void CPlayerInterface::battleNewRound(const BattleID & battleID) //called at the beginning of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->newRound();
}

void CPlayerInterface::actionStarted(const BattleID & battleID, const BattleAction &action)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->startAction(action);
}

void CPlayerInterface::actionFinished(const BattleID & battleID, const BattleAction &action)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->endAction(action);
}

void CPlayerInterface::activeStack(const BattleID & battleID, const CStack * stack) //called when it's turn of that stack
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	logGlobal->trace("Awaiting command for %s", stack->nodeName());

	assert(!cb->getBattle(battleID)->battleIsFinished());
	if (cb->getBattle(battleID)->battleIsFinished())
	{
		logGlobal->error("Received CPlayerInterface::activeStack after battle is finished!");

		cb->battleMakeUnitAction(battleID, BattleAction::makeDefend(stack));
		return ;
	}

	if (autofightingAI)
	{
		if (isAutoFightOn)
		{
			//FIXME: we want client rendering to proceed while AI is making actions
			// so unlock mutex while AI is busy since this might take quite a while, especially if hero has many spells
			auto unlockInterface = vstd::makeUnlockGuard(GH.interfaceMutex);
			autofightingAI->activeStack(battleID, stack);
			return;
		}

		cb->unregisterBattleInterface(autofightingAI);
		autofightingAI.reset();
	}

	assert(battleInt);
	if(!battleInt)
	{
		// probably battle is finished already
		cb->battleMakeUnitAction(battleID, BattleAction::makeDefend(stack));
	}

	battleInt->stackActivated(stack);
}

void CPlayerInterface::battleEnd(const BattleID & battleID, const BattleResult *br, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if(isAutoFightOn || autofightingAI)
	{
		isAutoFightOn = false;
		cb->unregisterBattleInterface(autofightingAI);
		autofightingAI.reset();

		if(!battleInt)
		{
			bool allowManualReplay = queryID != QueryID::NONE && !isAutoFightEndBattle;

			auto wnd = std::make_shared<BattleResultWindow>(*br, *this, allowManualReplay);

			if (allowManualReplay || isAutoFightEndBattle)
			{
				wnd->resultCallback = [=](ui32 selection)
				{
					cb->selectionMade(selection, queryID);
				};
			}
			
			isAutoFightEndBattle = false;

			GH.windows().pushWindow(wnd);
			// #1490 - during AI turn when quick combat is on, we need to display the message and wait for user to close it.
			// Otherwise NewTurn causes freeze.
			waitWhileDialog();
			return;
		}
	}

	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->battleFinished(*br, queryID);
}

void CPlayerInterface::battleLogMessage(const BattleID & battleID, const std::vector<MetaString> & lines)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->displayBattleLog(lines);
}

void CPlayerInterface::battleStackMoved(const BattleID & battleID, const CStack * stack, std::vector<BattleHex> dest, int distance, bool teleport)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->stackMoved(stack, dest, distance, teleport);
}
void CPlayerInterface::battleSpellCast(const BattleID & battleID, const BattleSpellCast * sc)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->spellCast(sc);
}
void CPlayerInterface::battleStacksEffectsSet(const BattleID & battleID, const SetStackEffect & sse)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->battleStacksEffectsSet(sse);
}
void CPlayerInterface::battleTriggerEffect(const BattleID & battleID, const BattleTriggerEffect & bte)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->effectsController->battleTriggerEffect(bte);

	if(bte.effect == vstd::to_underlying(BonusType::MANA_DRAIN))
	{
		const CGHeroInstance * manaDrainedHero = LOCPLINT->cb->getHero(ObjectInstanceID(bte.additionalInfo));
		battleInt->windowObject->heroManaPointsChanged(manaDrainedHero);
	}
}
void CPlayerInterface::battleStacksAttacked(const BattleID & battleID, const std::vector<BattleStackAttacked> & bsa, bool ranged)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	std::vector<StackAttackedInfo> arg;
	for(auto & elem : bsa)
	{
		const CStack * defender = cb->getBattle(battleID)->battleGetStackByID(elem.stackAttacked, false);
		const CStack * attacker = cb->getBattle(battleID)->battleGetStackByID(elem.attackerID, false);

		assert(defender);

		StackAttackedInfo     info;
		info.defender       = defender;
		info.attacker       = attacker;
		info.damageDealt    = elem.damageAmount;
		info.amountKilled   = elem.killedAmount;
		info.spellEffect    = SpellID::NONE;
		info.indirectAttack = ranged;
		info.killed         = elem.killed();
		info.rebirth        = elem.willRebirth();
		info.cloneKilled    = elem.cloneKilled();
		info.fireShield     = elem.fireShield();

		if (elem.isSpell())
			info.spellEffect = elem.spellID;

		arg.push_back(info);
	}
	battleInt->stacksAreAttacked(arg);
}
void CPlayerInterface::battleAttack(const BattleID & battleID, const BattleAttack * ba)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	StackAttackInfo info;
	info.attacker = cb->getBattle(battleID)->battleGetStackByID(ba->stackAttacking);
	info.defender = nullptr;
	info.indirectAttack = ba->shot();
	info.lucky = ba->lucky();
	info.unlucky = ba->unlucky();
	info.deathBlow = ba->deathBlow();
	info.lifeDrain = ba->lifeDrain();
	info.tile = ba->tile;
	info.spellEffect = SpellID::NONE;

	if (ba->spellLike())
		info.spellEffect = ba->spellID;

	for(auto & elem : ba->bsa)
	{
		if(!elem.isSecondary())
		{
			assert(info.defender == nullptr);
			info.defender = cb->getBattle(battleID)->battleGetStackByID(elem.stackAttacked);
		}
		else
		{
			info.secondaryDefender.push_back(cb->getBattle(battleID)->battleGetStackByID(elem.stackAttacked));
		}
	}
	assert(info.defender != nullptr);
	assert(info.attacker != nullptr);

	battleInt->stackAttacking(info);
}

void CPlayerInterface::battleGateStateChanged(const BattleID & battleID, const EGateState state)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->gateStateChanged(state);
}

void CPlayerInterface::yourTacticPhase(const BattleID & battleID, int distance)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
}

void CPlayerInterface::showInfoDialog(EInfoWindowMode type, const std::string &text, const std::vector<Component> & components, int soundID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	bool autoTryHover = settings["gameTweaks"]["infoBarPick"].Bool() && type == EInfoWindowMode::AUTO;
	auto timer = type == EInfoWindowMode::INFO ? 3000 : 4500; //Implement long info windows like in HD mod

	if(autoTryHover || type == EInfoWindowMode::INFO)
	{
		waitWhileDialog(); //Fix for mantis #98
		adventureInt->showInfoBoxMessage(components, text, timer);

		// abort movement, if any. Strictly speaking unnecessary, but prevents some edge cases, like movement sound on visiting Magi Hut with "show messages in status window" on
		movementController->requestMovementAbort();

		if (makingTurn && GH.windows().count() > 0 && LOCPLINT == this)
			CCS->soundh->playSound(static_cast<soundBase::soundID>(soundID));
		return;
	}

	if (settings["session"]["autoSkip"].Bool() && !GH.isKeyboardShiftDown())
	{
		return;
	}
	std::vector<Component> vect = components; //I do not know currently how to avoid copy here
	do
	{
		std::vector<Component> sender = {vect.begin(), vect.begin() + std::min(vect.size(), static_cast<size_t>(8))};
		std::vector<std::shared_ptr<CComponent>> intComps;
		for (auto & component : sender)
			intComps.push_back(std::make_shared<CComponent>(component));
		showInfoDialog(text,intComps,soundID);
		vect.erase(vect.begin(), vect.begin() + std::min(vect.size(), static_cast<size_t>(8)));
	}
	while(!vect.empty());
}

void CPlayerInterface::showInfoDialog(const std::string & text, std::shared_ptr<CComponent> component)
{
	std::vector<std::shared_ptr<CComponent>> intComps;
	intComps.push_back(component);

	showInfoDialog(text, intComps, soundBase::sound_todo);
}

void CPlayerInterface::showInfoDialog(const std::string &text, const std::vector<std::shared_ptr<CComponent>> & components, int soundID)
{
	LOG_TRACE_PARAMS(logGlobal, "player=%s, text=%s, is LOCPLINT=%d", playerID % text % (this==LOCPLINT));
	waitWhileDialog();

	if (settings["session"]["autoSkip"].Bool() && !GH.isKeyboardShiftDown())
	{
		return;
	}
	std::shared_ptr<CInfoWindow> temp = CInfoWindow::create(text, playerID, components);

	if (makingTurn && GH.windows().count() > 0 && LOCPLINT == this)
	{
		CCS->soundh->playSound(static_cast<soundBase::soundID>(soundID));
		showingDialog->set(true);
		movementController->requestMovementAbort(); // interrupt movement to show dialog
		GH.windows().pushWindow(temp);
	}
	else
	{
		dialogs.push_back(temp);
	}
}

void CPlayerInterface::showInfoDialogAndWait(std::vector<Component> & components, const MetaString & text)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	std::string str = text.toString();

	showInfoDialog(EInfoWindowMode::MODAL, str, components, 0);
	waitWhileDialog();
}

void CPlayerInterface::showYesNoDialog(const std::string &text, CFunctionList<void()> onYes, CFunctionList<void()> onNo, const std::vector<std::shared_ptr<CComponent>> & components)
{
	movementController->requestMovementAbort();
	LOCPLINT->showingDialog->setn(true);
	CInfoWindow::showYesNoDialog(text, components, onYes, onNo, playerID);
}

void CPlayerInterface::showBlockingDialog( const std::string &text, const std::vector<Component> &components, QueryID askID, const int soundID, bool selection, bool cancel )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();

	movementController->requestMovementAbort();
	CCS->soundh->playSound(static_cast<soundBase::soundID>(soundID));

	if (!selection && cancel) //simple yes/no dialog
	{
		std::vector<std::shared_ptr<CComponent>> intComps;
		for (auto & component : components)
			intComps.push_back(std::make_shared<CComponent>(component)); //will be deleted by close in window

		showYesNoDialog(text, [=](){ cb->selectionMade(1, askID); }, [=](){ cb->selectionMade(0, askID); }, intComps);
	}
	else if (selection)
	{
		std::vector<std::shared_ptr<CSelectableComponent>> intComps;
		for (auto & component : components)
			intComps.push_back(std::make_shared<CSelectableComponent>(component)); //will be deleted by CSelWindow::close

		std::vector<std::pair<AnimationPath,CFunctionList<void()> > > pom;
		pom.push_back({ AnimationPath::builtin("IOKAY.DEF"),0});
		if (cancel)
		{
			pom.push_back({AnimationPath::builtin("ICANCEL.DEF"),0});
		}

		int charperline = 35;
		if (pom.size() > 1)
			charperline = 50;
		GH.windows().createAndPushWindow<CSelWindow>(text, playerID, charperline, intComps, pom, askID);
		intComps[0]->clickPressed(GH.getCursorPosition());
		intComps[0]->clickReleased(GH.getCursorPosition());
	}
}

void CPlayerInterface::showTeleportDialog(const CGHeroInstance * hero, TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	movementController->showTeleportDialog(hero, channel, exits, impassable, askID);
}

void CPlayerInterface::showMapObjectSelectDialog(QueryID askID, const Component & icon, const MetaString & title, const MetaString & description, const std::vector<ObjectInstanceID> & objects)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	std::vector<ObjectInstanceID> tmpObjects;
	if(objects.size() && dynamic_cast<const CGTownInstance *>(cb->getObj(objects[0])))
	{
		// sorting towns (like in client)
		std::vector <const CGTownInstance*> Towns = LOCPLINT->localState->getOwnedTowns();
		for(auto town : Towns)
			for(auto item : objects)
				if(town == cb->getObj(item))
					tmpObjects.push_back(item);
	}
	else // other object list than town
		tmpObjects = objects;

	auto selectCallback = [=](int selection)
	{
		cb->sendQueryReply(selection, askID);
	};

	auto cancelCallback = [=]()
	{
		cb->sendQueryReply(std::nullopt, askID);
	};

	const std::string localTitle = title.toString();
	const std::string localDescription = description.toString();

	std::vector<int> tempList;
	tempList.reserve(tmpObjects.size());

	for(auto item : tmpObjects)
		tempList.push_back(item.getNum());

	CComponent localIconC(icon);

	std::shared_ptr<CIntObject> localIcon = localIconC.image;
	localIconC.removeChild(localIcon.get(), false);

	std::vector<std::shared_ptr<IImage>> images;
	for(auto & obj : tmpObjects)
	{
		if(!settings["general"]["enableUiEnhancements"].Bool())
			break;
		const CGTownInstance * t = dynamic_cast<const CGTownInstance *>(cb->getObj(obj));
		if(t)
		{
			std::shared_ptr<CAnimation> a = GH.renderHandler().loadAnimation(AnimationPath::builtin("ITPA"));
			a->preload();
			images.push_back(a->getImage(t->town->clientInfo.icons[t->hasFort()][false] + 2)->scaleFast(Point(35, 23)));
		}
	}

	auto wnd = std::make_shared<CObjectListWindow>(tempList, localIcon, localTitle, localDescription, selectCallback, 0, images);
	wnd->onExit = cancelCallback;
	wnd->onPopup = [this, tmpObjects](int index) { CRClickPopup::createAndPush(cb->getObj(tmpObjects[index]), GH.getCursorPosition()); };
	wnd->onClicked = [this, tmpObjects](int index) { adventureInt->centerOnObject(cb->getObj(tmpObjects[index])); GH.windows().totalRedraw(); };
	GH.windows().pushWindow(wnd);
}

void CPlayerInterface::tileRevealed(const std::unordered_set<int3> &pos)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	//FIXME: wait for dialog? Magi hut/eye would benefit from this but may break other areas
	adventureInt->onMapTilesChanged(pos);
}

void CPlayerInterface::tileHidden(const std::unordered_set<int3> &pos)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->onMapTilesChanged(pos);
}

void CPlayerInterface::openHeroWindow(const CGHeroInstance *hero)
{
	GH.windows().createAndPushWindow<CHeroWindow>(hero);
}

void CPlayerInterface::availableCreaturesChanged( const CGDwelling *town )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (const CGTownInstance * townObj = dynamic_cast<const CGTownInstance*>(town))
	{
		for (auto fortScreen : GH.windows().findWindows<CFortScreen>())
			fortScreen->creaturesChangedEventHandler();

		for (auto castleInterface : GH.windows().findWindows<CCastleInterface>())
			if(castleInterface->town == town)
				castleInterface->creaturesChangedEventHandler();

		if (townObj)
			for (auto ki : GH.windows().findWindows<CKingdomInterface>())
				ki->townChanged(townObj);
	}
	else if(town && GH.windows().count() > 0 && (town->ID == Obj::CREATURE_GENERATOR1
		||  town->ID == Obj::CREATURE_GENERATOR4  ||  town->ID == Obj::WAR_MACHINE_FACTORY))
	{
		for (auto crw : GH.windows().findWindows<CRecruitmentWindow>())
			if (crw->dwelling == town)
				crw->availableCreaturesChanged();
	}
}

void CPlayerInterface::heroBonusChanged( const CGHeroInstance *hero, const Bonus &bonus, bool gain )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (bonus.type == BonusType::NONE)
		return;

	adventureInt->onHeroChanged(hero);
	if ((bonus.type == BonusType::FLYING_MOVEMENT || bonus.type == BonusType::WATER_WALKING) && !gain)
	{
		//recalculate paths because hero has lost bonus influencing pathfinding
		localState->erasePath(hero);
	}
}

void CPlayerInterface::saveGame( BinarySerializer & h )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	localState->serialize(h);
}

void CPlayerInterface::loadGame( BinaryDeserializer & h )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	localState->serialize(h);
	firstCall = -1;
}

void CPlayerInterface::moveHero( const CGHeroInstance *h, const CGPath& path )
{
	assert(LOCPLINT->makingTurn);
	assert(h);
	assert(!showingDialog->get());
	assert(dialogs.empty());

	LOG_TRACE(logGlobal);
	if (!LOCPLINT->makingTurn)
		return;
	if (!h)
		return; //can't find hero

	//It shouldn't be possible to move hero with open dialog (or dialog waiting in bg)
	if (showingDialog->get() || !dialogs.empty())
		return;

	if (localState->isHeroSleeping(h))
		localState->setHeroAwaken(h);

	movementController->requestMovementStart(h, path);
}

void CPlayerInterface::showGarrisonDialog( const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto onEnd = [=](){ cb->selectionMade(0, queryID); };

	if (movementController->isHeroMovingThroughGarrison(down, up))
	{
		onEnd();
		return;
	}

	waitForAllDialogs();

	auto cgw = std::make_shared<CGarrisonWindow>(up, down, removableUnits);
	cgw->quit->addCallback(onEnd);
	GH.windows().pushWindow(cgw);
}

/**
 * Shows the dialog that appears when right-clicking an artifact that can be assembled
 * into a combinational one on an artifact screen. Does not require the combination of
 * artifacts to be legal.
 */
void CPlayerInterface::showArtifactAssemblyDialog(const Artifact * artifact, const Artifact * assembledArtifact, CFunctionList<void()> onYes)
{
	std::string text = artifact->getDescriptionTranslated();
	text += "\n\n";
	std::vector<std::shared_ptr<CComponent>> scs;

	if(assembledArtifact)
	{
		// You possess all of the components to...
		text += boost::str(boost::format(CGI->generaltexth->allTexts[732]) % assembledArtifact->getNameTranslated());

		// Picture of assembled artifact at bottom.
		auto sc = std::make_shared<CComponent>(ComponentType::ARTIFACT, assembledArtifact->getId());
		scs.push_back(sc);
	}
	else
	{
		// Do you wish to disassemble this artifact?
		text += CGI->generaltexth->allTexts[733];
	}

	showYesNoDialog(text, onYes, nullptr, scs);
}

void CPlayerInterface::requestRealized( PackageApplied *pa )
{
	if(pa->packType == CTypeList::getInstance().getTypeID<MoveHero>(nullptr))
		movementController->onMoveHeroApplied();

	if(pa->packType == CTypeList::getInstance().getTypeID<QueryReply>(nullptr))
		movementController->onQueryReplyApplied();
}

void CPlayerInterface::showHeroExchange(ObjectInstanceID hero1, ObjectInstanceID hero2)
{
	heroExchangeStarted(hero1, hero2, QueryID(-1));
}

void CPlayerInterface::heroExchangeStarted(ObjectInstanceID hero1, ObjectInstanceID hero2, QueryID query)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	GH.windows().createAndPushWindow<CExchangeWindow>(hero1, hero2, query);
}

void CPlayerInterface::beforeObjectPropertyChanged(const SetObjectProperty * sop)
{
	if (sop->what == ObjProperty::OWNER)
	{
		const CGObjectInstance * obj = cb->getObj(sop->id);

		if(obj->ID == Obj::TOWN)
		{
			auto town = static_cast<const CGTownInstance *>(obj);

			if(obj->tempOwner == playerID)
			{
				localState->removeOwnedTown(town);
				adventureInt->onTownChanged(town);
			}
		}
	}
}

void CPlayerInterface::objectPropertyChanged(const SetObjectProperty * sop)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	if (sop->what == ObjProperty::OWNER)
	{
		const CGObjectInstance * obj = cb->getObj(sop->id);

		if(obj->ID == Obj::TOWN)
		{
			auto town = static_cast<const CGTownInstance *>(obj);

			if(obj->tempOwner == playerID)
			{
				localState->addOwnedTown(town);
				adventureInt->onTownChanged(town);
			}
		}

		//redraw minimap if owner changed
		std::set<int3> pos = obj->getBlockedPos();
		std::unordered_set<int3> upos(pos.begin(), pos.end());
		adventureInt->onMapTilesChanged(upos);

		assert(cb->getTownsInfo().size() == localState->getOwnedTowns().size());
	}
}

void CPlayerInterface::initializeHeroTownList()
{
	if(localState->getWanderingHeroes().empty())
	{
		for(auto & hero : cb->getHeroesInfo())
		{
			if(!hero->inTownGarrison)
				localState->addWanderingHero(hero);
		}
	}

	if(localState->getOwnedTowns().empty())
	{
		for(auto & town : cb->getTownsInfo())
			localState->addOwnedTown(town);
	}

	if(adventureInt)
		adventureInt->onHeroChanged(nullptr);
}

void CPlayerInterface::showRecruitmentDialog(const CGDwelling *dwelling, const CArmedInstance *dst, int level, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();
	auto recruitCb = [=](CreatureID id, int count)
	{
		cb->recruitCreatures(dwelling, dst, id, count, -1);
	};
	auto closeCb = [=]()
	{
		cb->selectionMade(0, queryID);
	};
	GH.windows().createAndPushWindow<CRecruitmentWindow>(dwelling, level, dst, recruitCb, closeCb);
}

void CPlayerInterface::waitWhileDialog()
{
	if (GH.amIGuiThread())
	{
		logGlobal->warn("Cannot wait for dialogs in gui thread (deadlock risk)!");
		return;
	}

	auto unlockInterface = vstd::makeUnlockGuard(GH.interfaceMutex);
	boost::unique_lock<boost::mutex> un(showingDialog->mx);
	while(showingDialog->data)
		showingDialog->cond.wait(un);
}

void CPlayerInterface::showShipyardDialog(const IShipyard *obj)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto state = obj->shipyardStatus();
	TResources cost;
	obj->getBoatCost(cost);
	GH.windows().createAndPushWindow<CShipyardWindow>(cost, state, obj->getBoatType(), [=](){ cb->buildBoat(obj); });
}

void CPlayerInterface::newObject( const CGObjectInstance * obj )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	//we might have built a boat in shipyard in opened town screen
	if (obj->ID == Obj::BOAT
		&& LOCPLINT->castleInt
		&&  obj->visitablePos() == LOCPLINT->castleInt->town->bestLocation())
	{
		CCS->soundh->playSound(soundBase::newBuilding);
		LOCPLINT->castleInt->addBuilding(BuildingID::SHIP);
	}
}

void CPlayerInterface::centerView (int3 pos, int focusTime)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();
	CCS->curh->hide();
	adventureInt->centerOnTile(pos);
	if (focusTime)
	{
		GH.windows().totalRedraw();
		{
			IgnoreEvents ignore(*this);
			auto unlockInterface = vstd::makeUnlockGuard(GH.interfaceMutex);
			boost::this_thread::sleep_for(boost::chrono::milliseconds(focusTime));
		}
	}
	CCS->curh->show();
}

void CPlayerInterface::objectRemoved(const CGObjectInstance * obj, const PlayerColor & initiator)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if(playerID == initiator && obj->getRemovalSound())
	{
		waitWhileDialog();
		CCS->soundh->playSound(obj->getRemovalSound().value());
	}
	CGI->mh->waitForOngoingAnimations();

	if(obj->ID == Obj::HERO && obj->tempOwner == playerID)
	{
		const CGHeroInstance * h = static_cast<const CGHeroInstance *>(obj);
		heroKilled(h);
	}
	GH.fakeMouseMove();
}

void CPlayerInterface::objectRemovedAfter()
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->onMapTilesChanged(boost::none);

	// visiting or garrisoned hero removed - update window
	if (castleInt)
		castleInt->updateGarrisons();

	for (auto ki : GH.windows().findWindows<CKingdomInterface>())
		ki->heroRemoved();
}

void CPlayerInterface::playerBlocked(int reason, bool start)
{
	if(reason == PlayerBlocked::EReason::UPCOMING_BATTLE)
	{
		if(CSH->howManyPlayerInterfaces() > 1 && LOCPLINT != this && LOCPLINT->makingTurn == false)
		{
			//one of our players who isn't last in order got attacked not by our another player (happens for example in hotseat mode)
			LOCPLINT = this;
			GH.curInt = this;
			adventureInt->onCurrentPlayerChanged(playerID);
			std::string msg = CGI->generaltexth->translate("vcmi.adventureMap.playerAttacked");
			boost::replace_first(msg, "%s", cb->getStartInfo()->playerInfos.find(playerID)->second.name);
			std::vector<std::shared_ptr<CComponent>> cmp;
			cmp.push_back(std::make_shared<CComponent>(ComponentType::FLAG, playerID));
			makingTurn = true; //workaround for stiff showInfoDialog implementation
			showInfoDialog(msg, cmp);
			makingTurn = false;
		}
	}
}

void CPlayerInterface::update()
{
	// Make sure that gamestate won't change when GUI objects may obtain its parts on event processing or drawing request
	boost::shared_lock<boost::shared_mutex> gsLock(CGameState::mutex);

	// While mutexes were locked away we may be have stopped being the active interface
	if (LOCPLINT != this)
		return;

	//if there are any waiting dialogs, show them
	if ((CSH->howManyPlayerInterfaces() <= 1 || makingTurn) && !dialogs.empty() && !showingDialog->get())
	{
		showingDialog->set(true);
		GH.windows().pushWindow(dialogs.front());
		dialogs.pop_front();
	}

	assert(adventureInt);

	// Handles mouse and key input
	GH.handleEvents();
	GH.windows().simpleRedraw();
}

int CPlayerInterface::getLastIndex( std::string namePrefix)
{
	using namespace boost::filesystem;
	using namespace boost::algorithm;

	path gamesDir = VCMIDirs::get().userSavePath();
	std::map<std::time_t, int> dates; //save number => datestamp

	const directory_iterator enddir;
	if (!exists(gamesDir))
		create_directory(gamesDir);
	else
	for (directory_iterator dir(gamesDir); dir != enddir; ++dir)
	{
		if (is_regular_file(dir->status()))
		{
			std::string name = dir->path().filename().string();
			if (starts_with(name, namePrefix) && ends_with(name, ".vcgm1"))
			{
				char nr = name[namePrefix.size()];
				if (std::isdigit(nr))
					dates[last_write_time(dir->path())] = boost::lexical_cast<int>(nr);
			}
		}
	}

	if (!dates.empty())
		return (--dates.end())->second; //return latest file number
	return 0;
}

void CPlayerInterface::gameOver(PlayerColor player, const EVictoryLossCheckResult & victoryLossCheckResult )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	if (player == playerID)
	{
		if (victoryLossCheckResult.loss())
			showInfoDialog(CGI->generaltexth->allTexts[95]);

		assert(GH.curInt == LOCPLINT);
		auto previousInterface = LOCPLINT; //without multiple player interfaces some of lines below are useless, but for hotseat we wanna swap player interface temporarily

		LOCPLINT = this; //this is needed for dialog to show and avoid freeze, dialog showing logic should be reworked someday
		GH.curInt = this; //waiting for dialogs requires this to get events

		if(!makingTurn)
		{
			makingTurn = true; //also needed for dialog to show with current implementation
			waitForAllDialogs();
			makingTurn = false;
		}
		else
			waitForAllDialogs();

		GH.curInt = previousInterface;
		LOCPLINT = previousInterface;

		if(CSH->howManyPlayerInterfaces() == 1 && !settings["session"]["spectate"].Bool()) //all human players eliminated
		{
			if(adventureInt)
			{
				GH.windows().popWindows(GH.windows().count());
				adventureInt.reset();
			}
		}

		if (victoryLossCheckResult.victory() && LOCPLINT == this)
		{
			// end game if current human player has won
			CSH->sendClientDisconnecting();
			requestReturningToMainMenu(true);
		}
		else if(CSH->howManyPlayerInterfaces() == 1 && !settings["session"]["spectate"].Bool())
		{
			//all human players eliminated
			CSH->sendClientDisconnecting();
			requestReturningToMainMenu(false);
		}

		if (GH.curInt == this)
			GH.curInt = nullptr;
	}
}

void CPlayerInterface::playerBonusChanged( const Bonus &bonus, bool gain )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
}

void CPlayerInterface::showPuzzleMap()
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();

	//TODO: interface should not know the real position of Grail...
	double ratio = 0;
	int3 grailPos = cb->getGrailPos(&ratio);

	GH.windows().createAndPushWindow<CPuzzleWindow>(grailPos, ratio);
}

void CPlayerInterface::viewWorldMap()
{
	adventureInt->openWorldView();
}

void CPlayerInterface::advmapSpellCast(const CGHeroInstance * caster, SpellID spellID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	if(GH.windows().topWindow<CSpellWindow>())
		GH.windows().popWindows(1);

	if(spellID == SpellID::FLY || spellID == SpellID::WATER_WALK)
		localState->erasePath(caster);

	auto castSoundPath = spellID.toSpell()->getCastSound();
	if(!castSoundPath.empty())
		CCS->soundh->playSound(castSoundPath);
}

void CPlayerInterface::tryDigging(const CGHeroInstance * h)
{
	int msgToShow = -1;

	const auto diggingStatus = h->diggingStatus();

	switch(diggingStatus)
	{
	case EDiggingStatus::CAN_DIG:
		break;
	case EDiggingStatus::LACK_OF_MOVEMENT:
		msgToShow = 56; //"Digging for artifacts requires a whole day, try again tomorrow."
		break;
	case EDiggingStatus::TILE_OCCUPIED:
		msgToShow = 97; //Try searching on clear ground.
		break;
	case EDiggingStatus::WRONG_TERRAIN:
		msgToShow = 60; ////Try looking on land!
		break;
	case EDiggingStatus::BACKPACK_IS_FULL:
		msgToShow = 247; //Searching for the Grail is fruitless...
		break;
	default:
		assert(0);
	}

	if(msgToShow < 0)
		cb->dig(h);
	else
		showInfoDialog(CGI->generaltexth->allTexts[msgToShow]);
}

void CPlayerInterface::battleNewRoundFirst(const BattleID & battleID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->newRoundFirst();
}

void CPlayerInterface::showMarketWindow(const IMarket *market, const CGHeroInstance *visitor, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto onWindowClosed = [this, queryID](){
		cb->selectionMade(0, queryID);
	};

	if(market->allowsTrade(EMarketMode::ARTIFACT_EXP) && visitor->getAlignment() != EAlignment::EVIL)
		GH.windows().createAndPushWindow<CAltarWindow>(market, visitor, onWindowClosed, EMarketMode::ARTIFACT_EXP);
	else if(market->allowsTrade(EMarketMode::CREATURE_EXP) && visitor->getAlignment() != EAlignment::GOOD)
		GH.windows().createAndPushWindow<CAltarWindow>(market, visitor, onWindowClosed, EMarketMode::CREATURE_EXP);
	else if(market->allowsTrade(EMarketMode::CREATURE_UNDEAD))
		GH.windows().createAndPushWindow<CTransformerWindow>(market, visitor, onWindowClosed);
	else if(!market->availableModes().empty())
		GH.windows().createAndPushWindow<CMarketplaceWindow>(market, visitor, onWindowClosed, market->availableModes().front());
}

void CPlayerInterface::showUniversityWindow(const IMarket *market, const CGHeroInstance *visitor, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto onWindowClosed = [this, queryID](){
		cb->selectionMade(0, queryID);
	};
	GH.windows().createAndPushWindow<CUniversityWindow>(visitor, market, onWindowClosed);
}

void CPlayerInterface::showHillFortWindow(const CGObjectInstance *object, const CGHeroInstance *visitor)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	GH.windows().createAndPushWindow<CHillFortWindow>(visitor, object);
}

void CPlayerInterface::availableArtifactsChanged(const CGBlackMarket * bm)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	for (auto cmw : GH.windows().findWindows<CMarketplaceWindow>())
		cmw->artifactsChanged(false);
}

void CPlayerInterface::showTavernWindow(const CGObjectInstance * object, const CGHeroInstance * visitor, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto onWindowClosed = [this, queryID](){
		if (queryID != QueryID::NONE)
			cb->selectionMade(0, queryID);
	};
	GH.windows().createAndPushWindow<CTavernWindow>(object, onWindowClosed);
}

void CPlayerInterface::showThievesGuildWindow (const CGObjectInstance * obj)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	GH.windows().createAndPushWindow<CThievesGuildWindow>(obj);
}

void CPlayerInterface::showQuestLog()
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	GH.windows().createAndPushWindow<CQuestLog>(LOCPLINT->cb->getMyQuests());
}

void CPlayerInterface::showShipyardDialogOrProblemPopup(const IShipyard *obj)
{
	if (obj->shipyardStatus() != IBoatGenerator::GOOD)
	{
		MetaString txt;
		obj->getProblemText(txt);
		showInfoDialog(txt.toString());
	}
	else
		showShipyardDialog(obj);
}

void CPlayerInterface::requestReturningToMainMenu(bool won)
{
	HighScoreParameter param;
	param.difficulty = cb->getStartInfo()->difficulty;
	param.day = cb->getDate();
	param.townAmount = cb->howManyTowns();
	param.usedCheat = cb->getPlayerState(*cb->getPlayerID())->cheated;
	param.hasGrail = false;
	for(const CGHeroInstance * h : cb->getHeroesInfo())
		if(h->hasArt(ArtifactID::GRAIL))
			param.hasGrail = true;
	for(const CGTownInstance * t : cb->getTownsInfo())
		if(t->builtBuildings.count(BuildingID::GRAIL))
			param.hasGrail = true;
	param.allDefeated = true;
	for (PlayerColor player(0); player < PlayerColor::PLAYER_LIMIT; ++player)
	{
		auto ps = cb->getPlayerState(player, false);
		if(ps && player != *cb->getPlayerID())
			if(!ps->checkVanquished())
				param.allDefeated = false;
	}
	param.scenarioName = cb->getMapHeader()->name.toString();
	param.playerName = cb->getStartInfo()->playerInfos.find(*cb->getPlayerID())->second.name;
	HighScoreCalculation highScoreCalc;
	highScoreCalc.parameters.push_back(param);
	highScoreCalc.isCampaign = false;

	if(won && cb->getStartInfo()->campState)
		CSH->startCampaignScenario(param, cb->getStartInfo()->campState);
	else
	{
		GH.dispatchMainThread(
			[won, highScoreCalc]()
			{
				CSH->endGameplay();
				GH.defActionsDef = 63;
				CMM->menu->switchToTab("main");
				GH.windows().createAndPushWindow<CHighScoreInputScreen>(won, highScoreCalc);
			}
		);
	}
}

void CPlayerInterface::askToAssembleArtifact(const ArtifactLocation &al)
{
	if(auto hero = cb->getHero(al.artHolder))
	{
		auto art = hero->getArt(al.slot);
		if(art == nullptr)
		{
			logGlobal->error("artifact location %d points to nothing",
							 al.slot.num);
			return;
		}
		ArtifactUtilsClient::askToAssemble(hero, al.slot);
	}
}

void CPlayerInterface::artifactPut(const ArtifactLocation &al)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->onHeroChanged(cb->getHero(al.artHolder));
}

void CPlayerInterface::artifactRemoved(const ArtifactLocation &al)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->onHeroChanged(cb->getHero(al.artHolder));

	for(auto artWin : GH.windows().findWindows<CArtifactHolder>())
		artWin->artifactRemoved(al);

	waitWhileDialog();
}

void CPlayerInterface::artifactMoved(const ArtifactLocation &src, const ArtifactLocation &dst)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->onHeroChanged(cb->getHero(dst.artHolder));

	bool redraw = true;
	// If a bulk transfer has arrived, then redrawing only the last art movement.
	if(numOfMovedArts != 0)
	{
		numOfMovedArts--;
		if(numOfMovedArts != 0)
			redraw = false;
	}

	for(auto artWin : GH.windows().findWindows<CArtifactHolder>())
		artWin->artifactMoved(src, dst, redraw);

	waitWhileDialog();
}

void CPlayerInterface::bulkArtMovementStart(size_t numOfArts)
{
	numOfMovedArts = numOfArts;
}

void CPlayerInterface::artifactAssembled(const ArtifactLocation &al)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->onHeroChanged(cb->getHero(al.artHolder));

	for(auto artWin : GH.windows().findWindows<CArtifactHolder>())
		artWin->artifactAssembled(al);
}

void CPlayerInterface::artifactDisassembled(const ArtifactLocation &al)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->onHeroChanged(cb->getHero(al.artHolder));

	for(auto artWin : GH.windows().findWindows<CArtifactHolder>())
		artWin->artifactDisassembled(al);
}

void CPlayerInterface::waitForAllDialogs()
{
	while(!dialogs.empty())
	{
		auto unlockInterface = vstd::makeUnlockGuard(GH.interfaceMutex);
		boost::this_thread::sleep_for(boost::chrono::milliseconds(5));
	}
	waitWhileDialog();
}

void CPlayerInterface::proposeLoadingGame()
{
	showYesNoDialog(
		CGI->generaltexth->allTexts[68],
		[]()
		{
			CSH->endGameplay();
			GH.defActionsDef = 63;
			CMM->menu->switchToTab("load");
		},
		nullptr
	);
}

bool CPlayerInterface::capturedAllEvents()
{
	if(movementController->isHeroMoving())
	{
		//just inform that we are capturing events. they will be processed by heroMoved() in client thread.
		return true;
	}

	bool needToLockAdventureMap = adventureInt && adventureInt->isActive() && CGI->mh->hasOngoingAnimations();
	bool quickCombatOngoing = isAutoFightOn && !battleInt;

	if (ignoreEvents || needToLockAdventureMap || quickCombatOngoing )
	{
		GH.input().ignoreEventsUntilInput();
		return true;
	}

	return false;
}

void CPlayerInterface::showWorldViewEx(const std::vector<ObjectPosInfo>& objectPositions, bool showTerrain)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->openWorldView(objectPositions, showTerrain );
}

std::optional<BattleAction> CPlayerInterface::makeSurrenderRetreatDecision(const BattleID & battleID, const BattleStateInfoForRetreat & battleState)
{
	return std::nullopt;
}
