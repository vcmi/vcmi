#include "StdInc.h"

#include "CMusicHandler.h"
#include "../lib/mapping/CCampaignHandler.h"
#include "../CCallback.h"
#include "../lib/CConsoleHandler.h"
#include "CGameInfo.h"
#include "../lib/CGameState.h"
#include "CPlayerInterface.h"
#include "../lib/StartInfo.h"
#include "../lib/BattleState.h"
#include "../lib/CModHandler.h"
#include "../lib/CArtHandler.h"
#include "../lib/CDefObjInfoHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CTownHandler.h"
#include "../lib/CObjectHandler.h"
#include "../lib/CBuildingHandler.h"
#include "../lib/CSpellHandler.h"
#include "../lib/Connection.h"
#include "../lib/Interprocess.h"
#include "../lib/NetPacks.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/VCMIDirs.h"
#include "../lib/mapping/CMap.h"
#include "../lib/JsonNode.h"
#include "mapHandler.h"
#include "../lib/CConfigHandler.h"
#include "Client.h"
#include "CPreGame.h"
#include "battle/CBattleInterface.h"
#include "../lib/CThreadHelper.h"
#include "../lib/CScriptingModule.h"
#include "../lib/RegisterTypes.h"
#include "gui/CGuiHandler.h"

extern std::string NAME;
namespace intpr = boost::interprocess;

/*
 * Client.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

template <typename T> class CApplyOnCL;

class CBaseForCLApply
{
public:
	virtual void applyOnClAfter(CClient *cl, void *pack) const =0; 
	virtual void applyOnClBefore(CClient *cl, void *pack) const =0; 
	virtual ~CBaseForCLApply(){}

	template<typename U> static CBaseForCLApply *getApplier(const U * t=NULL)
	{
		return new CApplyOnCL<U>;
	}
};

template <typename T> class CApplyOnCL : public CBaseForCLApply
{
public:
	void applyOnClAfter(CClient *cl, void *pack) const
	{
		T *ptr = static_cast<T*>(pack);
		ptr->applyCl(cl);
	}
	void applyOnClBefore(CClient *cl, void *pack) const
	{
		T *ptr = static_cast<T*>(pack);
		ptr->applyFirstCl(cl);
	}
};

static CApplier<CBaseForCLApply> *applier = nullptr;

void CClient::init()
{
	hotSeat = false;
	connectionHandler = nullptr;
	pathInfo = nullptr;
	applier = new CApplier<CBaseForCLApply>;
	registerTypes2(*applier);
	IObjectInterface::cb = this;
	serv = nullptr;
	gs = nullptr;
	cb = nullptr;
	erm = nullptr;
	terminate = false;
}

CClient::CClient(void)
{
	init();
}

CClient::CClient(CConnection *con, StartInfo *si)
{
	init();
	newGame(con,si);
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
		MakeAction temp_action(ba);
		sendRequest(&temp_action, color);
		return;
	}
	catch(boost::thread_interrupted&)
	{
        logNetwork->debugStream() << "Wait for move thread was interrupted and no action will be send. Was a battle ended by spell?";
		return;
	}
	HANDLE_EXCEPTION
    logNetwork->errorStream() << "We should not be here!";
}

void CClient::run()
{
	setThreadName("CClient::run");
	try
	{
		CPack *pack = NULL;
		while(!terminate)
		{
			pack = serv->retreivePack(); //get the package from the server
			
			if (terminate) 
			{
				delete pack;
				pack = NULL;
				break;
			}

			handlePack(pack);
			pack = NULL;
		}
	} 
	//catch only asio exceptions
	catch (const boost::system::system_error& e)
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
	sendRequest((CPackForClient*)&save_game, PlayerColor::NEUTRAL);
}

void CClient::endGame( bool closeConnection /*= true*/ )
{
	//suggest interfaces to finish their stuff (AI should interrupt any bg working threads)
	BOOST_FOREACH(auto i, playerint)
		i.second->finish();

	// Game is ending
	// Tell the network thread to reach a stable state
	if(closeConnection)
		stopConnection();
    logNetwork->infoStream() << "Closed connection.";

	GH.curInt = NULL;
	LOCPLINT->terminate_cond.setn(true);
	{
		boost::unique_lock<boost::recursive_mutex> un(*LOCPLINT->pim);
        logNetwork->infoStream() << "Ending current game!";
		if(GH.topInt())
			GH.topInt()->deactivate();
		GH.listInt.clear();
		GH.objsToBlit.clear();
		GH.statusbar = NULL;
        logNetwork->infoStream() << "Removed GUI.";

		vstd::clear_pointer(const_cast<CGameInfo*>(CGI)->mh);
		vstd::clear_pointer(gs);

        logNetwork->infoStream() << "Deleted mapHandler and gameState.";
		LOCPLINT = NULL;
	}
	while (!playerint.empty())
	{
		CGameInterface *pint = playerint.begin()->second;
		playerint.erase(playerint.begin());
		delete pint;
	}

	callbacks.clear();
    logNetwork->infoStream() << "Deleted playerInts.";

    logNetwork->infoStream() << "Client stopped.";
}

void CClient::loadGame( const std::string & fname )
{
    logNetwork->infoStream() <<"Loading procedure started!";

	CServerHandler sh;
	sh.startServer();

	CStopWatch tmh;
	{
		auto clientSaveName = CResourceHandler::get()->getResourceName(ResourceID(fname, EResType::CLIENT_SAVEGAME));
		auto controlServerSaveName = CResourceHandler::get()->getResourceName(ResourceID(fname, EResType::SERVER_SAVEGAME));

		unique_ptr<CLoadFile> loader;
		{
			CLoadIntegrityValidator checkingLoader(clientSaveName, controlServerSaveName);
			loadCommonState(checkingLoader);
			loader = checkingLoader.decay();
		}
        logNetwork->infoStream() << "Loaded common part of save " << tmh.getDiff();
		const_cast<CGameInfo*>(CGI)->mh = new CMapHandler();
		const_cast<CGameInfo*>(CGI)->mh->map = gs->map;
		pathInfo = make_unique<CPathsInfo>(getMapSize());
		CGI->mh->init();
        logNetwork->infoStream() <<"Initing maphandler: "<<tmh.getDiff();

		*loader >> *this;
        logNetwork->infoStream() << "Loaded client part of save " << tmh.getDiff();
	}

	serv = sh.connectToServer();
	serv->addStdVecItems(gs);

	tmh.update();
	ui8 pom8;
	*serv << ui8(3) << ui8(1); //load game; one client
	*serv << fname;
	*serv >> pom8;
	if(pom8) 
		throw std::runtime_error("Server cannot open the savegame!");
	else
        logNetwork->infoStream() << "Server opened savegame properly.";

	*serv << ui32(gs->scenarioOps->playerInfos.size()+1); //number of players + neutral
	for(auto it = gs->scenarioOps->playerInfos.begin(); 
		it != gs->scenarioOps->playerInfos.end(); ++it)
	{
		*serv << ui8(it->first.getNum()); //players
	}
	*serv << ui8(PlayerColor::NEUTRAL.getNum());
    logNetwork->infoStream() <<"Sent info to server: "<<tmh.getDiff();

	serv->enableStackSendingByID();
	serv->disableSmartPointerSerialization();

}

void CClient::newGame( CConnection *con, StartInfo *si )
{
	enum {SINGLE, HOST, GUEST} networkMode = SINGLE;

	if (con == NULL) 
	{
		CServerHandler sh;
		serv = sh.connectToServer();
	}
	else
	{
		serv = con;
		networkMode = (con->connectionID == 1) ? HOST : GUEST;
	}

	CConnection &c = *serv;
	////////////////////////////////////////////////////

	logNetwork->infoStream() <<"\tWill send info to server...";
	CStopWatch tmh;

	if(networkMode == SINGLE)
	{
		ui8 pom8;
		c << ui8(2) << ui8(1); //new game; one client
		c << *si;
		c >> pom8;
		if(pom8) 
			throw std::runtime_error("Server cannot open the map!");
		else
            logNetwork->infoStream() << "Server opened map properly.";
	}

	c >> si;
    logNetwork->infoStream() <<"\tSending/Getting info to/from the server: "<<tmh.getDiff();
	c.enableStackSendingByID();
	c.disableSmartPointerSerialization();

	// Initialize game state
	gs = new CGameState();
	logNetwork->infoStream() <<"\tCreating gamestate: "<<tmh.getDiff();

	gs->scenarioOps = si;
	gs->init(si);
    logNetwork->infoStream() <<"Initializing GameState (together): "<<tmh.getDiff();

	// Now after possible random map gen, we know exact player count.
	// Inform server about how many players client handles
	std::set<PlayerColor> myPlayers;
	for(auto it = gs->scenarioOps->playerInfos.begin(); it != gs->scenarioOps->playerInfos.end(); ++it)
	{
		if((networkMode == SINGLE)                                                      //single - one client has all player
		   || (networkMode != SINGLE && serv->connectionID == it->second.playerID)      //multi - client has only "its players"
		   || (networkMode == HOST && it->second.playerID == PlayerSettings::PLAYER_AI))//multi - host has all AI players
		{
			myPlayers.insert(it->first); //add player
		}
	}
	if(networkMode != GUEST)
		myPlayers.insert(PlayerColor::NEUTRAL);
	c << myPlayers;

	// Init map handler
	if(gs->map)
	{
		const_cast<CGameInfo*>(CGI)->mh = new CMapHandler();
		CGI->mh->map = gs->map;
        logNetwork->infoStream() <<"Creating mapHandler: "<<tmh.getDiff();
		CGI->mh->init();
		pathInfo = make_unique<CPathsInfo>(getMapSize());
        logNetwork->infoStream() <<"Initializing mapHandler (together): "<<tmh.getDiff();
	}

	int humanPlayers = 0;
	int sensibleAILimit = settings["session"]["oneGoodAI"].Bool() ? 1 : PlayerColor::PLAYER_LIMIT_I;
	for(auto it = gs->scenarioOps->playerInfos.begin(); 
		it != gs->scenarioOps->playerInfos.end(); ++it)//initializing interfaces for players
	{
		PlayerColor color = it->first;
		gs->currentPlayer = color;
		if(!vstd::contains(myPlayers, color))
			continue;

        logNetwork->traceStream() << "Preparing interface for player " << color;
		if(si->mode != StartInfo::DUEL)
		{
			auto cb = make_shared<CCallback>(gs,color,this);
			if(it->second.playerID == PlayerSettings::PLAYER_AI)
			{
				std::string AItoGive = settings["server"]["playerAI"].String();
				if(!sensibleAILimit)
					AItoGive = "EmptyAI";
				else
					sensibleAILimit--;
				playerint[color] = static_cast<CGameInterface*>(CDynLibHandler::getNewAI(AItoGive));
                logNetwork->infoStream() << "Player " << static_cast<int>(color.getNum()) << " will be lead by " << AItoGive;
			}
			else 
			{
				playerint[color] = new CPlayerInterface(color);
				humanPlayers++;
			}
			battleints[color] = playerint[color];

            logNetwork->traceStream() << "\tInitializing the interface";
			playerint[color]->init(cb.get());
			battleCallbacks[color] = callbacks[color] = cb;
		}
		else
		{
			auto cbc = make_shared<CBattleCallback>(gs, color, this);
			battleCallbacks[color] = cbc;
			if(color == PlayerColor(0))
				battleints[color] = CDynLibHandler::getNewBattleAI(settings["server"]["neutralAI"].String());
			else
				battleints[color] = CDynLibHandler::getNewBattleAI("StupidAI");
			battleints[color]->init(cbc.get());
		}
	}

	if(si->mode == StartInfo::DUEL)
	{
		boost::unique_lock<boost::recursive_mutex> un(*LOCPLINT->pim);
		CPlayerInterface *p = new CPlayerInterface(PlayerColor::NEUTRAL); //TODO: check if neutral really works -- was -1, but CPlayerInterface seems to cooperate with this value not too well
		p->observerInDuelMode = true;
		battleints[PlayerColor::UNFLAGGABLE] = playerint[PlayerColor::UNFLAGGABLE] = p;
		privilagedBattleEventReceivers.push_back(p);
		GH.curInt = p;
		auto cb = make_shared<CCallback>(gs, boost::optional<PlayerColor>(), this);
		battleCallbacks[PlayerColor::NEUTRAL] = callbacks[PlayerColor::NEUTRAL] = cb;
		p->init(cb.get());
		battleStarted(gs->curB);
	}
	else
	{
		loadNeutralBattleAI();
	}

	serv->addStdVecItems(gs);
	hotSeat = (humanPlayers > 1);

// 	std::vector<FileInfo> scriptModules;
// 	CFileUtility::getFilesWithExt(scriptModules, LIB_DIR "/scripting", "." LIB_EXT);
// 	BOOST_FOREACH(FileInfo &m, scriptModules)
// 	{
// 		CScriptingModule * nm = CDynLibHandler::getNewScriptingModule(m.name);
// 		privilagedGameEventReceivers.push_back(nm);
// 		privilagedBattleEventReceivers.push_back(nm);
// 		nm->giveActionCB(this);
// 		nm->giveInfoCB(this);
// 		nm->init();
// 
// 		erm = nm; //something tells me that there'll at most one module and it'll be ERM
// 	}
}

template <typename Handler>
void CClient::serialize( Handler &h, const int version )
{
	h & hotSeat;
	if(h.saving)
	{
		ui8 players = playerint.size();
		h & players;

		for(auto i = playerint.begin(); i != playerint.end(); i++)
		{
			LOG_TRACE_PARAMS(logGlobal, "Saving player %s interface", i->first);
			assert(i->first == i->second->playerID);
			h & i->first & i->second->dllName & i->second->human;
			i->second->saveGame(dynamic_cast<COSer<CSaveFile>&>(h), version); 
			//evil cast that i still like better than sfinae-magic. If I had a "static if"...
		}
	}
	else
	{
		ui8 players = 0; //fix for uninitialized warning
		h & players;

		for(int i=0; i < players; i++)
		{
			std::string dllname;
			PlayerColor pid; 
			bool isHuman = false;

			h & pid & dllname & isHuman;
			LOG_TRACE_PARAMS(logGlobal, "Loading player %s interface", pid);

			CGameInterface *nInt = nullptr;
			if(dllname.length())
			{
				if(pid == PlayerColor::NEUTRAL)
				{
					//CBattleCallback * cbc = new CBattleCallback(gs, pid, this);//FIXME: unused?
					CBattleGameInterface *cbgi = CDynLibHandler::getNewBattleAI(dllname);
					battleints[pid] = cbgi;
					cbgi->init(cb);
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
				nInt = new CPlayerInterface(pid);
			}

			nInt->dllName = dllname;
			nInt->human = isHuman;
			nInt->playerID = pid;

			battleCallbacks[pid] = callbacks[pid] = make_shared<CCallback>(gs,pid,this);
			battleints[pid] = playerint[pid] = nInt;
			nInt->init(callbacks[pid].get());
			nInt->loadGame(dynamic_cast<CISer<CLoadFile>&>(h), version); //another evil cast, check above
		}

		if(!vstd::contains(battleints, PlayerColor::NEUTRAL))
			loadNeutralBattleAI();
	}
}

void CClient::handlePack( CPack * pack )
{			
	CBaseForCLApply *apply = applier->apps[typeList.getTypeID(pack)]; //find the applier
	if(apply)
	{
		boost::unique_lock<boost::recursive_mutex> guiLock(*LOCPLINT->pim);
		apply->applyOnClBefore(this,pack);
        logNetwork->traceStream() << "\tMade first apply on cl";
		gs->apply(pack);
        logNetwork->traceStream() << "\tApplied on gs";
		apply->applyOnClAfter(this,pack);
        logNetwork->traceStream() << "\tMade second apply on cl";
	}
	else
	{
        logNetwork->errorStream() << "Message cannot be applied, cannot find applier! TypeID " << typeList.getTypeID(pack);
	}
	delete pack;
}

void CClient::updatePaths()
{
	//TODO? lazy evaluation? paths now can get recalculated multiple times upon various game events
	const CGHeroInstance *h = getSelectedHero();
	if (h)//if we have selected hero...
		calculatePaths(h);
}

void CClient::finishCampaign( shared_ptr<CCampaignState> camp )
{
}

void CClient::proposeNextMission(shared_ptr<CCampaignState> camp)
{
	GH.pushInt(new CBonusSelection(camp));
}

void CClient::stopConnection()
{
	terminate = true;

	if (serv) //request closing connection
	{
        logNetwork->infoStream() << "Connection has been requested to be closed.";
		boost::unique_lock<boost::mutex>(*serv->wmx);
		CloseServer close_server;
		sendRequest(&close_server, PlayerColor::NEUTRAL);
        logNetwork->infoStream() << "Sent closing signal to the server";
	}

	if(connectionHandler)//end connection handler
	{
		if(connectionHandler->get_id() != boost::this_thread::get_id())
			connectionHandler->join();

        logNetwork->infoStream() << "Connection handler thread joined";

		delete connectionHandler;
		connectionHandler = NULL;
	}

	if (serv) //and delete connection
	{
		serv->close();
		delete serv;
		serv = NULL;
        logNetwork->warnStream() << "Our socket has been closed.";
	}
}

void CClient::battleStarted(const BattleInfo * info)
{
	BOOST_FOREACH(auto &battleCb, battleCallbacks)
	{
		if(vstd::contains(info->sides, battleCb.first)  ||  battleCb.first >= PlayerColor::PLAYER_LIMIT)
			battleCb.second->setBattle(info);
	}
// 	BOOST_FOREACH(ui8 side, info->sides)
// 		if(battleCallbacks.count(side))
// 			battleCallbacks[side]->setBattle(info);

	CPlayerInterface * att, * def;
	if(vstd::contains(playerint, info->sides[0]) && playerint[info->sides[0]]->human)
		att = static_cast<CPlayerInterface*>( playerint[info->sides[0]] );
	else
		att = NULL;

	if(vstd::contains(playerint, info->sides[1]) && playerint[info->sides[1]]->human)
		def = static_cast<CPlayerInterface*>( playerint[info->sides[1]] );
	else
		def = NULL;

	if(att || def || gs->scenarioOps->mode == StartInfo::DUEL)
	{
		boost::unique_lock<boost::recursive_mutex> un(*LOCPLINT->pim);
		new CBattleInterface(info->belligerents[0], info->belligerents[1], info->heroes[0], info->heroes[1],
			Rect((screen->w - 800)/2, 
			     (screen->h - 600)/2, 800, 600), att, def);
	}

	if(vstd::contains(battleints,info->sides[0]))
		battleints[info->sides[0]]->battleStart(info->belligerents[0], info->belligerents[1], info->tile, info->heroes[0], info->heroes[1], 0);
	if(vstd::contains(battleints,info->sides[1]))
		battleints[info->sides[1]]->battleStart(info->belligerents[0], info->belligerents[1], info->tile, info->heroes[0], info->heroes[1], 1);
	if(vstd::contains(battleints,PlayerColor::UNFLAGGABLE))
		battleints[PlayerColor::UNFLAGGABLE]->battleStart(info->belligerents[0], info->belligerents[1], info->tile, info->heroes[0], info->heroes[1], 1);

	if(info->tacticDistance && vstd::contains(battleints,info->sides[info->tacticsSide]))
	{
		boost::thread(&CClient::commenceTacticPhaseForInt, this, battleints[info->sides[info->tacticsSide]]);
	}
}

void CClient::battleFinished()
{
	BOOST_FOREACH(PlayerColor side, gs->curB->sides)
		if(battleCallbacks.count(side))
			battleCallbacks[side]->setBattle(nullptr);
}

void CClient::loadNeutralBattleAI()
{
	battleints[PlayerColor::NEUTRAL] = CDynLibHandler::getNewBattleAI(settings["server"]["neutralAI"].String());
	auto cbc = make_shared<CBattleCallback>(gs, PlayerColor::NEUTRAL, this);
	battleCallbacks[PlayerColor::NEUTRAL] = cbc;
	battleints[PlayerColor::NEUTRAL]->init(cbc.get());
}

void CClient::commitPackage( CPackForClient *pack )
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

void CClient::calculatePaths(const CGHeroInstance *h)
{
	assert(h);
	boost::unique_lock<boost::mutex> pathLock(pathMx);
	gs->calculatePaths(h, *pathInfo);
}

void CClient::commenceTacticPhaseForInt(CBattleGameInterface *battleInt)
{
	setThreadName("CClient::commenceTacticPhaseForInt");
	try
	{
		battleInt->yourTacticPhase(gs->curB->tacticDistance);
		if(gs && !!gs->curB && gs->curB->tacticDistance) //while awaiting for end of tactics phase, many things can happen (end of battle... or game)
		{
			MakeAction ma(BattleAction::makeEndOFTacticPhase(gs->curB->playerToSide(battleInt->playerID)));
			sendRequest(&ma, battleInt->playerID);
		}
	} HANDLE_EXCEPTION
}

void CClient::invalidatePaths(const CGHeroInstance *h /*= NULL*/)
{
	if(!h || pathInfo->hero == h)
		pathInfo->isValid = false;
}

int CClient::sendRequest(const CPack *request, PlayerColor player)
{
	static ui32 requestCounter = 0;

	ui32 requestID = requestCounter++;
    logNetwork->traceStream() << boost::format("Sending a request \"%s\". It'll have an ID=%d.")
				% typeid(*request).name() % requestID;

	waitingRequest.pushBack(requestID);
	serv->sendPackToServer(*request, player, requestID);
	if(vstd::contains(playerint, player))
		playerint[player]->requestSent(dynamic_cast<const CPackForServer*>(request), requestID);

	return requestID;
}

void CClient::campaignMapFinished( shared_ptr<CCampaignState> camp )
{
	endGame(false);
	LOCPLINT = nullptr; //TODO free res

	GH.curInt = CGPreGame::create();
	auto & epilogue = camp->camp->scenarios[camp->mapsConquered.back()].epilog;
	auto finisher = [&]()
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

template void CClient::serialize( CISer<CLoadFile> &h, const int version );
template void CClient::serialize( COSer<CSaveFile> &h, const int version );

void CServerHandler::startServer()
{
	th.update();
	serverThread = new boost::thread(&CServerHandler::callServer, this); //runs server executable;
	if(verbose)
        logNetwork->infoStream() << "Setting up thread calling server: " << th.getDiff();
}

void CServerHandler::waitForServer()
{
	if(!serverThread)
		startServer();

	th.update();
	intpr::scoped_lock<intpr::interprocess_mutex> slock(shared->sr->mutex);
	while(!shared->sr->ready)
	{
		shared->sr->cond.wait(slock);
	}
	if(verbose)
        logNetwork->infoStream() << "Waiting for server: " << th.getDiff();
}

CConnection * CServerHandler::connectToServer()
{
	if(!shared->sr->ready)
		waitForServer();

	th.update(); //put breakpoint here to attach to server before it does something stupid
	CConnection *ret = justConnectToServer(settings["server"]["server"].String(), port);

	if(verbose)
        logNetwork->infoStream()<<"\tConnecting to the server: "<<th.getDiff();

	return ret;
}

CServerHandler::CServerHandler(bool runServer /*= false*/)
{
	serverThread = NULL;
	shared = NULL;
	port = boost::lexical_cast<std::string>(settings["server"]["port"].Float());
	verbose = true;

	boost::interprocess::shared_memory_object::remove("vcmi_memory"); //if the application has previously crashed, the memory may not have been removed. to avoid problems - try to destroy it
	try
	{
		shared = new SharedMem();
    } HANDLE_EXCEPTIONC(logNetwork->errorStream() << "Cannot open interprocess memory: ";)
}

CServerHandler::~CServerHandler()
{
	delete shared;
	delete serverThread; //detaches, not kills thread
}

void CServerHandler::callServer()
{
	setThreadName("CServerHandler::callServer");
	std::string logName = VCMIDirs::get().localPath() + "/server_log.txt";
	std::string comm = VCMIDirs::get().serverPath() + " " + port + " > " + logName;
	int result = std::system(comm.c_str());
	if (result == 0)
        logNetwork->infoStream() << "Server closed correctly";
	else
	{
        logNetwork->errorStream() << "Error: server failed to close correctly or crashed!";
        logNetwork->errorStream() << "Check " << logName << " for more info";
		exit(1);// exit in case of error. Othervice without working server VCMI will hang
	}
}

CConnection * CServerHandler::justConnectToServer(const std::string &host, const std::string &port)
{
	CConnection *ret = NULL;
	while(!ret)
	{
		try
		{
            logNetwork->infoStream() << "Establishing connection...";
			ret = new CConnection(	host.size() ? host : settings["server"]["server"].String(), 
									port.size() ? port : boost::lexical_cast<std::string>(settings["server"]["port"].Float()), 
									NAME);
		}
		catch(...)
		{
            logNetwork->errorStream() << "\nCannot establish connection! Retrying within 2 seconds";
			SDL_Delay(2000);
		}
	}
	return ret;
}
