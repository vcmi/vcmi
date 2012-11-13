// CMT.cpp : Defines the entry point for the console application.
//
#include "StdInc.h"
#include <boost/filesystem/operations.hpp>
#include <SDL_mixer.h>
#include "UIFramework/SDL_Extensions.h"
#include "CGameInfo.h"
#include "mapHandler.h"

#include "../lib/Filesystem/CResourceLoader.h"
#include "CPreGame.h"
#include "CCastleInterface.h"
#include "../lib/CConsoleHandler.h"
#include "UIFramework/CCursorHandler.h"
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
#include "CDefHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "Graphics.h"
#include "Client.h"
#include "../lib/CConfigHandler.h"
#include "../lib/Connection.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/VCMIDirs.h"
#include "../lib/NetPacks.h"
#include "CMessage.h"
#include "../lib/CModHandler.h"
#include "../lib/CObjectHandler.h"
#include "../lib/CArtHandler.h"
#include "../lib/CScriptingModule.h"
#include "../lib/GameConstants.h"
#include "UIFramework/CGuiHandler.h"

#ifdef _WIN32
#include "SDL_syswm.h"
#endif
#include "../lib/CDefObjInfoHandler.h"
#include "../lib/UnlockGuard.h"

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
std::string NAME = GameConstants::VCMI_VERSION + std::string(" (") + NAME_AFFIX + ')'; //application name
CGuiHandler GH;
static CClient *client=NULL;
SDL_Surface *screen = NULL, //main screen surface
	*screen2 = NULL,//and hlp surface (used to store not-active interfaces layer)
	*screenBuf = screen; //points to screen (if only advmapint is present) or screen2 (else) - should be used when updating controls which are not regularly redrawed
static boost::thread *mainGUIThread;

std::queue<SDL_Event> events;
boost::mutex eventsM;

static bool gOnlyAI = false;
//static bool setResolution = false; //set by event handling thread after resolution is adjusted

static bool ermInteractiveMode = false; //structurize when time is right
void processCommand(const std::string &message);
static void setScreenRes(int w, int h, int bpp, bool fullscreen, bool resetVideo=true);
void dispose();
void playIntro();
static void listenForEvents();
//void requestChangingResolution();
void startGame(StartInfo * options, CConnection *serv = NULL);

#ifndef _WIN32
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>
#endif

void startGameFromFile(const std::string &fname)
{
	if(fname.size() && boost::filesystem::exists(fname))
	{
		StartInfo si;
		CLoadFile out(fname);
		if(!out.sfile || !*out.sfile)
		{
			tlog1 << "Failed to open startfile, falling back to the main menu!\n";
			GH.curInt = CGPreGame::create();
			return;
		}
		out >> si;
		while(GH.topInt())
			GH.popIntTotally(GH.topInt());

		startGame(&si);
	}
}

void init()
{
	CStopWatch tmh, pomtime;
	tlog0 << "\tInitializing minors: " << pomtime.getDiff() << std::endl;

	//initializing audio
	// Note: because of interface button range, volume can only be a
	// multiple of 11, from 0 to 99.
	CCS->soundh = new CSoundHandler;
	CCS->soundh->init();
	CCS->soundh->setVolume(settings["general"]["sound"].Float());
	CCS->musich = new CMusicHandler;
	CCS->musich->init();
	CCS->musich->setVolume(settings["general"]["music"].Float());
	tlog0<<"\tInitializing sound: "<<pomtime.getDiff()<<std::endl;
	tlog0<<"Initializing screen and sound handling: "<<tmh.getDiff()<<std::endl;

	initDLL(::console,logfile);
	const_cast<CGameInfo*>(CGI)->setFromLib();
	CCS->soundh->initCreaturesSounds(CGI->creh->creatures);
	CCS->soundh->initSpellsSounds(CGI->spellh->spells);
	tlog0<<"Initializing VCMI_Lib: "<<tmh.getDiff()<<std::endl;

	pomtime.getDiff();
	CCS->curh = new CCursorHandler;
	CCS->curh->initCursor();
	CCS->curh->show();
	tlog0<<"Screen handler: "<<pomtime.getDiff()<<std::endl;
	pomtime.getDiff();
	graphics = new Graphics();
	graphics->loadHeroAnims();
	tlog0<<"\tMain graphics: "<<tmh.getDiff()<<std::endl;
	tlog0<<"Initializing game graphics: "<<tmh.getDiff()<<std::endl;

	CMessage::init();
	tlog0<<"Message handler: "<<tmh.getDiff()<<std::endl;
	//CPG = new CPreGame(); //main menu and submenus
	//tlog0<<"Initialization CPreGame (together): "<<tmh.getDif()<<std::endl;
}

static void prog_version(void)
{
	printf("%s\n", GameConstants::VCMI_VERSION.c_str());
	printf("  data directory:    %s\n", GameConstants::DATA_DIR.c_str());
	printf("  library directory: %s\n", GameConstants::LIB_DIR.c_str());
	printf("  binary directory:  %s\n", GameConstants::BIN_DIR.c_str());
}

static void prog_help(const po::options_description &opts)
{
	printf("%s - A Heroes of Might and Magic 3 clone\n", GameConstants::VCMI_VERSION.c_str());
    printf("Copyright (C) 2007-2012 VCMI dev team - see AUTHORS file\n");
    printf("This is free software; see the source for copying conditions. There is NO\n");
    printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
	printf("\n");
	printf("Usage:\n");
	std::cout << opts;
// 	printf("  -h, --help        display this help and exit\n");
// 	printf("  -v, --version     display version information and exit\n");
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
		("start", po::value<std::string>(), "starts game from saved StartInfo file")
		("onlyAI", "runs without GUI, all players will be default AI")
		("oneGoodAI", "puts one default AI and the rest will be EmptyAI")
		("autoSkip", "automatically skip turns in GUI")
		("disable-video", "disable video player")
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
		prog_help(opts);
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

	CStopWatch total, pomtime;
	std::cout.flags(std::ios::unitbuf);
	logfile = new std::ofstream((GVCMIDirs.UserPath + "/VCMI_Client_log.txt").c_str());
	console = new CConsoleHandler;
	*console->cb = boost::bind(&processCommand, _1);
	console->start();
	atexit(dispose);
	tlog0 <<"Creating console and logfile: "<<pomtime.getDiff() << std::endl;

	LibClasses::loadFilesystem();

	settings.init();
	conf.init();
	tlog0 <<"Loading settings: "<<pomtime.getDiff() << std::endl;
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

	const JsonNode& video = settings["video"];
	const JsonNode& res = video["screenRes"];

	//something is really wrong...
	if (res["width"].Float() < 100 || res["height"].Float() < 100)
	{
		tlog0 << "Fatal error: failed to load settings!\n";
		tlog0 << "Possible reasons:\n";
		tlog0 << "\tCorrupted local configuration file at " << GVCMIDirs.UserPath << "/config/settings.json\n";
		tlog0 << "\tMissing or corrupted global configuration file at " << GameConstants::DATA_DIR << "/config/defaultSettings.json\n";
		tlog0 << "VCMI will now exit...\n";
		exit(EXIT_FAILURE);
	}

	setScreenRes(res["width"].Float(), res["height"].Float(), video["bitsPerPixel"].Float(), video["fullscreen"].Bool());

	tlog0 <<"\tInitializing screen: "<<pomtime.getDiff() << std::endl;

	// Initialize video
#if DISABLE_VIDEO
	CCS->videoh = new CEmptyVideoPlayer;
#else
	if (!vm.count("disable-video"))
		CCS->videoh = new CVideoPlayer;
	else
		CCS->videoh = new CEmptyVideoPlayer;
#endif

	tlog0<<"\tInitializing video: "<<pomtime.getDiff()<<std::endl;

	//we can properly play intro only in the main thread, so we have to move loading to the separate thread
	boost::thread loading(init);

	if(!vm.count("battle") && !vm.count("nointro"))
		playIntro();

	SDL_FillRect(screen,NULL,0);
	CSDL_Ext::update(screen);
	loading.join();
	tlog0<<"Initialization of VCMI (together): "<<total.getDiff()<<std::endl;

	if(!vm.count("battle"))
	{
		gOnlyAI = vm.count("onlyAI");
		Settings session = settings.write["session"];
		session["autoSkip"].Bool()  = vm.count("autoSkip");
		session["oneGoodAI"].Bool() = vm.count("oneGoodAI");

		std::string fileToStartFrom; //none by default
		if(vm.count("start"))
			fileToStartFrom = vm["start"].as<std::string>();

		if(fileToStartFrom.size() && boost::filesystem::exists(fileToStartFrom))
			startGameFromFile(fileToStartFrom); //ommit pregame and start the game using settings from fiel
		else
		{
			if(fileToStartFrom.size())
			{
				tlog3 << "Warning: cannot find given file to start from (" << fileToStartFrom
					<< "). Falling back to main menu.\n";
			}
			GH.curInt = CGPreGame::create(); //will set CGP pointer to itself
		}
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
	tlog4 << std::string(level, '\t');

	tlog4 << typeid(*obj).name() << " *** ";
	if (obj->active)
	{
#define PRINT(check, text) if (obj->active & CIntObject::check) tlog4 << text
		PRINT(LCLICK, 'L');
		PRINT(RCLICK, 'R');
		PRINT(HOVER, 'H');
		PRINT(MOVE, 'M');
		PRINT(KEYBOARD, 'K');
		PRINT(TIME, 'T');
		PRINT(GENERAL, 'A');
		PRINT(WHEEL, 'W');
		PRINT(DOUBLECLICK, 'D');
#undef  PRINT
	}
	else
		tlog4 << "inactive";
	tlog4 << " at " << obj->pos.x <<"x"<< obj->pos.y;
	tlog4 << " (" << obj->pos.w <<"x"<< obj->pos.h << ")\n";

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
	else if(cn=="load")
	{
		// TODO: this code should end the running game and manage to call startGame instead
		std::string fname;
		readed >> fname;
		client->loadGame(fname);
	}/*
	else if(cn=="resolution" || cn == "r")
	{
		if(LOCPLINT)
		{
			tlog1 << "Resolution can be set only before starting the game.\n";
			return;
		}
		std::map<std::pair<int,int>, config::GUIOptions >::iterator j;
		int i=1;
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
			auto j = conf.guiOptions.begin();
			std::advance(j, i - 1); //move j to the i-th resolution info
			const int w = j->first.first, h = j->first.second;
			Settings screen = settings.write["video"]["screenRes"];
			screen["width"].Float()  = w;
			screen["height"].Float() = h;
			conf.SetResolution(screen["width"].Float(), screen["height"].Float());
			tlog0 << "Screen resolution set to " << (int)screen["width"].Float() << " x "
												 << (int)screen["height"].Float() <<". It will be applied afters game restart.\n";
		}
	}*/
	else if(message=="get txt")
	{
		tlog0<<"Command accepted.\t";
		boost::filesystem::create_directory("Extracted_txts");
		auto iterator = CResourceHandler::get()->getIterator([](const ResourceID & ident)
		{
			return ident.getType() == EResType::TEXT && boost::algorithm::starts_with(ident.getName(), "DATA/");
		});

		std::string basePath = CResourceHandler::get()->getResourceName(ResourceID("DATA")) + "/Extracted_txts/";
		while (iterator.hasNext())
		{
			std::ofstream file(basePath + iterator->getName() + ".TXT");
			auto text = CResourceHandler::get()->loadData(*iterator);

			file.write((char*)text.first.get(), text.second);
			++iterator;
		}

		tlog0 << "\rExtracting done :)\n";
		tlog0 << " Extracted files can be found in " << basePath << " directory\n";
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
		BOOST_FOREACH(const IShowActivatable *child, GH.listInt)
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
	}
	else if (cn == "set")
	{
		std::string what, value;
		readed >> what;

		Settings conf = settings.write["session"][what];

		readed >> value;
		if (value == "on")
			conf->Bool() = true;
		else if (value == "off")
			conf->Bool() = false;
	}
	else if(cn == "sinfo")
	{
		std::string fname;
		readed >> fname;
		if(fname.size() && SEL)
		{
			CSaveFile out(fname);
			out << SEL->sInfo;
		}
	}
	else if(cn == "start")
	{
		std::string fname;
		readed >> fname;
		startGameFromFile(fname);
	}
	else if(cn == "unlock")
	{
		std::string mxname;
		readed >> mxname;
		if(mxname == "pim" && LOCPLINT)
			LOCPLINT->pim->unlock();
	}
	else if(cn == "setBattleAI")
	{
		std::string fname;
		readed >> fname;
		tlog0 << "Will try loading that AI to see if it is correct name...\n";
		if(auto ai = CDynLibHandler::getNewBattleAI(fname)) //test that given AI is indeed available... heavy but it is easy to make a typo and break the game
		{
			delete ai;
			Settings neutralAI = settings.write["server"]["neutralAI"];
			neutralAI->String() = fname;
			tlog0 << "Setting changed, from now the battle ai will be " << fname << "!\n";
		}
		else
		{
			tlog3 << "Setting not changes, no such AI found!\n";
		}
	}
	else if(client && client->serv && client->serv->connected && LOCPLINT) //send to server
	{
		boost::unique_lock<boost::recursive_mutex> un(*LOCPLINT->pim);
		LOCPLINT->cb->sendMessage(message);
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

//used only once during initialization
static void setScreenRes(int w, int h, int bpp, bool fullscreen, bool resetVideo)
{
	// VCMI will only work with 2, 3 or 4 bytes per pixel
	vstd::amax(bpp, 16);
	vstd::amin(bpp, 32);

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
		tlog2 << "Note: SDL suggests to use " << suggestedBpp << " bpp instead of"  << bpp << " bpp "  << std::endl;
	}

	//For some reason changing fullscreen via config window checkbox result in SDL_Quit event
	if (resetVideo)
	{
		if(screen) //screen has been already initialized
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
		SDL_InitSubSystem(SDL_INIT_VIDEO);
	}
	
	if((screen = SDL_SetVideoMode(w, h, suggestedBpp, SDL_SWSURFACE|(fullscreen?SDL_FULLSCREEN:0))) == NULL)
	{
		tlog1 << "Requested screen resolution is not available (" << w << "x" << h << "x" << suggestedBpp << "bpp)\n";
		throw std::runtime_error("Requested screen resolution is not available\n");
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
	//setResolution = true;
}

static void fullScreenChanged()
{
	boost::unique_lock<boost::recursive_mutex> lock(*LOCPLINT->pim);

	Settings full = settings.write["video"]["fullscreen"];
	const bool toFullscreen = full->Bool();

	int bitsPerPixel = screen->format->BitsPerPixel;

	bitsPerPixel = SDL_VideoModeOK(screen->w, screen->h, bitsPerPixel, SDL_SWSURFACE|(toFullscreen?SDL_FULLSCREEN:0));
	if(bitsPerPixel == 0)
	{
		tlog1 << "Error: SDL says that " << screen->w << "x" << screen->h << " resolution is not available!\n";
		return;
	}

	bool bufOnScreen = (screenBuf == screen);
	screen = SDL_SetVideoMode(screen->w, screen->h, bitsPerPixel, SDL_SWSURFACE|(toFullscreen?SDL_FULLSCREEN:0));
	screenBuf = bufOnScreen ? screen : screen2;

	GH.totalRedraw();
}

static void listenForEvents()
{
	SettingsListener resChanged = settings.listen["video"]["fullscreen"];
	resChanged([](const JsonNode &newState){  CGuiHandler::pushSDLEvent(SDL_USEREVENT, FULLSCREEN_TOGGLED); });

	while(1) //main SDL events loop
	{
		SDL_Event ev;

		//tlog0 << "Waiting... ";
		int ret = SDL_WaitEvent(&ev);
		//tlog0 << "got " << (int)ev.type;
		if (ret == 0 || (ev.type==SDL_QUIT) ||
			(ev.type == SDL_KEYDOWN && ev.key.keysym.sym==SDLK_F4 && (ev.key.keysym.mod & KMOD_ALT)))
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
		else if(LOCPLINT && ev.type == SDL_KEYDOWN && ev.key.keysym.sym==SDLK_F4)
		{
			Settings full = settings.write["video"]["fullscreen"];
			full->Bool() = !full->Bool();
			continue;
		}
		else if(ev.type == SDL_USEREVENT)
		{
			auto endGame = []
			{
				client->endGame();
				vstd::clear_pointer(client);

				delete CGI->dobjinfo.get();
				const_cast<CGameInfo*>(CGI)->dobjinfo = new CDefObjInfoHandler; 
				const_cast<CGameInfo*>(CGI)->dobjinfo->load();
				const_cast<CGameInfo*>(CGI)->modh->reload(); //add info about new creatures to dobjinfo
			};

			switch(ev.user.code)
			{
		/*	case CHANGE_SCREEN_RESOLUTION:
			{
				tlog0 << "Changing resolution has been requested\n";
				const JsonNode& video = settings["video"];
				const JsonNode& res = video["gameRes"];
				setScreenRes(res["width"].Float(), res["height"].Float(), video["bitsPerPixel"].Float(), video["fullscreen"].Bool());
				break;
			}*/
			case RETURN_TO_MAIN_MENU:
				{
// 					StartInfo si = *client->getStartInfo();
// 					if(si.mode == StartInfo::CAMPAIGN)
// 						GH.pushInt( new CBonusSelection(si.campState) );
// 					else
					{
						endGame();
						CGPreGame::create();
						GH.curInt = CGP;
						GH.defActionsDef = 63;
					}
				}
				break;
			case STOP_CLIENT:
				client->endGame(false);
				break;
			case RESTART_GAME:
				{
					StartInfo si = *client->getStartInfo(true);
					endGame();
					startGame(&si);
				}
				break;
			case RETURN_TO_MENU_LOAD:
				endGame();
				CGPreGame::create();
				GH.defActionsDef = 63;
				CGP->update();
				CGP->menu->switchToTab(vstd::find_pos(CGP->menu->menuNameToEntry, "load"));
				GH.curInt = CGP;
				break;
			case FULLSCREEN_TOGGLED:
				fullScreenChanged();
				break;
			default:
				tlog1 << "Error: unknown user event. Code " << ev.user.code << std::endl;
				assert(0);
			}

			continue;
		} 

		//tlog0 << " pushing ";
		{
			boost::unique_lock<boost::mutex> lock(eventsM); 
			events.push(ev);
		}
		//tlog0 << " done\n";
	}
}

void startGame(StartInfo * options, CConnection *serv/* = NULL*/)
{
	GH.curInt =NULL;
	SDL_FillRect(screen, 0, 0);
	if(gOnlyAI)
	{
		for(auto it = options->playerInfos.begin(); it != options->playerInfos.end(); ++it)
		{
			it->second.human = false;
		}
	}
/*	const JsonNode& res = settings["video"]["screenRes"];

	if(screen->w != res["width"].Float()   ||   screen->h != res["height"].Float())
	{
		requestChangingResolution();

		//allow event handling thread change resolution
		auto unlock = vstd::makeUnlockGuard(eventsM);
		while(!setResolution) boost::this_thread::sleep(boost::posix_time::milliseconds(50));
	}
	else
		setResolution = true;*/

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
/*
void requestChangingResolution()
{
	//mark that we are going to change resolution
	setResolution = false;

	//push special event to order event reading thread to change resolution
	SDL_Event ev;
	ev.type = SDL_USEREVENT;
	ev.user.code = CHANGE_SCREEN_RESOLUTION;
	SDL_PushEvent(&ev);
}
*/
