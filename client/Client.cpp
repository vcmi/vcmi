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
#include "Client.h"

#include <SDL.h>

#include "CMusicHandler.h"
#include "../lib/mapping/CCampaignHandler.h"
#include "../CCallback.h"
#include "../lib/CConsoleHandler.h"
#include "CGameInfo.h"
#include "../lib/CGameState.h"
#include "CPlayerInterface.h"
#include "../lib/StartInfo.h"
#include "../lib/battle/BattleInfo.h"
#include "../lib/CModHandler.h"
#include "../lib/CArtHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CTownHandler.h"
#include "../lib/CBuildingHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/serializer/CTypeList.h"
#include "../lib/serializer/Connection.h"
#include "../lib/serializer/CLoadIntegrityValidator.h"
#include "../lib/NetPacks.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/VCMIDirs.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/JsonNode.h"
#include "mapHandler.h"
#include "../lib/CConfigHandler.h"
#include "mainmenu/CMainMenu.h"
#include "mainmenu/CCampaignScreen.h"
#include "lobby/CBonusSelection.h"
#include "battle/BattleInterface.h"
#include "../lib/CThreadHelper.h"
#include "../lib/registerTypes/RegisterTypes.h"
#include "gui/CGuiHandler.h"
#include "CMT.h"
#include "CServerHandler.h"
#include "../lib/ScriptHandler.h"
#include <vcmi/events/EventBus.h>

#ifdef VCMI_ANDROID
#include "lib/CAndroidVMHelper.h"
#endif

#ifdef VCMI_ANDROID
std::atomic_bool androidTestServerReadyFlag;
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
		ptr->applyCl(cl);
	}
	void applyOnClBefore(CClient * cl, void * pack) const override
	{
		T * ptr = static_cast<T *>(pack);
		ptr->applyFirstCl(cl);
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

const CPlayerEnvironment::BattleCb * CPlayerEnvironment::battle() const
{
	return mainCallback.get();
}

const CPlayerEnvironment::GameCb * CPlayerEnvironment::game() const
{
	return mainCallback.get();
}


CClient::CClient()
{
	waitingRequest.clear();
	applier = std::make_shared<CApplier<CBaseForCLApply>>();
	registerTypesClientPacks1(*applier);
	registerTypesClientPacks2(*applier);
	IObjectInterface::cb = this;
	gs = nullptr;
}

CClient::~CClient()
{
	IObjectInterface::cb = nullptr;
}

const Services * CClient::services() const
{
	return VLC; //todo: this should be CGI
}

const CClient::BattleCb * CClient::battle() const
{
	return this;
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
	gs = initializedGameState ? initializedGameState : new CGameState();
	gs->preInit(VLC);
	logNetwork->trace("\tCreating gamestate: %i", CSH->th->getDiff());
	if(!initializedGameState)
		gs->init(&mapService, CSH->si.get(), settings["general"]["saveRandomMaps"].Bool());
	logNetwork->trace("Initializing GameState (together): %d ms", CSH->th->getDiff());

	initMapHandler();
	reinitScripting();
	initPlayerEnvironments();
	initPlayerInterfaces();
}

void CClient::loadGame(CGameState * initializedGameState)
{
	logNetwork->info("Loading procedure started!");
	
	std::unique_ptr<CLoadFile> loader;

	if(initializedGameState)
	{
		logNetwork->info("Game state was transferred over network, loading.");
		gs = initializedGameState;
	}
	else
	{
		try
		{
			boost::filesystem::path clientSaveName = *CResourceHandler::get("local")->getResourceName(ResourceID(CSH->si->mapname, EResType::CLIENT_SAVEGAME));
			boost::filesystem::path controlServerSaveName;

			if(CResourceHandler::get("local")->existsResource(ResourceID(CSH->si->mapname, EResType::SERVER_SAVEGAME)))
			{
				controlServerSaveName = *CResourceHandler::get("local")->getResourceName(ResourceID(CSH->si->mapname, EResType::SERVER_SAVEGAME));
			}
			else // create entry for server savegame. Triggered if save was made after launch and not yet present in res handler
			{
				controlServerSaveName = boost::filesystem::path(clientSaveName).replace_extension(".vsgm1");
				CResourceHandler::get("local")->createResource(controlServerSaveName.string(), true);
			}

			if(clientSaveName.empty())
				throw std::runtime_error("Cannot open client part of " + CSH->si->mapname);
			if(controlServerSaveName.empty() || !boost::filesystem::exists(controlServerSaveName))
				throw std::runtime_error("Cannot open server part of " + CSH->si->mapname);

			{
				CLoadIntegrityValidator checkingLoader(clientSaveName, controlServerSaveName, MINIMAL_SERIALIZATION_VERSION);
				loadCommonState(checkingLoader);
				loader = checkingLoader.decay();
			}
		}
		catch(std::exception & e)
		{
			logGlobal->error("Cannot load game %s. Error: %s", CSH->si->mapname, e.what());
			throw; //obviously we cannot continue here
		}
		logNetwork->trace("Loaded common part of save %d ms", CSH->th->getDiff());
	}
	gs->preInit(VLC);
	gs->updateOnLoad(CSH->si.get());
	logNetwork->info("Game loaded, initialize interfaces.");
	
	initMapHandler();

	reinitScripting();

	initPlayerEnvironments();
	
	if(loader)
		serialize(loader->serializer, loader->serializer.fileVersion);

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

		nInt->loadGame(h, version);

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
			continue;
		}
		nInt.reset();
		LOCPLINT = prevInt;
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
	if(gs->curB)
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
		boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim);
		logNetwork->info("Ending current game!");
		removeGUI();

		vstd::clear_pointer(const_cast<CGameInfo *>(CGI)->mh);
		vstd::clear_pointer(gs);

		logNetwork->info("Deleted mapHandler and gameState.");
	}

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
		const_cast<CGameInfo *>(CGI)->mh = new CMapHandler();
		CGI->mh->map = gs->map;
		logNetwork->trace("Creating mapHandler: %d ms", CSH->th->getDiff());
		CGI->mh->init();
		logNetwork->trace("Initializing mapHandler (together): %d ms", CSH->th->getDiff());
	}

	pathCache.clear();
}

void CClient::initPlayerEnvironments()
{
	playerEnvironments.clear();

	auto allPlayers = CSH->getAllClientPlayers(CSH->c->connectionID);

	for(auto & color : allPlayers)
	{
		logNetwork->info("Preparing environment for player %s", color.getStr());
		playerEnvironments[color] = std::make_shared<CPlayerEnvironment>(color, this, std::make_shared<CCallback>(gs, color, this));
	}

	if(settings["session"]["spectate"].Bool())
	{
		playerEnvironments[PlayerColor::SPECTATOR] = std::make_shared<CPlayerEnvironment>(PlayerColor::SPECTATOR, this, std::make_shared<CCallback>(gs, boost::none, this));
	}
}

void CClient::initPlayerInterfaces()
{
	for(auto & elem : gs->scenarioOps->playerInfos)
	{
		PlayerColor color = elem.first;
		if(!vstd::contains(CSH->getAllClientPlayers(CSH->c->connectionID), color))
			continue;

		if(!vstd::contains(playerint, color))
		{
			logNetwork->info("Preparing interface for player %s", color.getStr());
			if(elem.second.isControlledByAI())
			{
				auto AiToGive = aiNameForPlayer(elem.second, false);
				logNetwork->info("Player %s will be lead by %s", color.getStr(), AiToGive);
				installNewPlayerInterface(CDynLibHandler::getNewAI(AiToGive), color);
			}
			else
			{
				logNetwork->info("Player %s will be lead by human", color.getStr());
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

std::string CClient::aiNameForPlayer(const PlayerSettings & ps, bool battleAI)
{
	if(ps.name.size())
	{
		const boost::filesystem::path aiPath = VCMIDirs::get().fullLibraryPath("AI", ps.name);
		if(boost::filesystem::exists(aiPath))
			return ps.name;
	}

	return aiNameForPlayer(battleAI);
}

std::string CClient::aiNameForPlayer(bool battleAI)
{
	const int sensibleAILimit = settings["session"]["oneGoodAI"].Bool() ? 1 : PlayerColor::PLAYER_LIMIT_I;
	std::string goodAI = battleAI ? settings["server"]["neutralAI"].String() : settings["server"]["playerAI"].String();
	std::string badAI = battleAI ? "StupidAI" : "EmptyAI";

	//TODO what about human players
	if(battleints.size() >= sensibleAILimit)
		return badAI;

	return goodAI;
}

void CClient::installNewPlayerInterface(std::shared_ptr<CGameInterface> gameInterface, PlayerColor color, bool battlecb)
{
	boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim);

	playerint[color] = gameInterface;

	logGlobal->trace("\tInitializing the interface for player %s", color.getStr());
	auto cb = std::make_shared<CCallback>(gs, color, this);
	battleCallbacks[color] = cb;
	gameInterface->initGameInterface(playerEnvironments.at(color), cb);

	installNewBattleInterface(gameInterface, color, battlecb);
}

void CClient::installNewBattleInterface(std::shared_ptr<CBattleGameInterface> battleInterface, PlayerColor color, bool needCallback)
{
	boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim);

	battleints[color] = battleInterface;

	if(needCallback)
	{
		logGlobal->trace("\tInitializing the battle interface for player %s", color.getStr());
		auto cbc = std::make_shared<CBattleCallback>(color, this);
		battleCallbacks[color] = cbc;
		battleInterface->initBattleInterface(playerEnvironments.at(color), cbc);
	}
}

void CClient::handlePack(CPack * pack)
{
	CBaseForCLApply * apply = applier->getApplier(typeList.getTypeID(pack)); //find the applier
	if(apply)
	{
		boost::unique_lock<boost::recursive_mutex> guiLock(*CPlayerInterface::pim);
		apply->applyOnClBefore(this, pack);
		logNetwork->trace("\tMade first apply on cl: %s", typeList.getTypeInfo(pack)->name());
		gs->apply(pack);
		logNetwork->trace("\tApplied on gs: %s", typeList.getTypeInfo(pack)->name());
		apply->applyOnClAfter(this, pack);
		logNetwork->trace("\tMade second apply on cl: %s", typeList.getTypeInfo(pack)->name());
	}
	else
	{
		logNetwork->error("Message %s cannot be applied, cannot find applier!", typeList.getTypeInfo(pack)->name());
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
	setBattle(info);

	for(auto & battleCb : battleCallbacks)
	{
		if(vstd::contains_if(info->sides, [&](const SideInBattle& side) {return side.color == battleCb.first; })
			|| battleCb.first >= PlayerColor::PLAYER_LIMIT)
		{
			battleCb.second->setBattle(info);
		}
	}

	std::shared_ptr<CPlayerInterface> att, def;
	auto & leftSide = info->sides[0], & rightSide = info->sides[1];

	//If quick combat is not, do not prepare interfaces for battleint
	if(!settings["adventure"]["quickCombat"].Bool())
	{
		if(vstd::contains(playerint, leftSide.color) && playerint[leftSide.color]->human)
			att = std::dynamic_pointer_cast<CPlayerInterface>(playerint[leftSide.color]);

		if(vstd::contains(playerint, rightSide.color) && playerint[rightSide.color]->human)
			def = std::dynamic_pointer_cast<CPlayerInterface>(playerint[rightSide.color]);
	}

	if(!settings["session"]["headless"].Bool())
	{
		if(!!att || !!def)
		{
			boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim);
			CPlayerInterface::battleInt = std::make_shared<BattleInterface>(leftSide.armyObject, rightSide.armyObject, leftSide.hero, rightSide.hero, att, def);
		}
		else if(settings["session"]["spectate"].Bool() && !settings["session"]["spectate-skip-battle"].Bool())
		{
			//TODO: This certainly need improvement
			auto spectratorInt = std::dynamic_pointer_cast<CPlayerInterface>(playerint[PlayerColor::SPECTATOR]);
			spectratorInt->cb->setBattle(info);
			boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim);
			CPlayerInterface::battleInt = std::make_shared<BattleInterface>(leftSide.armyObject, rightSide.armyObject, leftSide.hero, rightSide.hero, att, def, spectratorInt);
		}
	}

	auto callBattleStart = [&](PlayerColor color, ui8 side)
	{
		if(vstd::contains(battleints, color))
			battleints[color]->battleStart(leftSide.armyObject, rightSide.armyObject, info->tile, leftSide.hero, rightSide.hero, side);
	};

	callBattleStart(leftSide.color, 0);
	callBattleStart(rightSide.color, 1);
	callBattleStart(PlayerColor::UNFLAGGABLE, 1);
	if(settings["session"]["spectate"].Bool() && !settings["session"]["spectate-skip-battle"].Bool())
		callBattleStart(PlayerColor::SPECTATOR, 1);

	if(info->tacticDistance && vstd::contains(battleints, info->sides[info->tacticsSide].color))
	{
		boost::thread(&CClient::commenceTacticPhaseForInt, this, battleints[info->sides[info->tacticsSide].color]);
	}
}

void CClient::commenceTacticPhaseForInt(std::shared_ptr<CBattleGameInterface> battleInt)
{
	setThreadName("CClient::commenceTacticPhaseForInt");
	try
	{
		battleInt->yourTacticPhase(gs->curB->tacticDistance);
		if(gs && !!gs->curB && gs->curB->tacticDistance) //while awaiting for end of tactics phase, many things can happen (end of battle... or game)
		{
			MakeAction ma(BattleAction::makeEndOFTacticPhase(gs->curB->playerToSide(battleInt->playerID).get()));
			sendRequest(&ma, battleInt->playerID);
		}
	}
	catch(...)
	{
		handleException();
	}
}

void CClient::battleFinished()
{
	stopAllBattleActions();
	for(auto & side : gs->curB->sides)
		if(battleCallbacks.count(side.color))
			battleCallbacks[side.color]->setBattle(nullptr);

	if(settings["session"]["spectate"].Bool() && !settings["session"]["spectate-skip-battle"].Bool())
		battleCallbacks[PlayerColor::SPECTATOR]->setBattle(nullptr);

	setBattle(nullptr);
}

void CClient::startPlayerBattleAction(PlayerColor color)
{
	stopPlayerBattleAction(color);

	if(vstd::contains(battleints, color))
	{
		auto thread = std::make_shared<boost::thread>(std::bind(&CClient::waitForMoveAndSend, this, color));
		playerActionThreads[color] = thread;
	}
}

void CClient::stopPlayerBattleAction(PlayerColor color)
{
	if(vstd::contains(playerActionThreads, color))
	{
		auto thread = playerActionThreads.at(color);
		if(thread->joinable())
		{
			thread->interrupt();
			thread->join();
		}
		playerActionThreads.erase(color);
	}

}

void CClient::stopAllBattleActions()
{
	while(!playerActionThreads.empty())
		stopPlayerBattleAction(playerActionThreads.begin()->first);
}

void CClient::waitForMoveAndSend(PlayerColor color)
{
	try
	{
		setThreadName("CClient::waitForMoveAndSend");
		assert(vstd::contains(battleints, color));
		BattleAction ba = battleints[color]->activeStack(gs->curB->battleGetStackByID(gs->curB->activeStack, false));
		if(ba.actionType != EActionType::CANCEL)
		{
			logNetwork->trace("Send battle action to server: %s", ba.toString());
			MakeAction temp_action(ba);
			sendRequest(&temp_action, color);
		}
	}
	catch(boost::thread_interrupted &)
	{
		logNetwork->debug("Wait for move thread was interrupted and no action will be send. Was a battle ended by spell?");
	}
	catch(...)
	{
		handleException();
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
		std::shared_ptr<CPathsInfo> paths = std::make_shared<CPathsInfo>(getMapSize(), h);

		gs->calculatePaths(h, *paths.get());

		pathCache[h] = paths;
		return paths;
	}
	else
	{
		return iter->second;
	}
}

PlayerColor CClient::getLocalPlayer() const
{
	if(LOCPLINT)
		return LOCPLINT->playerID;
	return getCurrentPlayer();
}

#if SCRIPTING_ENABLED
scripting::Pool * CClient::getGlobalContextPool() const
{
	return clientScripts.get();
}

scripting::Pool * CClient::getContextPool() const
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


#ifdef VCMI_ANDROID
extern "C" JNIEXPORT void JNICALL Java_eu_vcmi_vcmi_NativeMethods_notifyServerClosed(JNIEnv * env, jobject cls)
{
	logNetwork->info("Received server closed signal");
	if (CSH) {
		CSH->campaignServerRestartLock.setn(false);
	}
}

extern "C" JNIEXPORT void JNICALL Java_eu_vcmi_vcmi_NativeMethods_notifyServerReady(JNIEnv * env, jobject cls)
{
	logNetwork->info("Received server ready signal");
	androidTestServerReadyFlag.store(true);
}

extern "C" JNIEXPORT bool JNICALL Java_eu_vcmi_vcmi_NativeMethods_tryToSaveTheGame(JNIEnv * env, jobject cls)
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
