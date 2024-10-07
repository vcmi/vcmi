/*
 * Client.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "Global.h"
#include "Client.h"

#include "CGameInfo.h"
#include "CPlayerInterface.h"
#include "PlayerLocalState.h"
#include "CServerHandler.h"
#include "ClientNetPackVisitors.h"
#include "adventureMap/AdventureMapInterface.h"
#include "battle/BattleInterface.h"
#include "gui/CGuiHandler.h"
#include "gui/WindowHandler.h"
#include "mapView/mapHandler.h"

#include "../CCallback.h"
#include "../lib/CConfigHandler.h"
#include "../lib/gameState/CGameState.h"
#include "../lib/CPlayerState.h"
#include "../lib/CThreadHelper.h"
#include "../lib/VCMIDirs.h"
#include "../lib/UnlockGuard.h"
#include "../lib/battle/BattleInfo.h"
#include "../lib/serializer/Connection.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/pathfinder/CGPathNode.h"
#include "../lib/filesystem/Filesystem.h"

#include <memory>
#include <vcmi/events/EventBus.h>

#if SCRIPTING_ENABLED
#include "../lib/ScriptHandler.h"
#endif

#ifdef VCMI_ANDROID
#include "lib/CAndroidVMHelper.h"
#endif

ThreadSafeVector<int> CClient::waitingRequest;

CPlayerEnvironment::CPlayerEnvironment(PlayerColor player_, CClient * cl_, std::shared_ptr<CCallback> mainCallback_)
	: player(player_),
	cl(cl_),
	mainCallback(mainCallback_)
{

}

const Services * CPlayerEnvironment::services() const
{
	return VLC;
}

vstd::CLoggerBase * CPlayerEnvironment::logger() const
{
	return logGlobal;
}

events::EventBus * CPlayerEnvironment::eventBus() const
{
	return cl->eventBus();//always get actual value
}

const CPlayerEnvironment::BattleCb * CPlayerEnvironment::battle(const BattleID & battleID) const
{
	return mainCallback->getBattle(battleID).get();
}

const CPlayerEnvironment::GameCb * CPlayerEnvironment::game() const
{
	return mainCallback.get();
}

CClient::CClient()
{
	waitingRequest.clear();
	gs = nullptr;
}

CClient::~CClient() = default;

const Services * CClient::services() const
{
	return VLC; //todo: this should be CGI
}

const CClient::BattleCb * CClient::battle(const BattleID & battleID) const
{
	return nullptr; //todo?
}

const CClient::GameCb * CClient::game() const
{
	return this;
}

vstd::CLoggerBase * CClient::logger() const
{
	return logGlobal;
}

events::EventBus * CClient::eventBus() const
{
	return clientEventBus.get();
}

void CClient::newGame(CGameState * initializedGameState)
{
	CSH->th->update();
	CMapService mapService;
	assert(initializedGameState);
	gs = initializedGameState;
	gs->preInit(VLC, this);
	logNetwork->trace("\tCreating gamestate: %i", CSH->th->getDiff());
	if(!initializedGameState)
	{
		Load::ProgressAccumulator progressTracking;
		gs->init(&mapService, CSH->si.get(), progressTracking, settings["general"]["saveRandomMaps"].Bool());
	}
	logNetwork->trace("Initializing GameState (together): %d ms", CSH->th->getDiff());

	initMapHandler();
	reinitScripting();
	initPlayerEnvironments();
	initPlayerInterfaces();
}

void CClient::loadGame(CGameState * initializedGameState)
{
	logNetwork->info("Loading procedure started!");

	logNetwork->info("Game state was transferred over network, loading.");
	gs = initializedGameState;

	gs->preInit(VLC, this);
	gs->updateOnLoad(CSH->si.get());
	logNetwork->info("Game loaded, initialize interfaces.");

	initMapHandler();

	reinitScripting();

	initPlayerEnvironments();
	initPlayerInterfaces();
}

void CClient::save(const std::string & fname)
{
	if(!gs->currentBattles.empty())
	{
		logNetwork->error("Game cannot be saved during battle!");
		return;
	}

	SaveGame save_game(fname);
	sendRequest(save_game, PlayerColor::NEUTRAL);
}

void CClient::endNetwork()
{
	if (CGI->mh)
		CGI->mh->endNetwork();

	if (CPlayerInterface::battleInt)
		CPlayerInterface::battleInt->endNetwork();

	for(auto & i : playerint)
	{
		auto interface = std::dynamic_pointer_cast<CPlayerInterface>(i.second);
		if (interface)
			interface->endNetwork();
	}
}

void CClient::endGame()
{
#if SCRIPTING_ENABLED
	clientScripts.reset();
#endif

	//suggest interfaces to finish their stuff (AI should interrupt any bg working threads)
	for(auto & i : playerint)
		i.second->finish();

	GH.curInt = nullptr;
	{
		logNetwork->info("Ending current game!");
		removeGUI();

		CGI->mh.reset();
		vstd::clear_pointer(gs);

		logNetwork->info("Deleted mapHandler and gameState.");
	}

	CPlayerInterface::battleInt.reset();
	playerint.clear();
	battleints.clear();
	battleCallbacks.clear();
	playerEnvironments.clear();
	logNetwork->info("Deleted playerInts.");
	logNetwork->info("Client stopped.");
}

void CClient::initMapHandler()
{
	// TODO: CMapHandler initialization can probably go somewhere else
	// It's can't be before initialization of interfaces
	// During loading CPlayerInterface from serialized state it's depend on MH
	if(!settings["session"]["headless"].Bool())
	{
		CGI->mh = std::make_shared<CMapHandler>(gs->map);
		logNetwork->trace("Creating mapHandler: %d ms", CSH->th->getDiff());
	}

	pathCache.clear();
}

void CClient::initPlayerEnvironments()
{
	playerEnvironments.clear();

	auto allPlayers = CSH->getAllClientPlayers(CSH->logicConnection->connectionID);
	bool hasHumanPlayer = false;
	for(auto & color : allPlayers)
	{
		logNetwork->info("Preparing environment for player %s", color.toString());
		playerEnvironments[color] = std::make_shared<CPlayerEnvironment>(color, this, std::make_shared<CCallback>(gs, color, this));
		
		if(!hasHumanPlayer && gs->players[color].isHuman())
			hasHumanPlayer = true;
	}

	if(!hasHumanPlayer && !settings["session"]["headless"].Bool())
	{
		Settings session = settings.write["session"];
		session["spectate"].Bool() = true;
		session["spectate-skip-battle-result"].Bool() = true;
		session["spectate-ignore-hero"].Bool() = true;
	}
	
	if(settings["session"]["spectate"].Bool())
	{
		playerEnvironments[PlayerColor::SPECTATOR] = std::make_shared<CPlayerEnvironment>(PlayerColor::SPECTATOR, this, std::make_shared<CCallback>(gs, std::nullopt, this));
	}
}

void CClient::initPlayerInterfaces()
{
	for(auto & playerInfo : gs->scenarioOps->playerInfos)
	{
		PlayerColor color = playerInfo.first;
		if(!vstd::contains(CSH->getAllClientPlayers(CSH->logicConnection->connectionID), color))
			continue;

		if(!vstd::contains(playerint, color))
		{
			logNetwork->info("Preparing interface for player %s", color.toString());
			if(playerInfo.second.isControlledByAI() || settings["session"]["onlyai"].Bool())
			{
				bool alliedToHuman = false;
				for(auto & allyInfo : gs->scenarioOps->playerInfos)
					if (gs->getPlayerTeam(allyInfo.first) == gs->getPlayerTeam(playerInfo.first) && allyInfo.second.isControlledByHuman())
						alliedToHuman = true;

				auto AiToGive = aiNameForPlayer(playerInfo.second, false, alliedToHuman);
				logNetwork->info("Player %s will be lead by %s", color.toString(), AiToGive);
				installNewPlayerInterface(CDynLibHandler::getNewAI(AiToGive), color);
			}
			else
			{
				logNetwork->info("Player %s will be lead by human", color.toString());
				installNewPlayerInterface(std::make_shared<CPlayerInterface>(color), color);
			}
		}
	}

	if(settings["session"]["spectate"].Bool())
	{
		installNewPlayerInterface(std::make_shared<CPlayerInterface>(PlayerColor::SPECTATOR), PlayerColor::SPECTATOR, true);
	}

	if(CSH->getAllClientPlayers(CSH->logicConnection->connectionID).count(PlayerColor::NEUTRAL))
		installNewBattleInterface(CDynLibHandler::getNewBattleAI(settings["server"]["neutralAI"].String()), PlayerColor::NEUTRAL);

	logNetwork->trace("Initialized player interfaces %d ms", CSH->th->getDiff());
}

std::string CClient::aiNameForPlayer(const PlayerSettings & ps, bool battleAI, bool alliedToHuman) const
{
	if(ps.name.size())
	{
		const boost::filesystem::path aiPath = VCMIDirs::get().fullLibraryPath("AI", ps.name);
		if(boost::filesystem::exists(aiPath))
			return ps.name;
	}

	return aiNameForPlayer(battleAI, alliedToHuman);
}

std::string CClient::aiNameForPlayer(bool battleAI, bool alliedToHuman) const
{
	const int sensibleAILimit = settings["session"]["oneGoodAI"].Bool() ? 1 : PlayerColor::PLAYER_LIMIT_I;
	std::string goodAdventureAI = alliedToHuman ? settings["server"]["alliedAI"].String() : settings["server"]["playerAI"].String();
	std::string goodBattleAI = settings["server"]["neutralAI"].String();
	std::string goodAI = battleAI ? goodBattleAI : goodAdventureAI;
	std::string badAI = battleAI ? "StupidAI" : "EmptyAI";

	//TODO what about human players
	if(battleints.size() >= sensibleAILimit)
		return badAI;

	return goodAI;
}

void CClient::installNewPlayerInterface(std::shared_ptr<CGameInterface> gameInterface, PlayerColor color, bool battlecb)
{
	playerint[color] = gameInterface;

	logGlobal->trace("\tInitializing the interface for player %s", color.toString());
	auto cb = std::make_shared<CCallback>(gs, color, this);
	battleCallbacks[color] = cb;
	gameInterface->initGameInterface(playerEnvironments.at(color), cb);

	installNewBattleInterface(gameInterface, color, battlecb);
}

void CClient::installNewBattleInterface(std::shared_ptr<CBattleGameInterface> battleInterface, PlayerColor color, bool needCallback)
{
	battleints[color] = battleInterface;

	if(needCallback)
	{
		logGlobal->trace("\tInitializing the battle interface for player %s", color.toString());
		auto cbc = std::make_shared<CBattleCallback>(color, this);
		battleCallbacks[color] = cbc;
		battleInterface->initBattleInterface(playerEnvironments.at(color), cbc);
	}
}

void CClient::handlePack(CPackForClient & pack)
{
	ApplyClientNetPackVisitor afterVisitor(*this, *gameState());
	ApplyFirstClientNetPackVisitor beforeVisitor(*this, *gameState());

	pack.visit(beforeVisitor);
	logNetwork->trace("\tMade first apply on cl: %s", typeid(pack).name());
	{
		boost::unique_lock lock(CGameState::mutex);
		gs->apply(pack);
	}
	logNetwork->trace("\tApplied on gs: %s", typeid(pack).name());
	pack.visit(afterVisitor);
	logNetwork->trace("\tMade second apply on cl: %s", typeid(pack).name());
}

int CClient::sendRequest(const CPackForServer & request, PlayerColor player)
{
	static ui32 requestCounter = 1;

	ui32 requestID = requestCounter++;
	logNetwork->trace("Sending a request \"%s\". It'll have an ID=%d.", typeid(request).name(), requestID);

	waitingRequest.pushBack(requestID);
	request.requestID = requestID;
	request.player = player;
	CSH->logicConnection->sendPack(request);
	if(vstd::contains(playerint, player))
		playerint[player]->requestSent(&request, requestID);

	return requestID;
}

void CClient::battleStarted(const BattleInfo * info)
{
	std::shared_ptr<CPlayerInterface> att;
	std::shared_ptr<CPlayerInterface> def;
	const auto & leftSide = info->getSide(BattleSide::LEFT_SIDE);
	const auto & rightSide = info->getSide(BattleSide::RIGHT_SIDE);

	for(auto & battleCb : battleCallbacks)
	{
		if(!battleCb.first.isValidPlayer() || battleCb.first == leftSide.color || battleCb.first == rightSide.color)
			battleCb.second->onBattleStarted(info);
	}

	//If quick combat is not, do not prepare interfaces for battleint
	auto callBattleStart = [&](PlayerColor color, BattleSide side)
	{
		if(vstd::contains(battleints, color))
			battleints[color]->battleStart(info->battleID, leftSide.armyObject, rightSide.armyObject, info->tile, leftSide.hero, rightSide.hero, side, info->replayAllowed);
	};
	
	callBattleStart(leftSide.color, BattleSide::LEFT_SIDE);
	callBattleStart(rightSide.color, BattleSide::RIGHT_SIDE);
	callBattleStart(PlayerColor::UNFLAGGABLE, BattleSide::RIGHT_SIDE);
	if(settings["session"]["spectate"].Bool() && !settings["session"]["spectate-skip-battle"].Bool())
		callBattleStart(PlayerColor::SPECTATOR, BattleSide::RIGHT_SIDE);
	
	if(vstd::contains(playerint, leftSide.color) && playerint[leftSide.color]->human)
		att = std::dynamic_pointer_cast<CPlayerInterface>(playerint[leftSide.color]);

	if(vstd::contains(playerint, rightSide.color) && playerint[rightSide.color]->human)
		def = std::dynamic_pointer_cast<CPlayerInterface>(playerint[rightSide.color]);
	
	//Remove player interfaces for auto battle (quickCombat option)
	if((att && att->isAutoFightOn) || (def && def->isAutoFightOn))
	{
		auto endTacticPhaseIfEligible = [info](const CPlayerInterface * interface)
		{
			if (interface->cb->getBattle(info->battleID)->battleGetTacticDist())
			{
				auto side = interface->cb->getBattle(info->battleID)->playerToSide(interface->playerID);

				if(interface->playerID == info->getSide(info->tacticsSide).color)
				{
					auto action = BattleAction::makeEndOFTacticPhase(side);
					interface->cb->battleMakeTacticAction(info->battleID, action);
				}
			}
		};

		if(att && att->isAutoFightOn)
			endTacticPhaseIfEligible(att.get());
		else // def && def->isAutoFightOn
			endTacticPhaseIfEligible(def.get());

		att.reset();
		def.reset();
	}

	if(!settings["session"]["headless"].Bool())
	{
		if(att || def)
		{
			CPlayerInterface::battleInt = std::make_shared<BattleInterface>(info->getBattleID(), leftSide.armyObject, rightSide.armyObject, leftSide.hero, rightSide.hero, att, def);
		}
		else if(settings["session"]["spectate"].Bool() && !settings["session"]["spectate-skip-battle"].Bool())
		{
			//TODO: This certainly need improvement
			auto spectratorInt = std::dynamic_pointer_cast<CPlayerInterface>(playerint[PlayerColor::SPECTATOR]);
			spectratorInt->cb->onBattleStarted(info);
			CPlayerInterface::battleInt = std::make_shared<BattleInterface>(info->getBattleID(), leftSide.armyObject, rightSide.armyObject, leftSide.hero, rightSide.hero, att, def, spectratorInt);
		}
	}

	if(info->tacticDistance)
	{
		auto tacticianColor = info->getSide(info->tacticsSide).color;

		if (vstd::contains(battleints, tacticianColor))
			battleints[tacticianColor]->yourTacticPhase(info->battleID, info->tacticDistance);
	}
}

void CClient::battleFinished(const BattleID & battleID)
{
	for(auto side : { BattleSide::ATTACKER, BattleSide::DEFENDER })
	{
		if(battleCallbacks.count(gs->getBattle(battleID)->getSide(side).color))
			battleCallbacks[gs->getBattle(battleID)->getSide(side).color]->onBattleEnded(battleID);
	}

	if(settings["session"]["spectate"].Bool() && !settings["session"]["spectate-skip-battle"].Bool())
		battleCallbacks[PlayerColor::SPECTATOR]->onBattleEnded(battleID);
}

void CClient::startPlayerBattleAction(const BattleID & battleID, PlayerColor color)
{
	if (battleints.count(color) == 0)
		return; // not our combat in MP

	auto battleint = battleints.at(color);

	if (!battleint->human)
	{
		// we want to avoid locking gamestate and causing UI to freeze while AI is making turn
		auto unlockInterface = vstd::makeUnlockGuard(GH.interfaceMutex);
		battleint->activeStack(battleID, gs->getBattle(battleID)->battleGetStackByID(gs->getBattle(battleID)->activeStack, false));
	}
	else
	{
		battleint->activeStack(battleID, gs->getBattle(battleID)->battleGetStackByID(gs->getBattle(battleID)->activeStack, false));
	}
}

void CClient::updatePath(const ObjectInstanceID & id)
{
	invalidatePaths();
	auto hero = getHero(id);
	updatePath(hero);
}

void CClient::updatePath(const CGHeroInstance * hero)
{
	if(LOCPLINT && hero)
		LOCPLINT->localState->verifyPath(hero);
}

void CClient::invalidatePaths()
{
	boost::unique_lock<boost::mutex> pathLock(pathCacheMutex);
	pathCache.clear();
}

vstd::RNG & CClient::getRandomGenerator()
{
	// Client should use CRandomGenerator::getDefault() for UI logic
	// Gamestate should never call this method on client!
	throw std::runtime_error("Illegal access to random number generator from client code!");
}

std::shared_ptr<const CPathsInfo> CClient::getPathsInfo(const CGHeroInstance * h)
{
	assert(h);
	boost::unique_lock<boost::mutex> pathLock(pathCacheMutex);

	auto iter = pathCache.find(h);

	if(iter == std::end(pathCache))
	{
		auto paths = std::make_shared<CPathsInfo>(getMapSize(), h);

		gs->calculatePaths(h, *paths.get());

		pathCache[h] = paths;
		return paths;
	}
	else
	{
		return iter->second;
	}
}

#if SCRIPTING_ENABLED
scripting::Pool * CClient::getGlobalContextPool() const
{
	return clientScripts.get();
}
#endif

void CClient::reinitScripting()
{
	clientEventBus = std::make_unique<events::EventBus>();
#if SCRIPTING_ENABLED
	clientScripts.reset(new scripting::PoolImpl(this));
#endif
}

void CClient::removeGUI() const
{
	// CClient::endGame
	GH.curInt = nullptr;
	GH.windows().clear();
	adventureInt.reset();
	logGlobal->info("Removed GUI.");

	LOCPLINT = nullptr;
}

#ifdef VCMI_ANDROID
extern "C" JNIEXPORT jboolean JNICALL Java_eu_vcmi_vcmi_NativeMethods_tryToSaveTheGame(JNIEnv * env, jclass cls)
{
	boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);

	logGlobal->info("Received emergency save game request");
	if(!LOCPLINT || !LOCPLINT->cb)
	{
		logGlobal->info("... but no active player interface found!");
		return false;
	}

	if (!CSH || !CSH->logicConnection)
	{
		logGlobal->info("... but no active connection found!");
		return false;
	}

	LOCPLINT->cb->save("Saves/_Android_Autosave");
	return true;
}
#endif
