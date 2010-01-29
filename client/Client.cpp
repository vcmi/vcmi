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
#include "../mapHandler.h"
#include "CConfigHandler.h"
#include "Client.h"
#include "GUIBase.h"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <sstream>

#undef DLL_EXPORT
#define DLL_EXPORT
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

class CBaseForCLApply
{
public:
	virtual void applyOnClAfter(CClient *cl, void *pack) const =0; 
	virtual void applyOnClBefore(CClient *cl, void *pack) const =0; 
	virtual ~CBaseForCLApply(){}
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

class CCLApplier
{
public:
	std::map<ui16,CBaseForCLApply*> apps; 

	CCLApplier()
	{
		registerTypes2(*this);
	}
	~CCLApplier()
	{
		std::map<ui16,CBaseForCLApply*>::iterator iter;

		for(iter = apps.begin(); iter != apps.end(); iter++)
			delete iter->second;
	}
	template<typename T> void registerType(const T * t=NULL)
	{
		ui16 ID = typeList.registerType(t);
		apps[ID] = new CApplyOnCL<T>;
	}

} *applier = NULL;

void CClient::init()
{
	connectionHandler = NULL;
	pathInfo = NULL;
	applier = new CCLApplier;
	IObjectInterface::cb = this;
	serv = NULL;
	gs = NULL;
	cb = NULL;
	terminate = false;
	boost::interprocess::shared_memory_object::remove("vcmi_memory"); //if the application has previously crashed, the memory may not have been removed. to avoid problems - try to destroy it
	try
	{
		shared = new SharedMem();
	} HANDLE_EXCEPTIONC(tlog1 << "Cannot open interprocess memory: ";)
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
	delete shared;
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
			pack = retreivePack(); //get the package from the server
			
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

void CClient::stop()
{
	// Game is ending
	// Tell the network thread to reach a stable state
	terminate = true;
	LOCPLINT->pim->lock();
	endGame();
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

void CClient::endGame()
{
	tlog0 << "\n\nEnding current game!" << std::endl;
	GH.curInt = NULL;
	GH.topInt()->deactivate();
	GH.listInt.clear();
	GH.objsToBlit.clear();

	delete CGI->mh;
	CGI->mh = NULL;

	delete CGI->state;
	CGI->state = NULL;

	LOCPLINT = NULL;
	while (!playerint.empty())
	{
		delete playerint.begin()->second;
		playerint.erase(playerint.begin());
	}

	BOOST_FOREACH(CCallback *cb, callbacks)
	{
		delete cb;
	}

	if (serv) 
	{
		tlog3 << "Connection has been requested to be closed.\n";
		boost::unique_lock<boost::mutex>(*serv->wmx);
		*serv << &CloseServer();
		tlog3 << "Sent closing signal to the server\n";

		serv->close();
		delete serv;
		serv = NULL;
		tlog3 << "Our socket has been closed." << std::endl;
	}

	connectionHandler->join();
	delete connectionHandler;
	connectionHandler = NULL;
}

void CClient::loadGame( const std::string & fname )
{
	tlog0 <<"\n\nLoading procedure started!\n\n";

	timeHandler tmh;

	char portc[10];
	SDL_itoa(conf.cc.port,portc,10);
	runServer(portc); //create new server
	tlog0 <<"Restarting server: "<<tmh.getDif()<<std::endl;

	{
		ui32 ver;
		char sig[8];
		CMapHeader dum;
		CGI->mh = new CMapHandler();

		CLoadFile lf(fname + ".vlgm1");
		lf >> sig >> dum >> *sig;
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
		tlog0 <<"Initing maphandler: "<<tmh.getDif()<<std::endl;
	}

	waitForServer();
	tlog0 <<"Waiting for server: "<<tmh.getDif()<<std::endl;

	serv = new CConnection(conf.cc.server,portc,NAME);
	serv->setGS(gs);
	tlog0 <<"Setting up connection: "<<tmh.getDif()<<std::endl;

	ui8 pom8;
	*serv << ui8(3) << ui8(1); //load game; one client
	*serv << fname;
	*serv >> pom8;
	if(pom8) 
		throw "Server cannot open the savegame!";
	else
		tlog0 << "Server opened savegame properly.\n";

	*serv << ui8(gs->scenarioOps->playerInfos.size()+1); //number of players + neutral
	for(size_t i=0;i<gs->scenarioOps->playerInfos.size();i++) 
	{
		*serv << ui8(gs->scenarioOps->playerInfos[i].color); //players
	}
	*serv << ui8(255); // neutrals
	tlog0 <<"Sent info to server: "<<tmh.getDif()<<std::endl;
	
	{
		CLoadFile lf(fname + ".vcgm1");
		lf >> *this;
	}
	//for (size_t i=0; i<gs->scenarioOps->playerInfos.size();++i) //initializing interfaces for players
	//{ 
	//	ui8 color = gs->scenarioOps->playerInfos[i].color;
	//	CCallback *cb = new CCallback(gs,color,this);
	//	if(!gs->scenarioOps->playerInfos[i].human) {
	//		playerint[color] = static_cast<CGameInterface*>(CAIHandler::getNewAI(cb,conf.cc.defaultAI));
	//	}
	//	else {
	//		playerint[color] = new CPlayerInterface(color,i);
	//	}
	//	gs->currentPlayer = color;
	//	playerint[color]->init(cb);
	//	tlog0 <<"Setting up interface for player "<< (int)color <<": "<<tmh.getDif()<<std::endl;
	//}
	//playerint[255] =  CAIHandler::getNewAI(cb,conf.cc.defaultAI);
	//playerint[255]->init(new CCallback(gs,255,this));
	//tlog0 <<"Setting up interface for neutral \"player\"" << tmh.getDif() << std::endl;


}

int CClient::getCurrentPlayer()
{
	return gs->currentPlayer;
}

int CClient::getSelectedHero()
{
	return IGameCallback::getSelectedHero(getCurrentPlayer())->id;
}

void CClient::newGame( CConnection *con, StartInfo *si )
{
	if (con == NULL) 
	{
		timeHandler pomtime;
		char portc[10];
		SDL_itoa(conf.cc.port,portc,10);
		CClient::runServer(portc);
		tlog0<<"Preparing shared memory and starting server: "<<pomtime.getDif()<<std::endl;

		pomtime.getDif();//reset timers

		//wait until server is ready
		tlog0<<"Waiting for server... ";
		waitForServer();
		tlog0 << pomtime.getDif()<<std::endl;
		while(!con)
		{
			try
			{
				tlog0 << "Establishing connection...\n";
				con = new CConnection(conf.cc.server,portc,NAME);
			}
			catch(...)
			{
				tlog1 << "\nCannot establish connection! Retrying within 2 seconds" <<std::endl;
				SDL_Delay(2000);
			}
		}
		THC tlog0<<"\tConnecting to the server: "<<pomtime.getDif()<<std::endl;
	}

	timeHandler tmh;
	CGI->state = new CGameState();
	tlog0 <<"\tGamestate: "<<tmh.getDif()<<std::endl;
	serv = con;
	serv->setGS(CGI->state);
	CConnection &c(*con);
	////////////////////////////////////////////////////
	ui8 pom8;
	c << ui8(2) << ui8(1); //new game; one client
	c << *si;
	c >> pom8;
	if(pom8) 
		throw "Server cannot open the map!";
	else
		tlog0 << "Server opened map properly.\n";
	c << ui8(si->playerInfos.size()+1); //number of players + neutral
	for(size_t i=0;i<si->playerInfos.size();i++) 
	{
		c << ui8(si->playerInfos[i].color); //players
	}
	c << ui8(255); // neutrals


	ui32 seed, sum;
	std::string mapname;
	c >> mapname >> sum >> seed;
	tlog0 <<"\tSending/Getting info to/from the server: "<<tmh.getDif()<<std::endl;

	Mapa * mapa = new Mapa(mapname);
	tlog0 <<"Reading and detecting map file (together): "<<tmh.getDif()<<std::endl;
	tlog0 << "\tServer checksum for "<<mapname <<": "<<sum << std::endl;
	tlog0 << "\tOur checksum for the map: "<< mapa->checksum << std::endl;

	if(mapa->checksum != sum)
	{
		tlog1 << "Wrong map checksum!!!" << std::endl;
		throw std::string("Wrong checksum");
	}
	tlog0 << "\tUsing random seed: "<<seed << std::endl;

	gs = CGI->state;
	gs->scenarioOps = si;
	gs->init(si,mapa,seed);

	CGI->mh = new CMapHandler();
	tlog0 <<"Initializing GameState (together): "<<tmh.getDif()<<std::endl;
	CGI->mh->map = mapa;
	tlog0 <<"Creating mapHandler: "<<tmh.getDif()<<std::endl;
	CGI->mh->init();
	pathInfo = new CPathsInfo(int3(mapa->width, mapa->height, mapa->twoLevel+1));
	tlog0 <<"Initializing mapHandler (together): "<<tmh.getDif()<<std::endl;

	for (size_t i=0; i<gs->scenarioOps->playerInfos.size();++i) //initializing interfaces for players
	{ 
		ui8 color = gs->scenarioOps->playerInfos[i].color;
		CCallback *cb = new CCallback(gs,color,this);
		if(!gs->scenarioOps->playerInfos[i].human) {
			playerint[color] = static_cast<CGameInterface*>(CAIHandler::getNewAI(cb,conf.cc.defaultAI));
		}
		else {
			playerint[color] = new CPlayerInterface(color,i);
		}
		gs->currentPlayer = color;
		playerint[color]->init(cb);
	}
	playerint[255] =  CAIHandler::getNewAI(cb,conf.cc.defaultAI);
	playerint[255]->init(new CCallback(gs,255,this));
}

void CClient::runServer(const char * portc)
{
	static std::string comm = std::string(BIN_DIR PATH_SEPARATOR SERVER_NAME " ") + portc + " > server_log.txt"; //needs to be static, if not - will be probably destroyed before new thread reads it
	boost::thread servthr(boost::bind(system,comm.c_str())); //runs server executable; 	//TODO: will it work on non-windows platforms?
}

void CClient::waitForServer()
{
	intpr::scoped_lock<intpr::interprocess_mutex> slock(shared->sr->mutex);
	while(!shared->sr->ready)
	{
		shared->sr->cond.wait(slock);
	}
}

template <typename Handler>
void CClient::serialize( Handler &h, const int version )
{
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
				nInt = new CPlayerInterface(pid,i);

			playerint[pid] = nInt;
			nInt->init(callback);
			nInt->serialize(h, version);
		}
	}
}

CPack * CClient::retreivePack()
{
	CPack *ret = NULL;
	boost::unique_lock<boost::mutex> lock(*serv->rmx);
	tlog5 << "Listening... ";
	*serv >> ret;
	tlog5 << "\treceived server message of type " << typeid(*ret).name() << std::endl;
	return ret;
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

template void CClient::serialize( CISer<CLoadFile> &h, const int version );
template void CClient::serialize( COSer<CSaveFile> &h, const int version );
