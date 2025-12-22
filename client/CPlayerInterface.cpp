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
#include "windows/CSpellWindow.h"
#include "windows/CTutorialWindow.h"
#include "windows/GUIClasses.h"
#include "windows/InfoWindows.h"
#include "windows/settings/SettingsMainWindow.h"

#include "../lib/callback/CDynLibHandler.h"
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

#include "../lib/spells/CSpellHandler.h"

#include "../lib/texts/TextOperations.h"

#include "../lib/filesystem/Filesystem.h"

#include <boost/lexical_cast.hpp>

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
	autosaveCount = 0;
	isAutoFightOn = false;
	isAutoFightEndBattle = false;
	ignoreEvents = false;
	hasQuickSave = checkQuickLoadingGame();
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

	pathfinderCache = std::make_unique<PathfinderCache>(cb.get(), PathfinderOptions(*cb));
	ENGINE->music().loadTerrainMusicThemes();
	initializeHeroTownList();

	adventureInt.reset(new AdventureMapInterface());
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
		closeAllDialogs();

		// remove all pending dialogs that do not expect query answer
		vstd::erase_if(dialogs, [](const std::shared_ptr<CInfoWindow> & window){
						   return window->ID == QueryID::NONE;
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

				TextOperations::trimRightUnicode(name, std::max(0, txtlen - 14));
				auto const & isSymbolIllegal = [&](char c) {
					static const std::string forbiddenChars("\\/:*?\"<>| ");

					bool charForbidden = forbiddenChars.find(c) != std::string::npos;
					bool charNonprintable = static_cast<unsigned char>(c) < static_cast<unsigned char>(' ');

					return charForbidden || charNonprintable;
				};
				std::replace_if(name.begin(), name.end(), isSymbolIllegal, '_' );

				prefix = vstd::getFormattedDateTime(cb->getStartInfo()->startTime, "%Y-%m-%d_%H-%M") + "_" + name + "/";
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
			std::string msg = LIBRARY->generaltexth->allTexts[13];
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
	LOG_TRACE_PARAMS(logGlobal, "Hero %s killed handler for player %s", hero->getNameTranslated() % playerID);

	// if hero is not in town garrison
	if (vstd::contains(localState->getWanderingHeroes(), hero))
		localState->removeWanderingHero(hero);

	adventureInt->onHeroChanged(hero);
	localState->erasePath(hero);
}

void CPlayerInterface::townRemoved(const CGTownInstance* town)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

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
	waitWhileDialog();
	ENGINE->sound().playSound(soundBase::heroNewLevel);
	ENGINE->windows().createAndPushWindow<CLevelWindow>(hero, pskill, skills, [this, queryID](ui32 selection)
	{
		cb->selectionMade(selection, queryID);
	});
}

void CPlayerInterface::commanderGotLevel (const CCommanderInstance * commander, std::vector<ui32> skills, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();
	ENGINE->sound().playSound(soundBase::heroNewLevel);
	ENGINE->windows().createAndPushWindow<CStackWindow>(commander, skills, [this, queryID](ui32 selection)
	{
		cb->selectionMade(selection, queryID);
	});
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
			auto unlockInterface = vstd::makeUnlockGuard(ENGINE->interfaceMutex);
			autofightingAI->activeStack(battleID, stack);
			return;
		}
		unregisterBattleInterface(autofightingAI);
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
		waitWhileDialog(); //Fix for mantis #98
		adventureInt->showInfoBoxMessage(components, text, timer);

		// abort movement, if any. Strictly speaking unnecessary, but prevents some edge cases, like movement sound on visiting Magi Hut with "show messages in status window" on
		movementController->requestMovementAbort();

		if (makingTurn && ENGINE->windows().count() > 0 && GAME->interface() == this)
			ENGINE->sound().playSound(static_cast<soundBase::soundID>(soundID));
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
	waitWhileDialog();

	if (settings["session"]["autoSkip"].Bool() && !ENGINE->isKeyboardShiftDown())
	{
		return;
	}
	std::shared_ptr<CInfoWindow> temp = CInfoWindow::create(text, playerID, components);

	if ((makingTurn || (battleInt && battleInt->curInt && battleInt->curInt.get() == this)) && ENGINE->windows().count() > 0 && GAME->interface() == this)
	{
		ENGINE->sound().playSound(static_cast<soundBase::soundID>(soundID));
		showingDialog->setBusy();
		movementController->requestMovementAbort(); // interrupt movement to show dialog
		ENGINE->windows().pushWindow(temp);
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
	waitWhileDialog();
	movementController->requestMovementAbort();
	GAME->interface()->showingDialog->setBusy();
	CInfoWindow::showYesNoDialog(text, components, onYes, onNo, playerID);
}

void CPlayerInterface::showBlockingDialog(const std::string &text, const std::vector<Component> &components, QueryID askID, const int soundID, bool selection, bool cancel, bool safeToAutoaccept)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();

	movementController->requestMovementAbort();
	ENGINE->sound().playSound(static_cast<soundBase::soundID>(soundID));

	if (!selection && cancel) //simple yes/no dialog
	{
		if(settings["general"]["enableUiEnhancements"].Bool() && safeToAutoaccept)
		{
			cb->selectionMade(1, askID); //as in HD mod, we try to skip dialogs that server considers visual fluff which does not affect gamestate
			return;
		}

		std::vector<std::shared_ptr<CComponent>> intComps;
		for (auto & component : components)
			intComps.push_back(std::make_shared<CComponent>(component)); //will be deleted by close in window

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

	auto selectCallback = [this, askID](int selection)
	{
		cb->sendQueryReply(selection, askID);
	};

	auto cancelCallback = [this, askID]()
	{
		cb->sendQueryReply(std::nullopt, askID);
	};

	const std::string localTitle = title.toString();
	const std::string localDescription = description.toString();

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
	assert(!showingDialog->isBusy());
	assert(dialogs.empty());

	if (!h)
		return; //can't find hero

	//It shouldn't be possible to move hero with open dialog (or dialog waiting in bg)
	if (showingDialog->isBusy() || !dialogs.empty())
		return;

	if (localState->isHeroSleeping(h))
		localState->setHeroAwaken(h);

	movementController->requestMovementStart(h, path);
}

void CPlayerInterface::showGarrisonDialog( const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto onEnd = [this, queryID](){ cb->selectionMade(0, queryID); };

	if (movementController->isHeroMovingThroughGarrison(down, up))
	{
		onEnd();
		return;
	}

	waitForAllDialogs();

	auto cgw = std::make_shared<CGarrisonWindow>(up, down, removableUnits);
	cgw->quit->addCallback(onEnd);
	ENGINE->windows().pushWindow(cgw);
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
	ENGINE->windows().createAndPushWindow<CExchangeWindow>(hero1, hero2, query);
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

	localState->deserialize(*cb->getPlayerState(playerID)->playerLocalSettings);

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
	adventureInt->onMapTilesChanged(boost::none);

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
			std::string msg = LIBRARY->generaltexth->translate("vcmi.adventureMap.playerAttacked");
			boost::replace_first(msg, "%s", cb->getStartInfo()->playerInfos.find(playerID)->second.name);
			std::vector<std::shared_ptr<CComponent>> cmp;
			cmp.push_back(std::make_shared<CComponent>(ComponentType::FLAG, playerID));
			makingTurn = true; //workaround for stiff showInfoDialog implementation
			showInfoDialog(msg, cmp);
			waitWhileDialog();
			makingTurn = false;
		}
	}
}

void CPlayerInterface::update()
{
	//if there are any waiting dialogs, show them
	if (makingTurn && !dialogs.empty() && !showingDialog->isBusy())
	{
		showingDialog->setBusy();
		ENGINE->windows().pushWindow(dialogs.front());
		dialogs.pop_front();
	}
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
	ENGINE->windows().createAndPushWindow<CQuestLog>(GAME->interface()->cb->getMyQuests());
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

void CPlayerInterface::askToAssembleArtifact(const ArtifactLocation &al)
{
	artifactController->askToAssemble(al, true, true);
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
	artifactController->artifactRemoved();
}

void CPlayerInterface::artifactMoved(const ArtifactLocation &src, const ArtifactLocation &dst)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->onHeroChanged(cb->getHero(dst.artHolder));
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
	// notify player about saving
	MetaString txt;
	txt.appendTextID("vcmi.adventureMap.savingQuickSave");	
	txt.replaceTextID(QUICKSAVE_PATH);
	GAME->server().getGameChat().sendMessageGameplay(txt.toString());
	GAME->interface()->cb->save(QUICKSAVE_PATH);
	hasQuickSave = true;
	if(adventureInt)
		adventureInt->updateActiveState();
}

bool CPlayerInterface::checkQuickLoadingGame(bool verbose)
{
	if(!CResourceHandler::get("local")->existsResource(ResourcePath(QUICKSAVE_PATH, EResType::SAVEGAME)))
	{
		if(verbose)
			logGlobal->error("No quicksave file found at %s", QUICKSAVE_PATH);
		else
			logGlobal->trace("No quicksave file found at %s", QUICKSAVE_PATH);
		hasQuickSave = false;
		if(cb && adventureInt)
			adventureInt->updateActiveState();
		return false;
	}
	auto error = GAME->server().canQuickLoadGame(QUICKSAVE_PATH);
	if(error)
	{
		if(verbose)
			logGlobal->error("Cannot quick load game at %s: %s", QUICKSAVE_PATH, *error);
		else
			logGlobal->trace("Cannot quick load game at %s: %s", QUICKSAVE_PATH, *error);
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

	auto onYes = [this]() -> void
	{
		GAME->server().quickLoadGame(QUICKSAVE_PATH);
	};

	GAME->interface()->showYesNoDialog(LIBRARY->generaltexth->translate("vcmi.adventureMap.confirmQuickLoadGame"), onYes, nullptr);
}

bool CPlayerInterface::capturedAllEvents()
{
	if(movementController->isHeroMoving())
	{
		//just inform that we are capturing events. they will be processed by heroMoved() in client thread.
		return true;
	}

	bool needToLockAdventureMap = adventureInt && adventureInt->isActive() && GAME->map().hasOngoingAnimations();
	bool quickCombatOngoing = isAutoFightOn && !battleInt;

	if (ignoreEvents || needToLockAdventureMap || quickCombatOngoing )
	{
		ENGINE->input().ignoreEventsUntilInput();
		return true;
	}

	return false;
}

void CPlayerInterface::prepareAutoFightingAI(const BattleID &bid, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, BattleSide side)
{
	autofightingAI = CDynLibHandler::getNewBattleAI(settings["server"]["friendlyAI"].String());

	AutocombatPreferences autocombatPreferences = AutocombatPreferences();
	autocombatPreferences.enableSpellsUsage = settings["battle"]["enableAutocombatSpells"].Bool();

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
