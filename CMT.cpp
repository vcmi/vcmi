// CMT.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <cmath>
#include <string>
#include <vector>
#include <queue>
#include <cmath>
#include <boost/algorithm/string.hpp>
#include "boost/filesystem/operations.hpp"
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/thread.hpp>
#include "SDL_ttf.h"
#include "SDL_mixer.h"
#include "SDL_Extensions.h"
#include "SDL_framerate.h"
#include "CGameInfo.h"
#include "mapHandler.h"
#include "global.h"
#include "CPreGame.h"
#include "CCastleInterface.h"
#include "CConsoleHandler.h"
#include "CCursorHandler.h"
#include "CPathfinder.h"
#include "CGameState.h"
#include "CCallback.h"
#include "CPlayerInterface.h"
#include "CLuaHandler.h"
#include "CAdvmapInterface.h"
#include "hch/CBuildingHandler.h"
#include "hch/CVideoHandler.h"
#include "hch/CAbilityHandler.h"
#include "hch/CHeroHandler.h"
#include "hch/CCreatureHandler.h"
#include "hch/CSpellHandler.h"
#include "hch/CMusicHandler.h"
#include "hch/CLodHandler.h"
#include "hch/CDefHandler.h"
#include "hch/CAmbarCendamo.h"
#include "hch/CGeneralTextHandler.h"
#include "client/Graphics.h"
#include "client/Client.h"
#include "lib/Connection.h"
#include "lib/Interprocess.h"
#include "lib/VCMI_Lib.h"
std::string NAME = NAME_VER + std::string(" (client)");
DLL_EXPORT void initDLL(CLodHandler *b);
SDL_Surface * screen, * screen2;
extern SDL_Surface * CSDL_Ext::std32bppSurface;
std::queue<SDL_Event> events;
boost::mutex eventsM;
TTF_Font * TNRB16, *TNR, *GEOR13, *GEORXX, *GEORM, *GEOR16;
CLogger<0> log0;
CLogger<1> log1;
CLogger<2> log2;
CLogger<3> log3;
CLogger<4> log4;
CLogger<5> log5;
CConsoleHandler *console = NULL;
std::ostream *logfile = NULL;
namespace intpr = boost::interprocess;
void processCommand(const std::string &message);
#ifndef __GNUC__
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char** argv)
#endif
{ 
	boost::thread *console = NULL;
	if(argc>2)
	{
		std::cout << "Special mode without new support for console!" << std::endl;
	}
	else
	{
		logfile = new std::ofstream("VCMI_Client_log.txt");
		::console = new CConsoleHandler;
		*::console->cb = &processCommand;
		console = new boost::thread(boost::bind(&CConsoleHandler::run,::console));
	}
	log0 << "\tConsole and logifle ready!" << std::endl;
	int port;
	if(argc > 1)
	{
#ifdef _MSC_VER
		port = _tstoi(argv[1]);
#else
		port = _ttoi(argv[1]);
#endif
	}
	else
	{
		port = 3030;
		log0 << "Port " << port << " will be used." << std::endl;
	}
	std::cout.flags(ios::unitbuf);
	log0 << NAME << std::endl;
	srand ( time(NULL) );
	CPG=NULL;
	atexit(SDL_Quit);
	CGameInfo * cgi = CGI = new CGameInfo; //contains all global informations about game (texts, lodHandlers, map handler itp.)
	//CLuaHandler luatest;
	//luatest.test(); 
		//CBIKHandler cb;
		//cb.open("CSECRET.BIK");
	log0 << "Starting... " << std::endl;
	THC timeHandler tmh, total, pomtime;
	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_AUDIO)==0)
	{
		screen = SDL_SetVideoMode(800,600,24,SDL_SWSURFACE|SDL_DOUBLEBUF/*|SDL_FULLSCREEN*/);  //initializing important global surface
		log0 <<"\tInitializing screen: "<<pomtime.getDif();
			log0 << std::endl;
		SDL_WM_SetCaption(NAME.c_str(),""); //set window title
		#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			int rmask = 0xff000000;int gmask = 0x00ff0000;int bmask = 0x0000ff00;int amask = 0x000000ff;
		#else
			int rmask = 0x000000ff;	int gmask = 0x0000ff00;	int bmask = 0x00ff0000;	int amask = 0xff000000;
		#endif
		CSDL_Ext::std32bppSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, 1, 1, 32, rmask, gmask, bmask, amask);
		log0 << "\tInitializing minors: " << pomtime.getDif() << std::endl;
		TTF_Init();
		TNRB16 = TTF_OpenFont("Fonts" PATHSEPARATOR "tnrb.ttf",16);
		GEOR13 = TTF_OpenFont("Fonts" PATHSEPARATOR "georgia.ttf",13);
		GEOR16 = TTF_OpenFont("Fonts" PATHSEPARATOR "georgia.ttf",16);
		GEORXX = TTF_OpenFont("Fonts" PATHSEPARATOR "tnrb.ttf",22);
		GEORM = TTF_OpenFont("Fonts" PATHSEPARATOR "georgia.ttf",10);
		atexit(TTF_Quit);
		THC log0<<"\tInitializing fonts: "<<pomtime.getDif()<<std::endl;
		CMusicHandler * mush = new CMusicHandler;  //initializing audio
		mush->initMusics();
		//audio initialized 
		cgi->mush = mush;
		THC log0<<"\tInitializing sound: "<<pomtime.getDif()<<std::endl;
		THC log0<<"Initializing screen, fonts and sound handling: "<<tmh.getDif()<<std::endl;
		CDefHandler::Spriteh = cgi->spriteh = new CLodHandler();
		cgi->spriteh->init("Data" PATHSEPARATOR "H3sprite.lod","Sprites");
		BitmapHandler::bitmaph = cgi->bitmaph = new CLodHandler;
		cgi->bitmaph->init("Data" PATHSEPARATOR "H3bitmap.lod","Data");
		THC log0<<"Loading .lod files: "<<tmh.getDif()<<std::endl;
		initDLL(cgi->bitmaph,::console,logfile);
		CGI->arth = VLC->arth;
		CGI->creh = VLC->creh;
		CGI->townh = VLC->townh;
		CGI->heroh = VLC->heroh;
		CGI->objh = VLC->objh;
		CGI->spellh = VLC->spellh;
		CGI->dobjinfo = VLC->dobjinfo;
		CGI->buildh = VLC->buildh;
		log0<<"Initializing VCMI_Lib: "<<tmh.getDif()<<std::endl;
		//cgi->curh->initCursor();
		//cgi->curh->showGraphicCursor();
		pomtime.getDif();
		cgi->curh = new CCursorHandler; 
		cgi->curh->initCursor();
		//cgi->screenh = new CScreenHandler;
		//cgi->screenh->initScreen();
		log0<<"\tScreen handler: "<<pomtime.getDif()<<std::endl;
		CAbilityHandler * abilh = new CAbilityHandler;
		abilh->loadAbilities();
		cgi->abilh = abilh;
		log0<<"\tAbility handler: "<<pomtime.getDif()<<std::endl;
		log0<<"Preparing first handlers: "<<tmh.getDif()<<std::endl;
		pomtime.getDif();
		graphics = new Graphics();
		log0<<"\tMain graphics: "<<tmh.getDif()<<std::endl;
		std::vector<CDefHandler **> animacje;
		for(std::vector<CHeroClass *>::iterator i = cgi->heroh->heroClasses.begin();i!=cgi->heroh->heroClasses.end();i++)
			animacje.push_back(&((*i)->*(&CHeroClass::moveAnim)));
		graphics->loadHeroAnim(animacje);
		log0<<"\tHero animations: "<<tmh.getDif()<<std::endl;
		log0<<"Initializing game graphics: "<<tmh.getDif()<<std::endl;
		CMessage::init();
		cgi->generaltexth = new CGeneralTextHandler;
		cgi->generaltexth->load();
		log0<<"Preparing more handlers: "<<tmh.getDif()<<std::endl;
		CPreGame * cpg = new CPreGame(); //main menu and submenus
		log0<<"Initialization CPreGame (together): "<<tmh.getDif()<<std::endl;
		log0<<"Initialization of VCMI (togeter): "<<total.getDif()<<std::endl;
		cpg->mush = mush;

		StartInfo *options = new StartInfo(cpg->runLoop());
		tmh.getDif();
	////////////////////////SERVER STARTING/////////////////////////////////////////////////
		char portc[10]; SDL_itoa(port,portc,10);
		intpr::shared_memory_object smo(intpr::open_or_create,"vcmi_memory",intpr::read_write);
		smo.truncate(sizeof(ServerReady));
		intpr::mapped_region mr(smo,intpr::read_write);
		ServerReady *sr = new(mr.get_address())ServerReady();
		std::string comm = std::string(SERVER_NAME) + " " + portc + " > server_log.txt";
		boost::thread servthr(boost::bind(system,comm.c_str())); //runs server executable; 	//TODO: will it work on non-windows platforms?
		log0<<"Preparing shared memory and starting server: "<<tmh.getDif()<<std::endl;
	///////////////////////////////////////////////////////////////////////////////////////
		tmh.getDif();pomtime.getDif();//reset timers
		cgi->pathf = new CPathfinder();
		log0<<"\tPathfinder: "<<pomtime.getDif()<<std::endl;
		log0<<"Handlers initialization (together): "<<tmh.getDif()<<std::endl;
		std::ofstream lll("client_log.txt");

		CConnection *c=NULL;
		//wait until server is ready
		log0<<"Waiting for server... ";
		{
			intpr::scoped_lock<intpr::interprocess_mutex> slock(sr->mutex);
			while(!sr->ready)
			{
				sr->cond.wait(slock);
			}
		}
		intpr::shared_memory_object::remove("vcmi_memory");
		log0 << tmh.getDif()<<std::endl;
		while(!c)
		{
			try
			{
				log0 << "Establishing connection...\n";
				c = new CConnection("127.0.0.1",portc,NAME,lll);
			}
			catch(...)
			{
				log1 << "\nCannot establish connection! Retrying within 2 seconds" <<std::endl;
				SDL_Delay(2000);
			}
		}
		THC log0<<"\tConnecting to the server: "<<tmh.getDif()<<std::endl;
		CClient cl(c,options);
		boost::thread t(boost::bind(&CClient::run,&cl));
		SDL_Event ev;
		while(1) //main SDL events loop
		{
			SDL_WaitEvent(&ev);
			if(ev.type==SDL_QUIT) 
			{
				cl.close();
				::console->killConsole(console->native_handle());
				SDL_Delay(750);
				log0 << "Ending...\n";
				exit(0);
			}
			eventsM.lock();
			events.push(ev);
			eventsM.unlock();
		}
	}
	else
	{
		log1<<"Something was wrong: "<<SDL_GetError()<<std::endl;
		return -1;
	}
}

void processCommand(const std::string &message)
{
	std::istringstream readed;
	readed.str(message);
	std::string cn; //command name
	readed >> cn;
	int3 src, dst;

	int heronum;
	int3 dest;

	if(message==std::string("die, fool"))
		exit(0);
	else if(cn==std::string("activate"))
	{
		int what;
		readed >> what;
		switch (what)
		{
		case 0:
			LOCPLINT->curint->activate();
			break;
		case 1:
			LOCPLINT->adventureInt->activate();
			break;
		case 2:
			LOCPLINT->castleInt->activate();
			break;
		}
	}
	else if(message=="get txt")
	{
		boost::filesystem::create_directory("Extracted_txts");
		log0<<"Command accepted. Opening .lod file...\t";
		CLodHandler * txth = new CLodHandler;
		txth->init(std::string(DATA_DIR "Data" PATHSEPARATOR "H3bitmap.lod"),"data");
		log0<<"done.\nScanning .lod file\n";
		int curp=0;
		std::string pattern = ".TXT";
		for(int i=0;i<txth->entries.size(); i++)
		{
			std::string pom = txth->entries[i].nameStr;
			if(boost::algorithm::find_last(pom,pattern))
			{
				txth->extractFile(std::string("Extracted_txts\\")+pom,pom);
			}
			if(i%8) continue;
			int p2 = ((float)i/(float)txth->entries.size())*(float)100;
			if(p2!=curp)
			{
				curp = p2;
				log0<<"\r"<<curp<<"%";
			}
		}
		log0<<"\rExtracting done :)\n";
	}
}