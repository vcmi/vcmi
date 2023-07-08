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

#include "adventureMap/AdventureMapInterface.h"
#include "mapView/mapHandler.h"
#include "adventureMap/CList.h"
#include "battle/BattleInterface.h"
#include "battle/BattleEffectsController.h"
#include "battle/BattleFieldController.h"
#include "battle/BattleInterfaceClasses.h"
#include "battle/BattleWindow.h"
#include "../CCallback.h"
#include "windows/CCastleInterface.h"
#include "eventsSDL/InputHandler.h"
#include "mainmenu/CMainMenu.h"
#include "gui/CursorHandler.h"
#include "windows/CKingdomInterface.h"
#include "CGameInfo.h"
#include "PlayerLocalState.h"
#include "CMT.h"
#include "windows/CHeroWindow.h"
#include "windows/CCreatureWindow.h"
#include "windows/CQuestLog.h"
#include "windows/CPuzzleWindow.h"
#include "widgets/CComponent.h"
#include "widgets/Buttons.h"
#include "windows/CTradeWindow.h"
#include "windows/CSpellWindow.h"
#include "../lib/CConfigHandler.h"
#include "windows/GUIClasses.h"
#include "render/CAnimation.h"
#include "render/IImage.h"
#include "../lib/CArtHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/bonuses/CBonusSystemNode.h"
#include "../lib/bonuses/Limiters.h"
#include "../lib/bonuses/Updaters.h"
#include "../lib/bonuses/Propagators.h"
#include "../lib/serializer/CTypeList.h"
#include "../lib/serializer/BinaryDeserializer.h"
#include "../lib/serializer/BinarySerializer.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CTownHandler.h"
#include "../lib/mapObjects/CGTownInstance.h"
#include "../lib/mapObjects/MiscObjects.h"
#include "../lib/mapObjects/ObjectTemplate.h"
#include "../lib/mapping/CMapHeader.h"
#include "../lib/pathfinder/CGPathNode.h"
#include "../lib/CStack.h"
#include "../lib/JsonNode.h"
#include "CMusicHandler.h"
#include "../lib/CondSh.h"
#include "../lib/NetPacksBase.h"
#include "../lib/NetPacks.h"//todo: remove
#include "../lib/VCMIDirs.h"
#include "../lib/CStopWatch.h"
#include "../lib/StartInfo.h"
#include "../lib/CPlayerState.h"
#include "../lib/GameConstants.h"
#include "gui/CGuiHandler.h"
#include "gui/WindowHandler.h"
#include "windows/InfoWindows.h"
#include "../lib/UnlockGuard.h"
#include "../lib/RoadHandler.h"
#include "../lib/TerrainHandler.h"
#include "CServerHandler.h"
// FIXME: only needed for CGameState::mutex
#include "../lib/gameState/CGameState.h"
#include "eventsSDL/NotificationHandler.h"
#include "adventureMap/CInGameConsole.h"

// The macro below is used to mark functions that are called by client when game state changes.
// They all assume that CPlayerInterface::pim mutex is locked.
#define EVENT_HANDLER_CALLED_BY_CLIENT

// The macro marks functions that are run on a new thread by client.
// They do not own any mutexes intiially.
#define THREAD_CREATED_BY_CLIENT

#define RETURN_IF_QUICK_COMBAT		\
	if (isAutoFightOn && !battleInt)	\
		return;

#define BATTLE_EVENT_POSSIBLE_RETURN\
	if (LOCPLINT != this)			\
		return;						\
	RETURN_IF_QUICK_COMBAT

boost::recursive_mutex * CPlayerInterface::pim = new boost::recursive_mutex;

CPlayerInterface * LOCPLINT;

std::shared_ptr<BattleInterface> CPlayerInterface::battleInt;

enum  EMoveState {STOP_MOVE, WAITING_MOVE, CONTINUE_MOVE, DURING_MOVE};
CondSh<EMoveState> stillMoveHero(STOP_MOVE); //used during hero movement

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
	localState(std::make_unique<PlayerLocalState>(*this))
{
	logGlobal->trace("\tHuman player interface for player %s being constructed", Player.getStr());
	destinationTeleport = ObjectInstanceID();
	destinationTeleportPos = int3(-1);
	GH.defActionsDef = 0;
	LOCPLINT = this;
	curAction = nullptr;
	playerID=Player;
	human=true;
	battleInt = nullptr;
	castleInt = nullptr;
	makingTurn = false;
	showingDialog = new CondSh<bool>(false);
	cingconsole = new CInGameConsole();
	GH.terminate_cond->set(false);
	firstCall = 1; //if loading will be overwritten in serialize
	autosaveCount = 0;
	isAutoFightOn = false;

	duringMovement = false;
	ignoreEvents = false;
	numOfMovedArts = 0;
}

CPlayerInterface::~CPlayerInterface()
{
	logGlobal->trace("\tHuman player interface for player %s being destructed", playerID.getStr());
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

	// always recreate advmap interface to avoid possible memory-corruption bugs
	adventureInt.reset(new AdventureMapInterface());
}

void CPlayerInterface::playerStartsTurn(PlayerColor player)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	if(GH.windows().findWindows<AdventureMapInterface>().empty())
	{
		// after map load - remove all active windows and replace them with adventure map
		GH.windows().clear();
		GH.windows().pushWindow(adventureInt);
	}

	// remove all dialogs that do not expect query answer
	while (!GH.windows().topWindow<AdventureMapInterface>() && !GH.windows().topWindow<CInfoWindow>())
		GH.windows().popWindows(1);

	if (player != playerID && LOCPLINT == this)
	{
		waitWhileDialog();

		bool isHuman = cb->getStartInfo()->playerInfos.count(player) && cb->getStartInfo()->playerInfos.at(player).isControlledByHuman();

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
				prefix = cb->getMapHeader()->name.substr(0, 5) + "_";
			}
		}

		autosaveCount++;

		int autosaveCountLimit = settings["general"]["autosaveCountLimit"].Integer();
		if(autosaveCountLimit > 0)
		{
			cb->save("Saves/" + prefix + "Autosave_" + std::to_string(autosaveCount));
			autosaveCount %= autosaveCountLimit;
		}
		else
		{
			std::string stringifiedDate = (cb->getDate(Date::MONTH) < 10
				? std::string("0") + std::to_string(cb->getDate(Date::MONTH))
				: std::to_string(cb->getDate(Date::MONTH)))
					+ std::to_string(cb->getDate(Date::WEEK))
					+ std::to_string(cb->getDate(Date::DAY_OF_WEEK));

			cb->save("Saves/" + prefix + "Autosave_" + stringifiedDate);
		}
	}
}

void CPlayerInterface::yourTurn()
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	{
		LOCPLINT = this;
		GH.curInt = this;

		NotificationHandler::notify("Your turn");
		performAutosave();

		if (CSH->howManyPlayerInterfaces() > 1) //hot seat message
		{
			adventureInt->onHotseatWaitStarted(playerID);

			makingTurn = true;
			std::string msg = CGI->generaltexth->allTexts[13];
			boost::replace_first(msg, "%s", cb->getStartInfo()->playerInfos.find(playerID)->second.name);
			std::vector<std::shared_ptr<CComponent>> cmp;
			cmp.push_back(std::make_shared<CComponent>(CComponent::flag, playerID.getNum(), 0));
			showInfoDialog(msg, cmp);
		}
		else
		{
			makingTurn = true;
			adventureInt->onPlayerTurnStarted(playerID);
		}
	}
	acceptTurn();
}

void CPlayerInterface::acceptTurn()
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
		components.emplace_back(Component::EComponentType::FLAG, playerColor.getNum(), 0, 0);
		MetaString text;

		const auto & optDaysWithoutCastle = cb->getPlayerState(playerColor)->daysWithoutCastle;

		if(optDaysWithoutCastle)
		{
			auto daysWithoutCastle = optDaysWithoutCastle.value();
			if (daysWithoutCastle < 6)
			{
				text.appendLocalString(EMetaText::ARRAY_TXT,128); //%s, you only have %d days left to capture a town or you will be banished from this land.
				text.replaceLocalString(EMetaText::COLOR, playerColor.getNum());
				text.replaceNumber(7 - daysWithoutCastle);
			}
			else if (daysWithoutCastle == 6)
			{
				text.appendLocalString(EMetaText::ARRAY_TXT,129); //%s, this is your last day to capture a town or you will be banished from this land.
				text.replaceLocalString(EMetaText::COLOR, playerColor.getNum());
			}

			showInfoDialogAndWait(components, text);
		}
		else
			logGlobal->warn("Player has no towns, but daysWithoutCastle is not set");
	}
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

	if (details.result == TryMoveHero::EMBARK || details.result == TryMoveHero::DISEMBARK)
	{
		if(hero->getRemovalSound() && hero->tempOwner == playerID)
			CCS->soundh->playSound(hero->getRemovalSound().value());
	}

	std::unordered_set<int3> changedTiles {
		hero->convertToVisitablePos(details.start),
		hero->convertToVisitablePos(details.end)
	};
	adventureInt->onMapTilesChanged(changedTiles);
	adventureInt->onHeroMovementStarted(hero);

	bool directlyAttackingCreature = details.attackedFrom && localState->hasPath(hero) && localState->getPath(hero).endPos() == *details.attackedFrom;

	if(makingTurn && hero->tempOwner == playerID) //we are moving our hero - we may need to update assigned path
	{
		if(details.result == TryMoveHero::TELEPORTATION)
		{
			if(localState->hasPath(hero))
			{
				assert(localState->getPath(hero).nodes.size() >= 2);
				auto nodesIt = localState->getPath(hero).nodes.end() - 1;

				if((nodesIt)->coord == hero->convertToVisitablePos(details.start)
					&& (nodesIt - 1)->coord == hero->convertToVisitablePos(details.end))
				{
					//path was between entrance and exit of teleport -> OK, erase node as usual
					localState->removeLastNode(hero);
				}
				else
				{
					//teleport was not along current path, it'll now be invalid (hero is somewhere else)
					localState->erasePath(hero);

				}
			}
		}

		if(hero->pos != details.end //hero didn't change tile but visit succeeded
			|| directlyAttackingCreature) // or creature was attacked from endangering tile.
		{
			localState->erasePath(hero);
		}
		else if(localState->hasPath(hero) && hero->pos == details.end) //&& hero is moving
		{
			if(details.start != details.end) //so we don't touch path when revisiting with spacebar
				localState->removeLastNode(hero);
		}
	}

	if(details.stopMovement()) //hero failed to move
	{
		stillMoveHero.setn(STOP_MOVE);
		adventureInt->onHeroChanged(hero);
		return;
	}

	CGI->mh->waitForOngoingAnimations();

	//move finished
	adventureInt->onHeroChanged(hero);

	//check if user cancelled movement
	{
		if (GH.input().ignoreEventsUntilInput())
			stillMoveHero.setn(STOP_MOVE);
	}

	if (stillMoveHero.get() == WAITING_MOVE)
		stillMoveHero.setn(DURING_MOVE);

	// Hero attacked creature directly, set direction to face it.
	if (directlyAttackingCreature) {
		// Get direction to attacker.
		int3 posOffset = *details.attackedFrom - details.end + int3(2, 1, 0);
		static const ui8 dirLookup[3][3] = {
			{ 1, 2, 3 },
			{ 8, 0, 4 },
			{ 7, 6, 5 }
		};
		// FIXME: Avoid const_cast, make moveDir mutable in some other way?
		const_cast<CGHeroInstance *>(hero)->moveDir = dirLookup[posOffset.y][posOffset.x];
	}
}

void CPlayerInterface::heroKilled(const CGHeroInstance* hero)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	LOG_TRACE_PARAMS(logGlobal, "Hero %s killed handler for player %s", hero->getNameTranslated() % playerID);

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

void CPlayerInterface::heroPrimarySkillChanged(const CGHeroInstance * hero, int which, si64 val)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (which == 4)
	{
		for (auto ctw : GH.windows().findWindows<CAltarWindow>())
			ctw->setExpToLevel();
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

void CPlayerInterface::heroGotLevel(const CGHeroInstance *hero, PrimarySkill::PrimarySkill pskill, std::vector<SecondarySkill>& skills, QueryID queryID)
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
		if(town->garrisonHero->tempOwner == playerID && !vstd::contains(localState->getWanderingHeroes(), town->visitingHero))
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

	if(castleInt)
	{
		castleInt->garr->selectSlot(nullptr);
		castleInt->garr->setArmy(town->getUpperArmy(), 0);
		castleInt->garr->setArmy(town->visitingHero, 1);
		castleInt->garr->recreateSlots();
		castleInt->heroes->update();
		castleInt->redraw();
	}

	for (auto ki : GH.windows().findWindows<CKingdomInterface>())
	{
		ki->townChanged(town);
		ki->updateGarrisons();
		ki->redraw();
	}
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
	std::vector<const CGObjectInstance *> instances;

	if(auto obj = cb->getObj(id1))
		instances.push_back(obj);


	if(id2 != ObjectInstanceID() && id2 != id1)
	{
		if(auto obj = cb->getObj(id2))
			instances.push_back(obj);
	}

	garrisonsChanged(instances);
}

void CPlayerInterface::garrisonsChanged(std::vector<const CGObjectInstance *> objs)
{
	boost::unique_lock<boost::recursive_mutex> un(*pim);
	for (auto object : objs)
	{
		auto * hero = dynamic_cast<const CGHeroInstance*>(object);
		auto * town = dynamic_cast<const CGTownInstance*>(object);

		if (hero)
			adventureInt->onHeroChanged(hero);
		if (town)
			adventureInt->onTownChanged(town);
	}

	for (auto cgh : GH.windows().findWindows<CGarrisonHolder>())
		cgh->updateGarrisons();

	for (auto cmw : GH.windows().findWindows<CTradeWindow>())
	{
		if (vstd::contains(objs, cmw->hero))
			cmw->garrisonChanged();
	}

	GH.windows().totalRedraw();
}

void CPlayerInterface::buildChanged(const CGTownInstance *town, BuildingID buildingID, int what) //what: 1 - built, 2 - demolished
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (castleInt)
	{
		castleInt->townlist->update(town);

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
	}
	adventureInt->onTownChanged(town);
}

void CPlayerInterface::battleStartBefore(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2)
{
	//Don't wait for dialogs when we are non-active hot-seat player
	if (LOCPLINT == this)
		waitForAllDialogs();
}

void CPlayerInterface::battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	bool autoBattleResultRefused = (lastBattleArmies.first == army1 && lastBattleArmies.second == army2);
	lastBattleArmies.first = army1;
	lastBattleArmies.second = army2;
	//quick combat with neutral creatures only
	auto * army2_object = dynamic_cast<const CGObjectInstance *>(army2);
	if((!autoBattleResultRefused && !allowBattleReplay && army2_object
		&& army2_object->getOwner() == PlayerColor::UNFLAGGABLE
		&& settings["adventure"]["quickCombat"].Bool())
		|| settings["adventure"]["alwaysSkipCombat"].Bool())
	{
		autofightingAI = CDynLibHandler::getNewBattleAI(settings["server"]["friendlyAI"].String());
		autofightingAI->initBattleInterface(env, cb);
		autofightingAI->battleStart(army1, army2, int3(0,0,0), hero1, hero2, side);
		isAutoFightOn = true;
		cb->registerBattleInterface(autofightingAI);
		// Player shouldn't be able to move on adventure map if quick combat is going
		allowBattleReplay = true;
	}

	//Don't wait for dialogs when we are non-active hot-seat player
	if (LOCPLINT == this)
		waitForAllDialogs();

	BATTLE_EVENT_POSSIBLE_RETURN;
}

void CPlayerInterface::battleUnitsChanged(const std::vector<UnitChanges> & units)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	for(auto & info : units)
	{
		switch(info.operation)
		{
		case UnitChanges::EOperation::RESET_STATE:
			{
				const CStack * stack = cb->battleGetStackByID(info.id );

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
				const CStack * unit = cb->battleGetStackByID(info.id);
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

void CPlayerInterface::battleObstaclesChanged(const std::vector<ObstacleChanges> & obstacles)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	std::vector<std::shared_ptr<const CObstacleInstance>> newObstacles;
	std::vector<ObstacleChanges> removedObstacles;

	for(auto & change : obstacles)
	{
		if(change.operation == BattleChanges::EOperation::ADD)
		{
			auto instance = cb->battleGetObstacleByID(change.id);
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

void CPlayerInterface::battleCatapultAttacked(const CatapultAttack & ca)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->stackIsCatapulting(ca);
}

void CPlayerInterface::battleNewRound(int round) //called at the beginning of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->newRound(round);
}

void CPlayerInterface::actionStarted(const BattleAction &action)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	curAction = new BattleAction(action);
	battleInt->startAction(curAction);
}

void CPlayerInterface::actionFinished(const BattleAction &action)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->endAction(curAction);
	delete curAction;
	curAction = nullptr;
}

BattleAction CPlayerInterface::activeStack(const CStack * stack) //called when it's turn of that stack
{
	THREAD_CREATED_BY_CLIENT;
	logGlobal->trace("Awaiting command for %s", stack->nodeName());
	auto stackId = stack->unitId();
	auto stackName = stack->nodeName();
	if (autofightingAI)
	{
		if (isAutoFightOn)
		{
			auto ret = autofightingAI->activeStack(stack);

			if(cb->battleIsFinished())
			{
				return BattleAction::makeDefend(stack); // battle finished with spellcast
			}

			if (isAutoFightOn)
			{
				return ret;
			}
		}

		cb->unregisterBattleInterface(autofightingAI);
		autofightingAI.reset();
	}

	assert(battleInt);

	if(!battleInt)
	{
		return BattleAction::makeDefend(stack); // probably battle is finished already
	}

	if(BattleInterface::givenCommand.get())
	{
		logGlobal->error("Command buffer must be clean! (we don't want to use old command)");
		vstd::clear_pointer(BattleInterface::givenCommand.data);
	}

	{
		boost::unique_lock<boost::recursive_mutex> un(*pim);
		battleInt->stackActivated(stack);
		//Regeneration & mana drain go there
	}
	//wait till BattleInterface sets its command
	boost::unique_lock<boost::mutex> lock(BattleInterface::givenCommand.mx);
	while(!BattleInterface::givenCommand.data)
	{
		BattleInterface::givenCommand.cond.wait(lock);
		if (!battleInt) //battle ended while we were waiting for movement (eg. because of spell)
			throw boost::thread_interrupted(); //will shut the thread peacefully
	}

	//tidy up
	BattleAction ret = *(BattleInterface::givenCommand.data);
	vstd::clear_pointer(BattleInterface::givenCommand.data);

	if(ret.actionType == EActionType::CANCEL)
	{
		if(stackId != ret.stackNumber)
			logGlobal->error("Not current active stack action canceled");
		logGlobal->trace("Canceled command for %s", stackName);
	}
	else
		logGlobal->trace("Giving command for %s", stackName);

	return ret;
}

void CPlayerInterface::battleEnd(const BattleResult *br, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if(isAutoFightOn || autofightingAI)
	{
		isAutoFightOn = false;
		cb->unregisterBattleInterface(autofightingAI);
		autofightingAI.reset();

		if(!battleInt)
		{
			bool allowManualReplay = allowBattleReplay && !settings["adventure"]["alwaysSkipCombat"].Bool();
			allowBattleReplay = false;
			auto wnd = std::make_shared<BattleResultWindow>(*br, *this, allowManualReplay);
			wnd->resultCallback = [=](ui32 selection)
			{
				cb->selectionMade(selection, queryID);
			};
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

void CPlayerInterface::battleLogMessage(const std::vector<MetaString> & lines)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->displayBattleLog(lines);
}

void CPlayerInterface::battleStackMoved(const CStack * stack, std::vector<BattleHex> dest, int distance, bool teleport)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->stackMoved(stack, dest, distance, teleport);
}
void CPlayerInterface::battleSpellCast( const BattleSpellCast *sc )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->spellCast(sc);
}
void CPlayerInterface::battleStacksEffectsSet( const SetStackEffect & sse )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->battleStacksEffectsSet(sse);
}
void CPlayerInterface::battleTriggerEffect (const BattleTriggerEffect & bte)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	RETURN_IF_QUICK_COMBAT;
	battleInt->effectsController->battleTriggerEffect(bte);
}
void CPlayerInterface::battleStacksAttacked(const std::vector<BattleStackAttacked> & bsa, bool ranged)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	std::vector<StackAttackedInfo> arg;
	for(auto & elem : bsa)
	{
		const CStack * defender = cb->battleGetStackByID(elem.stackAttacked, false);
		const CStack * attacker = cb->battleGetStackByID(elem.attackerID, false);

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
void CPlayerInterface::battleAttack(const BattleAttack * ba)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	assert(curAction);

	StackAttackInfo info;
	info.attacker = cb->battleGetStackByID(ba->stackAttacking);
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
			info.defender = cb->battleGetStackByID(elem.stackAttacked);
		}
		else
		{
			info.secondaryDefender.push_back(cb->battleGetStackByID(elem.stackAttacked));
		}
	}
	assert(info.defender != nullptr);
	assert(info.attacker != nullptr);

	battleInt->stackAttacking(info);
}

void CPlayerInterface::battleGateStateChanged(const EGateState state)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->gateStateChanged(state);
}

void CPlayerInterface::yourTacticPhase(int distance)
{
	THREAD_CREATED_BY_CLIENT;
	while(battleInt && battleInt->tacticsMode)
		boost::this_thread::sleep(boost::posix_time::millisec(1));
}

void CPlayerInterface::forceEndTacticPhase()
{
	if (battleInt)
		battleInt->tacticsMode = false;
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
		stopMovement(); // interrupt movement to show dialog
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
	boost::unique_lock<boost::recursive_mutex> un(*pim);

	stopMovement();
	LOCPLINT->showingDialog->setn(true);
	CInfoWindow::showYesNoDialog(text, components, onYes, onNo, playerID);
}

void CPlayerInterface::showBlockingDialog( const std::string &text, const std::vector<Component> &components, QueryID askID, const int soundID, bool selection, bool cancel )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();

	stopMovement();
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

		std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
		pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
		if (cancel)
		{
			pom.push_back(std::pair<std::string,CFunctionList<void()> >("ICANCEL.DEF",0));
		}

		int charperline = 35;
		if (pom.size() > 1)
			charperline = 50;
		GH.windows().createAndPushWindow<CSelWindow>(text, playerID, charperline, intComps, pom, askID);
		intComps[0]->clickLeft(true, false);
	}
}

void CPlayerInterface::showTeleportDialog(TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	int choosenExit = -1;
	auto neededExit = std::make_pair(destinationTeleport, destinationTeleportPos);
	if (destinationTeleport != ObjectInstanceID() && vstd::contains(exits, neededExit))
		choosenExit = vstd::find_pos(exits, neededExit);

	cb->selectionMade(choosenExit, askID);
}

void CPlayerInterface::showMapObjectSelectDialog(QueryID askID, const Component & icon, const MetaString & title, const MetaString & description, const std::vector<ObjectInstanceID> & objects)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	auto selectCallback = [=](int selection)
	{
		JsonNode reply(JsonNode::JsonType::DATA_INTEGER);
		reply.Integer() = selection;
		cb->sendQueryReply(reply, askID);
	};

	auto cancelCallback = [=]()
	{
		JsonNode reply(JsonNode::JsonType::DATA_NULL);
		cb->sendQueryReply(reply, askID);
	};

	const std::string localTitle = title.toString();
	const std::string localDescription = description.toString();

	std::vector<int> tempList;
	tempList.reserve(objects.size());

	for(auto item : objects)
		tempList.push_back(item.getNum());

	CComponent localIconC(icon);

	std::shared_ptr<CIntObject> localIcon = localIconC.image;
	localIconC.removeChild(localIcon.get(), false);

	std::shared_ptr<CObjectListWindow> wnd = std::make_shared<CObjectListWindow>(tempList, localIcon, localTitle, localDescription, selectCallback);
	wnd->onExit = cancelCallback;
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
	boost::unique_lock<boost::recursive_mutex> un(*pim);
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

void CPlayerInterface::saveGame( BinarySerializer & h, const int version )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	localState->serialize(h, version);
}

void CPlayerInterface::loadGame( BinaryDeserializer & h, const int version )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	localState->serialize(h, version);
	firstCall = -1;
}

void CPlayerInterface::moveHero( const CGHeroInstance *h, const CGPath& path )
{
	LOG_TRACE(logGlobal);
	if (!LOCPLINT->makingTurn)
		return;
	if (!h)
		return; //can't find hero

	//It shouldn't be possible to move hero with open dialog (or dialog waiting in bg)
	if (showingDialog->get() || !dialogs.empty())
		return;

	setMovementStatus(true);

	if (localState->isHeroSleeping(h))
		localState->setHeroAwaken(h);

	boost::thread moveHeroTask(std::bind(&CPlayerInterface::doMoveHero,this,h,path));
}

void CPlayerInterface::showGarrisonDialog( const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, QueryID queryID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto onEnd = [=](){ cb->selectionMade(0, queryID); };

	if (stillMoveHero.get() == DURING_MOVE  && localState->hasPath(down) && localState->getPath(down).nodes.size() > 1) //to ignore calls on passing through garrisons
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
void CPlayerInterface::showArtifactAssemblyDialog(const Artifact * artifact, const Artifact * assembledArtifact, CFunctionList<bool()> onYes)
{
	std::string text = artifact->getDescriptionTranslated();
	text += "\n\n";
	std::vector<std::shared_ptr<CComponent>> scs;

	if(assembledArtifact)
	{
		// You possess all of the components to...
		text += boost::str(boost::format(CGI->generaltexth->allTexts[732]) % assembledArtifact->getNameTranslated());

		// Picture of assembled artifact at bottom.
		auto sc = std::make_shared<CComponent>(CComponent::artifact, assembledArtifact->getIndex(), 0);
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
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if (pa->packType == typeList.getTypeID<MoveHero>()  &&  stillMoveHero.get() == DURING_MOVE
	   && destinationTeleport == ObjectInstanceID())
		stillMoveHero.setn(CONTINUE_MOVE);

	if (destinationTeleport != ObjectInstanceID()
	   && pa->packType == typeList.getTypeID<QueryReply>()
	   && stillMoveHero.get() == DURING_MOVE)
	{ // After teleportation via CGTeleport object is finished
		destinationTeleport = ObjectInstanceID();
		destinationTeleportPos = int3(-1);
		stillMoveHero.setn(CONTINUE_MOVE);
	}
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

void CPlayerInterface::showRecruitmentDialog(const CGDwelling *dwelling, const CArmedInstance *dst, int level)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	waitWhileDialog();
	auto recruitCb = [=](CreatureID id, int count)
	{
		LOCPLINT->cb->recruitCreatures(dwelling, dst, id, count, -1);
	};
	GH.windows().createAndPushWindow<CRecruitmentWindow>(dwelling, level, dst, recruitCb);
}

void CPlayerInterface::waitWhileDialog(bool unlockPim)
{
	if (GH.amIGuiThread())
	{
		logGlobal->warn("Cannot wait for dialogs in gui thread (deadlock risk)!");
		return;
	}

	auto unlock = vstd::makeUnlockGuardIf(*pim, unlockPim);
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
			auto unlockPim = vstd::makeUnlockGuard(*pim);
			IgnoreEvents ignore(*this);
			boost::this_thread::sleep(boost::posix_time::milliseconds(focusTime));
		}
	}
	CCS->curh->show();
}

void CPlayerInterface::objectRemoved(const CGObjectInstance * obj)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	if(LOCPLINT->cb->getCurrentPlayer() == playerID && obj->getRemovalSound())
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
			cmp.push_back(std::make_shared<CComponent>(CComponent::flag, playerID.getNum(), 0));
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
				GH.terminate_cond->setn(true);
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
	else
	{
		if (victoryLossCheckResult.loss() && cb->getPlayerStatus(playerID) == EPlayerStatus::INGAME) //enemy has lost
		{
			MetaString message = victoryLossCheckResult.messageToSelf;
			message.appendLocalString(EMetaText::COLOR, player.getNum());
			showInfoDialog(message.toString(), std::vector<std::shared_ptr<CComponent>>(1, std::make_shared<CComponent>(CComponent::flag, player.getNum(), 0)));
		}
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

void CPlayerInterface::advmapSpellCast(const CGHeroInstance * caster, int spellID)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	if(GH.windows().topWindow<CSpellWindow>())
		GH.windows().popWindows(1);

	if(spellID == SpellID::FLY || spellID == SpellID::WATER_WALK)
		localState->erasePath(caster);

	const spells::Spell * spell = CGI->spells()->getByIndex(spellID);
	auto castSoundPath = spell->getCastSound();
	if(!castSoundPath.empty())
		CCS->soundh->playSound(castSoundPath);
}

void CPlayerInterface::tryDiggging(const CGHeroInstance * h)
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

void CPlayerInterface::battleNewRoundFirst( int round )
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	BATTLE_EVENT_POSSIBLE_RETURN;

	battleInt->newRoundFirst(round);
}

void CPlayerInterface::stopMovement()
{
	if (stillMoveHero.get() == DURING_MOVE)//if we are in the middle of hero movement
		stillMoveHero.setn(STOP_MOVE); //after showing dialog movement will be stopped
}

void CPlayerInterface::showMarketWindow(const IMarket *market, const CGHeroInstance *visitor)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;

	if(market->allowsTrade(EMarketMode::ARTIFACT_EXP) && visitor->getAlignment() != EAlignment::EVIL)
		GH.windows().createAndPushWindow<CAltarWindow>(market, visitor, EMarketMode::ARTIFACT_EXP);
	else if(market->allowsTrade(EMarketMode::CREATURE_EXP) && visitor->getAlignment() != EAlignment::GOOD)
		GH.windows().createAndPushWindow<CAltarWindow>(market, visitor, EMarketMode::CREATURE_EXP);
	else if(market->allowsTrade(EMarketMode::CREATURE_UNDEAD))
		GH.windows().createAndPushWindow<CTransformerWindow>(market, visitor);
	else if(!market->availableModes().empty())
		GH.windows().createAndPushWindow<CMarketplaceWindow>(market, visitor, market->availableModes().front());
}

void CPlayerInterface::showUniversityWindow(const IMarket *market, const CGHeroInstance *visitor)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	GH.windows().createAndPushWindow<CUniversityWindow>(visitor, market);
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

void CPlayerInterface::showTavernWindow(const CGObjectInstance *townOrTavern)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	GH.windows().createAndPushWindow<CTavernWindow>(townOrTavern);
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
	if(won && cb->getStartInfo()->campState)
		CSH->startCampaignScenario(cb->getStartInfo()->campState);
	else
	{
		GH.dispatchMainThread(
			[]()
			{
				CSH->endGameplay();
				GH.defActionsDef = 63;
				CMM->menu->switchToTab("main");
			}
		);
	}
}

void CPlayerInterface::askToAssembleArtifact(const ArtifactLocation &al)
{
	auto hero = std::visit(HeroObjectRetriever(), al.artHolder);
	if(hero)
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
	auto hero = std::visit(HeroObjectRetriever(), al.artHolder);
	adventureInt->onHeroChanged(hero);
}

void CPlayerInterface::artifactRemoved(const ArtifactLocation &al)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto hero = std::visit(HeroObjectRetriever(), al.artHolder);
	adventureInt->onHeroChanged(hero);

	for(auto artWin : GH.windows().findWindows<CArtifactHolder>())
		artWin->artifactRemoved(al);

	waitWhileDialog();
}

void CPlayerInterface::artifactMoved(const ArtifactLocation &src, const ArtifactLocation &dst)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto hero = std::visit(HeroObjectRetriever(), dst.artHolder);
	adventureInt->onHeroChanged(hero);

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
	auto hero = std::visit(HeroObjectRetriever(), al.artHolder);
	adventureInt->onHeroChanged(hero);

	for(auto artWin : GH.windows().findWindows<CArtifactHolder>())
		artWin->artifactAssembled(al);
}

void CPlayerInterface::artifactDisassembled(const ArtifactLocation &al)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	auto hero = std::visit(HeroObjectRetriever(), al.artHolder);
	adventureInt->onHeroChanged(hero);

	for(auto artWin : GH.windows().findWindows<CArtifactHolder>())
		artWin->artifactDisassembled(al);
}

void CPlayerInterface::waitForAllDialogs(bool unlockPim)
{
	while(!dialogs.empty())
	{
		auto unlock = vstd::makeUnlockGuardIf(*pim, unlockPim);
		boost::this_thread::sleep(boost::posix_time::milliseconds(5));
	}
	waitWhileDialog(unlockPim);
}

void CPlayerInterface::proposeLoadingGame()
{
	showYesNoDialog(
		CGI->generaltexth->allTexts[68],
		[]()
		{
			GH.dispatchMainThread(
				[]()
				{
					CSH->endGameplay();
					GH.defActionsDef = 63;
					CMM->menu->switchToTab("load");
				}
			);
		},
		nullptr
	);
}

bool CPlayerInterface::capturedAllEvents()
{
	if(duringMovement)
	{
		//just inform that we are capturing events. they will be processed by heroMoved() in client thread.
		return true;
	}

	bool needToLockAdventureMap = adventureInt && adventureInt->isActive() && CGI->mh->hasOngoingAnimations();

	if (ignoreEvents || needToLockAdventureMap || isAutoFightOn)
	{
		GH.input().ignoreEventsUntilInput();
		return true;
	}

	return false;
}

void CPlayerInterface::setMovementStatus(bool value)
{
	duringMovement = value;
	if (value)
	{
		CCS->curh->hide();
	}
	else
	{
		CCS->curh->show();
	}
}

void CPlayerInterface::doMoveHero(const CGHeroInstance * h, CGPath path)
{
	int i = 1;
	auto getObj = [&](int3 coord, bool ignoreHero)
	{
		return cb->getTile(h->convertToVisitablePos(coord))->topVisitableObj(ignoreHero);
	};

	auto isTeleportAction = [&](EPathNodeAction action) -> bool
	{
		if (action != EPathNodeAction::TELEPORT_NORMAL &&
			action != EPathNodeAction::TELEPORT_BLOCKING_VISIT &&
			action != EPathNodeAction::TELEPORT_BATTLE)
		{
			return false;
		}

		return true;
	};

	auto getDestTeleportObj = [&](const CGObjectInstance * currentObject, const CGObjectInstance * nextObjectTop, const CGObjectInstance * nextObject) -> const CGObjectInstance *
	{
		if (CGTeleport::isConnected(currentObject, nextObjectTop))
			return nextObjectTop;
		if (nextObjectTop && nextObjectTop->ID == Obj::HERO &&
			CGTeleport::isConnected(currentObject, nextObject))
		{
			return nextObject;
		}

		return nullptr;
	};

	boost::unique_lock<boost::mutex> un(stillMoveHero.mx);
	stillMoveHero.data = CONTINUE_MOVE;
	auto doMovement = [&](int3 dst, bool transit)
	{
		stillMoveHero.data = WAITING_MOVE;
		cb->moveHero(h, dst, transit);
		while(stillMoveHero.data != STOP_MOVE && stillMoveHero.data != CONTINUE_MOVE)
			stillMoveHero.cond.wait(un);
	};

	{
		for (auto & elem : path.nodes)
			elem.coord = h->convertFromVisitablePos(elem.coord);

		int soundChannel = -1;
		std::string soundName;

		auto getMovementSoundFor = [&](const CGHeroInstance * hero, int3 posPrev, int3 posNext) -> std::string
		{
			// flying movement sound
			if (hero->hasBonusOfType(BonusType::FLYING_MOVEMENT))
				return "HORSE10.wav";

			auto prevTile = cb->getTile(h->convertToVisitablePos(posPrev));
			auto nextTile = cb->getTile(h->convertToVisitablePos(posNext));

			auto prevRoad = prevTile->roadType;
			auto nextRoad = nextTile->roadType;
			bool movingOnRoad = prevRoad->getId() != Road::NO_ROAD && nextRoad->getId() != Road::NO_ROAD;

			if (movingOnRoad)
				return nextTile->terType->horseSound;
			else
				return nextTile->terType->horseSoundPenalty;
		};

		auto canStop = [&](CGPathNode * node) -> bool
		{
			if (node->layer == EPathfindingLayer::LAND || node->layer == EPathfindingLayer::SAIL)
				return true;

			if (node->accessible == EPathAccessibility::ACCESSIBLE)
				return true;

			return false;
		};

		for (i=(int)path.nodes.size()-1; i>0 && (stillMoveHero.data == CONTINUE_MOVE || !canStop(&path.nodes[i])); i--)
		{
			int3 prevCoord = path.nodes[i].coord;
			int3 nextCoord = path.nodes[i-1].coord;

			auto prevObject = getObj(prevCoord, prevCoord == h->pos);
			auto nextObjectTop = getObj(nextCoord, false);
			auto nextObject = getObj(nextCoord, true);
			auto destTeleportObj = getDestTeleportObj(prevObject, nextObjectTop, nextObject);
			if (isTeleportAction(path.nodes[i-1].action) && destTeleportObj != nullptr)
			{
				CCS->soundh->stopSound(soundChannel);
				destinationTeleport = destTeleportObj->id;
				destinationTeleportPos = nextCoord;
				doMovement(h->pos, false);
				if (path.nodes[i-1].action == EPathNodeAction::TELEPORT_BLOCKING_VISIT
					|| path.nodes[i-1].action == EPathNodeAction::TELEPORT_BATTLE)
				{
					destinationTeleport = ObjectInstanceID();
					destinationTeleportPos = int3(-1);
				}
				if(i != path.nodes.size() - 1)
				{
					soundName = getMovementSoundFor(h, prevCoord, nextCoord);
					soundChannel = CCS->soundh->playSound(soundName, -1);
				}
				continue;
			}

			if (path.nodes[i-1].turns)
			{ //stop sending move requests if the next node can't be reached at the current turn (hero exhausted his move points)
				stillMoveHero.data = STOP_MOVE;
				break;
			}

			{
				// Start a new sound for the hero movement or let the existing one carry on.
				std::string newSoundName = getMovementSoundFor(h, prevCoord, nextCoord);

				if(newSoundName != soundName)
				{
					soundName = newSoundName;

					CCS->soundh->stopSound(soundChannel);
					soundChannel = CCS->soundh->playSound(soundName, -1);
				}
			}

			assert(h->pos.z == nextCoord.z); // Z should change only if it's movement via teleporter and in this case this code shouldn't be executed at all
			int3 endpos(nextCoord.x, nextCoord.y, h->pos.z);
			logGlobal->trace("Requesting hero movement to %s", endpos.toString());

			bool useTransit = false;
			if ((i-2 >= 0) // Check there is node after next one; otherwise transit is pointless
				&& (CGTeleport::isConnected(nextObjectTop, getObj(path.nodes[i-2].coord, false))
					|| CGTeleport::isTeleport(nextObjectTop)))
			{ // Hero should be able to go through object if it's allow transit
				useTransit = true;
			}
			else if (path.nodes[i-1].layer == EPathfindingLayer::AIR)
				useTransit = true;

			doMovement(endpos, useTransit);

			logGlobal->trace("Resuming %s", __FUNCTION__);
			bool guarded = cb->isInTheMap(cb->getGuardingCreaturePosition(endpos - int3(1, 0, 0)));
			if ((!useTransit && guarded) || showingDialog->get() == true) // Abort movement if a guard was fought or there is a dialog to display (Mantis #1136)
				break;
		}

		CCS->soundh->stopSound(soundChannel);
	}

	//Update cursor so icon can change if needed when it reappears; doesn;'t apply if a dialog box pops up at the end of the movement
	if (!showingDialog->get())
		GH.fakeMouseMove();

	CGI->mh->waitForOngoingAnimations();
	setMovementStatus(false);
}

void CPlayerInterface::showWorldViewEx(const std::vector<ObjectPosInfo>& objectPositions, bool showTerrain)
{
	EVENT_HANDLER_CALLED_BY_CLIENT;
	adventureInt->openWorldView(objectPositions, showTerrain );
}
