#include "stdafx.h"
#include "../lib/CCampaignHandler.h"
#include "../global.h"
#include "../lib/Connection.h"
#include "../lib/CArtHandler.h"
#include "../lib/CDefObjInfoHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CTownHandler.h"
#include "../lib/CObjectHandler.h"
#include "../lib/CBuildingHandler.h"
#include "../lib/CSpellHandler.h"
#include "../lib/CCreatureHandler.h"
#include "zlib.h"
#ifndef __GNUC__
#include <tchar.h>
#endif
#include "CVCMIServer.h"
#include "../StartInfo.h"
#include "../lib/map.h"
#include "../lib/Interprocess.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/VCMIDirs.h"
#include "CGameHandler.h"
#include "../lib/CMapInfo.h"
#include "../lib/CondSh.h"

std::string RESULTS_PATH = "./results.txt",
	LOGS_DIR = ".";
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
	: host(Host), listeningThreads(0), acceptor(Acceptor), upcomingConnection(NULL),
	  curmap(NULL), curStartInfo(NULL), state(RUNNING)
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
				acceptor->get_io_service().reset();
				acceptor->get_io_service().poll();
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

	upcomingConnection = new TSocket(acceptor->get_io_service());
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
	tcp::socket * s = new tcp::socket(acceptor->get_io_service());
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
			tcp::socket * s = new tcp::socket(acceptor->get_io_service());
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

//memory monitoring
CondSh<bool> testMem;

volatile int monitringRes; //0 -- left AI violated mem limit; 1 -- right AI violated mem limit; 2 -- no mem limit violation;

bool memViolated(const int pid, const int refpid, const int limit) {
	char call[1000], c2[1000];
	//sprintf(call, "if (( `cat /proc/%d/statm | cut -d' ' -f1` > `cat /proc/%d/statm | cut -d' ' -f1` + %d )); then cat /kle; else echo b; fi;", pid, refpid, limit);
	sprintf(call, "/proc/%d/statm", pid);
	sprintf(c2, "/proc/%d/statm", refpid);
	std::ifstream proc(call),
		ref(c2);
	int procUsage, refUsage;
	proc >> procUsage;
	ref >> refUsage;
	
	return procUsage > refUsage + limit;
	//return 0 != ::system(call);
}

void memoryMonitor(int lAIpid, int rAIpid, int refPid) 
{
	setThreadName(-1, "memoryMonitor");
	const int MAX_MEM = 20000; //in blocks (of, I hope, 4096 B)
	monitringRes = 2;
	tlog0 << "Monitor is activated\n";
	try {
		while(testMem.get()) {
			tlog0 << "Monitor is active 12\n";
			if(memViolated(lAIpid, refPid, MAX_MEM)) {
				monitringRes = 0;
				break;
			}
			if(memViolated(rAIpid, refPid, MAX_MEM)) {
				monitringRes = 1;
				break;
			}
			//sleep(3);
			boost::this_thread::sleep(boost::posix_time::seconds(3));
			tlog0 << "Monitor is active 34\n";
		}
	}
	catch(...) {
		tlog0 << "Monitor is throwing something...\n";
	}
	tlog0 << "Monitor is closing\n";
}
////

void CVCMIServer::startDuel(const std::string &battle, const std::string &leftAI, const std::string &rightAI, int howManyClients)
{
	std::map<CConnection *, si32> pidsFromConns;
	si32 PIDs[3] = {0}; //[0] left [1] right; [2] reference
	//we need three connections
	std::vector<boost::thread*> threads(howManyClients, (boost::thread*)NULL);
	std::vector<CConnection*> conns(howManyClients, (CConnection*)NULL);

	for (int i = 0; i < howManyClients ; i++)
	{
		boost::system::error_code error;

		tlog0<<"Listening for connections at port " << acceptor->local_endpoint().port() << std::endl;
		tcp::socket * s = new tcp::socket(acceptor->get_io_service());
		acceptor->accept(*s, error);

		if (error)
		{
			tlog2<<"Got connection but there is an error " << std::endl << error;
			i--;
			delNull(s);
		}
		else
		{
			tlog0<<"We've accepted someone... " << std::endl;
			conns[i] = new CConnection(s, NAME);
			tlog0<<"Got connection!" << std::endl;
		}
	}

	StartInfo si;
	si.mode = StartInfo::DUEL;
	si.mapname = battle;
	

	tlog0 << "Preparing gh!\n";
	CGameHandler *gh = new CGameHandler();
	gh->init(&si,std::time(NULL));
	gh->ais[0] = leftAI;
	gh->ais[1] = rightAI;

	BOOST_FOREACH(CConnection *c, conns)
	{
		ui8 player = gh->conns.size();
		tlog0 << boost::format("Preparing connection %d!\n") % (int)player;
		c->connectionID = player;
		c->addStdVecItems(gh->gs, VLC);
		gh->connections[player] = c;
		gh->conns.insert(c);
		gh->states.addPlayer(player);
		*c << si;
		*c >> pidsFromConns[c];

		std::set<int> pom;
		pom.insert(player);
		threads[player] = new boost::thread(boost::bind(&CGameHandler::handleConnection, gh, pom, boost::ref(*c)));
	}

	boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
	tlog0 << boost::format("Sending start info to connections!\n");
	int aisSoFar = 0;
	for (int i = 0; i < howManyClients ; i++)
	{
		tlog0 << "Connection nr " << i;
		CConnection *c = conns[i];
		
		if(c->contactName.find("client") != std::string::npos)
		{
			tlog0 << " is a visualization client!\n";
			*c << std::string() << ui8(254);
		}
		else
		{
			if(aisSoFar < 2)
			{
				tlog0 << " will run " << (aisSoFar ? "right" : "left") << " AI " << std::endl;
				*c << gh->ais[aisSoFar] << ui8(aisSoFar);
				PIDs[aisSoFar] = pidsFromConns[c];
				aisSoFar++;
			}
			else
			{
				tlog0 << " will serve as a memory reference.\n";
				*c << std::string() << ui8(254);
				PIDs[2] = pidsFromConns[c];
			}
		}
	}

	//TODO monitor memory of PIDs
	testMem.set(true);
	boost::thread* memMon = new boost::thread(boost::bind(memoryMonitor, PIDs[0], PIDs[1], PIDs[2]));

	std::string logFName = LOGS_DIR + "/duel_log.vdat";
	tlog0 << "Logging battle activities (for replay possibility) in " << logFName << std::endl;
	gh->gameLog = new CSaveFile(logFName);
	gh->gameLog->smartPointerSerialization = false;
	*gh->gameLog << battle << leftAI << rightAI << ui8('$');
	
	tlog0 << "Starting battle!\n";
	gh->runBattle();
	tlog0 << "Battle over!\n";
	tlog0 << "Waiting for connections to close\n";
	
	testMem.set(false);
	memMon->join();
	delNull(memMon);
	tlog0 << "Memory violation checking result: " << monitringRes << std::endl;
	BOOST_FOREACH(boost::thread *t, threads)
	{
		t->join();
		delNull(t);
	}
	
	tlog0 << "Removing gh\n";
	delNull(gh);
	tlog0 << "Removed gh!\n";

	tlog0 << "Dying...\n";
	exit(0);
}

#ifndef __GNUC__
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char** argv)
#endif
{
	if(argc >= 6)
		LOGS_DIR = argv[5];

	std::string logName = LOGS_DIR + "/" + "VCMI_Server_log.txt";
	logfile = new std::ofstream(logName.c_str());
	console = new CConsoleHandler;
	//boost::thread t(boost::bind(&CConsoleHandler::run,::console));


	initDLL(console,logfile);
	srand ( (unsigned int)time(NULL) );
	try
	{
		io_service io_service;
		CVCMIServer server;
		if(argc == 6 || argc == 7)
		{
			RESULTS_PATH = argv[4];
			tlog1 << "Results path: " << RESULTS_PATH << std::endl;
			tlog1 << "Logs path: " << RESULTS_PATH << std::endl;
			server.startDuel(argv[1], argv[2], argv[3], argc-3);
		}
		else
			server.startDuel("b1.json", "StupidAI", "StupidAI", 2);

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
