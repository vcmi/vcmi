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
#include <boost/thread.hpp>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
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
#include "client/CConfigHandler.h"
#include "lib/Connection.h"
#include "lib/VCMI_Lib.h"
#include <cstdlib>

std::string NAME = NAME_VER + std::string(" (client)");
DLL_EXPORT void initDLL(CLodHandler *b);
SDL_Surface * screen, * screen2;
extern SDL_Surface * CSDL_Ext::std32bppSurface;
std::queue<SDL_Event> events;
boost::mutex eventsM;
TTF_Font * TNRB16, *TNR, *GEOR13, *GEORXX, *GEORM, *GEOR16;
void processCommand(const std::string &message, CClient *&client);
#ifndef __GNUC__
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char** argv)
#endif
{ 
	tlog0 << "Starting... " << std::endl;
	THC timeHandler tmh, total, pomtime;
	CClient *client = NULL;
	boost::thread *console = NULL;

	std::cout.flags(std::ios::unitbuf);
	logfile = new std::ofstream("VCMI_Client_log.txt");
	::console = new CConsoleHandler;
	*::console->cb = boost::bind(processCommand,_1,boost::ref(client));
	console = new boost::thread(boost::bind(&CConsoleHandler::run,::console));
	tlog0 <<"Creating console and logfile: "<<pomtime.getDif() << std::endl;

	conf.init();
	tlog0 <<"Loading settings: "<<pomtime.getDif() << std::endl;
	tlog0 << NAME << std::endl;

	srand ( time(NULL) );
	CPG=NULL;
	atexit(SDL_Quit);
	CGameInfo * cgi = CGI = new CGameInfo; //contains all global informations about game (texts, lodHandlers, map handler itp.)
	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_AUDIO)==0)
	{
		screen = SDL_SetVideoMode(conf.cc.resx,conf.cc.resy,conf.cc.bpp,SDL_SWSURFACE|SDL_DOUBLEBUF|(conf.cc.fullscreen?SDL_FULLSCREEN:0));  //initializing important global surface
		tlog0 <<"\tInitializing screen: "<<pomtime.getDif();
			tlog0 << std::endl;
		SDL_WM_SetCaption(NAME.c_str(),""); //set window title
		#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			int rmask = 0xff000000;int gmask = 0x00ff0000;int bmask = 0x0000ff00;int amask = 0x000000ff;
		#else
			int rmask = 0x000000ff;	int gmask = 0x0000ff00;	int bmask = 0x00ff0000;	int amask = 0xff000000;
		#endif
		CSDL_Ext::std32bppSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, 1, 1, 32, rmask, gmask, bmask, amask);
		tlog0 << "\tInitializing minors: " << pomtime.getDif() << std::endl;
		TTF_Init();
		TNRB16 = TTF_OpenFont("Fonts" PATHSEPARATOR "tnrb.ttf",16);
		GEOR13 = TTF_OpenFont("Fonts" PATHSEPARATOR "georgia.ttf",13);
		GEOR16 = TTF_OpenFont("Fonts" PATHSEPARATOR "georgia.ttf",16);
		GEORXX = TTF_OpenFont("Fonts" PATHSEPARATOR "tnrb.ttf",22);
		GEORM = TTF_OpenFont("Fonts" PATHSEPARATOR "georgia.ttf",10);
		atexit(TTF_Quit);
		THC tlog0<<"\tInitializing fonts: "<<pomtime.getDif()<<std::endl;
		CMusicHandler * mush = new CMusicHandler;  //initializing audio
		mush->initMusics();
		//audio initialized
		cgi->mush = mush;
		tlog0<<"\tInitializing sound: "<<pomtime.getDif()<<std::endl;
		tlog0<<"Initializing screen, fonts and sound handling: "<<tmh.getDif()<<std::endl;
		CDefHandler::Spriteh = cgi->spriteh = new CLodHandler();
		cgi->spriteh->init("Data" PATHSEPARATOR "H3sprite.lod","Sprites");
		BitmapHandler::bitmaph = cgi->bitmaph = new CLodHandler;
		cgi->bitmaph->init("Data" PATHSEPARATOR "H3bitmap.lod","Data");
		tlog0<<"Loading .lod files: "<<tmh.getDif()<<std::endl;
		initDLL(cgi->bitmaph,::console,logfile);
		CGI->setFromLib();
		tlog0<<"Initializing VCMI_Lib: "<<tmh.getDif()<<std::endl;
		pomtime.getDif();
		cgi->curh = new CCursorHandler; 
		cgi->curh->initCursor();
		cgi->curh->show();
		tlog0<<"\tScreen handler: "<<pomtime.getDif()<<std::endl;
		CAbilityHandler * abilh = new CAbilityHandler;
		abilh->loadAbilities();
		cgi->abilh = abilh;
		tlog0<<"\tAbility handler: "<<pomtime.getDif()<<std::endl;
		cgi->pathf = new CPathfinder();
		tlog0<<"\tPathfinder: "<<pomtime.getDif()<<std::endl;
		tlog0<<"Preparing first handlers: "<<tmh.getDif()<<std::endl;
		pomtime.getDif();
		graphics = new Graphics();
		graphics->loadHeroAnim();
		tlog0<<"\tMain graphics: "<<tmh.getDif()<<std::endl;
		tlog0<<"Initializing game graphics: "<<tmh.getDif()<<std::endl;
		CMessage::init();
		tlog0<<"Message handler: "<<tmh.getDif()<<std::endl;
		CPreGame * cpg = new CPreGame(); //main menu and submenus
		tlog0<<"Initialization CPreGame (together): "<<tmh.getDif()<<std::endl;
		tlog0<<"Initialization of VCMI (together): "<<total.getDif()<<std::endl;
		cpg->mush = mush;

		StartInfo *options = new StartInfo(cpg->runLoop());

		CClient cl;
		if(options->mode == 0) //new game
		{
			tmh.getDif();
			char portc[10]; 
			SDL_itoa(conf.cc.port,portc,10);
			CClient::runServer(portc);
			tlog0<<"Preparing shared memory and starting server: "<<tmh.getDif()<<std::endl;

			tmh.getDif();pomtime.getDif();//reset timers

			CConnection *c=NULL;
			//wait until server is ready
			tlog0<<"Waiting for server... ";
			cl.waitForServer();
			tlog0 << tmh.getDif()<<std::endl;
			while(!c)
			{
				try
				{
					tlog0 << "Establishing connection...\n";
					c = new CConnection(conf.cc.server,portc,NAME);
				}
				catch(...)
				{
					tlog1 << "\nCannot establish connection! Retrying within 2 seconds" <<std::endl;
					SDL_Delay(2000);
				}
			}
			THC tlog0<<"\tConnecting to the server: "<<tmh.getDif()<<std::endl;
			cl.newGame(c,options);
			client = &cl;
			boost::thread t(boost::bind(&CClient::run,&cl));
		}
		else //load game
		{
			std::string fname = options->mapname;
			boost::algorithm::erase_last(fname,".vlgm1");
			cl.load(fname);
			client = &cl;
			boost::thread t(boost::bind(&CClient::run,&cl));
		}

		SDL_Event ev;
		while(1) //main SDL events loop
		{
			SDL_WaitEvent(&ev);
			if((ev.type==SDL_QUIT)  ||  (ev.type == SDL_KEYDOWN && ev.key.keysym.sym==SDLK_F4 && (ev.key.keysym.mod & KMOD_ALT)))
			{
				cl.close();
#ifndef __unix__
				::console->killConsole(console->native_handle());
#endif
				LOCPLINT->pim->lock();
				SDL_Delay(750);
				tlog0 << "Ending...\n";
				exit(EXIT_SUCCESS);
			}
			else if(ev.type == SDL_KEYDOWN && ev.key.keysym.sym==SDLK_F4)
			{
				LOCPLINT->pim->lock();
				bool full = !(screen->flags&SDL_FULLSCREEN);
				SDL_QuitSubSystem(SDL_INIT_VIDEO);
				SDL_InitSubSystem(SDL_INIT_VIDEO);
				screen = SDL_SetVideoMode(conf.cc.resx,conf.cc.resy,conf.cc.bpp,SDL_SWSURFACE|SDL_DOUBLEBUF|(full?SDL_FULLSCREEN:0));  //initializing important global surface
				SDL_WM_SetCaption(NAME.c_str(),""); //set window title
				SDL_ShowCursor(SDL_DISABLE);
				LOCPLINT->curint->show();
				if(LOCPLINT->curint != LOCPLINT->adventureInt)
					LOCPLINT->adventureInt->show();
				if(LOCPLINT->curint == LOCPLINT->castleInt)
					LOCPLINT->castleInt->showAll(0,true);
				if(LOCPLINT->curint->subInt)
					LOCPLINT->curint->subInt->show();
				LOCPLINT->pim->unlock();
			}
			eventsM.lock();
			events.push(ev);
			eventsM.unlock();
		}
	}
	else
	{
		tlog1<<"Something was wrong: "<<SDL_GetError()<<std::endl;
		return -1;
	}
}

void processCommand(const std::string &message, CClient *&client)
{
	std::istringstream readed;
	readed.str(message);
	std::string cn; //command name
	readed >> cn;
	int3 src, dst;

//	int heronum;//TODO use me
	int3 dest;

	if(message==std::string("die, fool"))
		exit(EXIT_SUCCESS);
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
	else if(cn=="save")
	{
		std::string fname;
		readed >> fname;
		client->save(fname);
	}
	else if(cn=="load")
	{
		std::string fname;
		readed >> fname;
		client->load(fname);
	}
	else if(message=="get txt")
	{
		boost::filesystem::create_directory("Extracted_txts");
		tlog0<<"Command accepted. Opening .lod file...\t";
		CLodHandler * txth = new CLodHandler;
		txth->init(std::string(DATA_DIR "Data" PATHSEPARATOR "H3bitmap.lod"),"");
		tlog0<<"done.\nScanning .lod file\n";
		int curp=0;
		std::string pattern = ".TXT", pom;
		for(int i=0;i<txth->entries.size(); i++)
		{
			pom = txth->entries[i].nameStr;
			if(boost::algorithm::find_last(pom,pattern))
			{
				txth->extractFile(std::string("Extracted_txts\\")+pom,pom);
			}
			if(i%8) continue;
			int p2 = ((float)i/(float)txth->entries.size())*(float)100;
			if(p2!=curp)
			{
				curp = p2;
				tlog0<<"\r"<<curp<<"%";
			}
		}
		tlog0<<"\rExtracting done :)\n";
	}
	else if(client && client->serv && client->serv->connected) //send to server
	{
		*client->serv << ui16(513) << message;
	}
}