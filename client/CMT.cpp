// CMT.cpp : Defines the entry point for the console application.
//
#include "StdInc.h"
#include <SDL_mixer.h>
#include "gui/SDL_Extensions.h"
#include "CGameInfo.h"
#include "mapHandler.h"

#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/FileStream.h"
#include "CPreGame.h"
#include "windows/CCastleInterface.h"
#include "../lib/CConsoleHandler.h"
#include "gui/CCursorHandler.h"
#include "../lib/CGameState.h"
#include "../CCallback.h"
#include "CPlayerInterface.h"
#include "windows/CAdvmapInterface.h"
#include "../lib/CBuildingHandler.h"
#include "CVideoHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CCreatureHandler.h"
#include "../lib/spells/CSpellHandler.h"
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
#include "../lib/CTownHandler.h"
#include "../lib/CArtHandler.h"
#include "../lib/CScriptingModule.h"
#include "../lib/GameConstants.h"
#include "gui/CGuiHandler.h"
#include "../lib/logging/CBasicLogConfigurator.h"
#include "../lib/CondSh.h"
#include "../lib/StringConstants.h"
#include "../lib/CPlayerState.h"

#ifdef VCMI_WINDOWS
#include "SDL_syswm.h"
#endif
#include "../lib/UnlockGuard.h"
#include "CMT.h"

#if __MINGW32__
#undef main
#endif

namespace po = boost::program_options;
namespace bfs = boost::filesystem;

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
static CClient *client=nullptr;

int preferredDriverIndex = -1;
SDL_Window * mainWindow = nullptr;
SDL_Renderer * mainRenderer = nullptr;
SDL_Texture * screenTexture = nullptr;

extern boost::thread_specific_ptr<bool> inGuiThread;

SDL_Surface *screen = nullptr, //main screen surface
	*screen2 = nullptr, //and hlp surface (used to store not-active interfaces layer)
	*screenBuf = screen; //points to screen (if only advmapint is present) or screen2 (else) - should be used when updating controls which are not regularly redrawed

std::queue<SDL_Event> events;
boost::mutex eventsM;

bool gNoGUI = false;
static po::variables_map vm;

//static bool setResolution = false; //set by event handling thread after resolution is adjusted

static bool ermInteractiveMode = false; //structurize when time is right
void processCommand(const std::string &message);
static void setScreenRes(int w, int h, int bpp, bool fullscreen, bool resetVideo=true);
void dispose();
void playIntro();
static void mainLoop();
//void requestChangingResolution();
void startGame(StartInfo * options, CConnection *serv = nullptr);
void endGame();

#ifndef VCMI_WINDOWS
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>
#endif

void startGameFromFile(const bfs::path &fname)
{
	StartInfo si;
	try //attempt retrieving start info from given file
	{
		if(fname.empty() || !bfs::exists(fname))
			throw std::runtime_error("Startfile \"" + fname.string() + "\" does not exist!");

		CLoadFile out(fname);
		if (!out.sfile || !*out.sfile)
			throw std::runtime_error("Cannot read from startfile \"" + fname.string() +"\"!");
		out >> si;
	}
	catch(std::exception &e)
	{
		logGlobal->errorStream() << "Failed to start from the file: " << fname << ". Error: " << e.what()
			<< " Falling back to main menu.";
		GH.curInt = CGPreGame::create();
		return;
	}

	while(GH.topInt())
		GH.popIntTotally(GH.topInt());
	startGame(&si);
}

void init()
{
	CStopWatch tmh, pomtime;

	loadDLLClasses();
	const_cast<CGameInfo*>(CGI)->setFromLib();

	logGlobal->infoStream()<<"Initializing VCMI_Lib: "<<tmh.getDiff();


	if(!gNoGUI)
	{
		pomtime.getDiff();
		CCS->curh = new CCursorHandler;
		graphics = new Graphics(); // should be before curh->init()

		CCS->curh->initCursor();
		CCS->curh->show();
		logGlobal->infoStream()<<"Screen handler: "<<pomtime.getDiff();
		pomtime.getDiff();

		graphics->loadHeroAnims();
		logGlobal->infoStream()<<"\tMain graphics: "<<pomtime.getDiff();
		logGlobal->infoStream()<<"Initializing game graphics: "<<tmh.getDiff();

		CMessage::init();
		logGlobal->infoStream()<<"Message handler: "<<tmh.getDiff();
	}
}

static void prog_version(void)
{
	printf("%s\n", GameConstants::VCMI_VERSION.c_str());
	std::cout << VCMIDirs::get().genHelpString();
}

static void prog_help(const po::options_description &opts)
{
	printf("%s - A Heroes of Might and Magic 3 clone\n", GameConstants::VCMI_VERSION.c_str());
	printf("Copyright (C) 2007-2016 VCMI dev team - see AUTHORS file\n");
	printf("This is free software; see the source for copying conditions. There is NO\n");
	printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
	printf("\n");
	printf("Usage:\n");
	std::cout << opts;
// 	printf("  -h, --help        display this help and exit\n");
// 	printf("  -v, --version     display version information and exit\n");
}

static void SDLLogCallback(void*           userdata,
                           int             category,
                           SDL_LogPriority priority,
                           const char*     message)
{
	//todo: convert SDL log priority to vcmi log priority
	//todo: make separate log domain for SDL

	logGlobal->debugStream() << "SDL(category " << category << "; priority " <<priority <<") "<<message;
}

#ifdef VCMI_APPLE
void OSX_checkForUpdates();
#endif

#if defined(VCMI_WINDOWS) && !defined (__GNUC__)
int wmain(int argc, wchar_t* argv[])
#elif defined(VCMI_APPLE)
int SDL_main(int argc, char *argv[])
#else
int main(int argc, char** argv)
#endif
{
#ifdef VCMI_ANDROID
	// boost will crash without this
	setenv("LANG", "C", 1);
#endif
#ifdef VCMI_APPLE
	// Correct working dir executable folder (not bundle folder) so we can use executable relative paths
    std::string executablePath = argv[0];
    std::string workDir = executablePath.substr(0, executablePath.rfind('/'));
    chdir(workDir.c_str());

    // Check for updates
    OSX_checkForUpdates();

    // Check that game data is prepared. Otherwise run vcmibuilder helper application
    FILE* check = fopen((VCMIDirs::get().userDataPath() / "game_data_prepared").string().c_str(), "r");
    if (check == nullptr) {
        system("open ./vcmibuilder.app");
        return 0;
    }
    fclose(check);
#endif
    std::cout << "Starting... " << std::endl;
	po::options_description opts("Allowed options");
	opts.add_options()
		("help,h", "display help and exit")
		("version,v", "display version information and exit")
		("battle,b", po::value<std::string>(), "runs game in duel mode (battle-only")
		("start", po::value<bfs::path>(), "starts game from saved StartInfo file")
		("onlyAI", "runs without human player, all players will be default AI")
		("noGUI", "runs without GUI, implies --onlyAI")
		("ai", po::value<std::vector<std::string>>(), "AI to be used for the player, can be specified several times for the consecutive players")
		("oneGoodAI", "puts one default AI and the rest will be EmptyAI")
		("autoSkip", "automatically skip turns in GUI")
		("disable-video", "disable video player")
		("nointro,i", "skips intro movies")
        ("loadserver","specifies we are the multiplayer server for loaded games")
        ("loadnumplayers",po::value<int>(),"specifies the number of players connecting to a multiplayer game")
        ("loadhumanplayerindices",po::value<std::vector<int>>(),"Indexes of human players (0=Red, etc.)")
        ("loadplayer", po::value<int>(),"specifies which player we are in multiplayer loaded games (0=Red, etc.)")
        ("loadserverip",po::value<std::string>(),"IP for loaded game server")
		("loadserverport",po::value<std::string>(),"port for loaded game server")
		("testingport",po::value<std::string>(),"port for testing, override specified in config file")
		("testingfileprefix",po::value<std::string>(),"prefix for auto save files");

	if(argc > 1)
	{
		try
		{
			po::store(po::parse_command_line(argc, argv, opts), vm);
		}
		catch(std::exception &e)
		{
            std::cerr << "Failure during parsing command-line options:\n" << e.what() << std::endl;
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
	if(vm.count("noGUI"))
	{
		gNoGUI = true;
		vm.insert(std::pair<std::string, po::variable_value>("onlyAI", po::variable_value()));
	}

	// Have effect on X11 system only (Linux).
	// For whatever reason in fullscreen mode SDL takes "raw" mouse input from DGA X11 extension
	// (DGA = Direct graphics access). Because this is raw input (before any speed\acceleration proceesing)
	// it may result in very small \ very fast mouse when game in fullscreen mode
	putenv((char*)"SDL_VIDEO_X11_DGAMOUSE=0");

	// Init old logging system and new (temporary) logging system
	CStopWatch total, pomtime;
	std::cout.flags(std::ios::unitbuf);
	console = new CConsoleHandler;
	*console->cb = processCommand;
	console->start();
	atexit(dispose);

	const bfs::path logPath = VCMIDirs::get().userCachePath() / "VCMI_Client_log.txt";
	CBasicLogConfigurator logConfig(logPath, console);
	logConfig.configureDefault();
	logGlobal->infoStream() << "Creating console and configuring logger: " << pomtime.getDiff();
	logGlobal->infoStream() << "The log file will be saved to " << logPath;

	// Init filesystem and settings
	preinitDLL(::console);
	settings.init();

	// Init special testing settings
	Settings testingSettings = settings.write["testing"];
	if(vm.count("testingport") && vm.count("testingfileprefix"))
	{
		testingSettings["enabled"].Bool() = true;
		testingSettings["port"].String() = vm["testingport"].as<std::string>();
		testingSettings["prefix"].String() = vm["testingfileprefix"].as<std::string>();
	}

	// Initialize logging based on settings
	logConfig.configure();

	// Some basic data validation to produce better error messages in cases of incorrect install
	auto testFile = [](std::string filename, std::string message) -> bool
	{
		if (CResourceHandler::get()->existsResource(ResourceID(filename)))
			return true;

		logGlobal->errorStream() << "Error: " << message << " was not found!";
		return false;
	};

	if (!testFile("DATA/HELP.TXT", "Heroes III data") ||
		!testFile("MODS/VCMI/MOD.JSON", "VCMI data"))
	{
		exit(1); // These are unrecoverable errors
	}

	// these two are optional + some installs have them on CD and not in data directory
	testFile("VIDEO/GOOD1A.SMK", "campaign movies");
	testFile("SOUNDS/G1A.WAV", "campaign music"); //technically not a music but voiced intro sounds

	conf.init();
	logGlobal->infoStream() << "Loading settings: " << pomtime.getDiff();
	logGlobal->infoStream() << NAME;

	srand ( time(nullptr) );


	const JsonNode& video = settings["video"];
	const JsonNode& res = video["screenRes"];

	//something is really wrong...
	if (res["width"].Float() < 100 || res["height"].Float() < 100)
	{
		logGlobal->errorStream() << "Fatal error: failed to load settings!";
		logGlobal->errorStream() << "Possible reasons:";
		logGlobal->errorStream() << "\tCorrupted local configuration file at " << VCMIDirs::get().userConfigPath() << "/settings.json";
		logGlobal->errorStream() << "\tMissing or corrupted global configuration file at " << VCMIDirs::get().userConfigPath() << "/schemas/settings.json";
		logGlobal->errorStream() << "VCMI will now exit...";
		exit(EXIT_FAILURE);
	}

	if(!gNoGUI)
	{
		if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_AUDIO|SDL_INIT_NOPARACHUTE))
		{
			logGlobal->errorStream()<<"Something was wrong: "<< SDL_GetError();
			exit(-1);
		}
		GH.mainFPSmng->init(); //(!)init here AFTER SDL_Init() while using SDL for FPS management
		atexit(SDL_Quit);

		SDL_LogSetOutputFunction(&SDLLogCallback, nullptr);

		int driversCount = SDL_GetNumRenderDrivers();
		std::string preferredDriverName = video["driver"].String();

		logGlobal->infoStream() << "Found " << driversCount << " render drivers";

		for(int it = 0; it < driversCount; it++)
		{
			SDL_RendererInfo info;
			SDL_GetRenderDriverInfo(it,&info);

			std::string driverName(info.name);

			if(!preferredDriverName.empty() && driverName == preferredDriverName)
			{
				preferredDriverIndex = it;
				logGlobal->infoStream() << "\t" << driverName << " (active)";
			}
			else
				logGlobal->infoStream() << "\t" << driverName;
		}

		config::CConfigHandler::GuiOptionsMap::key_type resPair(res["width"].Float(), res["height"].Float());
		if (conf.guiOptions.count(resPair) == 0)
		{
			// selected resolution was not found - complain & fallback to something that we do have.
			logGlobal->errorStream() << "Selected resolution " << resPair.first << "x" << resPair.second << " was not found!";
			if (conf.guiOptions.empty())
			{
				logGlobal->errorStream() << "Unable to continue - no valid resolutions found! Please reinstall VCMI to fix this";
				exit(1);
			}
			else
			{
				Settings newRes = settings.write["video"]["screenRes"];
				newRes["width"].Float()  = conf.guiOptions.begin()->first.first;
				newRes["height"].Float() = conf.guiOptions.begin()->first.second;
				conf.SetResolution(newRes["width"].Float(), newRes["height"].Float());

				logGlobal->errorStream() << "Falling back to " << newRes["width"].Float() << "x" << newRes["height"].Float();
			}
		}

		setScreenRes(res["width"].Float(), res["height"].Float(), video["bitsPerPixel"].Float(), video["fullscreen"].Bool());
		logGlobal->infoStream() <<"\tInitializing screen: "<<pomtime.getDiff();
	}

	CCS = new CClientState;
	CGI = new CGameInfo; //contains all global informations about game (texts, lodHandlers, map handler etc.)
	// Initialize video
#ifdef DISABLE_VIDEO
	CCS->videoh = new CEmptyVideoPlayer;
#else
	if (!gNoGUI && !vm.count("disable-video"))
		CCS->videoh = new CVideoPlayer;
	else
		CCS->videoh = new CEmptyVideoPlayer;
#endif

	logGlobal->infoStream()<<"\tInitializing video: "<<pomtime.getDiff();

#if defined(VCMI_ANDROID)
	//on Android threaded init is broken
	#define VCMI_NO_THREADED_LOAD
#endif // defined

	//initializing audio
	CCS->soundh = new CSoundHandler;
	CCS->soundh->init();
	CCS->soundh->setVolume(settings["general"]["sound"].Float());
	CCS->musich = new CMusicHandler;
	CCS->musich->init();
	CCS->musich->setVolume(settings["general"]["music"].Float());
	logGlobal->infoStream()<<"Initializing screen and sound handling: "<<pomtime.getDiff();

#ifdef __APPLE__
	// Ctrl+click should be treated as a right click on Mac OS X
	SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");
#endif

#ifndef VCMI_NO_THREADED_LOAD
	//we can properly play intro only in the main thread, so we have to move loading to the separate thread
	boost::thread loading(init);
#else
	init();
#endif

	if(!gNoGUI )
	{
		if(!vm.count("battle") && !vm.count("nointro") && settings["video"]["showIntro"].Bool())
			playIntro();
		SDL_FillRect(screen,nullptr,0);
	}

	CSDL_Ext::update(screen);
#ifndef VCMI_NO_THREADED_LOAD
	loading.join();
#endif
	logGlobal->infoStream()<<"Initialization of VCMI (together): "<<total.getDiff();

	if(!vm.count("battle"))
	{
		Settings session = settings.write["session"];
		session["autoSkip"].Bool()  = vm.count("autoSkip");
		session["oneGoodAI"].Bool() = vm.count("oneGoodAI");
		session["aiSolo"].Bool() = false;

		bfs::path fileToStartFrom; //none by default
		if(vm.count("start"))
			fileToStartFrom = vm["start"].as<bfs::path>();

		if(!fileToStartFrom.empty() && bfs::exists(fileToStartFrom))
			startGameFromFile(fileToStartFrom); //ommit pregame and start the game using settings from file
		else
		{
			if(!fileToStartFrom.empty())
			{
				logGlobal->warnStream() << "Warning: cannot find given file to start from (" << fileToStartFrom
					<< "). Falling back to main menu.";
			}
			GH.curInt = CGPreGame::create(); //will set CGP pointer to itself
		}
	}
	else
	{
		auto  si = new StartInfo();
		si->mode = StartInfo::DUEL;
		si->mapname = vm["battle"].as<std::string>();
		si->playerInfos[PlayerColor(0)].color = PlayerColor(0);
		si->playerInfos[PlayerColor(1)].color = PlayerColor(1);
		startGame(si);
	}

	if(!gNoGUI)
	{
		mainLoop();
	}
	else
	{
		while(true)
			boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
	}

	return 0;
}

void printInfoAboutIntObject(const CIntObject *obj, int level)
{
	std::stringstream sbuffer;
	sbuffer << std::string(level, '\t');

	sbuffer << typeid(*obj).name() << " *** ";
	if (obj->active)
	{
#define PRINT(check, text) if (obj->active & CIntObject::check) sbuffer << text
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
		sbuffer << "inactive";
	sbuffer << " at " << obj->pos.x <<"x"<< obj->pos.y;
	sbuffer << " (" << obj->pos.w <<"x"<< obj->pos.h << ")";
	logGlobal->infoStream() << sbuffer.str();

	for(const CIntObject *child : obj->children)
		printInfoAboutIntObject(child, level+1);
}

void processCommand(const std::string &message)
{
	std::istringstream readed;
	readed.str(message);
	std::string cn; //command name
	readed >> cn;

// Check mantis issue 2292 for details
//	if(LOCPLINT && LOCPLINT->cingconsole)
//		LOCPLINT->cingconsole->print(message);

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
			std::cout << "erm>";
		}
	}
	else if(message==std::string("die, fool"))
	{
		exit(EXIT_SUCCESS);
	}
	else if(cn == "erm")
	{
		ermInteractiveMode = true;
		std::cout << "erm>";
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
		std::cout << "Screenbuf points to ";

		if(screenBuf == screen)
			logGlobal->errorStream() << "screen";
		else if(screenBuf == screen2)
			logGlobal->errorStream() << "screen2";
		else
			logGlobal->errorStream() << "?!?";

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
	}
	else if(message=="get txt")
	{
		std::cout << "Command accepted.\t";

		const bfs::path outPath =
			VCMIDirs::get().userCachePath() / "extracted";

		auto list = CResourceHandler::get()->getFilteredFiles([](const ResourceID & ident)
		{
			return ident.getType() == EResType::TEXT && boost::algorithm::starts_with(ident.getName(), "DATA/");
		});

		for (auto & filename : list)
		{
			const bfs::path filePath = outPath / (filename.getName() + ".TXT");

			bfs::create_directories(filePath.parent_path());

			bfs::ofstream file(filePath);
			auto text = CResourceHandler::get()->load(filename)->readAll();

			file.write((char*)text.first.get(), text.second);
		}

		std::cout << "\rExtracting done :)\n";
		std::cout << " Extracted files can be found in " << outPath << " directory\n";
	}
	else if(cn=="crash")
	{
		int *ptr = nullptr;
		*ptr = 666;
		//disaster!
	}
	else if(cn == "onlyai")
	{
		vm.insert(std::pair<std::string, po::variable_value>("onlyAI", po::variable_value()));
	}
	else if (cn == "ai")
	{
		VLC->IS_AI_ENABLED = !VLC->IS_AI_ENABLED;
		std::cout << "Current AI status: " << (VLC->IS_AI_ENABLED ? "enabled" : "disabled") << std::endl;
	}
	else if(cn == "mp" && adventureInt)
	{
		if(const CGHeroInstance *h = dynamic_cast<const CGHeroInstance *>(adventureInt->selection))
			std::cout << h->movement << "; max: " << h->maxMovePoints(true) << "/" << h->maxMovePoints(false) << std::endl;
	}
	else if(cn == "bonuses")
	{
		std::cout << "Bonuses of " << adventureInt->selection->getObjectName() << std::endl
			<< adventureInt->selection->getBonusList() << std::endl;

		std::cout << "\nInherited bonuses:\n";
		TCNodes parents;
		adventureInt->selection->getParents(parents);
		for(const CBonusSystemNode *parent : parents)
		{
			std::cout << "\nBonuses from " << typeid(*parent).name() << std::endl << parent->getBonusList() << std::endl;
		}
	}
	else if(cn == "not dialog")
	{
		LOCPLINT->showingDialog->setn(false);
	}
	else if(cn == "gui")
	{
		for(const IShowActivatable *child : GH.listInt)
		{
			if(const CIntObject *obj = dynamic_cast<const CIntObject *>(child))
				printInfoAboutIntObject(obj, 0);
			else
				std::cout << typeid(*obj).name() << std::endl;
		}
	}
	else if(cn=="tell")
	{
		std::string what;
		int id1, id2;
		readed >> what >> id1 >> id2;
		if(what == "hs")
		{
			for(const CGHeroInstance *h : LOCPLINT->cb->getHeroesInfo())
				if(h->type->ID.getNum() == id1)
					if(const CArtifactInstance *a = h->getArt(ArtifactPosition(id2)))
						std::cout << a->nodeName();
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
	else if(cn == "def2bmp")
	{
		std::string URI;
		readed >> URI;
		if (CResourceHandler::get()->existsResource(ResourceID("SPRITES/" + URI)))
		{
			CDefEssential * cde = CDefHandler::giveDefEss(URI);

			const bfs::path outPath = VCMIDirs::get().userCachePath() / "extracted" / URI;
			bfs::create_directories(outPath);

			for (size_t i = 0; i < cde->ourImages.size(); ++i)
			{
				const bfs::path filePath = outPath / (boost::lexical_cast<std::string>(i)+".bmp");
				SDL_SaveBMP(cde->ourImages[i].bitmap, filePath.string().c_str());
			}
		}
		else
			logGlobal->errorStream() << "File not found!";
	}
	else if(cn == "extract")
	{
		std::string URI;
		readed >> URI;

		if (CResourceHandler::get()->existsResource(ResourceID(URI)))
		{
			const bfs::path outPath = VCMIDirs::get().userCachePath() / "extracted" / URI;

			auto data = CResourceHandler::get()->load(ResourceID(URI))->readAll();

			bfs::create_directories(outPath.parent_path());
			bfs::ofstream outFile(outPath, bfs::ofstream::binary);
			outFile.write((char*)data.first.get(), data.second);
		}
		else
			logGlobal->errorStream() << "File not found!";
	}
	else if(cn == "setBattleAI")
	{
		std::string fname;
		readed >> fname;
		std::cout << "Will try loading that AI to see if it is correct name...\n";
		try
		{
			if(auto ai = CDynLibHandler::getNewBattleAI(fname)) //test that given AI is indeed available... heavy but it is easy to make a typo and break the game
			{
				Settings neutralAI = settings.write["server"]["neutralAI"];
				neutralAI->String() = fname;
				std::cout << "Setting changed, from now the battle ai will be " << fname << "!\n";
			}
		}
		catch(std::exception &e)
		{
			logGlobal->warnStream() << "Failed opening " << fname << ": " << e.what();
			logGlobal->warnStream() << "Setting not changes, AI not found or invalid!";
		}
	}

	auto removeGUI = [&]()
	{
		// CClient::endGame
		GH.curInt = nullptr;
		if(GH.topInt())
			GH.topInt()->deactivate();
		GH.listInt.clear();
		GH.objsToBlit.clear();
		GH.statusbar = nullptr;
		logNetwork->infoStream() << "Removed GUI.";

		LOCPLINT = nullptr;

	};
	auto giveTurn = [&](PlayerColor player)
	{
		YourTurn yt;
		yt.player = player;
		yt.daysWithoutCastle = client->getPlayer(player)->daysWithoutCastle;
		yt.applyCl(client);
	};

	Settings session = settings.write["session"];
	if(cn == "autoskip")
	{
		session["autoSkip"].Bool() = !session["autoSkip"].Bool();
	}
	else if(cn == "gosolo")
	{
		boost::unique_lock<boost::recursive_mutex> un(*LOCPLINT->pim);
		PlayerColor color;
		if(session["aiSolo"].Bool())
		{
			for(auto & elem : client->gameState()->players)
			{
				if(elem.second.human)
					client->installNewPlayerInterface(std::make_shared<CPlayerInterface>(elem.first), elem.first);
			}
		}
		else
		{
			color = LOCPLINT->playerID;
			removeGUI();
			for(auto & elem : client->gameState()->players)
			{
				if(elem.second.human)
				{
					auto AiToGive = client->aiNameForPlayer(*client->getPlayerSettings(elem.first), false);
					logNetwork->infoStream() << boost::format("Player %s will be lead by %s") % elem.first % AiToGive;
					client->installNewPlayerInterface(CDynLibHandler::getNewAI(AiToGive), elem.first);
				}
			}
			GH.totalRedraw();
			giveTurn(color);
		}
		session["aiSolo"].Bool() = !session["aiSolo"].Bool();
	}
	else if(cn == "controlai")
	{
		std::string colorName;
		readed >> colorName;
		boost::to_lower(colorName);

		boost::unique_lock<boost::recursive_mutex> un(*LOCPLINT->pim);
		PlayerColor color;
		if(LOCPLINT)
			color = LOCPLINT->playerID;
		for(auto & elem : client->gameState()->players)
		{
			if(elem.second.human || (colorName.length() &&
				elem.first.getNum() != vstd::find_pos(GameConstants::PLAYER_COLOR_NAMES, colorName)))
			{
				continue;
			}

			removeGUI();
			client->installNewPlayerInterface(std::make_shared<CPlayerInterface>(elem.first), elem.first);
		}
		GH.totalRedraw();
		if(color != PlayerColor::NEUTRAL)
			giveTurn(color);
	}
	// Check mantis issue 2292 for details
/* 	else if(client && client->serv && client->serv->connected && LOCPLINT) //send to server
	{
		boost::unique_lock<boost::recursive_mutex> un(*LOCPLINT->pim);
		LOCPLINT->cb->sendMessage(message);
	}*/
}

//plays intro, ends when intro is over or button has been pressed (handles events)
void playIntro()
{
	if(CCS->videoh->openAndPlayVideo("3DOLOGO.SMK", 60, 40, screen, true, true))
	{
		CCS->videoh->openAndPlayVideo("AZVS.SMK", 60, 80, screen, true, true);
	}
}

void dispose()
{
	if (console)
		delete console;

	// cleanup, mostly to remove false leaks from analyzer
	CResourceHandler::clear();
	if (CCS)
	{
		CCS->musich->release();
		CCS->soundh->release();
	}
	CMessage::dispose();
}

static bool checkVideoMode(int monitorIndex, int w, int h, int& bpp, bool fullscreen)
{
	SDL_DisplayMode mode;
	const int modeCount = SDL_GetNumDisplayModes(monitorIndex);
	for (int i = 0; i < modeCount; i++) {
		SDL_GetDisplayMode(0, i, &mode);
		if (!mode.w || !mode.h || (w >= mode.w && h >= mode.h)) {
			return true;
		}
	}
	return false;
}

static bool recreateWindow(int w, int h, int bpp, bool fullscreen)
{
	// VCMI will only work with 2 or 4 bytes per pixel
	vstd::amax(bpp, 16);
	vstd::amin(bpp, 32);
	if(bpp>16)
		bpp = 32;

	int suggestedBpp = bpp;

	if(!checkVideoMode(0,w,h,suggestedBpp,fullscreen))
	{
		logGlobal->errorStream() << "Error: SDL says that " << w << "x" << h << " resolution is not available!";
		return false;
	}

	bool bufOnScreen = (screenBuf == screen);

	screenBuf = nullptr; //it`s a link - just nullify

	if(nullptr != screen2)
	{
		SDL_FreeSurface(screen2);
		screen2 = nullptr;
	}


	if(nullptr != screen)
	{
		SDL_FreeSurface(screen);
		screen = nullptr;
	}


	if(nullptr != screenTexture)
	{
		SDL_DestroyTexture(screenTexture);
		screenTexture = nullptr;
	}

	if(nullptr != mainRenderer)
	{
		SDL_DestroyRenderer(mainRenderer);
		mainRenderer = nullptr;
	}

	if(nullptr != mainWindow)
	{
		SDL_DestroyWindow(mainWindow);
		mainWindow = nullptr;
	}


	if(fullscreen)
	{
		//in full-screen mode always use desktop resolution
		mainWindow = SDL_CreateWindow(NAME.c_str(), SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	}
	else
	{
		mainWindow = SDL_CreateWindow(NAME.c_str(), SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED, w, h, 0);
	}



	if(nullptr == mainWindow)
	{
		throw std::runtime_error("Unable to create window\n");
	}


	//create first available renderer if preferred not set. Use no flags, so HW accelerated will be preferred but SW renderer also will possible
	mainRenderer = SDL_CreateRenderer(mainWindow,preferredDriverIndex,0);

	if(nullptr == mainRenderer)
	{
		throw std::runtime_error("Unable to create renderer\n");
	}

	SDL_RendererInfo info;
	SDL_GetRendererInfo(mainRenderer,&info);
	logGlobal->infoStream() << "Created renderer " << info.name;

	SDL_RenderSetLogicalSize(mainRenderer, w, h);

	SDL_RenderSetViewport(mainRenderer, nullptr);



	#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
		int bmask = 0xff000000;
		int gmask = 0x00ff0000;
		int rmask = 0x0000ff00;
		int amask = 0x000000ff;
	#else
		int bmask = 0x000000ff;
		int gmask = 0x0000ff00;
		int rmask = 0x00ff0000;
		int amask = 0xFF000000;
	#endif

	screen = SDL_CreateRGBSurface(0,w,h,bpp,rmask,gmask,bmask,amask);
	if(nullptr == screen)
	{
		logGlobal->errorStream() << "Unable to create surface";
		logGlobal->errorStream() << w << " "<<  h << " "<< bpp;

		logGlobal->errorStream() << SDL_GetError();
		throw std::runtime_error("Unable to create surface");
	}
	//No blending for screen itself. Required for proper cursor rendering.
	SDL_SetSurfaceBlendMode(screen, SDL_BLENDMODE_NONE);

	screenTexture = SDL_CreateTexture(mainRenderer,
                                            SDL_PIXELFORMAT_ARGB8888,
                                            SDL_TEXTUREACCESS_STREAMING,
                                            w, h);

	if(nullptr == screenTexture)
	{
		logGlobal->errorStream() << "Unable to create screen texture";
		logGlobal->errorStream() << SDL_GetError();
		throw std::runtime_error("Unable to create screen texture");
	}

	screen2 = CSDL_Ext::copySurface(screen);


	if(nullptr == screen2)
	{
		throw std::runtime_error("Unable to copy surface\n");
	}

	screenBuf = bufOnScreen ? screen : screen2;

	SDL_SetRenderDrawColor(mainRenderer, 0, 0, 0, 0);
	SDL_RenderClear(mainRenderer);
	SDL_RenderPresent(mainRenderer);

	return true;
}

//used only once during initialization
static void setScreenRes(int w, int h, int bpp, bool fullscreen, bool resetVideo)
{
	if(!recreateWindow(w,h,bpp,fullscreen))
	{
		throw std::runtime_error("Requested screen resolution is not available\n");
	}
}

static void fullScreenChanged()
{
	boost::unique_lock<boost::recursive_mutex> lock(*LOCPLINT->pim);

	Settings full = settings.write["video"]["fullscreen"];
	const bool toFullscreen = full->Bool();

	auto bitsPerPixel = screen->format->BitsPerPixel;

	auto w = screen->w;
	auto h = screen->h;

	if(!recreateWindow(w,h,bitsPerPixel,toFullscreen))
	{
		//will return false and report error if video mode is not supported
		return;
	}

	GH.totalRedraw();
}

static void handleEvent(SDL_Event & ev)
{
	if((ev.type==SDL_QUIT) ||(ev.type == SDL_KEYDOWN && ev.key.keysym.sym==SDLK_F4 && (ev.key.keysym.mod & KMOD_ALT)))
	{
		handleQuit();
		return;
	}
	else if(ev.type == SDL_KEYDOWN && ev.key.keysym.sym==SDLK_F4)
	{
		Settings full = settings.write["video"]["fullscreen"];
		full->Bool() = !full->Bool();
		return;
	}
	else if(ev.type == SDL_USEREVENT)
	{
		switch(ev.user.code)
		{
		case FORCE_QUIT:
			{
				handleQuit(false);
				return;
			}
		    break;
		case RETURN_TO_MAIN_MENU:
			{
				endGame();
				GH.curInt = CGPreGame::create();;
				GH.defActionsDef = 63;
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
		case PREPARE_RESTART_CAMPAIGN:
			{
				auto si = reinterpret_cast<StartInfo *>(ev.user.data1);
				endGame();
				startGame(si);
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
			logGlobal->errorStream() << "Unknown user event. Code " << ev.user.code;
			break;
		}

		return;
	}
	else if(ev.type == SDL_WINDOWEVENT)
	{
		switch (ev.window.event) {
		case SDL_WINDOWEVENT_RESTORED:
			fullScreenChanged();
			break;
		}
		return;
	}
	{
		boost::unique_lock<boost::mutex> lock(eventsM);
		events.push(ev);
	}

}


static void mainLoop()
{
	SettingsListener resChanged = settings.listen["video"]["fullscreen"];
	resChanged([](const JsonNode &newState){  CGuiHandler::pushSDLEvent(SDL_USEREVENT, FULLSCREEN_TOGGLED); });

	inGuiThread.reset(new bool(true));
	GH.mainFPSmng->init();

	while(1) //main SDL events loop
	{
		SDL_Event ev;

		while(1 == SDL_PollEvent(&ev))
		{
			handleEvent(ev);
		}

		GH.renderFrame();

	}
}

void startGame(StartInfo * options, CConnection *serv/* = nullptr*/)
{
	if(vm.count("onlyAI"))
	{
		auto ais = vm.count("ai") ? vm["ai"].as<std::vector<std::string>>() : std::vector<std::string>();

		int i = 0;


		for(auto & elem : options->playerInfos)
		{
			elem.second.playerID = PlayerSettings::PLAYER_AI;
			if(i < ais.size())
				elem.second.name = ais[i++];
		}
	}

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
        if(!vm.count("loadplayer"))
            client->loadGame(fname);
        else
            client->loadGame(fname,vm.count("loadserver"),vm.count("loadhumanplayerindices") ? vm["loadhumanplayerindices"].as<std::vector<int>>() : std::vector<int>(),vm.count("loadnumplayers") ? vm["loadnumplayers"].as<int>() : 1,vm["loadplayer"].as<int>(),vm.count("loadserverip") ? vm["loadserverip"].as<std::string>() : "", vm.count("loadserverport") ? vm["loadserverport"].as<std::string>() : "3030");
		break;
	}

		client->connectionHandler = new boost::thread(&CClient::run, client);
}

void endGame()
{
	client->endGame();
	vstd::clear_pointer(client);
}

void handleQuit(bool ask/* = true*/)
{
	auto quitApplication = []()
	{
		if(client)
			endGame();

		delete console;
		console = nullptr;
		boost::this_thread::sleep(boost::posix_time::milliseconds(750));
		if(!gNoGUI)
			SDL_Quit();

		std::cout << "Ending...\n";
		exit(0);
	};

	if(client && LOCPLINT && ask)
	{
		CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);
		LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[69], quitApplication, 0);
	}
	else
	{
		quitApplication();
	}
}
