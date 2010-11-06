#include "../hch/CCampaignHandler.h"
#include "../CCallback.h"
#include "../CConsoleHandler.h"
#include "CGameInfo.h"
#include "../lib/CGameState.h"
#include "CPlayerInterface.h"
#include "../StartInfo.h"
#include "../hch/CArtHandler.h"
#include "../hch/CDefObjInfoHandler.h"
#include "../hch/CGeneralTextHandler.h"
#include "../hch/CHeroHandler.h"
#include "../hch/CTownHandler.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CBuildingHandler.h"
#include "../hch/CSpellHandler.h"
#include "../lib/Connection.h"
#include "../lib/Interprocess.h"
#include "../lib/NetPacks.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/map.h"
#include "mapHandler.h"
#include "CConfigHandler.h"
#include "Client.h"
#include "GUIBase.h"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#include "CPreGame.h"

#define NOT_LIB
#include "../lib/RegisterTypes.cpp"
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
	terminate = false;
}

CClient::CClient(void)
:waitingRequest(false)
{
	init();
}

CClient::CClient(CConnection *con, StartInfo *si)
:waitingRequest(false)
{
	init();
	newGame(con,si);
}

CClient::~CClient(void)
{
	delete pathInfo;
	delete applier;
}

void CClient::waitForMoveAndSend(int color)
{
	try
	{
		BattleAction ba = playerint[color]->activeStack(gs->curB->activeStack);
		*serv << &MakeAction(ba);
		return;
	}HANDLE_EXCEPTION
	tlog1 << "We should not be here!" << std::endl;
}

void CClient::run()
{
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
	catch (const std::exception& e)
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

	*serv << &SaveGame(fname);
}

#include <fstream>
void initVillagesCapitols(Mapa * map)
{
	std::ifstream ifs(DATA_DIR "/config/townsDefs.txt");
	int ccc;
	ifs>>ccc;
	for(int i=0;i<ccc*2;i++)
	{
		CGDefInfo *n;
		if(i<ccc)
		{
			n = CGI->state->villages[i];
			map->defy.push_back(CGI->state->forts[i]);
		}
		else 
			n = CGI->state->capitols[i%ccc];

		ifs >> n->name;
		if(!n)
			tlog1 << "*HUGE* Warning - missing town def for " << i << std::endl;
		else
			map->defy.push_back(n);
	}
}

void CClient::endGame( bool closeConnection /*= true*/ )
{
	// Game is ending
	// Tell the network thread to reach a stable state
	GH.curInt = NULL;
	LOCPLINT->terminate_cond.setn(true);
	LOCPLINT->pim->lock();

	tlog0 << "\n\nEnding current game!" << std::endl;
	if(GH.topInt())
		GH.topInt()->deactivate();
	GH.listInt.clear();
	GH.objsToBlit.clear();
	GH.statusbar = NULL;
	tlog0 << "Removed GUI." << std::endl;

	delete CGI->mh;
	CGI->mh = NULL;

	delete CGI->state;
	CGI->state = NULL;
	tlog0 << "Deleted mapHandler and gameState." << std::endl;

	LOCPLINT = NULL;
	while (!playerint.empty())
	{
		CGameInterface *pint = playerint.begin()->second;
		playerint.erase(playerint.begin());
		delete pint;
	}

	BOOST_FOREACH(CCallback *cb, callbacks)
	{
		delete cb;
	}
	tlog0 << "Deleted playerInts." << std::endl;

	if(closeConnection)
		stopConnection();

	tlog0 << "Client stopped." << std::endl;
}

void CClient::loadGame( const std::string & fname )
{
	tlog0 <<"\n\nLoading procedure started!\n\n";

	CServerHandler sh;
	sh.startServer();

	timeHandler tmh;
	{
		char sig[8];
		CMapHeader dum;
		CGI->mh = new CMapHandler();
		StartInfo *si;

		CLoadFile lf(fname + ".vlgm1");
		lf >> sig >> dum >> si;
		tlog0 <<"Reading save signature: "<<tmh.getDif()<<std::endl;
		
		lf >> *VLC;
		CGI->setFromLib();
		tlog0 <<"Reading handlers: "<<tmh.getDif()<<std::endl;

		lf >> gs;
		tlog0 <<"Reading gamestate: "<<tmh.getDif()<<std::endl;

		CGI->state = gs;
		CGI->mh->map = gs->map;
		pathInfo = new CPathsInfo(int3(gs->map->width, gs->map->height, gs->map->twoLevel+1));
		CGI->mh->init();
		initVillagesCapitols(gs->map);

		tlog0 <<"Initing maphandler: "<<tmh.getDif()<<std::endl;
	}
	serv = sh.connectToServer();
	serv->addStdVecItems(gs);

	tmh.update();
	ui8 pom8;
	*serv << ui8(3) << ui8(1); //load game; one client
	*serv << fname;
	*serv >> pom8;
	if(pom8) 
		throw "Server cannot open the savegame!";
	else
		tlog0 << "Server opened savegame properly.\n";

	*serv << ui32(gs->scenarioOps->playerInfos.size()+1); //number of players + neutral
	for(std::map<int, PlayerSettings>::iterator it = gs->scenarioOps->playerInfos.begin(); 
		it != gs->scenarioOps->playerInfos.end(); ++it)
	{
		*serv << ui8(it->first); //players
	}
	*serv << ui8(255); // neutrals
	tlog0 <<"Sent info to server: "<<tmh.getDif()<<std::endl;
	
	{
		CLoadFile lf(fname + ".vcgm1");
		lf >> *this;
	}
}

int CClient::getCurrentPlayer()
{
	return gs->currentPlayer;
}

int CClient::getSelectedHero()
{
	if(const CGHeroInstance *selHero = IGameCallback::getSelectedHero(getCurrentPlayer()))
		return selHero->id;
	else
		return -1;
}

void CClient::newGame( CConnection *con, StartInfo *si )
{
	enum {SINGLE, HOST, GUEST} networkMode = SINGLE;
	std::set<ui8> myPlayers;

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

	for(std::map<int, PlayerSettings>::iterator it =si->playerInfos.begin(); 
		it != si->playerInfos.end(); ++it)
	{
		if(networkMode == SINGLE												//single - one client has all player
			|| networkMode != SINGLE && serv->connectionID == it->second.human	//multi - client has only "its players"
			|| networkMode == HOST && it->second.human == false)				//multi - host has all AI players
		{
			myPlayers.insert(ui8(it->first)); //add player
		}
	}
	if(networkMode != GUEST)
		myPlayers.insert(255); //neutral



	timeHandler tmh;
	CGI->state = new CGameState();
	tlog0 <<"\tGamestate: "<<tmh.getDif()<<std::endl;
	CConnection &c(*serv);
	////////////////////////////////////////////////////

	if(networkMode == SINGLE)
	{
		ui8 pom8;
		c << ui8(2) << ui8(1); //new game; one client
		c << *si;
		c >> pom8;
		if(pom8) 
			throw "Server cannot open the map!";
		else
			tlog0 << "Server opened map properly.\n";
	}

	c << myPlayers;


	ui32 seed, sum;
	c >> si	>> sum >> seed;
	tlog0 <<"\tSending/Getting info to/from the server: "<<tmh.getDif()<<std::endl;
	tlog0 << "\tUsing random seed: "<<seed << std::endl;

	gs = CGI->state;
	gs->scenarioOps = si;
	gs->init(si, sum, seed);

	CGI->mh = new CMapHandler();
	tlog0 <<"Initializing GameState (together): "<<tmh.getDif()<<std::endl;
	CGI->mh->map = gs->map;
	tlog0 <<"Creating mapHandler: "<<tmh.getDif()<<std::endl;
	CGI->mh->init();
	initVillagesCapitols(gs->map);
	pathInfo = new CPathsInfo(int3(gs->map->width, gs->map->height, gs->map->twoLevel+1));
	tlog0 <<"Initializing mapHandler (together): "<<tmh.getDif()<<std::endl;

	int humanPlayers = 0;
	for(std::map<int, PlayerSettings>::iterator it = gs->scenarioOps->playerInfos.begin(); 
		it != gs->scenarioOps->playerInfos.end(); ++it)//initializing interfaces for players
	{ 
		ui8 color = it->first;
		if(!vstd::contains(myPlayers, color))
			continue;

		CCallback *cb = new CCallback(gs,color,this);
		if(!it->second.human) 
		{
			playerint[color] = static_cast<CGameInterface*>(CAIHandler::getNewAI(cb,conf.cc.defaultAI));
		}
		else 
		{
			playerint[color] = new CPlayerInterface(color);
			humanPlayers++;
		}
		gs->currentPlayer = color;
		playerint[color]->init(cb);
	}

	serv->addStdVecItems(CGI->state);
	hotSeat = (humanPlayers > 1);

	playerint[255] =  CAIHandler::getNewAI(cb,conf.cc.defaultAI);
	playerint[255]->init(new CCallback(gs,255,this));
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
		ui8 players;
		h & players;

		for(int i=0; i < players; i++)
		{
			std::string dllname;
			ui8 pid;
			h & pid & dllname;

			CCallback *callback = new CCallback(gs,pid,this);
			callbacks.insert(callback);
			CGameInterface *nInt = NULL;


			if(dllname.length())
				nInt = CAIHandler::getNewAI(callback,dllname);
			else
				nInt = new CPlayerInterface(pid);

			playerint[pid] = nInt;
			nInt->init(callback);
			nInt->serialize(h, version);
		}
	}
}

void CClient::handlePack( CPack * pack )
{			
	CBaseForCLApply *apply = applier->apps[typeList.getTypeID(pack)]; //find the applier
	if(apply)
	{
		apply->applyOnClBefore(this,pack);
		tlog5 << "\tMade first apply on cl\n";
		gs->apply(pack);
		tlog5 << "\tApplied on gs\n";
		apply->applyOnClAfter(this,pack);
		tlog5 << "\tMade second apply on cl\n";
	}
	else
	{
		tlog1 << "Message cannot be applied, cannot find applier!\n";
	}
	delete pack;
}

void CClient::updatePaths()
{	
	const CGHeroInstance *h = getHero(getSelectedHero());
	if (h)//if we have selected hero...
		gs->calculatePaths(h, *pathInfo);
}

void CClient::finishCampaign( CCampaignState * camp )
{
}

void CClient::proposeNextMission( CCampaignState * camp )
{
	GH.pushInt(new CBonusSelection(camp));
	GH.curInt = CGP;
}

void CClient::stopConnection()
{
	terminate = true;

	if (serv) 
	{
		tlog0 << "Connection has been requested to be closed.\n";
		boost::unique_lock<boost::mutex>(*serv->wmx);
		*serv << &CloseServer();
		tlog0 << "Sent closing signal to the server\n";

		serv->close();
		delete serv;
		serv = NULL;
		tlog3 << "Our socket has been closed." << std::endl;
	}

	if(connectionHandler)
	{
		if(connectionHandler->get_id() != boost::this_thread::get_id())
			connectionHandler->join();

		tlog0 << "Connection handler thread joined" << std::endl;

		delete connectionHandler;
		connectionHandler = NULL;
	}
}

template void CClient::serialize( CISer<CLoadFile> &h, const int version );
template void CClient::serialize( COSer<CSaveFile> &h, const int version );

void CServerHandler::startServer()
{
	th.update();
	
	serverThread = new boost::thread(&CServerHandler::callServer, this); //runs server executable; 	//TODO: will it work on non-windows platforms?
	if(verbose)
		tlog0 << "Setting up thread calling server: " << th.getDif() << std::endl;
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
		tlog0 << "Waiting for server: " << th.getDif() << std::endl;
}

CConnection * CServerHandler::connectToServer()
{
	if(!shared->sr->ready)
		waitForServer();

	th.update();
	CConnection *ret = justConnectToServer(conf.cc.server, port);

	if(verbose)
		tlog0<<"\tConnecting to the server: "<<th.getDif()<<std::endl;

	return ret;
}

CServerHandler::CServerHandler(bool runServer /*= false*/)
{
	serverThread = NULL;
	shared = NULL;
	port = boost::lexical_cast<std::string>(conf.cc.port);

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
	std::string comm = std::string(BIN_DIR PATH_SEPARATOR SERVER_NAME " ") + port + " > server_log.txt";
	std::system(comm.c_str());
	tlog0 << "Server finished\n";
}

CConnection * CServerHandler::justConnectToServer(const std::string &host, const std::string &port)
{
	CConnection *ret = NULL;
	while(!ret)
	{
		try
		{
			tlog0 << "Establishing connection...\n";
			ret = new CConnection(	host.size() ? host : conf.cc.server, 
									port.size() ? port : boost::lexical_cast<std::string>(conf.cc.port), 
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
