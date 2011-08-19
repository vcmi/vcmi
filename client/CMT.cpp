// CMT.cpp : Defines the entry point for the console application.
//
#include "../stdafx.h"
#include <cmath>
#include <string>
#include <vector>
#include <queue>
#include <cmath>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/thread.hpp>
#include <SDL_mixer.h>
#include "SDL_Extensions.h"
#include "SDL_framerate.h"
#include "CGameInfo.h"
#include "mapHandler.h"
#include "../global.h"
#include "CPreGame.h"
#include "CCastleInterface.h"
#include "../CConsoleHandler.h"
#include "CCursorHandler.h"
#include "../lib/CGameState.h"
#include "../CCallback.h"
#include "CPlayerInterface.h"
#include "CAdvmapInterface.h"
#include "../lib/CBuildingHandler.h"
#include "CVideoHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CCreatureHandler.h"
#include "../lib/CSpellHandler.h"
#include "CMusicHandler.h"
#include "CVideoHandler.h"
#include "../lib/CLodHandler.h"
#include "CDefHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "Graphics.h"
#include "Client.h"
#include "CConfigHandler.h"
#include "../lib/Connection.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/VCMIDirs.h"
#include <cstdlib>
#include "../lib/NetPacks.h"
#include "CMessage.h"
#include "../lib/CObjectHandler.h"
#include <boost/program_options.hpp>
#include "../lib/CArtHandler.h"
#include "../lib/CScriptingModule.h"

#ifdef _WIN32
#include "SDL_syswm.h"
#endif
#include <boost/foreach.hpp>
#include "../lib/CDefObjInfoHandler.h"

#if __MINGW32__
#undef main
#endif

namespace po = boost::program_options;

/*
 * CMT.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

std::string NAME_AFFIX = "client";
std::string NAME = NAME_VER + std::string(" (") + NAME_AFFIX + ')'; //application name
CGuiHandler GH;
static CClient *client;
SDL_Surface *screen = NULL, //main screen surface 
	*screen2 = NULL,//and hlp surface (used to store not-active interfaces layer) 
	*screenBuf = screen; //points to screen (if only advmapint is present) or screen2 (else) - should be used when updating controls which are not regularly redrawed
static boost::thread *mainGUIThread;

SystemOptions GDefaultOptions; 
VCMIDirs GVCMIDirs;
std::queue<SDL_Event*> events;
boost::mutex eventsM;

static bool gOnlyAI = false;
static bool setResolution = false; //set by event handling thread after resolution is adjusted

static bool ermInteractiveMode = false; //structurize when time is right
void processCommand(const std::string &message);
static void setScreenRes(int w, int h, int bpp, bool fullscreen);
void dispose();
void playIntro();
static void listenForEvents();
void requestChangingResolution();
void startGame(StartInfo * options, CConnection *serv = NULL);

#ifndef _WIN32
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>
#endif

void init()
{
	timeHandler tmh, pomtime;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	int rmask = 0xff000000;int gmask = 0x00ff0000;int bmask = 0x0000ff00;int amask = 0x000000ff;
#else
	int rmask = 0x000000ff;	int gmask = 0x0000ff00;	int bmask = 0x00ff0000;	int amask = 0xff000000;
#endif
	CSDL_Ext::std32bppSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, 1, 1, 32, rmask, gmask, bmask, amask);
	tlog0 << "\tInitializing minors: " << pomtime.getDif() << std::endl;
	{
		//read system options
		CLoadFile settings(GVCMIDirs.UserPath + "/config/sysopts.bin", 727);
		if(settings.sfile)
		{
			settings >> GDefaultOptions;
		}
		else //file not found (probably, may be also some kind of access problem
		{
			tlog2 << "Warning: Cannot read system options, default settings will be used.\n";
			
			//Try to create file
			tlog2 << "VCMI will try to save default system options...\n";
			GDefaultOptions.settingsChanged();
		}
	}
	THC tlog0<<"\tLoading default system settings: "<<pomtime.getDif()<<std::endl;

	//initializing audio
	// Note: because of interface button range, volume can only be a
	// multiple of 11, from 0 to 99.
	CCS->soundh = new CSoundHandler;
	CCS->soundh->init();
	CCS->soundh->setVolume(GDefaultOptions.soundVolume);
	CCS->musich = new CMusicHandler;
	CCS->musich->init();
	CCS->musich->setVolume(GDefaultOptions.musicVolume);
	tlog0<<"\tInitializing sound: "<<pomtime.getDif()<<std::endl;
	tlog0<<"Initializing screen and sound handling: "<<tmh.getDif()<<std::endl;

	initDLL(::console,logfile);
	const_cast<CGameInfo*>(CGI)->setFromLib();
	CCS->soundh->initCreaturesSounds(CGI->creh->creatures);
	CCS->soundh->initSpellsSounds(CGI->spellh->spells);
	tlog0<<"Initializing VCMI_Lib: "<<tmh.getDif()<<std::endl;

	pomtime.getDif();
	CCS->curh = new CCursorHandler;
	CCS->curh->initCursor();
	CCS->curh->show();
	tlog0<<"Screen handler: "<<pomtime.getDif()<<std::endl;
	pomtime.getDif();
	graphics = new Graphics();
	graphics->loadHeroAnims();
	tlog0<<"\tMain graphics: "<<tmh.getDif()<<std::endl;
	tlog0<<"Initializing game graphics: "<<tmh.getDif()<<std::endl;

	CMessage::init();
	tlog0<<"Message handler: "<<tmh.getDif()<<std::endl;
	//CPG = new CPreGame(); //main menu and submenus
	//tlog0<<"Initialization CPreGame (together): "<<tmh.getDif()<<std::endl;
}

static void prog_version(void)
{
	printf("%s\n", NAME_VER);
	printf("  data directory:    %s\n", DATA_DIR);
	printf("  library directory: %s\n", LIB_DIR);
	printf("  binary directory:  %s\n", BIN_DIR);
}

static void prog_help(const char *progname)
{
	printf("%s - A Heroes of Might and Magic 3 clone\n", NAME_VER);
    printf("Copyright (C) 2007-2010 VCMI dev team - see AUTHORS file\n");
    printf("This is free software; see the source for copying conditions. There is NO\n");
    printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
	printf("\n");
	printf("Usage:\n");
	printf("  -h, --help        display this help and exit\n");
	printf("  -v, --version     display version information and exit\n");
}


#ifdef _WIN32
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char** argv)
#endif
{
	tlog0 << "Starting... " << std::endl;      
	po::options_description opts("Allowed options");
	opts.add_options()
		("help,h", "display help and exit")
		("version,v", "display version information and exit")
		("battle,b", po::value<std::string>(), "runs game in duel mode (battle-only")
		("nointro,i", "skips intro movies");

	po::variables_map vm;
	if(argc > 1)
	{
		try
		{
			po::store(po::parse_command_line(argc, argv, opts), vm);
		}
		catch(std::exception &e) 
		{
			tlog1 << "Failure during parsing command-line options:\n" << e.what() << std::endl;
		}
	}

	po::notify(vm);
	if(vm.count("help"))
	{
		prog_help(0);
		return 0;
	}
	if(vm.count("version"))
	{
		prog_version();
		return 0;
	}

	//Set environment vars to make window centered. Sometimes work, sometimes not. :/
	putenv((char*)"SDL_VIDEO_WINDOW_POS");
	putenv((char*)"SDL_VIDEO_CENTERED=1");

	timeHandler total, pomtime;
	std::cout.flags(std::ios::unitbuf);
	logfile = new std::ofstream("VCMI_Client_log.txt");
	console = new CConsoleHandler;
	*console->cb = boost::bind(&processCommand, _1);
	console->start();
	atexit(dispose);
	tlog0 <<"Creating console and logfile: "<<pomtime.getDif() << std::endl;

	conf.init();
	tlog0 <<"Loading settings: "<<pomtime.getDif() << std::endl;
	tlog0 << NAME << std::endl;

	srand ( time(NULL) );
	
	CCS = new CClientState;
	CGI = new CGameInfo; //contains all global informations about game (texts, lodHandlers, map handler etc.)

	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_AUDIO))
	{
		tlog1<<"Something was wrong: "<< SDL_GetError() << std::endl;
		exit(-1);
	}
	atexit(SDL_Quit);

	setScreenRes(conf.cc.pregameResx, conf.cc.pregameResy, conf.cc.bpp, conf.cc.fullscreen);
	tlog0 <<"\tInitializing screen: "<<pomtime.getDif() << std::endl;

	// Initialize video
#if defined _M_X64 && defined _WIN32 //Win64 -> cannot load 32-bit DLLs for video handling
	CCS->videoh = new CEmptyVideoPlayer;
#else
	CCS->videoh = new CVideoPlayer;
#endif
	tlog0<<"\tInitializing video: "<<pomtime.getDif()<<std::endl;

	//we can properly play intro only in the main thread, so we have to move loading to the separate thread
	boost::thread loading(init);

	if(!vm.count("battle") && !vm.count("nointro"))
		playIntro();

	SDL_FillRect(screen,NULL,0);
	CSDL_Ext::update(screen);
	loading.join();
	tlog0<<"Initialization of VCMI (together): "<<total.getDif()<<std::endl;

	if(!vm.count("battle"))
	{
		//CCS->musich->playMusic(musicBase::mainMenu, -1);
		GH.curInt = new CGPreGame; //will set CGP pointer to itself
	}
	else
	{
		StartInfo *si = new StartInfo();
		si->mode = StartInfo::DUEL;
		si->mapname = vm["battle"].as<std::string>();
		si->playerInfos[0].color = 0;
		si->playerInfos[1].color = 1;
		startGame(si);
	}
	mainGUIThread = new boost::thread(&CGuiHandler::run, boost::ref(GH));
	listenForEvents();

	return 0;
}

void printInfoAboutIntObject(const CIntObject *obj, int level)
{
	int tabs = level;
	while(tabs--) tlog4 << '\t';

	tlog4 << typeid(*obj).name() << " *** " << (obj->active ? "" : "not ") << "active\n";

	BOOST_FOREACH(const CIntObject *child, obj->children)
		printInfoAboutIntObject(child, level+1);
}

void processCommand(const std::string &message)
{
	std::istringstream readed;
	readed.str(message);
	std::string cn; //command name
	readed >> cn;


	if(LOCPLINT && LOCPLINT->cingconsole)
		LOCPLINT->cingconsole->print(message);

	if(ermInteractiveMode)
	{
		if(cn == "exit")
		{
			ermInteractiveMode = false;
			return;
		}
		else
		{
			if(client && client->erm)
				client->erm->executeUserCommand(message);
			tlog0 << "erm>";
		}
	}
	else if(message==std::string("die, fool"))
	{
		exit(EXIT_SUCCESS);
	}
	else if(cn == "erm")
	{
		ermInteractiveMode = true;
		tlog0 << "erm>";
	}
	else if(cn==std::string("activate"))
	{
		int what;
		readed >> what;
		switch (what)
		{
		case 0:
			GH.topInt()->activate();
			break;
		case 1:
			adventureInt->activate();
			break;
		case 2:
			LOCPLINT->castleInt->activate();
			break;
		}
	}
	else if(cn=="redraw")
	{
		GH.totalRedraw();
	}
	else if(cn=="screen")
	{
		tlog0 << "Screenbuf points to ";

		if(screenBuf == screen)
			tlog1 << "screen";
		else if(screenBuf == screen2)
			tlog1 << "screen2";
		else
			tlog1 << "?!?";

		tlog1 << std::endl;

		SDL_SaveBMP(screen, "Screen_c.bmp");
		SDL_SaveBMP(screen2, "Screen2_c.bmp");
	}
	else if(cn=="save")
	{
		std::string fname;
		readed >> fname;
		client->save(fname);
	}
	//else if(cn=="list")
	//{
	//	if(CPG)
	//		for(int i = 0; i < CPG->ourScenSel->mapsel.ourGames.size(); i++)
	//			tlog0 << i << ".\t" << CPG->ourScenSel->mapsel.ourGames[i]->filename << std::endl;
	//}
	else if(cn=="load")
	{
		// TODO: this code should end the running game and manage to call startGame instead
		std::string fname;
		readed >> fname;
		client->loadGame(fname);
	}
	//else if(cn=="ln")
	//{
	//	int num;
	//	readed >> num;
	//	std::string &name = CPG->ourScenSel->mapsel.ourGames[num]->filename;
	//	client->load(name.substr(0, name.size()-6));
	//}
	else if(cn=="resolution" || cn == "r")
	{
		if(LOCPLINT)
		{
			tlog1 << "Resolution can be set only before starting the game.\n";
			return;
		}
		std::map<std::pair<int,int>, config::GUIOptions >::iterator j;
		int i=1, hlp=1;
		tlog4 << "Available screen resolutions:\n";
		for(j=conf.guiOptions.begin(); j!=conf.guiOptions.end(); j++)
			tlog4 << i++ <<". " << j->first.first << " x " << j->first.second << std::endl;
		tlog4 << "Type number from 1 to " << i-1 << " to set appropriate resolution or 0 to cancel.\n";
		std::cin >> i;
		if(i < 0  ||  i > conf.guiOptions.size() || std::cin.bad() || std::cin.fail())
		{
			std::cin.clear();
			tlog1 << "Invalid resolution ID! Not a number between 0 and  " << conf.guiOptions.size() << ". No settings changed.\n";
		}
		else if(!i)
		{
			return;
		}
		else
		{
			for(j=conf.guiOptions.begin(); j!=conf.guiOptions.end() && hlp++<i; j++); //move j to the i-th resolution info
			conf.cc.resx = j->first.first;
			conf.cc.resy = j->first.second;
			conf.cc.screenx = j->first.first;
			conf.cc.screeny = j->first.second;
			tlog0 << "Screen resolution set to " << conf.cc.resx << " x " << conf.cc.resy <<". It will be aplied when the game starts.\n";
		}
	}
	else if(message=="get txt")
	{
		boost::filesystem::create_directory("Extracted_txts");
		tlog0<<"Command accepted. Opening .lod file...\t";
		CLodHandler * txth = new CLodHandler;
		txth->init(std::string(DATA_DIR "/Data/H3bitmap.lod"),"");
		tlog0<<"done.\nScanning .lod file\n";
		
		BOOST_FOREACH(Entry e, txth->entries)
			if( e.type == FILE_TEXT )
				txth->extractFile(std::string(DATA_DIR "/Extracted_txts/")+e.name,e.name);
		tlog0<<"\rExtracting done :)\n";
	}
	else if(cn=="crash")
	{
		int *ptr = NULL;
		*ptr = 666;
		//disaster!
	}
	else if(cn == "onlyai")
	{
		gOnlyAI = true;
	}
	else if (cn == "ai")
	{
		VLC->IS_AI_ENABLED = !VLC->IS_AI_ENABLED;
		tlog4 << "Current AI status: " << (VLC->IS_AI_ENABLED ? "enabled" : "disabled") << std::endl;
	}
	else if(cn == "mp" && adventureInt)
	{
		if(const CGHeroInstance *h = dynamic_cast<const CGHeroInstance *>(adventureInt->selection))
			tlog0 << h->movement << "; max: " << h->maxMovePoints(true) << "/" << h->maxMovePoints(false) << std::endl;
	}
	else if(cn == "bonuses")
	{
		tlog0 << "Bonuses of " << adventureInt->selection->getHoverText() << std::endl
			<< adventureInt->selection->getBonusList() << std::endl;

		tlog0 << "\nInherited bonuses:\n";
		TCNodes parents;
		adventureInt->selection->getParents(parents);
		BOOST_FOREACH(const CBonusSystemNode *parent, parents)
		{
			tlog0 << "\nBonuses from " << typeid(*parent).name() << std::endl << parent->getBonusList() << std::endl;
		}
	}
	else if(cn == "not dialog")
	{
		LOCPLINT->showingDialog->setn(false);
	}
	else if(cn == "gui")
	{
		BOOST_FOREACH(const IShowActivable *child, GH.listInt)
		{
			if(const CIntObject *obj = dynamic_cast<const CIntObject *>(child))
				printInfoAboutIntObject(obj, 0);
			else
				tlog4 << typeid(*obj).name() << std::endl;
		}
	}
	else if(cn=="tell")
	{
		std::string what;
		int id1, id2;
		readed >> what >> id1 >> id2;
		if(what == "hs")
		{
			BOOST_FOREACH(const CGHeroInstance *h, LOCPLINT->cb->getHeroesInfo())
				if(h->type->ID == id1)
					if(const CArtifactInstance *a = h->getArt(id2))
						tlog4 << a->nodeName();
		}
		else if (what == "anim" )
		{
			CAnimation::getAnimInfo();
		}
	}
	else if (cn == "switchCreWin" )
	{
		conf.cc.classicCreatureWindow = !conf.cc.classicCreatureWindow;
	}
	else if(client && client->serv && client->serv->connected) //send to server
	{
		PlayerMessage pm(LOCPLINT->playerID,message);
		*client->serv << &pm;
	}
}

//plays intro, ends when intro is over or button has been pressed (handles events)
void playIntro()
{
	if(CCS->videoh->openAndPlayVideo("3DOLOGO.SMK", 60, 40, screen, true))
	{
		CCS->videoh->openAndPlayVideo("AZVS.SMK", 60, 80, screen, true);
	}
}

void dispose()
{
	if (console)
		delete console;
	delete logfile;
}

static void setScreenRes(int w, int h, int bpp, bool fullscreen)
{	
	// VCMI will only work with 2, 3 or 4 bytes per pixel
	amax(bpp, 16);
	amin(bpp, 32);

	// Try to use the best screen depth for the display
	int suggestedBpp = SDL_VideoModeOK(w, h, bpp, SDL_SWSURFACE|(fullscreen?SDL_FULLSCREEN:0));
	if(suggestedBpp == 0)
	{
		tlog1 << "Error: SDL says that " << w << "x" << h << " resolution is not available!\n";
		return;
	}

	bool bufOnScreen = (screenBuf == screen);
	
	if(suggestedBpp != bpp)
	{
		tlog2 << "Warning: SDL says that "  << bpp << "bpp is wrong and suggests " << suggestedBpp << std::endl;
	}

	if(screen) //screen has been already initialized
		SDL_QuitSubSystem(SDL_INIT_VIDEO);

	SDL_InitSubSystem(SDL_INIT_VIDEO);
	
	if((screen = SDL_SetVideoMode(w, h, suggestedBpp, SDL_SWSURFACE|(fullscreen?SDL_FULLSCREEN:0))) == NULL)
	{
		tlog1 << "Requested screen resolution is not available (" << w << "x" << h << "x" << suggestedBpp << "bpp)\n";
		throw "Requested screen resolution is not available\n";
	}

	tlog0 << "New screen flags: " << screen->flags << std::endl;

	if(screen2)
		SDL_FreeSurface(screen2);
	screen2 = CSDL_Ext::copySurface(screen);
	SDL_EnableUNICODE(1);
	SDL_WM_SetCaption(NAME.c_str(),""); //set window title
	SDL_ShowCursor(SDL_DISABLE);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

#ifdef _WIN32
	SDL_SysWMinfo wm;
	SDL_VERSION(&wm.version);
	int getwm = SDL_GetWMInfo(&wm);
	if(getwm == 1)
	{
		int sw = GetSystemMetrics(SM_CXSCREEN),
			sh = GetSystemMetrics(SM_CYSCREEN);
		RECT curpos;
		GetWindowRect(wm.window,&curpos);
		int ourw = curpos.right - curpos.left,
			ourh = curpos.bottom - curpos.top;
		SetWindowPos(wm.window, 0, (sw - ourw)/2, (sh - ourh)/2, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
	}
	else
	{
		tlog3 << "Something went wrong, getwm=" << getwm << std::endl;
		tlog3 << "SDL says: " << SDL_GetError() << std::endl;
		tlog3 << "Window won't be centered.\n";
	}
#endif
	//TODO: centering game window on other platforms (or does the environment do their job correctly there?)

	screenBuf = bufOnScreen ? screen : screen2;
	setResolution = true;
}

static void listenForEvents()
{
	while(1) //main SDL events loop
	{
		SDL_Event *ev = new SDL_Event();

		//tlog0 << "Waiting... ";
		int ret = SDL_WaitEvent(ev);
		//tlog0 << "got " << (int)ev->type;
		if (ret == 0 || (ev->type==SDL_QUIT) ||
			(ev->type == SDL_KEYDOWN && ev->key.keysym.sym==SDLK_F4 && (ev->key.keysym.mod & KMOD_ALT)))
		{
			if (client)
				client->endGame();
			if (mainGUIThread) 
			{
				GH.terminate = true;
				mainGUIThread->join();
				delete mainGUIThread;
				mainGUIThread = NULL;
			}
			delete console;
			console = NULL;
			SDL_Delay(750);
			SDL_Quit();
			tlog0 << "Ending...\n";
			break;
		}
		else if(LOCPLINT && ev->type == SDL_KEYDOWN && ev->key.keysym.sym==SDLK_F4)
		{
			boost::unique_lock<boost::recursive_mutex> lock(*LOCPLINT->pim);
			bool full = !(screen->flags&SDL_FULLSCREEN);
			setScreenRes(conf.cc.screenx, conf.cc.screeny, conf.cc.bpp, full);
			GH.totalRedraw();
			delete ev;
			continue;
		}
		else if(ev->type == SDL_USEREVENT)
		{
			switch(ev->user.code)
			{
			case 1:
				tlog0 << "Changing resolution has been requested\n";
				setScreenRes(conf.cc.screenx, conf.cc.screeny, conf.cc.bpp, conf.cc.fullscreen);
				break;

			case 2:
				client->endGame();
				delete client;
				client = NULL;

				delete CGI->dobjinfo.get();
				const_cast<CGameInfo*>(CGI)->dobjinfo = new CDefObjInfoHandler;
				const_cast<CGameInfo*>(CGI)->dobjinfo->load();

				GH.curInt = CGP;
				GH.defActionsDef = 63;
				break;

			case 3:
				client->endGame(false);
				break;
			}

			delete ev;
			continue;
		} 

		//tlog0 << " pushing ";
		eventsM.lock();
		events.push(ev);
		eventsM.unlock();
		//tlog0 << " done\n";
	}
}

void startGame(StartInfo * options, CConnection *serv/* = NULL*/) 
{
	GH.curInt =NULL;
	SDL_FillRect(screen, 0, 0);
	if(gOnlyAI)
	{
		for(std::map<int, PlayerSettings>::iterator it = options->playerInfos.begin(); 
			it != options->playerInfos.end(); ++it)
		{
			it->second.human = false;
		}
	}

	if(screen->w != conf.cc.screenx   ||   screen->h != conf.cc.screeny)
	{
		requestChangingResolution();

		//allow event handling thread change resolution
		eventsM.unlock();
		while(!setResolution) boost::this_thread::sleep(boost::posix_time::milliseconds(50));
		eventsM.lock();
	}
	else
		setResolution = true;



	client = new CClient;
	CPlayerInterface::howManyPeople = 0;
	switch(options->mode) //new game
	{
	case StartInfo::NEW_GAME:
	case StartInfo::CAMPAIGN:
	case StartInfo::DUEL:
		client->newGame(serv, options);
		break;
	case StartInfo::LOAD_GAME:
		std::string fname = options->mapname;
		boost::algorithm::erase_last(fname,".vlgm1");
		client->loadGame(fname);
		break;
	}

	client->connectionHandler = new boost::thread(&CClient::run, client);
}

void requestChangingResolution()
{
	//mark that we are going to change resolution
	setResolution = false;

	//push special event to order event reading thread to change resolution
	SDL_Event ev;
	ev.type = SDL_USEREVENT;
	ev.user.code = 1;
	SDL_PushEvent(&ev);
}
