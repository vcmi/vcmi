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

#include "Client.h"
#include "CServerHandler.h"
#include "HeroMovementController.h"
#include "PlayerLocalState.h"

#include "adventureMap/AdventureMapInterface.h"
#include "adventureMap/CInGameConsole.h"
#include "adventureMap/CList.h"

#include "battle/BattleEffectsController.h"
#include "battle/BattleFieldController.h"
#include "battle/BattleInterface.h"
#include "battle/BattleResultWindow.h"
#include "battle/BattleWindow.h"

#include "eventsSDL/InputHandler.h"
#include "eventsSDL/NotificationHandler.h"

#include "GameEngine.h"
#include "GameInstance.h"
#include "gui/CursorHandler.h"
#include "gui/WindowHandler.h"

#include "mainmenu/CMainMenu.h"
#include "mainmenu/CHighScoreScreen.h"
#include "mainmenu/CStatisticScreen.h"

#include "mapView/mapHandler.h"

#include "media/IMusicPlayer.h"
#include "media/ISoundPlayer.h"

#include "replay/GameplayReplayer.h"

#include "render/CAnimation.h"
#include "render/IImage.h"
#include "render/IRenderHandler.h"
#include "render/IScreenHandler.h"

#include "widgets/Buttons.h"
#include "widgets/CComponent.h"
#include "widgets/CGarrisonInt.h"

#include "windows/CCastleInterface.h"
#include "windows/CCreatureWindow.h"
#include "windows/CExchangeWindow.h"
#include "windows/CHeroWindow.h"
#include "windows/CKingdomInterface.h"
#include "windows/CMarketWindow.h"
#include "windows/CPuzzleWindow.h"
#include "windows/CQuestLog.h"
#include "windows/CScenarioEventJournal.h"
#include "windows/CSpellWindow.h"
#include "windows/CTutorialWindow.h"
#include "windows/GUIClasses.h"
#include "windows/InfoWindows.h"
#include "windows/settings/SettingsMainWindow.h"

#include "../lib/callback/AIFactory.h"
#include "../lib/CConfigHandler.h"
#include "../lib/GameLibrary.h"
#include "../lib/texts/CGeneralTextHandler.h"
#include "../lib/CPlayerState.h"
#include "../lib/CRandomGenerator.h"
#include "../lib/CStack.h"
#include "../lib/CStopWatch.h"
#include "../lib/CThreadHelper.h"
#include "../lib/GameConstants.h"
#include "../lib/RoadHandler.h"
#include "../lib/StartInfo.h"
#include "../lib/TerrainHandler.h"
#include "../lib/UnlockGuard.h"
#include "../lib/VCMIDirs.h"

#include "../lib/battle/CPlayerBattleCallback.h"

#include "../lib/bonuses/Limiters.h"
#include "../lib/bonuses/Propagators.h"
#include "../lib/bonuses/Updaters.h"

#include "../lib/callback/CCallback.h"

#include "../lib/gameState/CGameState.h"

#include "../lib/mapObjects/CGMarket.h"
#include "../lib/mapObjects/CGTownInstance.h"
#include "../lib/mapObjects/MiscObjects.h"
#include "../lib/mapObjects/ObjectTemplate.h"

#include "../lib/mapping/CMap.h"
#include "../lib/mapping/CMapHeader.h"

#include "../lib/networkPacks/PacksForClient.h"
#include "../lib/networkPacks/PacksForClientBattle.h"
#include "../lib/networkPacks/PacksForServer.h"

#include "../lib/pathfinder/CGPathNode.h"
#include "../lib/pathfinder/PathfinderCache.h"
#include "../lib/pathfinder/PathfinderOptions.h"

#include "../lib/serializer/CTypeList.h"
#include "../lib/serializer/ESerializationVersion.h"

#include "../lib/spells/CSpell.h"

#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/SavegamePath.h"


// The macro below is used to mark functions that are called by client when game state changes.
// They all assume that interface mutex is locked.
#define EVENT_HANDLER_CALLED_BY_CLIENT

#define BATTLE_EVENT_POSSIBLE_RETURN	if (GAME->interface() != this) return; if (isAutoFightOn && !battleInt) return

std::shared_ptr<BattleInterface> CPlayerInterface::battleInt;

CPlayerInterface::CPlayerInterface(PlayerColor Player):
	localState(std::make_unique<PlayerLocalState>(*this)),
	movementController(std::make_unique<HeroMovementController>()),
	artifactController(std::make_unique<ArtifactsUIController>())

{
	logGlobal->trace("\tHuman player interface for player %s being constructed", Player.toString());
	GAME->setInterfaceInstance(this);
	playerID=Player;
	human=true;
	battleInt.reset();
	castleInt = nullptr;
	makingTurn = false;
	showingDialog = new ConditionalWait();
	cingconsole = new CInGameConsole();
	isAutoFightOn = false;
	isAutoFightEndBattle = false;
	ignoreEvents = false;
	hasQuickSave = false;
}

CPlayerInterface::~CPlayerInterface()
{
	logGlobal->trace("\tHuman player interface for player %s being destructed", playerID.toString());
	delete showingDialog;
	delete cingconsole;
	if (GAME->interface() == this)
		GAME->setInterfaceInstance(nullptr);
}

void CPlayerInterface::initGameInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CCallback> CB)
{
	cb = CB;
	env = ENV;
	hasQuickSave = checkQuickLoadingGame();

	pathfinderCache = std::make_unique<PathfinderCache>(cb.get(), PathfinderOptions(*cb));
	ENGINE->music().loadTerrainMusicThemes();
	initializeHeroTownList();

	adventureInt.reset(new AdventureMapInterface());
	adventureInt->onCurrentPlayerChanged(playerID);
}

std::shared_ptr<const CPathsInfo> CPlayerInterface::getPathsInfo(const CGHeroInstance * h)
{
	return pathfinderCache->getPathsInfo(h);
}

void CPlayerInterface::invalidatePaths()
{
	pathfinderCache->invalidatePaths();
}

void CPlayerInterface::closeAllDialogs()
{
	// remove all active dialogs that do not expect query answer
	while(true)
	{
		auto adventureWindow = ENGINE->windows().topWindow<AdventureMapInterface>();
		auto settingsWindow = ENGINE->windows().topWindow<SettingsMainWindow>();
		auto infoWindow = ENGINE->windows().topWindow<CInfoWindow>();
		auto topWindow = ENGINE->windows().topWindow<WindowBase>();

		if(adventureWindow != nullptr)
			break;

		if(infoWindow && infoWindow->ID != QueryID::NONE)
			break;

		if (settingsWindow)
		{
			settingsWindow->close();
			continue;
		}

		if (topWindow)
			topWindow->close();
		else
			ENGINE->windows().popWindows(1); // does not inherits from WindowBase, e.g. settings dialog
	}
}

void CPlayerInterface::playerEndsTurn(PlayerColor player)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (player == playerID)
	{
		makingTurn = false;
		delayQueuedDialogsUntilInputSettles = false;
		levelUpChainPendingContinuation = false;
		closeAllDialogs();

		// remove all pending dialogs that do not expect query answer
		vstd::erase_if(dialogs, [](const PendingDialog & dialog){
						   return dialog.dropOnTurnEnd;
					   });
	}
}

void CPlayerInterface::playerStartsTurn(PlayerColor player)
{
	if(ENGINE->windows().findWindows<AdventureMapInterface>().empty())
	{
		// after map load - remove all active windows and replace them with adventure map
		ENGINE->windows().clear();
		ENGINE->windows().pushWindow(adventureInt);
	}

	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (player != playerID && GAME->interface() == this)
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
	if(frequency > 0 && cb->getCalendar().getCurrentDay() % frequency == 0)
	{
		const auto calendar = cb->getCalendar();
		const auto autosaveCountLimit = static_cast<int>(settings["general"]["autosaveCountLimit"].Integer());
		cb->saveAutosave(
			SavegamePath::getAutosavePath(*cb->getStartInfo(), *cb->getMapHeader(), calendar),
			autosaveCountLimit);
	}
}

void CPlayerInterface::gamePause(bool pause)
{
	cb->gamePause(pause);
}

void CPlayerInterface::yourTurn(QueryID queryID)
{
	closeAllDialogs();
	CTutorialWindow::openWindowFirstTime(TutorialMode::TOUCH_ADVENTUREMAP);

	EVENT_HANDLER_CALLED_BY_CLIENT;

	int humanPlayersCount = 0;
	for(const auto & info : cb->getStartInfo()->playerInfos)
		if (info.second.isControlledByHuman())
			humanPlayersCount++;

	bool hotseatWait = humanPlayersCount > 1;

		GAME->setInterfaceInstance(this);

		NotificationHandler::notify("Your turn");
		if(settings["general"]["startTurnAutosave"].Bool())
		{
			performAutosave();
		}

		if (hotseatWait) //hot seat or MP message
		{
			adventureInt->onHotseatWaitStarted(playerID);

			makingTurn = true;
			MetaString msg;
			msg.appendTextID("core.genrltxt.13");
			msg.replaceRawString(cb->getStartInfo()->playerInfos.find(playerID)->second.name);
			std::vector<std::shared_ptr<CComponent>> cmp;
			cmp.push_back(std::make_shared<CComponent>(ComponentType::FLAG, playerID));
			showInfoDialog(msg.toString(&GAME->translator()), cmp);
		}
		else
		{
			makingTurn = true;
			adventureInt->onPlayerTurnStarted(playerID);
		}

	acceptTurn(queryID, hotseatWait);
}

void CPlayerInterface::acceptTurn(QueryID queryID, bool hotseatWait)
{
	if (settings["session"]["autoSkip"].Bool())
	{
		while(auto iw = ENGINE->windows().topWindow<CInfoWindow>())
			iw->close();
	}

	if(hotseatWait)
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
				text.appendTextID("core.arraytxt.128"); //%s, you only have %d days left to capture a town or you will be banished from this land.
				text.replaceName(playerColor);
				text.replaceNumber(7 - daysWithoutCastle);
			}
			else if (daysWithoutCastle == 6)
			{
				text.appendTextID("core.arraytxt.129"); //%s, this is your last day to capture a town or you will be banished from this land.
				text.replaceName(playerColor);
			}

			showInfoDialogAndWait(components, text);
		}
		else
			logGlobal->warn("Player has no towns, but daysWithoutCastle is not set");
	}

	if (queryID.hasValue())
		cb->selectionMade(0, queryID);
	movementController->onPlayerTurnStarted();
}

void CPlayerInterface::heroMoved(const TryMoveHero & details, bool verbose)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();
	if(GAME->interface() != this)
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
	LOG_TRACE_PARAMS(logGlobal, "Hero %s killed handler for player %s", GAME->translator().translate(hero->getNameTextID()) % playerID);

	// if hero is not in town garrison
	if (vstd::contains(localState->getWanderingHeroes(), hero))
		localState->removeWanderingHero(hero);

	adventureInt->onHeroChanged(hero);
	localState->erasePath(hero);
}

void CPlayerInterface::townRemoved(const CGTownInstance* town)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	// close town screen if it shows the town being removed, otherwise objectRemovedAfter dereferences a dangling pointer
	if(castleInt && castleInt->town == town)
	{
		castleInt->close();
		castleInt = nullptr;
	}

	if(town->tempOwner == playerID)
	{
		localState->removeOwnedTown(town);
		adventureInt->onTownChanged(town);
	}
}


void CPlayerInterface::heroVisit(const CGHeroInstance * visitor, const CGObjectInstance * visitedObj, bool start)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if(start && visitedObj)
	{
		auto visitSound = visitedObj->getVisitSound(CRandomGenerator::getDefault());
		if (visitSound)
			ENGINE->sound().playSound(visitSound.value());
	}
}

void CPlayerInterface::heroCreated(const CGHeroInstance * hero)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	localState->addWanderingHero(hero);
	adventureInt->onHeroChanged(hero);
	if(castleInt)
		ENGINE->sound().playSound(soundBase::newBuilding);
}
void CPlayerInterface::openTownWindow(const CGTownInstance * town)
{
	if(castleInt)
		castleInt->close();
	castleInt = nullptr;

	auto newCastleInt = std::make_shared<CCastleInterface>(town);

	ENGINE->windows().pushWindow(newCastleInt);
}

void CPlayerInterface::heroExperienceChanged(const CGHeroInstance * hero, si64 val)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	for(auto ctw : ENGINE->windows().findWindows<IMarketHolder>())
		ctw->updateExperience();
}

void CPlayerInterface::heroPrimarySkillChanged(const CGHeroInstance * hero, PrimarySkill which, si64 val)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->onHeroChanged(hero);
}

void CPlayerInterface::heroSecondarySkillChanged(const CGHeroInstance * hero, int which, int val)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	for (auto cuw : ENGINE->windows().findWindows<IMarketHolder>())
		cuw->updateSecondarySkills();

	localState->verifyPath(hero);
	adventureInt->onHeroChanged(hero);// secondary skill can change primary skill / mana limit
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
	invalidatePaths();
	localState->verifyPath(hero);
}
void CPlayerInterface::receivedResource()
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	for (auto mw : ENGINE->windows().findWindows<IMarketHolder>())
		mw->updateResources();

	ENGINE->windows().totalRedraw();
}

void CPlayerInterface::heroGotLevel(const CGHeroInstance *hero, PrimarySkill pskill, std::vector<SecondarySkill>& skills, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto availableSkills = skills;

	auto showLevelUpDialog = [this, hero, pskill, availableSkills = std::move(availableSkills), queryID]() mutable
	{
		ENGINE->sound().playSound(soundBase::heroNewLevel);
		auto callback = [this, queryID](ui32 selection)
		{
			if(queryID < 0)
				return;

			cb->selectionMade(selection, queryID);
		};

		if(auto levelWindow = ENGINE->windows().topWindow<CLevelWindow>())
		{
			levelWindow->updateLevelUpData(hero, pskill, availableSkills, callback);
			return;
		}

		closeActiveLevelUpDialog();

		auto levelWindow = std::make_shared<CLevelWindow>(hero, pskill, availableSkills, callback);

		// Free the visible-dialog gate as soon as the player makes a choice.
		// The query-backed dialog queue still keeps manual input blocked until the
		// server resolves this level-up step and advances the chain.
		levelWindow->setCloseOnSelection(queryID < 0);
		ENGINE->windows().pushWindow(levelWindow);
	};

	createAndQueueDialog(PendingDialog::Type::Blocking, std::move(showLevelUpDialog), queryID);
	tryShowNextPendingDialog();
}

void CPlayerInterface::commanderGotLevel(const CCommanderInstance * commander, std::vector<ui32> skills, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto showCallback = [this, commander, skills = std::move(skills), queryID]() mutable
	{
		ENGINE->sound().playSound(soundBase::heroNewLevel);
		auto callback = [this, queryID](ui32 selection)
		{
			if(queryID < 0)
				return;

			cb->selectionMade(selection, queryID);
		};

		closeActiveLevelUpDialog();

		auto levelWindow = std::make_shared<CStackWindow>(commander, skills, callback);

		// Free the visible-dialog gate as soon as the player makes a choice.
		// The query-backed dialog queue still keeps manual input blocked until the
		// server resolves this level-up step and advances the chain.
		levelWindow->setCloseOnSelection(queryID < 0);
		ENGINE->windows().pushWindow(levelWindow);
	};

	createAndQueueDialog(PendingDialog::Type::Blocking, std::move(showCallback), queryID);
	tryShowNextPendingDialog();
}

void CPlayerInterface::heroInGarrisonChange(const CGTownInstance *town)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	if(town->getGarrisonHero()) //wandering hero moved to the garrison
	{
		// This method also gets called on hero recruitment -> garrisoned hero is already in garrison
		if(town->getGarrisonHero()->tempOwner == playerID && vstd::contains(localState->getWanderingHeroes(), town->getGarrisonHero()))
			localState->removeWanderingHero(town->getGarrisonHero());
	}

	if(town->getVisitingHero()) //hero leaves garrison
	{
		// This method also gets called on hero recruitment -> wandering heroes already contains new hero
		if(town->getVisitingHero()->tempOwner == playerID && !vstd::contains(localState->getWanderingHeroes(), town->getVisitingHero()))
			localState->addWanderingHero(town->getVisitingHero());
	}
	adventureInt->onHeroChanged(nullptr);
	adventureInt->onTownChanged(town);

	for (auto cgh : ENGINE->windows().findWindows<IGarrisonHolder>())
		if (cgh->holdsGarrison(town))
			cgh->updateGarrisons();

	for (auto ki : ENGINE->windows().findWindows<CKingdomInterface>())
		ki->townChanged(town);

	// Perform totalRedraw to update hero list on adventure map, if any dialogs are open
	ENGINE->windows().totalRedraw();
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
			localState->verifyPath(hero);

			adventureInt->onHeroChanged(hero);
			if(hero->isGarrisoned() && hero->getVisitedTown() != town)
				adventureInt->onTownChanged(hero->getVisitedTown());
		}
	}

	for (auto cgh : ENGINE->windows().findWindows<IGarrisonHolder>())
		if (cgh->holdsGarrisons(objs))
			cgh->updateGarrisons();

	ENGINE->windows().totalRedraw();
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
				castleInt->addBuilding(buildingID);
				break;
			case 2:
				castleInt->removeBuilding(buildingID);
				break;
			}
		}

		// Perform totalRedraw in order to force redraw of updated town list icon from adventure map
		ENGINE->windows().totalRedraw();
	}

	for (auto cgh : ENGINE->windows().findWindows<ITownHolder>())
		cgh->buildChanged();
}

void CPlayerInterface::battleStartBefore(const BattleID & battleID, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2)
{
	movementController->onBattleStarted();

	waitForAllDialogs();
}

void CPlayerInterface::battleStart(const BattleID & battleID, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, BattleSide side, bool replayAllowed)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	bool useQuickCombat = settings["adventure"]["quickCombat"].Bool() || GAME->map().getMap()->battleOnly;
	bool forceQuickCombat = settings["adventure"]["forceQuickCombat"].Bool();

	if ((replayAllowed && useQuickCombat) || forceQuickCombat)
	{
		prepareAutoFightingAI(battleID, army1, army2, tile, hero1, hero2, side);
	}

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
		case UnitChanges::EOperation::UPDATE:
			{
				const CStack * stack = cb->getBattle(battleID)->battleGetStackByID(info.id, false);

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

void CPlayerInterface::battleObstaclesChanged(const BattleID & battleID, const ObstacleChanges & obstacle)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	if(obstacle.operation == BattleChanges::EOperation::ADD || obstacle.operation == BattleChanges::EOperation::UPDATE)
	{
		auto instance = cb->getBattle(battleID)->battleGetObstacleByID(obstacle.id);
		if(instance)
			battleInt->obstaclePlaced(instance);
		else
			logNetwork->error("Invalid obstacle instance %d", obstacle.id);
	}

	if(obstacle.operation == BattleChanges::EOperation::REMOVE)
		battleInt->obstacleRemoved(obstacle); //Obstacle is already removed, so, show animation based on json struct

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

	if(battleInt)
		battleInt->trySetActivePlayer(cb->getBattle(battleID)->sideToPlayer(action.side));

	battleInt->startAction(action);
}

void CPlayerInterface::actionFinished(const BattleID & battleID, const BattleAction &action)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	if (autofightingAI && !isAutoFightOn)
		unregisterBattleInterface(autofightingAI);

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
		//FIXME: we want client rendering to proceed while AI is making actions
		// so unlock mutex while AI is busy since this might take quite a while, especially if hero has many spells
		auto unlockInterface = vstd::makeUnlockGuard(ENGINE->interfaceMutex);
		autofightingAI->activeStack(battleID, stack);
		return;
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
		unregisterBattleInterface(autofightingAI);
		waitForAllDialogs();		//eagle eye skill can pop up multiple dialogs before the battle
		if(!battleInt)
		{
			bool allowManualReplay = queryID != QueryID::NONE && !isAutoFightEndBattle;

			auto wnd = std::make_shared<BattleResultWindow>(*br, *this, allowManualReplay);

			if (allowManualReplay || isAutoFightEndBattle)
			{
				wnd->resultCallback = [this, queryID](ui32 selection)
				{
					cb->selectionMade(selection, queryID);
				};
			}

			isAutoFightEndBattle = false;

			ENGINE->windows().pushWindow(wnd);
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

void CPlayerInterface::battleStackMoved(const BattleID & battleID, const CStack * stack, const BattleHexArray & dest, int distance, bool teleport)
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
void CPlayerInterface::battleAnimationPlayed(const BattleID & battleID, const BattleAnimationPlayed & pack)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->effectsController->battleAnimationPlayed(pack);
}
void CPlayerInterface::battleTriggerEffect(const BattleID & battleID, const BattleTriggerEffect & bte)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->effectsController->battleTriggerEffect(bte);

	if(bte.effect == BonusType::MANA_DRAIN)
	{
		const CGHeroInstance * manaDrainedHero = GAME->interface()->cb->getHero(ObjectInstanceID(bte.additionalInfo));
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
	info.playCustomAnimation = ba->playCustomAnimation();
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
	assert(info.defender != nullptr || (info.spellEffect != SpellID::NONE && info.indirectAttack));
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
		auto showInfoBox = [this, components, text, timer, soundID](bool abortMovement)
		{
			adventureInt->showInfoBoxMessage(components, text, timer);

			// Abort movement only when the message is shown synchronously with the event that produced it.
			// Deferred info-box messages may be displayed later, after movement has already resumed.
			if(abortMovement)
				movementController->requestMovementAbort();

			if (makingTurn && ENGINE->windows().count() > 0 && GAME->interface() == this)
				ENGINE->sound().playSound(static_cast<soundBase::soundID>(soundID));
		};

		if(showingDialog->isBusy() || !dialogs.empty())
		{
			createAndQueueDialog(PendingDialog::Type::NonBlocking, [showInfoBox = std::move(showInfoBox)]() mutable
			{
				showInfoBox(false);
			});
			tryShowNextPendingDialog();
			return;
		}

		waitWhileDialog(); //Fix for mantis #98
		closeActiveLevelUpDialog();
		showInfoBox(true);
		return;
	}

	if (settings["session"]["autoSkip"].Bool() && !ENGINE->isKeyboardShiftDown())
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
	LOG_TRACE_PARAMS(logGlobal, "player=%s, text=%s, is GAME->interface()=%d", playerID % text % (this==GAME->interface()));
	if (settings["session"]["autoSkip"].Bool() && !ENGINE->isKeyboardShiftDown())
	{
		return;
	}
	std::shared_ptr<CInfoWindow> temp = CInfoWindow::create(text, playerID, components);
	auto showDialog = [this, temp, soundID]()
	{
		ENGINE->sound().playSound(static_cast<soundBase::soundID>(soundID));
		showingDialog->setBusy();
		movementController->requestMovementAbort(); // interrupt movement to show dialog
		ENGINE->windows().pushWindow(temp);
	};

	if(showingDialog->isBusy() || !dialogs.empty())
	{
		createAndQueueDialog(PendingDialog::Type::Blocking, std::move(showDialog));
		tryShowNextPendingDialog();
		return;
	}

	waitWhileDialog();

	if ((makingTurn || (battleInt && battleInt->curInt && battleInt->curInt.get() == this)) && ENGINE->windows().count() > 0 && GAME->interface() == this)
	{
		closeActiveLevelUpDialog();
		showDialog();
	}
	else
	{
		createAndQueueDialog(PendingDialog::Type::Blocking, std::move(showDialog));
		tryShowNextPendingDialog();
	}
}

void CPlayerInterface::showInfoDialogAndWait(std::vector<Component> & components, const MetaString & text)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	std::string str = text.toString(&GAME->translator());

	showInfoDialog(EInfoWindowMode::MODAL, str, components, 0);
	waitWhileDialog();
}

void CPlayerInterface::showYesNoDialog(const std::string &text, CFunctionList<void()> onYes, CFunctionList<void()> onNo, const std::vector<std::shared_ptr<CComponent>> & components)
{
	waitWhileDialog();
	movementController->requestMovementAbort();
	GAME->interface()->showingDialog->setBusy();
	CInfoWindow::showYesNoDialog(text, components, onYes, onNo, playerID);
}

void CPlayerInterface::showBlockingDialog(const std::string &text, const std::vector<Component> &components, QueryID askID, const int soundID, bool selection, bool cancel, bool safeToAutoaccept)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();
	closeActiveLevelUpDialog();

	movementController->requestMovementAbort();
	ENGINE->sound().playSound(static_cast<soundBase::soundID>(soundID));

	if (!selection && cancel) //simple yes/no dialog
	{
		if(settings["general"]["enableUiEnhancements"].Bool() && safeToAutoaccept)
		{
			cb->selectionMade(1, askID); //as in HD mod, we try to skip dialogs that server considers visual fluff which does not affect gamestate
			return;
		}

		const bool commanderResurrectionDialog = text == LIBRARY->generaltexth->translate("vcmi.commander.resurrectionOffer");
		std::vector<std::shared_ptr<CComponent>> intComps;
		for(const auto & component : components)
		{
			auto uiComponent = std::make_shared<CComponent>(component);
			if(commanderResurrectionDialog && intComps.empty() && component.type == ComponentType::CREATURE)
			{
				const auto subtitle = uiComponent->getSubtitle();
				const auto firstSpace = subtitle.find(' ');
				if(firstSpace != std::string::npos)
					uiComponent = std::make_shared<CComponent>(component.type, component.subType, subtitle.substr(firstSpace + 1)); //keep only commander name
				uiComponent->newLine = true;
			}
			intComps.push_back(uiComponent); //will be deleted by close in window
		}

		showYesNoDialog(text, [this, askID](){ cb->selectionMade(1, askID); }, [this, askID](){ cb->selectionMade(0, askID); }, intComps);
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
		ENGINE->windows().createAndPushWindow<CSelWindow>(text, playerID, charperline, intComps, pom, askID);
		intComps[0]->clickPressed(ENGINE->getCursorPosition());
		intComps[0]->clickReleased(ENGINE->getCursorPosition());
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

	std::vector<ObjectInstanceID> objectGuiOrdered = objects;

	std::map<ObjectInstanceID, int> townOrder;
	auto ownedTowns = localState->getOwnedTowns();

	for (int i = 0; i < ownedTowns.size(); ++i)
		townOrder[ownedTowns[i]->id] = i;

	auto townComparator = [&townOrder](const ObjectInstanceID & left, const ObjectInstanceID & right){
		uint32_t leftIndex= townOrder.count(left) ? townOrder.at(left) : std::numeric_limits<uint32_t>::max();
		uint32_t rightIndex = townOrder.count(right) ? townOrder.at(right) : std::numeric_limits<uint32_t>::max();
		return leftIndex < rightIndex;
	};
	std::stable_sort(objectGuiOrdered.begin(), objectGuiOrdered.end(), townComparator);

	const std::string localTitle = title.toString(&GAME->translator());
	const std::string localDescription = description.toString(&GAME->translator());

	std::vector<int> tempList;
	tempList.reserve(objectGuiOrdered.size());

	for(const auto & item : objectGuiOrdered)
		tempList.push_back(item.getNum());

	CComponent localIconC(icon);

	std::shared_ptr<CIntObject> localIcon = localIconC.image;
	localIconC.removeChild(localIcon.get(), false);

	std::vector<std::shared_ptr<IImage>> images;
	for(const auto & obj : objectGuiOrdered)
	{
		if(!settings["general"]["enableUiEnhancements"].Bool())
			break;
		const CGTownInstance * t = dynamic_cast<const CGTownInstance *>(cb->getObj(obj));
		if(t)
		{
			auto image = ENGINE->renderHandler().loadImage(AnimationPath::builtin("ITPA"), t->getTown()->clientInfo.icons[t->hasFort()][false] + 2, 0, EImageBlitMode::OPAQUE);
			image->scaleTo(Point(35, 23), EScalingAlgorithm::NEAREST);
			images.push_back(image);
		}
	}

	auto selectCallback = [this, askID, objectGuiOrdered](int selection)
	{
		cb->sendQueryReply(objectGuiOrdered[selection], askID);
	};

	auto cancelCallback = [this, askID]()
	{
		cb->sendQueryReply(std::nullopt, askID);
	};

	auto wnd = std::make_shared<CObjectListWindow>(tempList, localIcon, localTitle, localDescription, selectCallback, 0, images);
	wnd->onExit = cancelCallback;
	wnd->onPopup = [this, objectGuiOrdered](int index) { CRClickPopup::createAndPush(cb->getObj(objectGuiOrdered[index]), ENGINE->getCursorPosition()); };
	wnd->onClicked = [this, objectGuiOrdered](int index) { adventureInt->centerOnObject(cb->getObj(objectGuiOrdered[index])); ENGINE->windows().totalRedraw(); };
	ENGINE->windows().pushWindow(wnd);
}

void CPlayerInterface::tileRevealed(const FowTilesType &pos)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	//FIXME: wait for dialog? Magi hut/eye would benefit from this but may break other areas
	adventureInt->onMapTilesChanged(pos);
}

void CPlayerInterface::tileHidden(const FowTilesType &pos)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->onMapTilesChanged(pos);
}

void CPlayerInterface::openHeroWindow(const CGHeroInstance *hero)
{
	ENGINE->windows().createAndPushWindow<CHeroWindow>(hero);
}

void CPlayerInterface::availableCreaturesChanged( const CGDwelling *town )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (const CGTownInstance * townObj = dynamic_cast<const CGTownInstance*>(town))
	{
		for (auto fortScreen : ENGINE->windows().findWindows<CFortScreen>())
			fortScreen->creaturesChangedEventHandler();

		for (auto castleInterface : ENGINE->windows().findWindows<CCastleInterface>())
			if(castleInterface->town == town)
				castleInterface->creaturesChangedEventHandler();

		if (townObj)
			for (auto ki : ENGINE->windows().findWindows<CKingdomInterface>())
				ki->townChanged(townObj);
	}
	else if(town && ENGINE->windows().count() > 0 && (town->ID == Obj::CREATURE_GENERATOR1
		||  town->ID == Obj::CREATURE_GENERATOR4  ||  town->ID == Obj::WAR_MACHINE_FACTORY))
	{
		for (auto crw : ENGINE->windows().findWindows<CRecruitmentWindow>())
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

	//recalculate paths because hero has lost or gained bonus influencing pathfinding
	if (bonus.type == BonusType::FLYING_MOVEMENT || bonus.type == BonusType::WATER_WALKING || bonus.type == BonusType::ROUGH_TERRAIN_DISCOUNT || bonus.type == BonusType::NO_TERRAIN_PENALTY)
		localState->verifyPath(hero);
}

void CPlayerInterface::moveHero( const CGHeroInstance *h, const CGPath& path )
{
	LOG_TRACE(logGlobal);
	if (!GAME->interface()->makingTurn)
		return;

	assert(h);

	if (!h)
		return; //can't find hero

	// Query-backed level-up chains can keep input blocked briefly after the visible
	// window closes, until QueryResolved advances or completes the chain.
	if (showingDialog->isBusy() || !dialogs.empty())
		return;

	if (localState->isHeroSleeping(h))
		localState->setHeroAwaken(h);

	movementController->requestMovementStart(h, path);
}

void CPlayerInterface::showGarrisonDialog(const CArmedInstance * up, const CGHeroInstance * down, bool removableUnits, QueryID queryID, const MetaString & customTitle)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto onEnd = [this, queryID](){ cb->selectionMade(0, queryID); };

	if (movementController->isHeroMovingThroughGarrison(down, up))
	{
		onEnd();
		return;
	}

	waitForAllDialogs();

	auto cgw = std::make_shared<CGarrisonWindow>(up, down, removableUnits, customTitle);
	cgw->quit->addCallback(onEnd);
	ENGINE->windows().pushWindow(cgw);
}

void CPlayerInterface::requestRealized( PackageApplied *pa )
{
	if(pa->packType == CTypeList::getInstance().getTypeID<MoveHero>(nullptr))
		movementController->onMoveHeroApplied();

	if(pa->packType == CTypeList::getInstance().getTypeID<QueryReply>(nullptr))
	{
		movementController->onQueryReplyApplied();
	}
}

void CPlayerInterface::closeActiveLevelUpDialog()
{
	if(auto levelWindow = ENGINE->windows().topWindow<CLevelWindow>())
		levelWindow->close();
	else if(auto commanderWindow = ENGINE->windows().topWindow<CStackWindow>(); commanderWindow && commanderWindow->isCommanderLevelUpDialog())
		commanderWindow->close();
}

void CPlayerInterface::queryResolved(QueryID queryID)
{
	auto dialog = findPendingDialog(queryID);
	if(dialog == dialogs.end())
		return;

	const bool wasFront = dialog == dialogs.begin();
	const bool wasLevelUpDialog = dialog->isLevelUpDialog();
	dialogs.erase(dialog);

	if(wasFront)
	{
		showingDialog->setFree();
		if(wasLevelUpDialog)
		{
			levelUpChainPendingContinuation = true;
			// Drain any queued accept/click events from the just-confirmed query-backed
			// dialog before showing whatever comes next. Otherwise the same Enter can
			// instantly accept the next level-up step or close a queued info dialog.
			delayQueuedDialogsUntilInputSettles = true;
			return;
		}

		closeActiveLevelUpDialog();
		tryShowNextPendingDialog();
	}
}

void CPlayerInterface::showHeroExchange(ObjectInstanceID hero1, ObjectInstanceID hero2)
{
	heroExchangeStarted(hero1, hero2, QueryID(-1));
}

void CPlayerInterface::heroExchangeStarted(ObjectInstanceID hero1, ObjectInstanceID hero2, QueryID query)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	ENGINE->windows().createAndPushWindow<CExchangeWindow>(hero1, hero2, query);
}

void CPlayerInterface::beforeObjectPropertyChanged(const SetObjectProperty * sop)
{
	if (sop->what == ObjProperty::OWNER)
	{
		const CGObjectInstance * obj = cb->getObj(sop->id, false);

		if(!obj)
			return;

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
		const CGObjectInstance * obj = cb->getObj(sop->id, false);

		if(!obj)
			return;

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
		FowTilesType upos(pos.begin(), pos.end());
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
			if(!hero->isGarrisoned())
				localState->addWanderingHero(hero);
		}
	}

	if(localState->getOwnedTowns().empty())
	{
		for(auto & town : cb->getTownsInfo())
			localState->addOwnedTown(town);
	}

	const std::optional<PlayerColor> callbackPlayer = cb->getPlayerID();
	const PlayerColor localStatePlayer = callbackPlayer.value_or(playerID);
	const PlayerState * playerState = cb->getPlayerState(localStatePlayer);
	if(playerState)
		localState->deserialize(*playerState->playerLocalSettings);

	if(adventureInt)
		adventureInt->onHeroChanged(nullptr);
}

void CPlayerInterface::showRecruitmentDialog(const CGDwelling *dwelling, const CArmedInstance *dst, int level, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();
	auto recruitCb = [this, dwelling, dst](CreatureID id, int count)
	{
		cb->recruitCreatures(dwelling, dst, id, count, -1);
	};
	auto closeCb = [this, queryID]()
	{
		cb->selectionMade(0, queryID);
	};
	ENGINE->windows().createAndPushWindow<CRecruitmentWindow>(dwelling, level, dst, recruitCb, closeCb);
}

void CPlayerInterface::waitWhileDialog()
{
	if (ENGINE->amIGuiThread())
	{
		logGlobal->warn("Cannot wait for dialogs in gui thread (deadlock risk)!");
		return;
	}

	auto unlockInterface = vstd::makeUnlockGuard(ENGINE->interfaceMutex);
	showingDialog->waitWhileBusy();
}

void CPlayerInterface::showShipyardDialog(const IShipyard *obj)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto state = obj->shipyardStatus();
	TResources cost;
	obj->getBoatCost(cost);
	ENGINE->windows().createAndPushWindow<CShipyardWindow>(cost, state, obj->getBoatType(), [this, obj](){ cb->buildBoat(obj); });
}

void CPlayerInterface::newObject( const CGObjectInstance * obj )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	//we might have built a boat in shipyard in opened town screen
	if (obj->ID == Obj::BOAT
		&& GAME->interface()->castleInt
		&&  obj->visitablePos() == GAME->interface()->castleInt->town->bestLocation())
	{
		GAME->interface()->castleInt->addBuilding(BuildingID::SHIP);
	}
}

void CPlayerInterface::centerView (int3 pos, int focusTime)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	// while a replay follows another player, the camera stays with him
	if(replayFollowedPlayer())
		return;

	waitWhileDialog();
	ENGINE->cursor().hide();
	adventureInt->centerOnTile(pos);
	if (focusTime)
	{
		ENGINE->windows().totalRedraw();
		{
			IgnoreEvents ignore(*this);
			auto unlockInterface = vstd::makeUnlockGuard(ENGINE->interfaceMutex);
			std::this_thread::sleep_for(std::chrono::milliseconds(focusTime));
		}
	}
	ENGINE->cursor().show();
}

void CPlayerInterface::objectRemoved(const CGObjectInstance * obj, const PlayerColor & initiator)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if(playerID == initiator)
	{
		auto removalSound = obj->getRemovalSound(CRandomGenerator::getDefault());
		if (removalSound)
		{
			waitWhileDialog();
			ENGINE->sound().playSound(removalSound.value());
		}
	}
	GAME->map().waitForOngoingAnimations();

	if(obj->ID == Obj::HERO && obj->tempOwner == playerID)
	{
		const CGHeroInstance * h = static_cast<const CGHeroInstance *>(obj);
		heroKilled(h);
	}

	if(obj->ID == Obj::TOWN && obj->tempOwner == playerID)
	{
		const CGTownInstance * t = static_cast<const CGTownInstance *>(obj);
		townRemoved(t);
	}
	ENGINE->fakeMouseMove();
}

void CPlayerInterface::objectRemovedAfter()
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->onMapTilesChanged(std::nullopt);

	// visiting or garrisoned hero removed - update window
	if (castleInt)
		castleInt->updateGarrisons();

	for (auto ki : ENGINE->windows().findWindows<CKingdomInterface>())
		ki->heroRemoved();
}

void CPlayerInterface::playerBlocked(int reason, bool start)
{
	if(reason == PlayerBlocked::EReason::UPCOMING_BATTLE)
	{
		if(GAME->server().howManyPlayerInterfaces() > 1 && GAME->interface() != this && GAME->interface()->makingTurn == false && !GAME->map().getMap()->battleOnly)
		{
			//one of our players who isn't last in order got attacked not by our another player (happens for example in hotseat mode)
			GAME->setInterfaceInstance(this);
			adventureInt->onCurrentPlayerChanged(playerID);
			MetaString msg;
			msg.appendTextID("vcmi.adventureMap.playerAttacked");
			msg.replaceRawString(cb->getStartInfo()->playerInfos.find(playerID)->second.name);
			std::vector<std::shared_ptr<CComponent>> cmp;
			cmp.push_back(std::make_shared<CComponent>(ComponentType::FLAG, playerID));
			makingTurn = true; //workaround for stiff showInfoDialog implementation
			showInfoDialog(msg.toString(&GAME->translator()), cmp);
			waitWhileDialog();
			makingTurn = false;
		}
	}
}

void CPlayerInterface::update()
{
	tryShowNextPendingDialog();
}

void CPlayerInterface::endNetwork()
{
	showingDialog->requestTermination();
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
					dates[last_write_time(dir->path())] = nr - '0';
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
			showInfoDialog(LIBRARY->generaltexth->allTexts[95]);

		auto previousInterface = GAME->interface(); //without multiple player interfaces some of lines below are useless, but for hotseat we wanna swap player interface temporarily

		GAME->setInterfaceInstance(this); //this is needed for dialog to show and avoid freeze, dialog showing logic should be reworked someday

		if(!makingTurn)
		{
			makingTurn = true; //also needed for dialog to show with current implementation
			waitForAllDialogs();
			makingTurn = false;
		}
		else
			waitForAllDialogs();

		GAME->setInterfaceInstance(previousInterface);
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

	ENGINE->windows().createAndPushWindow<CPuzzleWindow>(grailPos, ratio);
}

void CPlayerInterface::viewWorldMap()
{
	adventureInt->openWorldView();
}

void CPlayerInterface::advmapSpellCast(const CGHeroInstance * caster, SpellID spellID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	if(ENGINE->windows().topWindow<CSpellWindow>())
		ENGINE->windows().popWindows(1);

	auto castSoundPath = spellID.toSpell()->getCastSound();
	if(!castSoundPath.empty())
		ENGINE->sound().playSound(castSoundPath);
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
		showInfoDialog(LIBRARY->generaltexth->allTexts[msgToShow]);
}

void CPlayerInterface::battleNewRoundFirst(const BattleID & battleID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->newRoundFirst();
}

void CPlayerInterface::showMarketWindow(const IMarket * market, const CGHeroInstance * visitor, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto onWindowClosed = [this, queryID](){
		cb->selectionMade(0, queryID);
	};

	if(market->allowsTrade(EMarketMode::ARTIFACT_EXP) && visitor->getAlignment() != EAlignment::EVIL)
		ENGINE->windows().createAndPushWindow<CMarketWindow>(market, visitor, onWindowClosed, EMarketMode::ARTIFACT_EXP);
	else if(market->allowsTrade(EMarketMode::CREATURE_EXP) && visitor->getAlignment() != EAlignment::GOOD)
		ENGINE->windows().createAndPushWindow<CMarketWindow>(market, visitor, onWindowClosed, EMarketMode::CREATURE_EXP);
	else if(market->allowsTrade(EMarketMode::CREATURE_UNDEAD))
		ENGINE->windows().createAndPushWindow<CTransformerWindow>(market, visitor, onWindowClosed);
	else if (!market->availableModes().empty())
		for(auto mode = EMarketMode::RESOURCE_RESOURCE; mode != EMarketMode::MARKET_AFTER_LAST_PLACEHOLDER; mode = vstd::next(mode, 1))
		{
			if(vstd::contains(market->availableModes(), mode))
			{
				ENGINE->windows().createAndPushWindow<CMarketWindow>(market, visitor, onWindowClosed, mode);
				break;
			}
		}
	else
		onWindowClosed();
}

void CPlayerInterface::showUniversityWindow(const IMarket *market, const CGHeroInstance *visitor, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto onWindowClosed = [this, queryID](){
		cb->selectionMade(0, queryID);
	};
	ENGINE->windows().createAndPushWindow<CUniversityWindow>(visitor, BuildingID::NONE, market, onWindowClosed);
}

void CPlayerInterface::showHillFortWindow(const CGObjectInstance *object, const CGHeroInstance *visitor)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	ENGINE->windows().createAndPushWindow<CHillFortWindow>(visitor, object);
}

void CPlayerInterface::availableArtifactsChanged(const CGBlackMarket * bm)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	for (auto cmw : ENGINE->windows().findWindows<IMarketHolder>())
		cmw->updateArtifacts();
}

void CPlayerInterface::showTavernWindow(const CGObjectInstance * object, const CGHeroInstance * visitor, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto onWindowClosed = [this, queryID](){
		if (queryID != QueryID::NONE)
			cb->selectionMade(0, queryID);
	};
	ENGINE->windows().createAndPushWindow<CTavernWindow>(object, onWindowClosed);
}

void CPlayerInterface::showThievesGuildWindow (const CGObjectInstance * obj)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	ENGINE->windows().createAndPushWindow<CThievesGuildWindow>(obj);
}

void CPlayerInterface::showQuestLog()
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto quests = cb->getMyQuests();
	vstd::erase_if(quests, [this](const QuestInfo & quest)
	{
		return !quest.isDisplayable(cb.get());
	});
	if(quests.empty())
	{
		const auto entries = cb->getMyScenarioEventJournal();
		ENGINE->windows().createAndPushWindow<ScenarioEventJournal>(entries);
		return;
	}
	ENGINE->windows().createAndPushWindow<CQuestLog>(quests);
}

void CPlayerInterface::showScenarioEventJournal()
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	const auto entries = cb->getMyScenarioEventJournal();
	ENGINE->windows().createAndPushWindow<ScenarioEventJournal>(entries);
}

bool CPlayerInterface::hasDisplayableQuests() const
{
	const auto quests = cb->getMyQuests();
	return std::any_of(quests.begin(), quests.end(), [this](const QuestInfo & quest)
	{
		return quest.isDisplayable(cb.get());
	});
}

bool CPlayerInterface::hasScenarioEventJournalEntries() const
{
	return !cb->getMyScenarioEventJournal().empty();
}

bool CPlayerInterface::hasJournalEntries() const
{
	return hasDisplayableQuests() || hasScenarioEventJournalEntries();
}

void CPlayerInterface::scenarioEventJournalChanged()
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if(adventureInt)
		adventureInt->updateActiveState();
}

void CPlayerInterface::showShipyardDialogOrProblemPopup(const IShipyard *obj)
{
	if (obj->shipyardStatus() != IBoatGenerator::GOOD)
	{
		MetaString txt;
		obj->getProblemText(txt);
		showInfoDialog(txt.toString(&GAME->translator()));
	}
	else
		showShipyardDialog(obj);
}

void CPlayerInterface::askToAssembleArtifact(const ArtifactLocation &al)
{
	artifactController->askToAssemble(al, true, true);
}

void CPlayerInterface::artifactPut(const ArtifactLocation &al)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->onHeroChanged(cb->getHero(al.artHolder));
	garrisonsChanged(al.artHolder, ObjectInstanceID());
}

void CPlayerInterface::artifactRemoved(const ArtifactLocation &al)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->onHeroChanged(cb->getHero(al.artHolder));
	garrisonsChanged(al.artHolder, ObjectInstanceID());
	artifactController->artifactRemoved();
}

void CPlayerInterface::artifactMoved(const ArtifactLocation &src, const ArtifactLocation &dst)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->onHeroChanged(cb->getHero(dst.artHolder));
	garrisonsChanged(src.artHolder, dst.artHolder);
	artifactController->artifactMoved();
}

void CPlayerInterface::bulkArtMovementStart(size_t totalNumOfArts, size_t possibleAssemblyNumOfArts)
{
	artifactController->bulkArtMovementStart(totalNumOfArts, possibleAssemblyNumOfArts);
}

void CPlayerInterface::artifactAssembled(const ArtifactLocation &al)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->onHeroChanged(cb->getHero(al.artHolder));
	artifactController->artifactAssembled();
}

void CPlayerInterface::artifactDisassembled(const ArtifactLocation &al)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->onHeroChanged(cb->getHero(al.artHolder));
	artifactController->artifactDisassembled();
}

void CPlayerInterface::waitForAllDialogs()
{
	if (!makingTurn)
		return;

	while(!dialogs.empty())
	{
		auto unlockInterface = vstd::makeUnlockGuard(ENGINE->interfaceMutex);
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	waitWhileDialog();
}

void CPlayerInterface::createAndQueueDialog(PendingDialog::Type blockingPolicy, std::function<void()> showCallback, QueryID queryID)
{
	PendingDialog dialog;
	dialog.queryID = queryID >= 0 ? queryID : QueryID::NONE;
	dialog.blockingPolicy = blockingPolicy;
	// Level-up dialogs currently mean hero/commander level-up prompts.
	// Keep them alive across turn-end and keep the whole query-backed chain
	// ahead of ordinary queued info/reward dialogs.
	dialog.dropOnTurnEnd = !dialog.isLevelUpDialog();
	dialog.showCallback = std::move(showCallback);

	if(dialog.isLevelUpDialog() && (levelUpChainPendingContinuation || (!dialogs.empty() && dialogs.front().isLevelUpDialog())))
		dialogs.insert(findQueryBackedDialogInsertionPoint(), std::move(dialog));
	else
		dialogs.push_back(std::move(dialog));
}

std::list<CPlayerInterface::PendingDialog>::iterator CPlayerInterface::findQueryBackedDialogInsertionPoint()
{
	return std::find_if(dialogs.begin(), dialogs.end(), [](const PendingDialog & dialog)
	{
		return !dialog.isLevelUpDialog();
	});
}

void CPlayerInterface::tryShowNextPendingDialog()
{
	if(delayQueuedDialogsUntilInputSettles)
		return;

	if(dialogs.empty())
	{
		if(levelUpChainPendingContinuation)
		{
			levelUpChainPendingContinuation = false;
			closeActiveLevelUpDialog();
		}
		return;
	}

	while(!dialogs.empty() && !showingDialog->isBusy())
	{
		auto & dialog = dialogs.front();
		// Level-up dialogs currently mean hero/commander level-up prompts.
		// Keep showing those even after makingTurn becomes false, but stop normal queued dialogs.
		if(!makingTurn && !dialog.isLevelUpDialog())
			return;

		if(dialog.state != PendingDialog::State::Queued)
			return;

		if(!dialog.isLevelUpDialog())
		{
			levelUpChainPendingContinuation = false;
			closeActiveLevelUpDialog();
		}

		dialog.showCallback();
		if(dialog.isLevelUpDialog())
		{
			dialog.state = PendingDialog::State::AwaitingQueryResolution;
			return;
		}

		dialogs.pop_front();

		if(dialog.blockingPolicy == PendingDialog::Type::Blocking || showingDialog->isBusy())
			return;
	}
}

std::list<CPlayerInterface::PendingDialog>::iterator CPlayerInterface::findPendingDialog(QueryID queryID)
{
	return std::find_if(dialogs.begin(), dialogs.end(), [queryID](const PendingDialog & dialog)
	{
		return dialog.queryID == queryID;
	});
}

void CPlayerInterface::proposeLoadingGame()
{
	showYesNoDialog(
		LIBRARY->generaltexth->allTexts[68],
		[]()
		{
			GAME->server().endGameplay();
			GAME->mainmenu()->menu->switchToTab("load");
		},
		nullptr
	);
}

void CPlayerInterface::quickSaveGame()
{
	const std::string quickSavePath = getQuickSavePath();

	// notify player about saving
	MetaString txt;
	txt.appendTextID("vcmi.adventureMap.savingQuickSave");
	txt.replaceRawString(quickSavePath);
	GAME->server().getGameChat().sendMessageGameplay(txt.toString(&GAME->translator()));
	GAME->interface()->cb->save(quickSavePath, false);
	hasQuickSave = true;
	if(adventureInt)
		adventureInt->updateActiveState();
}

bool CPlayerInterface::checkQuickLoadingGame(bool verbose)
{
	const std::string quickSavePath = getQuickSavePath();
	if(!CResourceHandler::get("local")->existsResource(ResourcePath(quickSavePath, EResType::SAVEGAME)))
	{
		if(verbose)
			logGlobal->error("No quicksave file found at %s", quickSavePath);
		else
			logGlobal->trace("No quicksave file found at %s", quickSavePath);
		hasQuickSave = false;
		if(cb && adventureInt)
			adventureInt->updateActiveState();
		return false;
	}
	auto error = GAME->server().canQuickLoadGame(quickSavePath);
	if(error)
	{
		if(verbose)
			logGlobal->error("Cannot quick load game at %s: %s", quickSavePath, *error);
		else
			logGlobal->trace("Cannot quick load game at %s: %s", quickSavePath, *error);
		hasQuickSave = false;
		if(cb && adventureInt)
			adventureInt->updateActiveState();
		return false;
	}
	return true;
}

void CPlayerInterface::proposeQuickLoadingGame()
{
	if(!checkQuickLoadingGame(true))
		return;

	const std::string quickSavePath = getQuickSavePath();
	auto onYes = [quickSavePath]() -> void
	{
		GAME->server().quickLoadGame(quickSavePath);
	};

	GAME->interface()->showYesNoDialog(LIBRARY->generaltexth->translate("vcmi.adventureMap.confirmQuickLoadGame"), onYes, nullptr);
}

std::string CPlayerInterface::getQuickSavePath() const
{
	return SavegamePath::getPath(*cb->getStartInfo(), *cb->getMapHeader(), "Quicksave");
}

bool CPlayerInterface::capturedAllEvents()
{
	if(movementController->isHeroMoving())
	{
		//just inform that we are capturing events. they will be processed by heroMoved() in client thread.
		return true;
	}

	bool needToLockAdventureMap = adventureInt && adventureInt->isActive() && GAME->map().hasOngoingAnimations();
	bool quickCombatWithoutDialogs = isAutoFightOn && !battleInt && !showingDialog->isBusy();
	bool waitingForQueuedDialogInputToSettle = false;
	bool waitingForQueuedDialogResolution =
		!showingDialog->isBusy() &&
		!dialogs.empty() &&
		dialogs.front().state == PendingDialog::State::AwaitingQueryResolution;

	if(delayQueuedDialogsUntilInputSettles)
	{
		waitingForQueuedDialogInputToSettle = ENGINE->input().ignoreEventsUntilInput();

		if(!waitingForQueuedDialogInputToSettle)
		{
			delayQueuedDialogsUntilInputSettles = false;
			tryShowNextPendingDialog();
		}
	}

	if (ignoreEvents || needToLockAdventureMap || quickCombatWithoutDialogs || waitingForQueuedDialogResolution || waitingForQueuedDialogInputToSettle)
	{
		if(!delayQueuedDialogsUntilInputSettles)
			ENGINE->input().ignoreEventsUntilInput();
		return true;
	}

	return false;
}

void CPlayerInterface::prepareAutoFightingAI(const BattleID &bid, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, BattleSide side)
{
	autofightingAI = AIFactory::createBattleAI(settings["ai"]["combatAlliedAI"].String());

	AutocombatPreferences autocombatPreferences = AutocombatPreferences();
	autocombatPreferences.enableSpellsUsage = settings["battle"]["enableAutocombatSpells"].Bool();
	autocombatPreferences.enableTacticsUsage = settings["battle"]["enableAutocombatTactics"].Bool();

	autofightingAI->initBattleInterface(env, cb, autocombatPreferences);
	autofightingAI->battleStart(bid, army1, army2, tile, hero1, hero2, side, false);
	isAutoFightOn = true;
	registerBattleInterface(autofightingAI);
}

void CPlayerInterface::showWorldViewEx(const std::vector<ObjectPosInfo>& objectPositions, bool showTerrain)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->openWorldView(objectPositions, showTerrain );
}

void CPlayerInterface::setColorScheme(ColorScheme scheme)
{
	ENGINE->screenHandler().setColorScheme(scheme);
}

std::optional<BattleAction> CPlayerInterface::makeSurrenderRetreatDecision(const BattleID & battleID, const BattleStateInfoForRetreat & battleState)
{
	return std::nullopt;
}

void CPlayerInterface::registerBattleInterface(std::shared_ptr<CBattleGameInterface> battleEvents)
{
	autofightingAI = battleEvents;
	GAME->server().client->registerBattleInterface(battleEvents, playerID);
}

void CPlayerInterface::unregisterBattleInterface(std::shared_ptr<CBattleGameInterface> battleEvents)
{
	assert(battleEvents == autofightingAI);
	GAME->server().client->unregisterBattleInterface(autofightingAI, playerID);
	autofightingAI.reset();
}

void CPlayerInterface::responseStatistic(StatisticDataSet & statistic)
{
	ENGINE->windows().createAndPushWindow<CStatisticScreen>(statistic);
}
