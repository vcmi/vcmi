/*
 * Client.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "Global.h"
#include "StdInc.h"
#include "Client.h"

#include "CGameInfo.h"
#include "CPlayerInterface.h"
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
#include "../lib/serializer/BinaryDeserializer.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/pathfinder/CGPathNode.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/registerTypes/RegisterTypesClientPacks.h"
#include "../lib/serializer/Connection.h"

#include <memory>
#include <vcmi/events/EventBus.h>

#if SCRIPTING_ENABLED
#include "../lib/ScriptHandler.h"
#endif

#ifdef VCMI_ANDROID
#include "lib/CAndroidVMHelper.h"

#ifndef SINGLE_PROCESS_APP
std::atomic_bool androidTestServerReadyFlag;
#endif
#endif

ThreadSafeVector<int> CClient::waitingRequest;

template<typename T> class CApplyOnCL;

class CBaseForCLApply
{
public:
	virtual void applyOnClAfter(CClient * cl, void * pack) const =0;
	virtual void applyOnClBefore(CClient * cl, void * pack) const =0;
	virtual ~CBaseForCLApply(){}

	template<typename U> static CBaseForCLApply * getApplier(const U * t = nullptr)
	{
		return new CApplyOnCL<U>();
	}
};

template<typename T> class CApplyOnCL : public CBaseForCLApply
{
public:
	void applyOnClAfter(CClient * cl, void * pack) const override
	{
		T * ptr = static_cast<T *>(pack);
		ApplyClientNetPackVisitor visitor(*cl, *cl->gameState());
		ptr->visit(visitor);
	}
	void applyOnClBefore(CClient * cl, void * pack) const override
	{
		T * ptr = static_cast<T *>(pack);
		ApplyFirstClientNetPackVisitor visitor(*cl, *cl->gameState());
		ptr->visit(visitor);
	}
};

template<> class CApplyOnCL<CPack>: public CBaseForCLApply
{
public:
	void applyOnClAfter(CClient * cl, void * pack) const override
	{
		logGlobal->error("Cannot apply on CL plain CPack!");
		assert(0);
	}
	void applyOnClBefore(CClient * cl, void * pack) const override
	{
		logGlobal->error("Cannot apply on CL plain CPack!");
		assert(0);
	}
};

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
	applier = std::make_shared<CApplier<CBaseForCLApply>>();
	registerTypesClientPacks(*applier);
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
	
	// Loading of client state - disabled for now
	// Since client no longer writes or loads its own state and instead receives it from server
	// client state serializer will serialize its own copies of all pointers, e.g. heroes/towns/objects
	// and on deserialize will create its own copies (instead of using copies from state received from server)
	// Potential solutions:
	// 1) Use server gamestate to deserialize pointers, so any pointer to same object will point to server instance and not our copy
	// 2) Remove all serialization of pointers with instance ID's and restore them on load (including AI deserializer code)
	// 3) Completely remove client savegame and send all information, like hero paths and sleeping status to server (either in form of hero properties or as some generic "client options" message
#ifdef BROKEN_CLIENT_STATE_SERIALIZATION_HAS_BEEN_FIXED
	// try to deserialize client data including sleepingHeroes
	try
	{
		boost::filesystem::path clientSaveName = *CResourceHandler::get()->getResourceName(ResourcePath(CSH->si->mapname, EResType::CLIENT_SAVEGAME));

		if(clientSaveName.empty())
			throw std::runtime_error("Cannot open client part of " + CSH->si->mapname);

		std::unique_ptr<CLoadFile> loader (new CLoadFile(clientSaveName));
		serialize(loader->serializer, loader->serializer.fileVersion);

		logNetwork->info("Client data loaded.");
	}
	catch(std::exception & e)
	{
		logGlobal->info("Cannot load client data for game %s. Error: %s", CSH->si->mapname, e.what());
	}
#endif

	initPlayerInterfaces();
}

void CClient::serialize(BinarySerializer & h, const int version)
{
	assert(h.saving);
	ui8 players = static_cast<ui8>(playerint.size());
	h & players;

	for(auto i = playerint.begin(); i != playerint.end(); i++)
	{
		logGlobal->trace("Saving player %s interface", i->first);
		assert(i->first == i->second->playerID);
		h & i->first;
		h & i->second->dllName;
		h & i->second->human;
		i->second->saveGame(h, version);
	}

#if SCRIPTING_ENABLED
	if(version >= 800)
	{
		JsonNode scriptsState;
		clientScripts->serializeState(h.saving, scriptsState);
		h & scriptsState;
	}
#endif
}

void CClient::serialize(BinaryDeserializer & h, const int version)
{
	assert(!h.saving);
	ui8 players = 0;
	h & players;

	for(int i = 0; i < players; i++)
	{
		std::string dllname;
		PlayerColor pid;
		bool isHuman = false;
		auto prevInt = LOCPLINT;

		h & pid;
		h & dllname;
		h & isHuman;
		assert(dllname.length() == 0 || !isHuman);
		if(pid == PlayerColor::NEUTRAL)
		{
			logGlobal->trace("Neutral battle interfaces are not serialized.");
			continue;
		}

		logGlobal->trace("Loading player %s interface", pid);
		std::shared_ptr<CGameInterface> nInt;
		if(dllname.length())
			nInt = CDynLibHandler::getNewAI(dllname);
		else
			nInt = std::make_shared<CPlayerInterface>(pid);

		nInt->dllName = dllname;
		nInt->human = isHuman;
		nInt->playerID = pid;

		bool shouldResetInterface = true;
		// Client no longer handle this player at all
		if(!vstd::contains(CSH->getAllClientPlayers(CSH->c->connectionID), pid))
		{
			logGlobal->trace("Player %s is not belong to this client. Destroying interface", pid);
		}
		else if(isHuman && !vstd::contains(CSH->getHumanColors(), pid))
		{
			logGlobal->trace("Player %s is no longer controlled by human. Destroying interface", pid);
		}
		else if(!isHuman && vstd::contains(CSH->getHumanColors(), pid))
		{
			logGlobal->trace("Player %s is no longer controlled by AI. Destroying interface", pid);
		}
		else
		{
			installNewPlayerInterface(nInt, pid);
			shouldResetInterface = false;
		}

		// loadGame needs to be called after initGameInterface to load paths correctly
		// initGameInterface is called in installNewPlayerInterface
		nInt->loadGame(h, version);

		if (shouldResetInterface)
		{
			nInt.reset();
			LOCPLINT = prevInt;
		}
	}

#if SCRIPTING_ENABLED
	{
		JsonNode scriptsState;
		h & scriptsState;
		clientScripts->serializeState(h.saving, scriptsState);
	}
#endif

	logNetwork->trace("Loaded client part of save %d ms", CSH->th->getDiff());
}

void CClient::save(const std::string & fname)
{
	if(!gs->currentBattles.empty())
	{
		logNetwork->error("Game cannot be saved during battle!");
		return;
	}

	SaveGame save_game(fname);
	sendRequest(&save_game, PlayerColor::NEUTRAL);
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

	auto allPlayers = CSH->getAllClientPlayers(CSH->c->connectionID);
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
		if(!vstd::contains(CSH->getAllClientPlayers(CSH->c->connectionID), color))
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

	if(CSH->getAllClientPlayers(CSH->c->connectionID).count(PlayerColor::NEUTRAL))
		installNewBattleInterface(CDynLibHandler::getNewBattleAI(settings["server"]["neutralAI"].String()), PlayerColor::NEUTRAL);

	logNetwork->trace("Initialized player interfaces %d ms", CSH->th->getDiff());
}

std::string CClient::aiNameForPlayer(const PlayerSettings & ps, bool battleAI, bool alliedToHuman)
{
	if(ps.name.size())
	{
		const boost::filesystem::path aiPath = VCMIDirs::get().fullLibraryPath("AI", ps.name);
		if(boost::filesystem::exists(aiPath))
			return ps.name;
	}

	return aiNameForPlayer(battleAI, alliedToHuman);
}

std::string CClient::aiNameForPlayer(bool battleAI, bool alliedToHuman)
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

void CClient::handlePack(CPack * pack)
{
	CBaseForCLApply * apply = applier->getApplier(CTypeList::getInstance().getTypeID(pack)); //find the applier
	if(apply)
	{
		boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);
		apply->applyOnClBefore(this, pack);
		logNetwork->trace("\tMade first apply on cl: %s", typeid(pack).name());
		gs->apply(pack);
		logNetwork->trace("\tApplied on gs: %s", typeid(pack).name());
		apply->applyOnClAfter(this, pack);
		logNetwork->trace("\tMade second apply on cl: %s", typeid(pack).name());
	}
	else
	{
		logNetwork->error("Message %s cannot be applied, cannot find applier!", typeid(pack).name());
	}
	delete pack;
}

int CClient::sendRequest(const CPackForServer * request, PlayerColor player)
{
	static ui32 requestCounter = 0;

	ui32 requestID = requestCounter++;
	logNetwork->trace("Sending a request \"%s\". It'll have an ID=%d.", typeid(*request).name(), requestID);

	waitingRequest.pushBack(requestID);
	request->requestID = requestID;
	request->player = player;
	CSH->c->sendPack(request);
	if(vstd::contains(playerint, player))
		playerint[player]->requestSent(request, requestID);

	return requestID;
}

void CClient::battleStarted(const BattleInfo * info)
{
	std::shared_ptr<CPlayerInterface> att;
	std::shared_ptr<CPlayerInterface> def;
	auto & leftSide = info->sides[0];
	auto & rightSide = info->sides[1];

	for(auto & battleCb : battleCallbacks)
	{
		if(!battleCb.first.isValidPlayer() || battleCb.first == leftSide.color || battleCb.first == rightSide.color)
			battleCb.second->onBattleStarted(info);
	}

	//If quick combat is not, do not prepare interfaces for battleint
	auto callBattleStart = [&](PlayerColor color, ui8 side)
	{
		if(vstd::contains(battleints, color))
			battleints[color]->battleStart(info->battleID, leftSide.armyObject, rightSide.armyObject, info->tile, leftSide.hero, rightSide.hero, side, info->replayAllowed);
	};
	
	callBattleStart(leftSide.color, 0);
	callBattleStart(rightSide.color, 1);
	callBattleStart(PlayerColor::UNFLAGGABLE, 1);
	if(settings["session"]["spectate"].Bool() && !settings["session"]["spectate-skip-battle"].Bool())
		callBattleStart(PlayerColor::SPECTATOR, 1);
	
	if(vstd::contains(playerint, leftSide.color) && playerint[leftSide.color]->human)
		att = std::dynamic_pointer_cast<CPlayerInterface>(playerint[leftSide.color]);

	if(vstd::contains(playerint, rightSide.color) && playerint[rightSide.color]->human)
		def = std::dynamic_pointer_cast<CPlayerInterface>(playerint[rightSide.color]);
	
	//Remove player interfaces for auto battle (quickCombat option)
	if(att && att->isAutoFightOn)
	{
		if (att->cb->getBattle(info->battleID)->battleGetTacticDist())
		{
			auto side = att->cb->getBattle(info->battleID)->playerToSide(att->playerID);
			auto action = BattleAction::makeEndOFTacticPhase(*side);
			att->cb->battleMakeTacticAction(info->battleID, action);
		}

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
		auto tacticianColor = info->sides[info->tacticsSide].color;

		if (vstd::contains(battleints, tacticianColor))
			battleints[tacticianColor]->yourTacticPhase(info->battleID, info->tacticDistance);
	}
}

void CClient::battleFinished(const BattleID & battleID)
{
	for(auto & side : gs->getBattle(battleID)->sides)
		if(battleCallbacks.count(side.color))
			battleCallbacks[side.color]->onBattleEnded(battleID);

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

void CClient::invalidatePaths()
{
	boost::unique_lock<boost::mutex> pathLock(pathCacheMutex);
	pathCache.clear();
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
#ifndef SINGLE_PROCESS_APP
extern "C" JNIEXPORT void JNICALL Java_eu_vcmi_vcmi_NativeMethods_notifyServerClosed(JNIEnv * env, jclass cls)
{
	logNetwork->info("Received server closed signal");
	if (CSH) {
		CSH->campaignServerRestartLock.setn(false);
	}
}

extern "C" JNIEXPORT void JNICALL Java_eu_vcmi_vcmi_NativeMethods_notifyServerReady(JNIEnv * env, jclass cls)
{
	logNetwork->info("Received server ready signal");
	androidTestServerReadyFlag.store(true);
}
#endif

extern "C" JNIEXPORT jboolean JNICALL Java_eu_vcmi_vcmi_NativeMethods_tryToSaveTheGame(JNIEnv * env, jclass cls)
{
	logGlobal->info("Received emergency save game request");
	if(!LOCPLINT || !LOCPLINT->cb)
	{
		return false;
	}

	LOCPLINT->cb->save("Saves/_Android_Autosave");
	return true;
}
#endif
