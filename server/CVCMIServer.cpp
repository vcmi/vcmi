/*
 * CVCMIServer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include <boost/asio.hpp>

#include "../lib/filesystem/Filesystem.h"
#include "../lib/mapping/CCampaignHandler.h"
#include "../lib/CThreadHelper.h"
#include "../lib/serializer/Connection.h"
#include "../lib/CModHandler.h"
#include "../lib/CArtHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CTownHandler.h"
#include "../lib/CBuildingHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CCreatureHandler.h"
#include "zlib.h"
#include "CVCMIServer.h"
#include "../lib/StartInfo.h"
#include "../lib/mapping/CMap.h"
#include "../lib/rmg/CMapGenOptions.h"
#ifdef VCMI_ANDROID
#include "lib/CAndroidVMHelper.h"
#else
#include "../lib/Interprocess.h"
#endif
#include "../lib/VCMI_Lib.h"
#include "../lib/VCMIDirs.h"
#include "CGameHandler.h"
#include "../lib/mapping/CMapInfo.h"
#include "../lib/GameConstants.h"
#include "../lib/logging/CBasicLogConfigurator.h"
#include "../lib/CConfigHandler.h"
#include "../lib/ScopeGuard.h"

#include "../lib/UnlockGuard.h"

#if defined(__GNUC__) && !defined (__MINGW32__) && !defined(VCMI_ANDROID)
#include <execinfo.h>
#endif

std::string NAME_AFFIX = "server";
std::string NAME = GameConstants::VCMI_VERSION + std::string(" (") + NAME_AFFIX + ')'; //application name
std::atomic<bool> serverShuttingDown(false);

boost::program_options::variables_map cmdLineOptions;

static void vaccept(boost::asio::ip::tcp::acceptor *ac, boost::asio::ip::tcp::socket *s, boost::system::error_code *error)
{
	ac->accept(*s,*error);
}



CPregameServer::CPregameServer(CConnection * Host, TAcceptor * Acceptor)
	: host(Host), listeningThreads(0), acceptor(Acceptor), upcomingConnection(nullptr),
	  curmap(nullptr), curStartInfo(nullptr), state(RUNNING)
{
	initConnection(host);
}

void CPregameServer::handleConnection(CConnection *cpc)
{
	setThreadName("CPregameServer::handleConnection");
	try
	{
		while(!cpc->receivedStop)
		{
			CPackForSelectionScreen *cpfs = nullptr;
			*cpc >> cpfs;

			logNetwork->infoStream() << "Got package to announce " << typeid(*cpfs).name() << " from " << *cpc;

			boost::unique_lock<boost::recursive_mutex> queueLock(mx);
			bool quitting = dynamic_ptr_cast<QuitMenuWithoutStarting>(cpfs),
				startingGame = dynamic_ptr_cast<StartWithCurrentSettings>(cpfs);
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
				auto unlock = vstd::makeUnlockGuard(mx);
				while(state == RUNNING) boost::this_thread::sleep(boost::posix_time::milliseconds(50));
			}
			else if(quitting) // Server must be stopped if host is leaving from lobby to avoid crash
			{
				serverShuttingDown = true;
			}
		}
	}
	catch (const std::exception& e)
	{
		boost::unique_lock<boost::recursive_mutex> queueLock(mx);
		logNetwork->errorStream() << *cpc << " dies... \nWhat happened: " << e.what();
	}

	boost::unique_lock<boost::recursive_mutex> queueLock(mx);
	if(state != ENDING_AND_STARTING_GAME)
	{
		connections -= cpc;

		//notify other players about leaving
		auto pl = new PlayerLeft();
		pl->playerID = cpc->connectionID;
		announceTxt(cpc->name + " left the game");
		toAnnounce.push_back(pl);

		if(connections.empty())
		{
			logNetwork->error("Last connection lost, server will close itself...");
			boost::this_thread::sleep(boost::posix_time::seconds(2)); //we should never be hasty when networking
			state = ENDING_WITHOUT_START;
		}
	}

	logNetwork->infoStream() << "Thread listening for " << *cpc << " ended";
	listeningThreads--;
	vstd::clear_pointer(cpc->handler);
}

void CPregameServer::run()
{
	startListeningThread(host);
	start_async_accept();

	while(state == RUNNING)
	{
		{
			boost::unique_lock<boost::recursive_mutex> myLock(mx);
			while(!toAnnounce.empty())
			{
				processPack(toAnnounce.front());
				toAnnounce.pop_front();
			}

// 			//we end sending thread if we ordered all our connections to stop
// 			ending = true;
// 			for(CPregameConnection *pc : connections)
// 				if(!pc->sendStop)
// 					ending = false;

			if(state != RUNNING)
			{
				logNetwork->info("Stopping listening for connections...");
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

	logNetwork->info("Thread handling connections ended");

	if(state == ENDING_AND_STARTING_GAME)
	{
		logNetwork->info("Waiting for listening thread to finish...");
		while(listeningThreads) boost::this_thread::sleep(boost::posix_time::milliseconds(50));
		logNetwork->info("Preparing new game");
	}
}

CPregameServer::~CPregameServer()
{
	delete acceptor;
	delete upcomingConnection;

	for(CPackForSelectionScreen *pack : toAnnounce)
		delete pack;

	toAnnounce.clear();

	//TODO pregameconnections
}

void CPregameServer::connectionAccepted(const boost::system::error_code& ec)
{
	if(ec)
	{
		logNetwork->info("Something wrong during accepting: %s", ec.message());
		return;
	}

	try
	{
		logNetwork->info("We got a new connection! :)");
		std::string name = NAME;
		CConnection *pc = new CConnection(upcomingConnection, name.append(" STATE_PREGAME"));
		initConnection(pc);
		upcomingConnection = nullptr;

		startListeningThread(pc);

		*pc << (ui8)pc->connectionID << curmap;

		announceTxt(pc->name + " joins the game");
		auto pj = new PlayerJoined();
		pj->playerName = pc->name;
		pj->connectionID = pc->connectionID;
		toAnnounce.push_back(pj);
	}
	catch(std::exception& e)
	{
		upcomingConnection = nullptr;
		logNetwork->info("I guess it was just my imagination!");
	}

	start_async_accept();
}

void CPregameServer::start_async_accept()
{
	assert(!upcomingConnection);
	assert(acceptor);

	upcomingConnection = new TSocket(acceptor->get_io_service());
	acceptor->async_accept(*upcomingConnection, std::bind(&CPregameServer::connectionAccepted, this, _1));
}

void CPregameServer::announceTxt(const std::string &txt, const std::string &playerName)
{
	logNetwork->info("%s says: %s", playerName, txt);
	ChatMessage cm;
	cm.playerName = playerName;
	cm.message = txt;

	boost::unique_lock<boost::recursive_mutex> queueLock(mx);
	toAnnounce.push_front(new ChatMessage(cm));
}

void CPregameServer::announcePack(const CPackForSelectionScreen &pack)
{
	for(CConnection *pc : connections)
		sendPack(pc, pack);
}

void CPregameServer::sendPack(CConnection * pc, const CPackForSelectionScreen & pack)
{
	if(!pc->sendStop)
	{
		logNetwork->infoStream() << "\tSending pack of type " << typeid(pack).name() << " to " << *pc;
		*pc << &pack;
	}

	if(dynamic_ptr_cast<QuitMenuWithoutStarting>(&pack))
	{
		pc->sendStop = true;
	}
	else if(dynamic_ptr_cast<StartWithCurrentSettings>(&pack))
	{
		pc->sendStop = true;
	}
}

void CPregameServer::processPack(CPackForSelectionScreen * pack)
{
	if(dynamic_ptr_cast<CPregamePackToHost>(pack))
	{
		sendPack(host, *pack);
	}
	else if(SelectMap *sm = dynamic_ptr_cast<SelectMap>(pack))
	{
		vstd::clear_pointer(curmap);
		curmap = sm->mapInfo;
		sm->free = false;
		announcePack(*pack);
	}
	else if(UpdateStartOptions *uso = dynamic_ptr_cast<UpdateStartOptions>(pack))
	{
		vstd::clear_pointer(curStartInfo);
		curStartInfo = uso->options;
		uso->free = false;
		announcePack(*pack);
	}
	else if(dynamic_ptr_cast<StartWithCurrentSettings>(pack))
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
	logNetwork->info("Pregame connection with player %s established!", c->name);
}

void CPregameServer::startListeningThread(CConnection * pc)
{
	listeningThreads++;
	pc->enterPregameConnectionMode();
	pc->handler = new boost::thread(&CPregameServer::handleConnection, this, pc);
}

CVCMIServer::CVCMIServer()
	: port(3030), io(new boost::asio::io_service()), firstConnection(nullptr), shared(nullptr)
{
	logNetwork->trace("CVCMIServer created!");
	if(cmdLineOptions.count("port"))
		port = cmdLineOptions["port"].as<ui16>();
	logNetwork->info("Port %d will be used", port);
	try
	{
		acceptor = new TAcceptor(*io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
	}
	catch(...)
	{
		logNetwork->info("Port %d is busy, trying to use random port instead", port);
		if(cmdLineOptions.count("run-by-client") && !cmdLineOptions.count("enable-shm"))
		{
			logNetwork->error("Cant pass port number to client without shared memory!", port);
			exit(0);
		}
		acceptor = new TAcceptor(*io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0));
		port = acceptor->local_endpoint().port();
	}
	logNetwork->info("Listening for connections at port %d", port);
}
CVCMIServer::~CVCMIServer()
{
	//delete io;
	//delete acceptor;
	//delete firstConnection;
}

CGameHandler * CVCMIServer::initGhFromHostingConnection(CConnection &c)
{
	auto gh = new CGameHandler();
	StartInfo si;
	c >> si; //get start options

	if(!si.createRandomMap())
	{
		bool mapFound = CResourceHandler::get()->existsResource(ResourceID(si.mapname, EResType::MAP));

		//TODO some checking for campaigns
		if(!mapFound && si.mode == StartInfo::NEW_GAME)
		{
			c << ui8(1); //WRONG!
			return nullptr;
		}
	}

	c << ui8(0); //OK!

	gh->init(&si);
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

	auto onExit = vstd::makeScopeGuard([&]()
	{
		vstd::clear_pointer(gh);
	});

	gh->run(false);
}

void CVCMIServer::newPregame()
{
	auto cps = new CPregameServer(firstConnection, acceptor);
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
		gh.init(cps->curStartInfo);

		for(CConnection *c : gh.conns)
			c->addStdVecItems(gh.gs);

		gh.run(false);
	}
}

void CVCMIServer::start()
{
#ifndef VCMI_ANDROID
	if(cmdLineOptions.count("enable-shm"))
	{
		std::string sharedMemoryName = "vcmi_memory";
		if(cmdLineOptions.count("enable-shm-uuid") && cmdLineOptions.count("uuid"))
		{
			sharedMemoryName += "_" + cmdLineOptions["uuid"].as<std::string>();
		}
		shared = new SharedMemory(sharedMemoryName);
	}
#endif

	boost::system::error_code error;
	for (;;)
	{
		try
		{
			auto s = new boost::asio::ip::tcp::socket(acceptor->get_io_service());
			boost::thread acc(std::bind(vaccept,acceptor,s,&error));
#ifdef VCMI_ANDROID
			{ // in block to clean-up vm helper after use, because we don't need to keep this thread attached to vm
				CAndroidVMHelper envHelper;
				envHelper.callStaticVoidMethod(CAndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS, "onServerReady");
				logNetwork->info("Sending server ready message to client");
			}
#else
			if(shared)
			{
				shared->sr->setToReadyAndNotify(port);
			}
#endif

			acc.join();
			if (error)
			{
				logNetwork->warnStream()<<"Got connection but there is an error " << error;
				return;
			}
			logNetwork->info("We've accepted someone... ");
			std::string name = NAME;
			firstConnection = new CConnection(s, name.append(" STATE_WAITING"));
			logNetwork->info("Got connection!");
			while(!serverShuttingDown)
			{
				ui8 mode;
				*firstConnection >> mode;
				switch (mode)
				{
				case 0:
					firstConnection->close();
					exit(0);
				case 1:
					firstConnection->close();
					return;
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
			break;
		}
		catch(std::exception& e)
		{
			vstd::clear_pointer(firstConnection);
			logNetwork->info("I guess it was just my imagination!");
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

	c >> clients >> fname; //how many clients should be connected

	{
		CLoadFile lf(*CResourceHandler::get("local")->getResourceName(ResourceID(fname, EResType::SERVER_SAVEGAME)), MINIMAL_SERIALIZATION_VERSION);
		gh.loadCommonState(lf);
		lf >> gh;
	}

	c << ui8(0);

	gh.conns.insert(firstConnection);

	for(int i=1; i<clients; i++)
	{
		auto s = make_unique<boost::asio::ip::tcp::socket>(acceptor->get_io_service());
		acceptor->accept(*s,error);
		if(error) //retry
		{
			logNetwork->warn("Cannot establish connection - retrying...");
			i--;
			continue;
		}

		gh.conns.insert(new CConnection(s.release(),NAME));
	}

	gh.run(true);
}



static void handleCommandOptions(int argc, char *argv[])
{
	namespace po = boost::program_options;
	po::options_description opts("Allowed options");
	opts.add_options()
		("help,h", "display help and exit")
		("version,v", "display version information and exit")
		("run-by-client", "indicate that server launched by client on same machine")
		("uuid", po::value<std::string>(), "")
		("enable-shm-uuid", "use UUID for shared memory identifier")
		("enable-shm", "enable usage of shared memory")
		("port", po::value<ui16>(), "port at which server will listen to connections from client");

	if(argc > 1)
	{
		try
		{
			po::store(po::parse_command_line(argc, argv, opts), cmdLineOptions);
		}
		catch(std::exception &e)
		{
			std::cerr << "Failure during parsing command-line options:\n" << e.what() << std::endl;
		}
	}

	po::notify(cmdLineOptions);
	if (cmdLineOptions.count("help"))
	{
		auto time = std::time(0);
		printf("%s - A Heroes of Might and Magic 3 clone\n", GameConstants::VCMI_VERSION.c_str());
		printf("Copyright (C) 2007-%d VCMI dev team - see AUTHORS file\n", std::localtime(&time)->tm_year + 1900);
		printf("This is free software; see the source for copying conditions. There is NO\n");
		printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
		printf("\n");
		std::cout << opts;
		exit(0);
	}

	if (cmdLineOptions.count("version"))
	{
		printf("%s\n", GameConstants::VCMI_VERSION.c_str());
		std::cout << VCMIDirs::get().genHelpString();
		exit(0);
	}
}

#if defined(__GNUC__) && !defined (__MINGW32__) && !defined(VCMI_ANDROID)
void handleLinuxSignal(int sig)
{
	const int STACKTRACE_SIZE = 100;
	void * buffer[STACKTRACE_SIZE];
	int ptrCount = backtrace(buffer, STACKTRACE_SIZE);
	char ** strings;

	logGlobal->error("Error: signal %d :", sig);
	strings = backtrace_symbols(buffer, ptrCount);
	if(strings == nullptr)
	{
		logGlobal->error("There are no symbols.");
	}
	else
	{
		for(int i = 0; i < ptrCount; ++i)
		{
			logGlobal->error(strings[i]);
		}
		free(strings);
	}

	_exit(EXIT_FAILURE);
}
#endif

int main(int argc, char * argv[])
{
#ifdef VCMI_APPLE
	// Correct working dir executable folder (not bundle folder) so we can use executable relative paths
	std::string executablePath = argv[0];
	std::string workDir = executablePath.substr(0, executablePath.rfind('/'));
	chdir(workDir.c_str());
#endif
	// Installs a sig sev segmentation violation handler
	// to log stacktrace
	#if defined(__GNUC__) && !defined (__MINGW32__) && !defined(VCMI_ANDROID)
	signal(SIGSEGV, handleLinuxSignal);
    #endif

	console = new CConsoleHandler();
	CBasicLogConfigurator logConfig(VCMIDirs::get().userCachePath() / "VCMI_Server_log.txt", console);
	logConfig.configureDefault();
	logGlobal->info(NAME);

	handleCommandOptions(argc, argv);
	preinitDLL(console);
	settings.init();
	logConfig.configure();

	loadDLLClasses();
	srand ( (ui32)time(nullptr) );
	try
	{
		boost::asio::io_service io_service;
		CVCMIServer server;

		try
		{
			while(!serverShuttingDown)
			{
				server.start();
			}
			io_service.run();
		}
		catch (boost::system::system_error &e) //for boost errors just log, not crash - probably client shut down connection
		{
			logNetwork->error(e.what());
			serverShuttingDown = true;
		}
		catch (...)
		{
			handleException();
		}
	}
	catch(boost::system::system_error &e)
	{
		logNetwork->error(e.what());
		//catch any startup errors (e.g. can't access port) errors
		//and return non-zero status so client can detect error
		throw;
	}
#ifdef VCMI_ANDROID
	CAndroidVMHelper envHelper;
	envHelper.callStaticVoidMethod(CAndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS, "killServer");
#endif
	vstd::clear_pointer(VLC);
	CResourceHandler::clear();
	return 0;
}

#ifdef VCMI_ANDROID

void CVCMIServer::create()
{
	const char * foo[1] = {"android-server"};
	main(1, const_cast<char **>(foo));
}

#endif
