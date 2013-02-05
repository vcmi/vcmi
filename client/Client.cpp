#include "StdInc.h"

#include "CMusicHandler.h"
#include "../lib/Mapping/CCampaignHandler.h"
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
#include "../lib/Mapping/CMap.h"
#include "../lib/JsonNode.h"
#include "mapHandler.h"
#include "../lib/CConfigHandler.h"
#include "Client.h"
#include "CPreGame.h"
#include "BattleInterface/CBattleInterface.h"
#include "../lib/CThreadHelper.h"
#include "../lib/CScriptingModule.h"
#include "../lib/RegisterTypes.h"
#include "UIFramework/CGuiHandler.h"

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

static CApplier<CBaseForCLApply> *applier = NULL;

void CClient::init()
{
	hotSeat = false;
	connectionHandler = NULL;
	pathInfo = NULL;
	applier = new CApplier<CBaseForCLApply>;
	registerTypes2(*applier);
	IObjectInterface::cb = this;
	serv = NULL;
	gs = NULL;
	cb = NULL;
	erm = NULL;
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
	delete pathInfo;
	delete applier;
}

void CClient::waitForMoveAndSend(TPlayerColor color)
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
		tlog5 << "Wait for move thread was interrupted and no action will be send. Was a battle ended by spell?\n";
		return;
	}
	HANDLE_EXCEPTION
	tlog1 << "We should not be here!" << std::endl;
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
		tlog3 << "Lost connection to server, ending listening thread!\n";
		tlog1 << e.what() << std::endl;
		if(!terminate) //rethrow (-> boom!) only if closing connection was unexpected
		{
			tlog1 << "Something wrong, lost connection while game is still ongoing...\n";
			throw;
		}
	}
}

void CClient::save(const std::string & fname)
{
	if(gs->curB)
	{
		tlog1 << "Game cannot be saved during battle!\n";
		return;
	}

	SaveGame save_game(fname);
	sendRequest((CPackForClient*)&save_game, 255);
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
	tlog0 << "Closed connection." << std::endl;

	GH.curInt = NULL;
	LOCPLINT->terminate_cond.setn(true);
	{
		boost::unique_lock<boost::recursive_mutex> un(*LOCPLINT->pim);
		tlog0 << "\n\nEnding current game!" << std::endl;
		if(GH.topInt())
			GH.topInt()->deactivate();
		GH.listInt.clear();
		GH.objsToBlit.clear();
		GH.statusbar = NULL;
		tlog0 << "Removed GUI." << std::endl;


		delete CGI->mh;
		const_cast<CGameInfo*>(CGI)->mh = NULL;

		const_cast<CGameInfo*>(CGI)->state.dellNull();
		tlog0 << "Deleted mapHandler and gameState." << std::endl;
		LOCPLINT = NULL;
	}
	while (!playerint.empty())
	{
		CGameInterface *pint = playerint.begin()->second;
		playerint.erase(playerint.begin());
		delete pint;
	}

	callbacks.clear();
	tlog0 << "Deleted playerInts." << std::endl;

	tlog0 << "Client stopped." << std::endl;
}

void CClient::loadGame( const std::string & fname )
{
	tlog0 <<"\n\nLoading procedure started!\n\n";

	CServerHandler sh;
	sh.startServer();

	CStopWatch tmh;
	{
		char sig[8];
		CMapHeader dum;
		const_cast<CGameInfo*>(CGI)->mh = new CMapHandler();
		StartInfo *si;

		CLoadFile lf(CResourceHandler::get()->getResourceName(ResourceID(fname, EResType::LIB_SAVEGAME)));
		lf >> sig >> dum >> si;
		tlog0 <<"Reading save signature: "<<tmh.getDiff()<<std::endl;
		
		lf >> *VLC;
		const_cast<CGameInfo*>(CGI)->setFromLib();
		tlog0 <<"Reading handlers: "<<tmh.getDiff()<<std::endl;

		lf >> gs;
		tlog0 <<"Reading gamestate: "<<tmh.getDiff()<<std::endl;

		const_cast<CGameInfo*>(CGI)->state = gs;
		const_cast<CGameInfo*>(CGI)->mh->map = gs->map;
		pathInfo = new CPathsInfo(int3(gs->map->width, gs->map->height, gs->map->twoLevel ? 2 : 1));
		CGI->mh->init();

		tlog0 <<"Initing maphandler: "<<tmh.getDiff()<<std::endl;
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
		tlog0 << "Server opened savegame properly.\n";

	*serv << ui32(gs->scenarioOps->playerInfos.size()+1); //number of players + neutral
	for(auto it = gs->scenarioOps->playerInfos.begin(); 
		it != gs->scenarioOps->playerInfos.end(); ++it)
	{
		*serv << ui8(it->first); //players
	}
	*serv << ui8(GameConstants::NEUTRAL_PLAYER);
	tlog0 <<"Sent info to server: "<<tmh.getDiff()<<std::endl;

	serv->enableStackSendingByID();
	serv->disableSmartPointerSerialization();

	{
		CLoadFile lf(CResourceHandler::get()->getResourceName(ResourceID(fname, EResType::CLIENT_SAVEGAME)));
		lf >> *this;
	}
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

	CStopWatch tmh;
	const_cast<CGameInfo*>(CGI)->state = new CGameState();
	tlog0 <<"\tGamestate: "<<tmh.getDiff()<<std::endl;
	CConnection &c(*serv);
	////////////////////////////////////////////////////

	if(networkMode == SINGLE)
	{
		ui8 pom8;
		c << ui8(2) << ui8(1); //new game; one client
		c << *si;
		c >> pom8;
		if(pom8) 
			throw std::runtime_error("Server cannot open the map!");
		else
			tlog0 << "Server opened map properly.\n";
	}

	c >> si;
	tlog0 <<"\tSending/Getting info to/from the server: "<<tmh.getDiff()<<std::endl;
	c.enableStackSendingByID();
	c.disableSmartPointerSerialization();

	// Initialize game state
	gs = const_cast<CGameInfo*>(CGI)->state;
	gs->scenarioOps = si;
	gs->init(si);
	tlog0 <<"Initializing GameState (together): "<<tmh.getDiff()<<std::endl;

	// Now after possible random map gen, we know exact player count.
	// Inform server about how many players client handles
	std::set<TPlayerColor> myPlayers;
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
		myPlayers.insert(GameConstants::NEUTRAL_PLAYER);
	c << myPlayers;

	// Init map handler
	if(gs->map)
	{
		const_cast<CGameInfo*>(CGI)->mh = new CMapHandler();
		CGI->mh->map = gs->map;
		tlog0 <<"Creating mapHandler: "<<tmh.getDiff()<<std::endl;
		CGI->mh->init();
		pathInfo = new CPathsInfo(int3(gs->map->width, gs->map->height, gs->map->twoLevel ? 2 : 1));
		tlog0 <<"Initializing mapHandler (together): "<<tmh.getDiff()<<std::endl;
	}

	int humanPlayers = 0;
	int sensibleAILimit = settings["session"]["oneGoodAI"].Bool() ? 1 : GameConstants::PLAYER_LIMIT;
	for(auto it = gs->scenarioOps->playerInfos.begin(); 
		it != gs->scenarioOps->playerInfos.end(); ++it)//initializing interfaces for players
	{ 
		TPlayerColor color = it->first;
		gs->currentPlayer = color;
		if(!vstd::contains(myPlayers, color))
			continue;

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
				tlog1 << "Player " << (int)color << " will be lead by " << AItoGive << std::endl;
			}
			else 
			{
				playerint[color] = new CPlayerInterface(color);
				humanPlayers++;
			}
			battleints[color] = playerint[color];

			playerint[color]->init(cb.get());
			battleCallbacks[color] = callbacks[color] = cb;
		}
		else
		{
			auto cbc = make_shared<CBattleCallback>(gs, color, this);
			battleCallbacks[color] = cbc;
			if(!color)
				battleints[color] = CDynLibHandler::getNewBattleAI(settings["server"]["neutralAI"].String());
			else
				battleints[color] = CDynLibHandler::getNewBattleAI("StupidAI");
			battleints[color]->init(cbc.get());
		}
	}

	if(si->mode == StartInfo::DUEL)
	{
		boost::unique_lock<boost::recursive_mutex> un(*LOCPLINT->pim);
		CPlayerInterface *p = new CPlayerInterface(-1);
		p->observerInDuelMode = true;
		battleints[254] = playerint[254] = p;
		privilagedBattleEventReceivers.push_back(p);
		GH.curInt = p;
		auto cb = make_shared<CCallback>(gs, -1, this);
		battleCallbacks[-1] = callbacks[-1] = cb;
		p->init(cb.get());
		battleStarted(gs->curB);
	}
	else
	{
		loadNeutralBattleAI();
	}

	serv->addStdVecItems(const_cast<CGameInfo*>(CGI)->state);
	hotSeat = (humanPlayers > 1);

// 	std::vector<FileInfo> scriptModules;
// 	CFileUtility::getFilesWithExt(scriptModules, LIB_DIR "/Scripting", "." LIB_EXT);
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

		for(std::map<ui8,CGameInterface *>::iterator i = playerint.begin(); i != playerint.end(); i++)
		{
			h & i->first & i->second->dllName;
			i->second->serialize(h,version);
		}
	}
	else
	{
		ui8 players = 0; //fix for uninitialized warning
		h & players;

		for(int i=0; i < players; i++)
		{
			std::string dllname;
			ui8 pid = 0; //fix for uninitialized warning
			h & pid & dllname;


			CGameInterface *nInt = NULL;
			if(dllname.length())
			{
				if(pid == GameConstants::NEUTRAL_PLAYER)
				{
					//CBattleCallback * cbc = new CBattleCallback(gs, pid, this);//FIXME: unused?
					CBattleGameInterface *cbgi = CDynLibHandler::getNewBattleAI(dllname);
					battleints[pid] = cbgi;
					cbgi->init(cb);
					//TODO? consider serialization 
					continue;
				}
				else
					nInt = CDynLibHandler::getNewAI(dllname);
			}
			else
				nInt = new CPlayerInterface(pid);

			battleCallbacks[pid] = callbacks[pid] = make_shared<CCallback>(gs,pid,this);
			battleints[pid] = playerint[pid] = nInt;
			nInt->init(callbacks[pid].get());
			nInt->serialize(h, version);
		}

		if(!vstd::contains(battleints, GameConstants::NEUTRAL_PLAYER))
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
		tlog5 << "\tMade first apply on cl\n";
		gs->apply(pack);
		tlog5 << "\tApplied on gs\n";
		apply->applyOnClAfter(this,pack);
		tlog5 << "\tMade second apply on cl\n";
	}
	else
	{
		tlog1 << "Message cannot be applied, cannot find applier! TypeID " << typeList.getTypeID(pack) << std::endl;
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
	endGame(false);
	LOCPLINT = nullptr; //TODO free res
	GH.pushInt(new CBonusSelection(camp));
	GH.curInt = CGPreGame::create();
}

void CClient::stopConnection()
{
	terminate = true;

	if (serv) //request closing connection
	{
		tlog0 << "Connection has been requested to be closed.\n";
		boost::unique_lock<boost::mutex>(*serv->wmx);
		CloseServer close_server;
		sendRequest(&close_server, 255);
		tlog0 << "Sent closing signal to the server\n";
	}

	if(connectionHandler)//end connection handler
	{
		if(connectionHandler->get_id() != boost::this_thread::get_id())
			connectionHandler->join();

		tlog0 << "Connection handler thread joined" << std::endl;

		delete connectionHandler;
		connectionHandler = NULL;
	}

	if (serv) //and delete connection
	{
		serv->close();
		delete serv;
		serv = NULL;
		tlog3 << "Our socket has been closed." << std::endl;
	}
}

void CClient::battleStarted(const BattleInfo * info)
{
	BOOST_FOREACH(auto &battleCb, battleCallbacks)
	{
		if(vstd::contains(info->sides, battleCb.first)  ||  battleCb.first >= GameConstants::PLAYER_LIMIT)
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
	if(vstd::contains(battleints,254))
		battleints[254]->battleStart(info->belligerents[0], info->belligerents[1], info->tile, info->heroes[0], info->heroes[1], 1);

	if(info->tacticDistance && vstd::contains(battleints,info->sides[info->tacticsSide]))
	{
		boost::thread(&CClient::commenceTacticPhaseForInt, this, battleints[info->sides[info->tacticsSide]]);
	}
}

void CClient::battleFinished()
{
	BOOST_FOREACH(ui8 side, gs->curB->sides)
		if(battleCallbacks.count(side))
			battleCallbacks[side]->setBattle(nullptr);
}

void CClient::loadNeutralBattleAI()
{
	battleints[GameConstants::NEUTRAL_PLAYER] = CDynLibHandler::getNewBattleAI(settings["server"]["neutralAI"].String());
	auto cbc = make_shared<CBattleCallback>(gs, GameConstants::NEUTRAL_PLAYER, this);
	battleCallbacks[GameConstants::NEUTRAL_PLAYER] = cbc;
	battleints[GameConstants::NEUTRAL_PLAYER]->init(cbc.get());
}

void CClient::commitPackage( CPackForClient *pack )
{
	CommitPackage cp;
	cp.freePack = false;
	cp.packToCommit = pack;
	sendRequest(&cp, 255);
}

int CClient::getLocalPlayer() const
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
			MakeAction ma(BattleAction::makeEndOFTacticPhase(battleInt->playerID));
			sendRequest(&ma, battleInt->playerID);
		}
	} HANDLE_EXCEPTION
}

void CClient::invalidatePaths(const CGHeroInstance *h /*= NULL*/)
{
	if(!h || pathInfo->hero == h)
		pathInfo->isValid = false;
}

int CClient::sendRequest(const CPack *request, TPlayerColor player)
{
	static ui32 requestCounter = 0;

	ui32 requestID = requestCounter++;
	tlog5 << boost::format("Sending a request \"%s\". It'll have an ID=%d.\n") 
				% typeid(*request).name() % requestID;

	waitingRequest.pushBack(requestID);
	serv->sendPackToServer(*request, player, requestID);
	if(vstd::contains(playerint, player))
		playerint[player]->requestSent(dynamic_cast<const CPackForServer*>(request), requestID);

	return requestID;
}

template void CClient::serialize( CISer<CLoadFile> &h, const int version );
template void CClient::serialize( COSer<CSaveFile> &h, const int version );

void CServerHandler::startServer()
{
	th.update();
	
	serverThread = new boost::thread(&CServerHandler::callServer, this); //runs server executable;
	if(verbose)
		tlog0 << "Setting up thread calling server: " << th.getDiff() << std::endl;
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
		tlog0 << "Waiting for server: " << th.getDiff() << std::endl;
}

CConnection * CServerHandler::connectToServer()
{
	if(!shared->sr->ready)
		waitForServer();

	th.update(); //put breakpoint here to attach to server before it does something stupid
	CConnection *ret = justConnectToServer(settings["server"]["server"].String(), port);

	if(verbose)
		tlog0<<"\tConnecting to the server: "<<th.getDiff()<<std::endl;

	return ret;
}

CServerHandler::CServerHandler(bool runServer /*= false*/)
{
	serverThread = NULL;
	shared = NULL;
	port = boost::lexical_cast<std::string>(settings["server"]["port"].Float());
	verbose = false;

	boost::interprocess::shared_memory_object::remove("vcmi_memory"); //if the application has previously crashed, the memory may not have been removed. to avoid problems - try to destroy it
	try
	{
		shared = new SharedMem();
	} HANDLE_EXCEPTIONC(tlog1 << "Cannot open interprocess memory: ";)
}

CServerHandler::~CServerHandler()
{
	delete shared;
	delete serverThread; //detaches, not kills thread
}

void CServerHandler::callServer()
{
	setThreadName("CServerHandler::callServer");
	std::string logName = GVCMIDirs.UserPath + "/server_log.txt";
	std::string comm = GameConstants::BIN_DIR + GameConstants::PATH_SEPARATOR + GameConstants::SERVER_NAME + " " + port + " > " + logName;
	int result = std::system(comm.c_str());
	if (result == 0)
		tlog1 << "Server closed correctly\n";
	else
	{
		tlog0 << "Error: server failed to close correctly or crashed!\n";
		tlog0 << "Check " << logName << " for more info\n";
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
			tlog0 << "Establishing connection...\n";
			ret = new CConnection(	host.size() ? host : settings["server"]["server"].String(), 
									port.size() ? port : boost::lexical_cast<std::string>(settings["server"]["port"].Float()), 
									NAME);
		}
		catch(...)
		{
			tlog1 << "\nCannot establish connection! Retrying within 2 seconds" << std::endl;
			SDL_Delay(2000);
		}
	}
	return ret;
}
