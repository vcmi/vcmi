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
#ifndef VCMI_ANDROID
#include "../lib/Interprocess.h"
#endif
#include "../lib/NetPacks.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/VCMIDirs.h"
#include "../lib/mapping/CMap.h"
#include "../lib/JsonNode.h"
#include "mapHandler.h"
#include "../lib/CConfigHandler.h"
#include "CPreGame.h"
#include "battle/CBattleInterface.h"
#include "../lib/CThreadHelper.h"
#include "../lib/CScriptingModule.h"
#include "../lib/registerTypes/RegisterTypes.h"
#include "gui/CGuiHandler.h"
#include "CMT.h"

extern std::string NAME;
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
	virtual void applyOnClAfter(CClient * cl, void * pack) const = 0;
	virtual void applyOnClBefore(CClient * cl, void * pack) const = 0;
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
		logGlobal->errorStream() << "Cannot apply on CL plain CPack!";
		assert(0);
	}
	void applyOnClBefore(CClient * cl, void * pack) const override
	{
		logGlobal->errorStream() << "Cannot apply on CL plain CPack!";
		assert(0);
	}
};


static CApplier<CBaseForCLApply> * applier = nullptr;

void CClient::init()
{
	waitingRequest.clear();
	hotSeat = false;
	{
		TLockGuard _(connectionHandlerMutex);
		connectionHandler.reset();
	}
	pathInfo = nullptr;
	applier = new CApplier<CBaseForCLApply>();
	registerTypesClientPacks1(*applier);
	registerTypesClientPacks2(*applier);
	IObjectInterface::cb = this;
	serv = nullptr;
	gs = nullptr;
	erm = nullptr;
	terminate = false;
}

CClient::CClient(void)
{
	init();
}

CClient::CClient(CConnection * con, StartInfo * si)
{
	init();
	newGame(con, si);
}

CClient::~CClient(void)
{
	delete applier;
}

void CClient::waitForMoveAndSend(PlayerColor color)
{
	try
	{
		setThreadName("CClient::waitForMoveAndSend");
		assert(vstd::contains(battleints, color));
		BattleAction ba = battleints[color]->activeStack(gs->curB->battleGetStackByID(gs->curB->activeStack, false));
		if(ba.actionType != Battle::CANCEL)
		{
			logNetwork->traceStream() << "Send battle action to server: " << ba;
			MakeAction temp_action(ba);
			sendRequest(&temp_action, color);
		}
	}
	catch(boost::thread_interrupted &)
	{
		logNetwork->debugStream() << "Wait for move thread was interrupted and no action will be send. Was a battle ended by spell?";
	}
	catch(...)
	{
		handleException();
	}
}

void CClient::run()
{
	setThreadName("CClient::run");
	try
	{
		while(!terminate)
		{
			CPack * pack = serv->retreivePack(); //get the package from the server

			if(terminate)
			{
				vstd::clear_pointer(pack);
				break;
			}

			handlePack(pack);
		}
	}
	//catch only asio exceptions
	catch(const boost::system::system_error & e)
	{
		logNetwork->errorStream() << "Lost connection to server, ending listening thread!";
		logNetwork->errorStream() << e.what();
		if(!terminate) //rethrow (-> boom!) only if closing connection was unexpected
		{
			logNetwork->errorStream() << "Something wrong, lost connection while game is still ongoing...";
			throw;
		}
	}
}

void CClient::save(const std::string & fname)
{
	if(gs->curB)
	{
		logNetwork->errorStream() << "Game cannot be saved during battle!";
		return;
	}

	SaveGame save_game(fname);
	sendRequest((CPackForClient *)&save_game, PlayerColor::NEUTRAL);
}

void CClient::endGame(bool closeConnection)
{
	//suggest interfaces to finish their stuff (AI should interrupt any bg working threads)
	for(auto & i : playerint)
		i.second->finish();

	// Game is ending
	// Tell the network thread to reach a stable state
	if(closeConnection)
		stopConnection();
	logNetwork->infoStream() << "Closed connection.";

	GH.curInt = nullptr;
	{
		boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim);
		logNetwork->infoStream() << "Ending current game!";
		if(GH.topInt())
		{
			GH.topInt()->deactivate();
		}
		GH.listInt.clear();
		GH.objsToBlit.clear();
		GH.statusbar = nullptr;
		logNetwork->infoStream() << "Removed GUI.";

		vstd::clear_pointer(const_cast<CGameInfo *>(CGI)->mh);
		vstd::clear_pointer(gs);

		logNetwork->infoStream() << "Deleted mapHandler and gameState.";
		LOCPLINT = nullptr;
	}

	playerint.clear();
	battleints.clear();
	callbacks.clear();
	battleCallbacks.clear();
	CGKeys::reset();
	CGMagi::reset();
	CGObelisk::reset();
	logNetwork->infoStream() << "Deleted playerInts.";
	logNetwork->infoStream() << "Client stopped.";
}

#if 1
void CClient::loadGame(const std::string & fname, const bool server, const std::vector<int> & humanplayerindices, const int loadNumPlayers, int player_, const std::string & ipaddr, const ui16 port)
{
	PlayerColor player(player_); //intentional shadowing
	logNetwork->infoStream() << "Loading procedure started!";

	CServerHandler sh;
	if(server)
		sh.startServer();
	else
		serv = sh.justConnectToServer(ipaddr, port);

	CStopWatch tmh;
	std::unique_ptr<CLoadFile> loader;
	try
	{
		boost::filesystem::path clientSaveName = *CResourceHandler::get("local")->getResourceName(ResourceID(fname, EResType::CLIENT_SAVEGAME));
		boost::filesystem::path controlServerSaveName;

		if(CResourceHandler::get("local")->existsResource(ResourceID(fname, EResType::SERVER_SAVEGAME)))
		{
			controlServerSaveName = *CResourceHandler::get("local")->getResourceName(ResourceID(fname, EResType::SERVER_SAVEGAME));
		}
		else // create entry for server savegame. Triggered if save was made after launch and not yet present in res handler
		{
			controlServerSaveName = boost::filesystem::path(clientSaveName).replace_extension(".vsgm1");
			CResourceHandler::get("local")->createResource(controlServerSaveName.string(), true);
		}

		if(clientSaveName.empty())
			throw std::runtime_error("Cannot open client part of " + fname);
		if(controlServerSaveName.empty() || !boost::filesystem::exists(controlServerSaveName))
			throw std::runtime_error("Cannot open server part of " + fname);

		{
			CLoadIntegrityValidator checkingLoader(clientSaveName, controlServerSaveName, MINIMAL_SERIALIZATION_VERSION);
			loadCommonState(checkingLoader);
			loader = checkingLoader.decay();
		}
		logNetwork->infoStream() << "Loaded common part of save " << tmh.getDiff();
		const_cast<CGameInfo *>(CGI)->mh = new CMapHandler();
		const_cast<CGameInfo *>(CGI)->mh->map = gs->map;
		pathInfo = make_unique<CPathsInfo>(getMapSize());
		CGI->mh->init();
		logNetwork->infoStream() << "Initing maphandler: " << tmh.getDiff();
	}
	catch(std::exception & e)
	{
		logGlobal->errorStream() << "Cannot load game " << fname << ". Error: " << e.what();
		throw; //obviously we cannot continue here
	}

/*
    if(!server)
         player = PlayerColor(player_);
*/

	std::set<PlayerColor> clientPlayers;
	if(server)
		serv = sh.connectToServer();
	//*loader >> *this;

	if(server)
	{
		tmh.update();
		ui8 pom8;
		*serv << ui8(3) << ui8(loadNumPlayers); //load game; one client if single-player
		*serv << fname;
		*serv >> pom8;
		if(pom8)
			throw std::runtime_error("Server cannot open the savegame!");
		else
			logNetwork->infoStream() << "Server opened savegame properly.";
	}

	if(server)
	{
		for(auto & elem : gs->scenarioOps->playerInfos)
		{
			if(!std::count(humanplayerindices.begin(), humanplayerindices.end(), elem.first.getNum()) || elem.first == player)
				clientPlayers.insert(elem.first);
		}
		clientPlayers.insert(PlayerColor::NEUTRAL);
	}
	else
	{
		clientPlayers.insert(player);
	}

	std::cout << "CLIENTPLAYERS:\n";
	for(auto x : clientPlayers)
		std::cout << x << std::endl;
	std::cout << "ENDCLIENTPLAYERS\n";

	serialize(loader->serializer, loader->serializer.fileVersion, clientPlayers);
	*serv << ui32(clientPlayers.size());
	for(auto & elem : clientPlayers)
		*serv << ui8(elem.getNum());
	serv->addStdVecItems(gs); /*why is this here?*/

	//*loader >> *this;
	logNetwork->infoStream() << "Loaded client part of save " << tmh.getDiff();

	logNetwork->infoStream() << "Sent info to server: " << tmh.getDiff();

	//*serv << clientPlayers;
	serv->enableStackSendingByID();
	serv->disableSmartPointerSerialization();

//      logGlobal->traceStream() << "Objects:";
//      for(int i = 0; i < gs->map->objects.size(); i++)
//      {
//              auto o = gs->map->objects[i];
//              if(o)
//                      logGlobal->traceStream() << boost::format("\tindex=%5d, id=%5d; address=%5d, pos=%s, name=%s") % i % o->id % (int)o.get() % o->pos % o->getHoverText();
//              else
//                      logGlobal->traceStream() << boost::format("\tindex=%5d --- nullptr") % i;
//      }
}
#endif

void CClient::newGame(CConnection * con, StartInfo * si)
{
	enum
	{
		SINGLE,
		HOST,
		GUEST
	} networkMode = SINGLE;

	if(con == nullptr)
	{
		CServerHandler sh;
		serv = sh.connectToServer();
	}
	else
	{
		serv = con;
		networkMode = con->isHost() ? HOST : GUEST;
	}

	CConnection & c = *serv;
	////////////////////////////////////////////////////

	logNetwork->infoStream() << "\tWill send info to server...";
	CStopWatch tmh;

	if(networkMode == SINGLE)
	{
		ui8 pom8;
		c << ui8(2) << ui8(1); //new game; one client
		c << *si;
		c >> pom8;
		if(pom8)
			throw std::runtime_error("Server cannot open the map!");
	}

	c >> si;
	logNetwork->infoStream() << "\tSending/Getting info to/from the server: " << tmh.getDiff();
	c.enableStackSendingByID();
	c.disableSmartPointerSerialization();

	// Initialize game state
	gs = new CGameState();
	logNetwork->info("\tCreating gamestate: %i", tmh.getDiff());

	gs->init(si, settings["general"]["saveRandomMaps"].Bool());
	logNetwork->infoStream() << "Initializing GameState (together): " << tmh.getDiff();

	// Now after possible random map gen, we know exact player count.
	// Inform server about how many players client handles
	std::set<PlayerColor> myPlayers;
	for(auto & elem : gs->scenarioOps->playerInfos)
	{
		if((networkMode == SINGLE) //single - one client has all player
		   || (networkMode != SINGLE && serv->connectionID == elem.second.playerID) //multi - client has only "its players"
		   || (networkMode == HOST && elem.second.playerID == PlayerSettings::PLAYER_AI)) //multi - host has all AI players
		{
			myPlayers.insert(elem.first); //add player
		}
	}
	if(networkMode != GUEST)
		myPlayers.insert(PlayerColor::NEUTRAL);
	c << myPlayers;

	// Init map handler
	if(gs->map)
	{
		if(!settings["session"]["headless"].Bool())
		{
			const_cast<CGameInfo *>(CGI)->mh = new CMapHandler();
			CGI->mh->map = gs->map;
			logNetwork->infoStream() << "Creating mapHandler: " << tmh.getDiff();
			CGI->mh->init();
		}
		pathInfo = make_unique<CPathsInfo>(getMapSize());
		logNetwork->infoStream() << "Initializing mapHandler (together): " << tmh.getDiff();
	}

	int humanPlayers = 0;
	for(auto & elem : gs->scenarioOps->playerInfos) //initializing interfaces for players
	{
		PlayerColor color = elem.first;
		gs->currentPlayer = color;
		if(!vstd::contains(myPlayers, color))
			continue;

		logNetwork->traceStream() << "Preparing interface for player " << color;
		if(elem.second.playerID == PlayerSettings::PLAYER_AI)
		{
			auto AiToGive = aiNameForPlayer(elem.second, false);
			logNetwork->info("Player %s will be lead by %s", color, AiToGive);
			installNewPlayerInterface(CDynLibHandler::getNewAI(AiToGive), color);
		}
		else
		{
			installNewPlayerInterface(std::make_shared<CPlayerInterface>(color), color);
			humanPlayers++;
		}
	}

	if(settings["session"]["spectate"].Bool())
	{
		installNewPlayerInterface(std::make_shared<CPlayerInterface>(PlayerColor::SPECTATOR), PlayerColor::SPECTATOR, true);
	}
	loadNeutralBattleAI();

	serv->addStdVecItems(gs);
	hotSeat = (humanPlayers > 1);

//      std::vector<FileInfo> scriptModules;
//      CFileUtility::getFilesWithExt(scriptModules, LIB_DIR "/scripting", "." LIB_EXT);
//      for(FileInfo &m : scriptModules)
//      {
//              CScriptingModule * nm = CDynLibHandler::getNewScriptingModule(m.name);
//              privilagedGameEventReceivers.push_back(nm);
//              privilagedBattleEventReceivers.push_back(nm);
//              nm->giveActionCB(this);
//              nm->giveInfoCB(this);
//              nm->init();
//
//              erm = nm; //something tells me that there'll at most one module and it'll be ERM
//      }
}

void CClient::serialize(BinarySerializer & h, const int version)
{
	assert(h.saving);
	h & hotSeat;
	{
		ui8 players = playerint.size();
		h & players;

		for(auto i = playerint.begin(); i != playerint.end(); i++)
		{
			LOG_TRACE_PARAMS(logGlobal, "Saving player %s interface", i->first);
			assert(i->first == i->second->playerID);
			h & i->first & i->second->dllName & i->second->human;
			i->second->saveGame(h, version);
		}
	}
}

void CClient::serialize(BinaryDeserializer & h, const int version)
{
	assert(!h.saving);
	h & hotSeat;
	{
		ui8 players = 0; //fix for uninitialized warning
		h & players;

		for(int i = 0; i < players; i++)
		{
			std::string dllname;
			PlayerColor pid;
			bool isHuman = false;

			h & pid & dllname & isHuman;
			LOG_TRACE_PARAMS(logGlobal, "Loading player %s interface", pid);

			std::shared_ptr<CGameInterface> nInt;
			if(dllname.length())
			{
				if(pid == PlayerColor::NEUTRAL)
				{
					installNewBattleInterface(CDynLibHandler::getNewBattleAI(dllname), pid);
					//TODO? consider serialization
					continue;
				}
				else
				{
					assert(!isHuman);
					nInt = CDynLibHandler::getNewAI(dllname);
				}
			}
			else
			{
				assert(isHuman);
				nInt = std::make_shared<CPlayerInterface>(pid);
			}

			nInt->dllName = dllname;
			nInt->human = isHuman;
			nInt->playerID = pid;

			installNewPlayerInterface(nInt, pid);
			nInt->loadGame(h, version); //another evil cast, check above
		}

		if(!vstd::contains(battleints, PlayerColor::NEUTRAL))
			loadNeutralBattleAI();
	}
}

void CClient::serialize(BinarySerializer & h, const int version, const std::set<PlayerColor> & playerIDs)
{
	assert(h.saving);
	h & hotSeat;
	{
		ui8 players = playerint.size();
		h & players;

		for(auto i = playerint.begin(); i != playerint.end(); i++)
		{
			LOG_TRACE_PARAMS(logGlobal, "Saving player %s interface", i->first);
			assert(i->first == i->second->playerID);
			h & i->first & i->second->dllName & i->second->human;
			i->second->saveGame(h, version);
		}
	}
}

void CClient::serialize(BinaryDeserializer & h, const int version, const std::set<PlayerColor> & playerIDs)
{
	assert(!h.saving);
	h & hotSeat;
	{
		ui8 players = 0; //fix for uninitialized warning
		h & players;

		for(int i = 0; i < players; i++)
		{
			std::string dllname;
			PlayerColor pid;
			bool isHuman = false;

			h & pid & dllname & isHuman;
			LOG_TRACE_PARAMS(logGlobal, "Loading player %s interface", pid);

			std::shared_ptr<CGameInterface> nInt;
			if(dllname.length())
			{
				if(pid == PlayerColor::NEUTRAL)
				{
					if(playerIDs.count(pid))
						installNewBattleInterface(CDynLibHandler::getNewBattleAI(dllname), pid);
					//TODO? consider serialization
					continue;
				}
				else
				{
					assert(!isHuman);
					nInt = CDynLibHandler::getNewAI(dllname);
				}
			}
			else
			{
				assert(isHuman);
				nInt = std::make_shared<CPlayerInterface>(pid);
			}

			nInt->dllName = dllname;
			nInt->human = isHuman;
			nInt->playerID = pid;

			nInt->loadGame(h, version);
			if(settings["session"]["onlyai"].Bool() && isHuman)
			{
				removeGUI();
				nInt.reset();
				dllname = aiNameForPlayer(false);
				nInt = CDynLibHandler::getNewAI(dllname);
				nInt->dllName = dllname;
				nInt->human = false;
				nInt->playerID = pid;
				installNewPlayerInterface(nInt, pid);
				GH.totalRedraw();
			}
			else
			{
				if(playerIDs.count(pid))
					installNewPlayerInterface(nInt, pid);
			}
		}
		if(settings["session"]["spectate"].Bool())
		{
			removeGUI();
			auto p = std::make_shared<CPlayerInterface>(PlayerColor::SPECTATOR);
			installNewPlayerInterface(p, PlayerColor::SPECTATOR, true);
			GH.curInt = p.get();
			LOCPLINT->activateForSpectator();
			GH.totalRedraw();
		}

		if(playerIDs.count(PlayerColor::NEUTRAL))
			loadNeutralBattleAI();
	}
}

void CClient::handlePack(CPack * pack)
{
	if(pack == nullptr)
	{
		logNetwork->error("Dropping nullptr CPack! You should check whether client and server ABI matches.");
		return;
	}
	CBaseForCLApply * apply = applier->getApplier(typeList.getTypeID(pack)); //find the applier
	if(apply)
	{
		boost::unique_lock<boost::recursive_mutex> guiLock(*CPlayerInterface::pim);
		apply->applyOnClBefore(this, pack);
		logNetwork->trace("\tMade first apply on cl");
		gs->apply(pack);
		logNetwork->trace("\tApplied on gs");
		apply->applyOnClAfter(this, pack);
		logNetwork->trace("\tMade second apply on cl");
	}
	else
	{
		logNetwork->error("Message %s cannot be applied, cannot find applier!", typeList.getTypeInfo(pack)->name());
	}
	delete pack;
}

void CClient::finishCampaign(std::shared_ptr<CCampaignState> camp)
{
}

void CClient::proposeNextMission(std::shared_ptr<CCampaignState> camp)
{
	GH.pushInt(new CBonusSelection(camp));
}

void CClient::stopConnection()
{
	terminate = true;

	if(serv)
	{
		boost::unique_lock<boost::mutex>(*serv->wmx);
		if(serv->isHost()) //request closing connection
		{
			logNetwork->infoStream() << "Connection has been requested to be closed.";
			CloseServer close_server;
			sendRequest(&close_server, PlayerColor::NEUTRAL);
			logNetwork->infoStream() << "Sent closing signal to the server";
		}
		else
		{
			LeaveGame leave_Game;
			sendRequest(&leave_Game, PlayerColor::NEUTRAL);
			logNetwork->infoStream() << "Sent leaving signal to the server";
		}
	}

	{
		TLockGuard _(connectionHandlerMutex);
		if(connectionHandler) //end connection handler
		{
			if(connectionHandler->get_id() != boost::this_thread::get_id())
				connectionHandler->join();

			logNetwork->infoStream() << "Connection handler thread joined";
			connectionHandler.reset();
		}
	}


	if(serv) //and delete connection
	{
		serv->close();
		vstd::clear_pointer(serv);
		logNetwork->warnStream() << "Our socket has been closed.";
	}
}

void CClient::battleStarted(const BattleInfo * info)
{
	for(auto & battleCb : battleCallbacks)
	{
		if(vstd::contains_if(info->sides, [&](const SideInBattle & side) {return side.color == battleCb.first; })
		   || battleCb.first >= PlayerColor::PLAYER_LIMIT)
		{
			battleCb.second->setBattle(info);
		}
	}
//      for(ui8 side : info->sides)
//              if(battleCallbacks.count(side))
//                      battleCallbacks[side]->setBattle(info);

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
			auto bi = new CBattleInterface(leftSide.armyObject, rightSide.armyObject, leftSide.hero, rightSide.hero,
						       Rect((screen->w - 800) / 2,
							    (screen->h - 600) / 2, 800, 600), att, def);

			GH.pushInt(bi);
		}
		else if(settings["session"]["spectate"].Bool() && !settings["session"]["spectate-skip-battle"].Bool())
		{
			//TODO: This certainly need improvement
			auto spectratorInt = std::dynamic_pointer_cast<CPlayerInterface>(playerint[PlayerColor::SPECTATOR]);
			spectratorInt->cb->setBattle(info);
			boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim);
			auto bi = new CBattleInterface(leftSide.armyObject, rightSide.armyObject, leftSide.hero, rightSide.hero,
						       Rect((screen->w - 800) / 2,
							    (screen->h - 600) / 2, 800, 600), att, def, spectratorInt);

			GH.pushInt(bi);
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

void CClient::battleFinished()
{
	for(auto & side : gs->curB->sides)
		if(battleCallbacks.count(side.color))
			battleCallbacks[side.color]->setBattle(nullptr);

	if(settings["session"]["spectate"].Bool() && !settings["session"]["spectate-skip-battle"].Bool())
		battleCallbacks[PlayerColor::SPECTATOR]->setBattle(nullptr);
}

void CClient::loadNeutralBattleAI()
{
	installNewBattleInterface(CDynLibHandler::getNewBattleAI(settings["server"]["neutralAI"].String()), PlayerColor::NEUTRAL);
}

void CClient::commitPackage(CPackForClient * pack)
{
	CommitPackage cp;
	cp.freePack = false;
	cp.packToCommit = pack;
	sendRequest(&cp, PlayerColor::NEUTRAL);
}

PlayerColor CClient::getLocalPlayer() const
{
	if(LOCPLINT)
		return LOCPLINT->playerID;
	return getCurrentPlayer();
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

void CClient::invalidatePaths()
{
	// turn pathfinding info into invalid. It will be regenerated later
	boost::unique_lock<boost::mutex> pathLock(pathInfo->pathMx);
	pathInfo->hero = nullptr;
}

const CPathsInfo * CClient::getPathsInfo(const CGHeroInstance * h)
{
	assert(h);
	boost::unique_lock<boost::mutex> pathLock(pathInfo->pathMx);
	if(pathInfo->hero != h)
	{
		gs->calculatePaths(h, *pathInfo.get());
	}
	return pathInfo.get();
}

int CClient::sendRequest(const CPack * request, PlayerColor player)
{
	static ui32 requestCounter = 0;

	ui32 requestID = requestCounter++;
	logNetwork->traceStream() << boost::format("Sending a request \"%s\". It'll have an ID=%d.")
	% typeid(*request).name() % requestID;

	waitingRequest.pushBack(requestID);
	serv->sendPackToServer(*request, player, requestID);
	if(vstd::contains(playerint, player))
		playerint[player]->requestSent(dynamic_cast<const CPackForServer *>(request), requestID);

	return requestID;
}

void CClient::campaignMapFinished(std::shared_ptr<CCampaignState> camp)
{
	endGame(false);

	GH.curInt = CGPreGame::create();
	auto & epilogue = camp->camp->scenarios[camp->mapsConquered.back()].epilog;
	auto finisher = [=]()
		{
			if(camp->mapsRemaining.size())
				proposeNextMission(camp);
			else
				finishCampaign(camp);
		};
	if(epilogue.hasPrologEpilog)
	{
		GH.pushInt(new CPrologEpilogVideo(epilogue, finisher));
	}
	else
	{
		finisher();
	}
}

void CClient::installNewPlayerInterface(std::shared_ptr<CGameInterface> gameInterface, boost::optional<PlayerColor> color, bool battlecb)
{
	boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim);
	PlayerColor colorUsed = color.get_value_or(PlayerColor::UNFLAGGABLE);

	if(!color)
		privilagedGameEventReceivers.push_back(gameInterface);

	playerint[colorUsed] = gameInterface;

	logGlobal->traceStream() << boost::format("\tInitializing the interface for player %s") % colorUsed;
	auto cb = std::make_shared<CCallback>(gs, color, this);
	callbacks[colorUsed] = cb;
	battleCallbacks[colorUsed] = cb;
	gameInterface->init(cb);

	installNewBattleInterface(gameInterface, color, battlecb);
}

void CClient::installNewBattleInterface(std::shared_ptr<CBattleGameInterface> battleInterface, boost::optional<PlayerColor> color, bool needCallback)
{
	boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim);
	PlayerColor colorUsed = color.get_value_or(PlayerColor::UNFLAGGABLE);

	if(!color)
		privilagedBattleEventReceivers.push_back(battleInterface);

	battleints[colorUsed] = battleInterface;

	if(needCallback)
	{
		logGlobal->traceStream() << boost::format("\tInitializing the battle interface for player %s") % *color;
		auto cbc = std::make_shared<CBattleCallback>(gs, color, this);
		battleCallbacks[colorUsed] = cbc;
		battleInterface->init(cbc);
	}
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

void CServerHandler::startServer()
{
	if(settings["session"]["donotstartserver"].Bool())
		return;

	th.update();

#ifdef VCMI_ANDROID
	CAndroidVMHelper envHelper;
	envHelper.callStaticVoidMethod(CAndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS, "startServer", true);
#else
	serverThread = new boost::thread(&CServerHandler::callServer, this); //runs server executable;
#endif
	if(verbose)
		logNetwork->infoStream() << "Setting up thread calling server: " << th.getDiff();
}

void CServerHandler::waitForServer()
{
	if(settings["session"]["donotstartserver"].Bool())
		return;

	if(!serverThread)
		startServer();

	th.update();

#ifndef VCMI_ANDROID
	if(shared)
		shared->sr->waitTillReady();
#else
	logNetwork->infoStream() << "waiting for server";
	while(!androidTestServerReadyFlag.load())
	{
		logNetwork->infoStream() << "still waiting...";
		boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
	}
	logNetwork->infoStream() << "waiting for server finished...";
	androidTestServerReadyFlag = false;
#endif
	if(verbose)
		logNetwork->infoStream() << "Waiting for server: " << th.getDiff();
}

CConnection * CServerHandler::connectToServer()
{
	waitForServer();

	th.update(); //put breakpoint here to attach to server before it does something stupid

#ifndef VCMI_ANDROID
	CConnection * ret = justConnectToServer(settings["server"]["server"].String(), shared ? shared->sr->port : 0);
#else
	CConnection * ret = justConnectToServer(settings["server"]["server"].String());
#endif

	if(verbose)
		logNetwork->infoStream() << "\tConnecting to the server: " << th.getDiff();

	return ret;
}

ui16 CServerHandler::getDefaultPort()
{
	if(settings["session"]["serverport"].Integer())
		return settings["session"]["serverport"].Integer();
	else
		return settings["server"]["port"].Integer();
}

std::string CServerHandler::getDefaultPortStr()
{
	return boost::lexical_cast<std::string>(getDefaultPort());
}

CServerHandler::CServerHandler(bool runServer)
{
	serverThread = nullptr;
	shared = nullptr;
	verbose = true;
	uuid = boost::uuids::to_string(boost::uuids::random_generator()());

#ifndef VCMI_ANDROID
	if(settings["session"]["donotstartserver"].Bool() || settings["session"]["disable-shm"].Bool())
		return;

	std::string sharedMemoryName = "vcmi_memory";
	if(settings["session"]["enable-shm-uuid"].Bool())
	{
		//used or automated testing when multiple clients start simultaneously
		sharedMemoryName += "_" + uuid;
	}
	try
	{
		shared = new SharedMemory(sharedMemoryName, true);
	}
	catch(...)
	{
		vstd::clear_pointer(shared);
		logNetwork->error("Cannot open interprocess memory.");
		handleException();
		throw;
	}
#endif
}

CServerHandler::~CServerHandler()
{
	delete shared;
	delete serverThread; //detaches, not kills thread
}

void CServerHandler::callServer()
{
#ifndef VCMI_ANDROID
	setThreadName("CServerHandler::callServer");
	const std::string logName = (VCMIDirs::get().userCachePath() / "server_log.txt").string();
	std::string comm = VCMIDirs::get().serverPath().string()
		+ " --port=" + getDefaultPortStr()
		+ " --run-by-client"
		+ " --uuid=" + uuid;
	if(shared)
	{
		comm += " --enable-shm";
		if(settings["session"]["enable-shm-uuid"].Bool())
			comm += " --enable-shm-uuid";
	}
	comm += " > \"" + logName + '\"';

	int result = std::system(comm.c_str());
	if(result == 0)
	{
		logNetwork->infoStream() << "Server closed correctly";
		serverAlive.setn(false);
	}
	else
	{
		logNetwork->errorStream() << "Error: server failed to close correctly or crashed!";
		logNetwork->errorStream() << "Check " << logName << " for more info";
		exit(1); // exit in case of error. Othervice without working server VCMI will hang
	}
#endif
}

CConnection * CServerHandler::justConnectToServer(const std::string & host, const ui16 port)
{
	CConnection * ret = nullptr;
	while(!ret)
	{
		try
		{
			logNetwork->infoStream() << "Establishing connection...";
			ret = new CConnection(host.size() ? host : settings["server"]["server"].String(),
					      port ? port : getDefaultPort(),
					      NAME);
			ret->connectionID = 1; // TODO: Refactoring for the server so IDs set outside of CConnection
		}
		catch(...)
		{
			logNetwork->errorStream() << "\nCannot establish connection! Retrying within 2 seconds";
			SDL_Delay(2000);
		}
	}
	return ret;
}

#ifdef VCMI_ANDROID
extern "C" JNIEXPORT void JNICALL Java_eu_vcmi_vcmi_NativeMethods_notifyServerReady(JNIEnv * env, jobject cls)
{
	logNetwork->infoStream() << "Received server ready signal";
	androidTestServerReadyFlag.store(true);
}

extern "C" JNIEXPORT bool JNICALL Java_eu_vcmi_vcmi_NativeMethods_tryToSaveTheGame(JNIEnv * env, jobject cls)
{
	logGlobal->infoStream() << "Received emergency save game request";
	if(!LOCPLINT || !LOCPLINT->cb)
	{
		return false;
	}

	LOCPLINT->cb->save("Saves/_Android_Autosave");
	return true;
}
#endif
