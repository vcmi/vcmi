#include "../hch/CCampaignHandler.h"
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include "../global.h"
#include "../lib/Connection.h"
#include "../hch/CArtHandler.h"
#include "../hch/CDefObjInfoHandler.h"
#include "../hch/CGeneralTextHandler.h"
#include "../hch/CHeroHandler.h"
#include "../hch/CTownHandler.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CBuildingHandler.h"
#include "../hch/CSpellHandler.h"
#include "../hch/CCreatureHandler.h"
#include "zlib.h"
#ifndef __GNUC__
#include <tchar.h>
#endif
#include "CVCMIServer.h"
#include <boost/crc.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include "../StartInfo.h"
#include "../lib/map.h"
#include "../hch/CLodHandler.h" 
#include "../lib/Interprocess.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/VCMIDirs.h"
#include "CGameHandler.h"
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include "../lib/CMapInfo.h"

std::string NAME_AFFIX = "server";
std::string NAME = NAME_VER + std::string(" (") + NAME_AFFIX + ')'; //application name
using namespace boost;
using namespace boost::asio;
using namespace boost::asio::ip;
namespace intpr = boost::interprocess;
bool end2 = false;
int port = 3030;
VCMIDirs GVCMIDirs;

/*
 * CVCMIServer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

static void vaccept(tcp::acceptor *ac, tcp::socket *s, boost::system::error_code *error)
{
	ac->accept(*s,*error);
}



CPregameServer::CPregameServer(CConnection *Host, TAcceptor *Acceptor /*= NULL*/)
	: host(Host), state(RUNNING), acceptor(Acceptor), upcomingConnection(NULL), curmap(NULL), listeningThreads(0),
	  curStartInfo(NULL)
{
	initConnection(host);
}

void CPregameServer::handleConnection(CConnection *cpc)
{
	try
	{
		while(!cpc->receivedStop)
		{
			CPackForSelectionScreen *cpfs = NULL;
			*cpc >> cpfs;

			tlog0 << "Got package to announce " << typeid(*cpfs).name() << " from " << *cpc << std::endl; 

			boost::unique_lock<boost::recursive_mutex> queueLock(mx);
			bool quitting = dynamic_cast<QuitMenuWithoutStarting*>(cpfs), 
				startingGame = dynamic_cast<StartWithCurrentSettings*>(cpfs);
			if(quitting || startingGame) //host leaves main menu or wants to start game -> we end
			{
				cpc->receivedStop = true;
				if(!cpc->sendStop)
					sendPack(cpc, *cpfs);

				if(cpc == host)
					toAnnounce.push_back(cpfs);
			}
			else
				toAnnounce.push_back(cpfs);

			if(startingGame)
			{
				//wait for sending thread to announce start
				mx.unlock();
				while(state == RUNNING) boost::this_thread::sleep(boost::posix_time::milliseconds(50));
				mx.lock();
			}
		}
	} 
	catch (const std::exception& e)
	{
		boost::unique_lock<boost::recursive_mutex> queueLock(mx);
		tlog0 << *cpc << " dies... \nWhat happened: " << e.what() << std::endl;
	}

	boost::unique_lock<boost::recursive_mutex> queueLock(mx);
	if(state != ENDING_AND_STARTING_GAME)
	{
		connections -= cpc;

		//notify other players about leaving
		PlayerLeft *pl = new PlayerLeft();
		pl->playerID = cpc->connectionID;
		announceTxt(cpc->name + " left the game");
		toAnnounce.push_back(pl);

		if(!connections.size())
		{
			tlog0 << "Last connection lost, server will close itself...\n";
			boost::this_thread::sleep(boost::posix_time::seconds(2)); //we should never be hasty when networking
			state = ENDING_WITHOUT_START;
		}
	}

	tlog0 << "Thread listening for " << *cpc << " ended\n";
	listeningThreads--;
	delNull(cpc->handler);
}

void CPregameServer::run()
{
	startListeningThread(host);
	start_async_accept();

	while(state == RUNNING)
	{
		{
			boost::unique_lock<boost::recursive_mutex> myLock(mx);
			while(toAnnounce.size())
			{
				processPack(toAnnounce.front());
				toAnnounce.pop_front();
			}

// 			//we end sending thread if we ordered all our connections to stop
// 			ending = true;
// 			BOOST_FOREACH(CPregameConnection *pc, connections)
// 				if(!pc->sendStop)
// 					ending = false;

			if(state != RUNNING)
			{
				tlog0 << "Stopping listening for connections...\n";
				acceptor->close();
			}

			if(acceptor)
			{
				acceptor->io_service().reset();
				acceptor->io_service().poll();
			}
		} //frees lock

		boost::this_thread::sleep(boost::posix_time::milliseconds(50));
	}

	tlog0 << "Thread handling connections ended\n";

	if(state == ENDING_AND_STARTING_GAME)
	{
		tlog0 << "Waiting for listening thread to finish...\n";
		while(listeningThreads) boost::this_thread::sleep(boost::posix_time::milliseconds(50));
		tlog0 << "Preparing new game\n";
	}
}

CPregameServer::~CPregameServer()
{
	delete acceptor;
	delete upcomingConnection;

	BOOST_FOREACH(CPackForSelectionScreen *pack, toAnnounce)
		delete pack;

	toAnnounce.clear();

	//TODO pregameconnections
}

void CPregameServer::connectionAccepted(const boost::system::error_code& ec)
{
	if(ec)
	{
		tlog0 << "Something wrong during accepting: " << ec.message() << std::endl;
		return;
	}

	tlog0 << "We got a new connection! :)\n";
	CConnection *pc = new CConnection(upcomingConnection, NAME);
	initConnection(pc);
	upcomingConnection = NULL;

	*pc << (ui8)pc->connectionID << curmap;

	startListeningThread(pc);

	announceTxt(pc->name + " joins the game");
	PlayerJoined *pj = new PlayerJoined();
	pj->playerName = pc->name;
	pj->connectionID = pc->connectionID;
	toAnnounce.push_back(pj);

	start_async_accept();
}

void CPregameServer::start_async_accept()
{
	assert(!upcomingConnection);
	assert(acceptor);

	upcomingConnection = new TSocket(acceptor->io_service());
	acceptor->async_accept(*upcomingConnection, boost::bind(&CPregameServer::connectionAccepted, this, boost::asio::placeholders::error));
}

void CPregameServer::announceTxt(const std::string &txt, const std::string &playerName /*= "system"*/)
{
	tlog0 << playerName << " says: " << txt << std::endl;
	ChatMessage cm;
	cm.playerName = playerName;
	cm.message = txt;

	boost::unique_lock<boost::recursive_mutex> queueLock(mx);
	toAnnounce.push_front(new ChatMessage(cm));
}

void CPregameServer::announcePack(const CPackForSelectionScreen &pack)
{
	BOOST_FOREACH(CConnection *pc, connections)
		sendPack(pc, pack);
}

void CPregameServer::sendPack(CConnection * pc, const CPackForSelectionScreen & pack)
{
	if(!pc->sendStop)
	{
		tlog0 << "\tSending pack of type " << typeid(pack).name() << " to " << *pc << std::endl;
		*pc << &pack;
	}

	if(dynamic_cast<const QuitMenuWithoutStarting*>(&pack))
	{
		pc->sendStop = true;
	}
	else if(dynamic_cast<const StartWithCurrentSettings*>(&pack))
	{
		pc->sendStop = true;
	}
}

void CPregameServer::processPack(CPackForSelectionScreen * pack)
{
	if(dynamic_cast<CPregamePackToHost*>(pack))
	{
		sendPack(host, *pack);
	}
	else if(SelectMap *sm = dynamic_cast<SelectMap*>(pack))
	{
		delNull(curmap);
		curmap = sm->mapInfo;
		sm->free = false;
		announcePack(*pack);
	}
	else if(UpdateStartOptions *uso = dynamic_cast<UpdateStartOptions*>(pack))
	{
		delNull(curStartInfo);
		curStartInfo = uso->options;
		uso->free = false;
		announcePack(*pack);
	}
	else if(dynamic_cast<const StartWithCurrentSettings*>(pack))
	{
		state = ENDING_AND_STARTING_GAME;
		announcePack(*pack);
	}
	else
		announcePack(*pack);

	delete pack;
}

void CPregameServer::initConnection(CConnection *c)
{
	*c >> c->name;
	connections.insert(c);
	tlog0 << "Pregame connection with player " << c->name << " established!" << std::endl;
}

void CPregameServer::startListeningThread(CConnection * pc)
{	
	listeningThreads++;
	pc->handler = new boost::thread(&CPregameServer::handleConnection, this, pc);
}

CVCMIServer::CVCMIServer()
: io(new io_service()), acceptor(new TAcceptor(*io, tcp::endpoint(tcp::v4(), port))), firstConnection(NULL)
{
	tlog4 << "CVCMIServer created!" <<std::endl;
}
CVCMIServer::~CVCMIServer()
{
	//delete io;
	//delete acceptor;
}

CGameHandler * CVCMIServer::initGhFromHostingConnection(CConnection &c)
{
	CGameHandler *gh = new CGameHandler();
	StartInfo si;
	c >> si; //get start options
	int problem;

#ifdef _MSC_VER
	FILE *f;
	problem = fopen_s(&f,si.mapname.c_str(),"r");
#else
	FILE * f = fopen(si.mapname.c_str(),"r");
	problem = !f;
#endif

	if(problem && si.mode == StartInfo::NEW_GAME) //TODO some checking for campaigns
	{
		c << ui8(problem); //WRONG!
		return NULL;
	}
	else
	{
		if(f)	
			fclose(f);
		c << ui8(0); //OK!
	}

	gh->init(&si,std::clock());
	c.addStdVecItems(gh->gs);
	gh->conns.insert(&c);

	return gh;
}

void CVCMIServer::newGame()
{
	CConnection &c = *firstConnection;
	ui8 clients;
	c >> clients; //how many clients should be connected 
	assert(clients == 1); //multi goes now by newPregame, TODO: custom lobbies

	CGameHandler *gh = initGhFromHostingConnection(c);
	gh->run(false);
	delNull(gh);
}

void CVCMIServer::newPregame()
{
	CPregameServer *cps = new CPregameServer(firstConnection, acceptor);
	cps->run();
	if(cps->state == CPregameServer::ENDING_WITHOUT_START)
	{
		delete cps;
		return;
	}

	if(cps->state == CPregameServer::ENDING_AND_STARTING_GAME)
	{
		CGameHandler gh;
		gh.conns = cps->connections;
		gh.init(cps->curStartInfo,std::clock());

		BOOST_FOREACH(CConnection *c, gh.conns)
			c->addStdVecItems(gh.gs);

		gh.run(false);
	}
}

void CVCMIServer::start()
{
	ServerReady *sr = NULL;
	intpr::mapped_region *mr;
	try
	{
		intpr::shared_memory_object smo(intpr::open_only,"vcmi_memory",intpr::read_write);
		smo.truncate(sizeof(ServerReady));
		mr = new intpr::mapped_region(smo,intpr::read_write);
		sr = reinterpret_cast<ServerReady*>(mr->get_address());
	}
	catch(...)
	{
		intpr::shared_memory_object smo(intpr::create_only,"vcmi_memory",intpr::read_write);
		smo.truncate(sizeof(ServerReady));
		mr = new intpr::mapped_region(smo,intpr::read_write);
		sr = new(mr->get_address())ServerReady();
	}

	boost::system::error_code error;
	tlog0<<"Listening for connections at port " << acceptor->local_endpoint().port() << std::endl;
	tcp::socket * s = new tcp::socket(acceptor->io_service());
	boost::thread acc(boost::bind(vaccept,acceptor,s,&error));
	sr->setToTrueAndNotify();
	delete mr;

	acc.join();
	if (error)
	{
		tlog2<<"Got connection but there is an error " << std::endl << error;
		return;
	}
	tlog0<<"We've accepted someone... " << std::endl;
	firstConnection = new CConnection(s,NAME);
	tlog0<<"Got connection!" << std::endl;
	while(!end2)
	{
		ui8 mode;
		*firstConnection >> mode;
		switch (mode)
		{
		case 0:
			firstConnection->close();
			exit(0);
			break;
		case 1:
			firstConnection->close();
			return;
			break;
		case 2:
			newGame();
			break;
		case 3:
			loadGame();
			break;
		case 4:
			newPregame();
			break;
		}
	}
}

void CVCMIServer::loadGame()
{
	CConnection &c = *firstConnection;
	std::string fname;
	CGameHandler gh;
	boost::system::error_code error;
	ui8 clients;

	c >> clients >> fname; //how many clients should be connected - TODO: support more than one

	{
		ui32 ver;
		char sig[8];
		CMapHeader dum;
		StartInfo *si;

		CLoadFile lf(fname + ".vlgm1");
		lf >> sig >> dum >> si;
		tlog0 <<"Reading save signature"<<std::endl;

		lf >> *VLC;
		tlog0 <<"Reading handlers"<<std::endl;

		lf >> (gh.gs);
		c.addStdVecItems(gh.gs);
		tlog0 <<"Reading gamestate"<<std::endl;
	}

	{
		CLoadFile lf(fname + ".vsgm1");
		lf >> gh;
	}

	c << ui8(0);

	CConnection* cc; //tcp::socket * ss;
	for(int i=0; i<clients; i++)
	{
		if(!i) 
		{
			cc = &c;
		}
		else
		{
			tcp::socket * s = new tcp::socket(acceptor->io_service());
			acceptor->accept(*s,error);
			if(error) //retry
			{
				tlog3<<"Cannot establish connection - retrying..." << std::endl;
				i--;
				continue;
			}
			cc = new CConnection(s,NAME);
			cc->addStdVecItems(gh.gs);
		}	
		gh.conns.insert(cc);
	}

	gh.run(true);
}

#ifndef __GNUC__
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char** argv)
#endif
{
	logfile = new std::ofstream("VCMI_Server_log.txt");
	console = new CConsoleHandler;
	//boost::thread t(boost::bind(&CConsoleHandler::run,::console));
	if(argc > 1)
	{
#ifdef _MSC_VER
		port = _tstoi(argv[1]);
#else
		port = _ttoi(argv[1]);
#endif
	}
	tlog0 << "Port " << port << " will be used." << std::endl;
	CLodHandler h3bmp;
	h3bmp.init(DATA_DIR "/Data/H3bitmap.lod", DATA_DIR "/Data");
	initDLL(console,logfile);
	srand ( (unsigned int)time(NULL) );
	try
	{
		io_service io_service;
		CVCMIServer server;
		while(!end2)
		{
			server.start();
		}
		io_service.run();
	} 
	catch(boost::system::system_error &e) //for boost errors just log, not crash - probably client shut down connection
	{
		tlog1 << e.what() << std::endl;
		end2 = true;
	}HANDLE_EXCEPTION
  return 0;
}
