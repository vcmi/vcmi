/*
 * CMT.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
// CMT.cpp : Defines the entry point for the console application.
//
#include "StdInc.h"

#include <boost/program_options.hpp>

#include "gui/SDL_Extensions.h"
#include "CGameInfo.h"
#include "mapHandler.h"

#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/FileStream.h"
#include "mainmenu/CMainMenu.h"
#include "lobby/CSelectionBase.h"
#include "windows/CCastleInterface.h"
#include "../lib/CConsoleHandler.h"
#include "gui/CursorHandler.h"
#include "../lib/CGameState.h"
#include "../CCallback.h"
#include "CPlayerInterface.h"
#include "windows/CAdvmapInterface.h"
#include "../lib/CBuildingHandler.h"
#include "CVideoHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "CMusicHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "Graphics.h"
#include "Client.h"
#include "../lib/serializer/BinaryDeserializer.h"
#include "../lib/serializer/BinarySerializer.h"
#include "../lib/VCMIDirs.h"
#include "../lib/NetPacks.h"
#include "CMessage.h"
#include "../lib/CModHandler.h"
#include "../lib/CTownHandler.h"
#include "gui/CGuiHandler.h"
#include "../lib/logging/CBasicLogConfigurator.h"
#include "../lib/CPlayerState.h"
#include "gui/CAnimation.h"
#include "../lib/serializer/Connection.h"
#include "CServerHandler.h"
#include "gui/NotificationHandler.h"
#include "ClientCommandManager.h"

#include <boost/asio.hpp>

#include "mainmenu/CPrologEpilogVideo.h"
#include <vstd/StringUtils.h>
#include <SDL.h>

#ifdef VCMI_WINDOWS
#include "SDL_syswm.h"
#endif
#ifdef VCMI_ANDROID
#include "lib/CAndroidVMHelper.h"
#endif

#include "CMT.h"

#if __MINGW32__
#undef main
#endif

namespace po = boost::program_options;
namespace po_style = boost::program_options::command_line_style;
namespace bfs = boost::filesystem;

std::string NAME_AFFIX = "client";
std::string NAME = GameConstants::VCMI_VERSION + std::string(" (") + NAME_AFFIX + ')'; //application name
CGuiHandler GH;

int preferredDriverIndex = -1;
SDL_Window * mainWindow = nullptr;
SDL_Renderer * mainRenderer = nullptr;
SDL_Texture * screenTexture = nullptr;

extern boost::thread_specific_ptr<bool> inGuiThread;

SDL_Surface *screen = nullptr, //main screen surface
	*screen2 = nullptr, //and hlp surface (used to store not-active interfaces layer)
	*screenBuf = screen; //points to screen (if only advmapint is present) or screen2 (else) - should be used when updating controls which are not regularly redrawed

std::queue<SDL_Event> SDLEventsQueue;
boost::mutex eventsM;

static po::variables_map vm;

//static bool setResolution = false; //set by event handling thread after resolution is adjusted

#ifndef VCMI_IOS
void processCommand(const std::string &message);
#endif
static void setScreenRes(int w, int h, int bpp, bool fullscreen, int displayIndex, bool resetVideo=true);
void playIntro();
static void mainLoop();

static CBasicLogConfigurator *logConfig;

#ifndef VCMI_WINDOWS
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>
#endif

void init()
{
	CStopWatch tmh;

	loadDLLClasses();
	const_cast<CGameInfo*>(CGI)->setFromLib();

	logGlobal->info("Initializing VCMI_Lib: %d ms", tmh.getDiff());

}

static void prog_version()
{
	printf("%s\n", GameConstants::VCMI_VERSION.c_str());
	std::cout << VCMIDirs::get().genHelpString();
}

static void prog_help(const po::options_description &opts)
{
	auto time = std::time(0);
	printf("%s - A Heroes of Might and Magic 3 clone\n", GameConstants::VCMI_VERSION.c_str());
	printf("Copyright (C) 2007-%d VCMI dev team - see AUTHORS file\n", std::localtime(&time)->tm_year + 1900);
	printf("This is free software; see the source for copying conditions. There is NO\n");
	printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
	printf("\n");
	std::cout << opts;
}

static void SDLLogCallback(void*           userdata,
						   int             category,
						   SDL_LogPriority priority,
						   const char*     message)
{
	//todo: convert SDL log priority to vcmi log priority
	//todo: make separate log domain for SDL

	logGlobal->debug("SDL(category %d; priority %d) %s", category, priority, message);
}

#if defined(VCMI_WINDOWS) && !defined(__GNUC__) && defined(VCMI_WITH_DEBUG_CONSOLE)
int wmain(int argc, wchar_t* argv[])
#elif defined(VCMI_IOS) || defined(VCMI_ANDROID)
int SDL_main(int argc, char *argv[])
#else
int main(int argc, char * argv[])
#endif
{
#ifdef VCMI_ANDROID
	// boost will crash without this
	setenv("LANG", "C", 1);
#endif

#if !defined(VCMI_ANDROID) && !defined(VCMI_IOS)
	// Correct working dir executable folder (not bundle folder) so we can use executable relative paths
	boost::filesystem::current_path(boost::filesystem::system_complete(argv[0]).parent_path());
#endif
	std::cout << "Starting... " << std::endl;
	po::options_description opts("Allowed options");
	opts.add_options()
		("help,h", "display help and exit")
		("version,v", "display version information and exit")
		("disable-shm", "force disable shared memory usage")
		("enable-shm-uuid", "use UUID for shared memory identifier")
		("testmap", po::value<std::string>(), "")
		("testsave", po::value<std::string>(), "")
		("spectate,s", "enable spectator interface for AI-only games")
		("spectate-ignore-hero", "wont follow heroes on adventure map")
		("spectate-hero-speed", po::value<int>(), "hero movement speed on adventure map")
		("spectate-battle-speed", po::value<int>(), "battle animation speed for spectator")
		("spectate-skip-battle", "skip battles in spectator view")
		("spectate-skip-battle-result", "skip battle result window")
		("onlyAI", "allow to run without human player, all players will be default AI")
		("headless", "runs without GUI, implies --onlyAI")
		("ai", po::value<std::vector<std::string>>(), "AI to be used for the player, can be specified several times for the consecutive players")
		("oneGoodAI", "puts one default AI and the rest will be EmptyAI")
		("autoSkip", "automatically skip turns in GUI")
		("disable-video", "disable video player")
		("nointro,i", "skips intro movies")
		("donotstartserver,d","do not attempt to start server and just connect to it instead server")
		("serverport", po::value<si64>(), "override port specified in config file")
		("saveprefix", po::value<std::string>(), "prefix for auto save files")
		("savefrequency", po::value<si64>(), "limit auto save creation to each N days")
		("lobby", "parameters address, port, uuid to connect ro remote lobby session")
		("lobby-address", po::value<std::string>(), "address to remote lobby")
		("lobby-port", po::value<ui16>(), "port to remote lobby")
		("lobby-host", "if this client hosts session")
		("lobby-uuid", po::value<std::string>(), "uuid to the server")
		("lobby-connections", po::value<ui16>(), "connections of server")
		("lobby-username", po::value<std::string>(), "player name")
		("lobby-gamemode", po::value<ui16>(), "use 0 for new game and 1 for load game")
		("uuid", po::value<std::string>(), "uuid for the client");

	if(argc > 1)
	{
		try
		{
			po::store(po::parse_command_line(argc, argv, opts, po_style::unix_style|po_style::case_insensitive), vm);
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
#ifdef VCMI_IOS
		exit(0);
#else
		return 0;
#endif
	}
	if(vm.count("version"))
	{
		prog_version();
#ifdef VCMI_IOS
		exit(0);
#else
		return 0;
#endif
	}

	// Init old logging system and new (temporary) logging system
	CStopWatch total, pomtime;
	std::cout.flags(std::ios::unitbuf);
#ifndef VCMI_IOS
	console = new CConsoleHandler();

	auto callbackFunction = [](std::string buffer, bool calledFromIngameConsole)
	{
		ClientCommandManager commandController;
		commandController.processCommand(buffer, calledFromIngameConsole);
	};

	*console->cb = callbackFunction;
	console->start();
#endif

	const bfs::path logPath = VCMIDirs::get().userLogsPath() / "VCMI_Client_log.txt";
	logConfig = new CBasicLogConfigurator(logPath, console);
	logConfig->configureDefault();
	logGlobal->info(NAME);
	logGlobal->info("Creating console and configuring logger: %d ms", pomtime.getDiff());
	logGlobal->info("The log file will be saved to %s", logPath);

	// Init filesystem and settings
	preinitDLL(::console);
	settings.init();
	Settings session = settings.write["session"];
	auto setSettingBool = [](std::string key, std::string arg) {
		Settings s = settings.write(vstd::split(key, "/"));
		if(::vm.count(arg))
			s->Bool() = true;
		else if(s->isNull())
			s->Bool() = false;
	};
	auto setSettingInteger = [](std::string key, std::string arg, si64 defaultValue) {
		Settings s = settings.write(vstd::split(key, "/"));
		if(::vm.count(arg))
			s->Integer() = ::vm[arg].as<si64>();
		else if(s->isNull())
			s->Integer() = defaultValue;
	};
	auto setSettingString = [](std::string key, std::string arg, std::string defaultValue) {
		Settings s = settings.write(vstd::split(key, "/"));
		if(::vm.count(arg))
			s->String() = ::vm[arg].as<std::string>();
		else if(s->isNull())
			s->String() = defaultValue;
	};

	setSettingBool("session/onlyai", "onlyAI");
	if(vm.count("headless"))
	{
		session["headless"].Bool() = true;
		session["onlyai"].Bool() = true;
	}
	else if(vm.count("spectate"))
	{
		session["spectate"].Bool() = true;
		session["spectate-ignore-hero"].Bool() = vm.count("spectate-ignore-hero");
		session["spectate-skip-battle"].Bool() = vm.count("spectate-skip-battle");
		session["spectate-skip-battle-result"].Bool() = vm.count("spectate-skip-battle-result");
		if(vm.count("spectate-hero-speed"))
			session["spectate-hero-speed"].Integer() = vm["spectate-hero-speed"].as<int>();
		if(vm.count("spectate-battle-speed"))
			session["spectate-battle-speed"].Float() = vm["spectate-battle-speed"].as<int>();
	}
	// Server settings
	setSettingBool("session/donotstartserver", "donotstartserver");

	// Shared memory options
	setSettingBool("session/disable-shm", "disable-shm");
	setSettingBool("session/enable-shm-uuid", "enable-shm-uuid");

	// Init special testing settings
	setSettingInteger("session/serverport", "serverport", 0);
	setSettingString("session/saveprefix", "saveprefix", "");
	setSettingInteger("general/saveFrequency", "savefrequency", 1);

	// Initialize logging based on settings
	logConfig->configure();
	logGlobal->debug("settings = %s", settings.toJsonNode().toJson());

	// Some basic data validation to produce better error messages in cases of incorrect install
	auto testFile = [](std::string filename, std::string message) -> bool
	{
		if (CResourceHandler::get()->existsResource(ResourceID(filename)))
			return true;

		logGlobal->error("Error: %s was not found!", message);
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
	logGlobal->info("Loading settings: %d ms", pomtime.getDiff());

	srand ( (unsigned int)time(nullptr) );


	const JsonNode& video = settings["video"];
	const JsonNode& res = video["screenRes"];

	//something is really wrong...
	if (res["width"].Float() < 100 || res["height"].Float() < 100)
	{
		logGlobal->error("Fatal error: failed to load settings!");
		logGlobal->error("Possible reasons:");
		logGlobal->error("\tCorrupted local configuration file at %s/settings.json", VCMIDirs::get().userConfigPath());
		logGlobal->error("\tMissing or corrupted global configuration file at %s/schemas/settings.json", VCMIDirs::get().userConfigPath());
		logGlobal->error("VCMI will now exit...");
		exit(EXIT_FAILURE);
	}

	if(!settings["session"]["headless"].Bool())
	{
		if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_AUDIO|SDL_INIT_NOPARACHUTE))
		{
			logGlobal->error("Something was wrong: %s", SDL_GetError());
			exit(-1);
		}

		#ifdef VCMI_ANDROID
		// manually setting egl pixel format, as a possible solution for sdl2<->android problem
		// https://bugzilla.libsdl.org/show_bug.cgi?id=2291
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
		#endif // VCMI_ANDROID

		GH.mainFPSmng->init(); //(!)init here AFTER SDL_Init() while using SDL for FPS management

		SDL_LogSetOutputFunction(&SDLLogCallback, nullptr);

		int driversCount = SDL_GetNumRenderDrivers();
		std::string preferredDriverName = video["driver"].String();

		logGlobal->info("Found %d render drivers", driversCount);

		for(int it = 0; it < driversCount; it++)
		{
			SDL_RendererInfo info;
			SDL_GetRenderDriverInfo(it,&info);

			std::string driverName(info.name);

			if(!preferredDriverName.empty() && driverName == preferredDriverName)
			{
				preferredDriverIndex = it;
				logGlobal->info("\t%s (active)", driverName);
			}
			else
				logGlobal->info("\t%s", driverName);
		}

		setScreenRes((int)res["width"].Float(), (int)res["height"].Float(), (int)video["bitsPerPixel"].Float(), video["fullscreen"].Bool(), (int)video["displayIndex"].Float());
		logGlobal->info("\tInitializing screen: %d ms", pomtime.getDiff());
	}

	CCS = new CClientState();
	CGI = new CGameInfo(); //contains all global informations about game (texts, lodHandlers, map handler etc.)
	CSH = new CServerHandler();
	
	// Initialize video
#ifdef DISABLE_VIDEO
	CCS->videoh = new CEmptyVideoPlayer();
#else
	if (!settings["session"]["headless"].Bool() && !vm.count("disable-video"))
		CCS->videoh = new CVideoPlayer();
	else
		CCS->videoh = new CEmptyVideoPlayer();
#endif

	logGlobal->info("\tInitializing video: %d ms", pomtime.getDiff());

	if(!settings["session"]["headless"].Bool())
	{
		//initializing audio
		CCS->soundh = new CSoundHandler();
		CCS->soundh->init();
		CCS->soundh->setVolume((ui32)settings["general"]["sound"].Float());
		CCS->musich = new CMusicHandler();
		CCS->musich->init();
		CCS->musich->setVolume((ui32)settings["general"]["music"].Float());
		logGlobal->info("Initializing screen and sound handling: %d ms", pomtime.getDiff());
	}
#ifdef VCMI_MAC
	// Ctrl+click should be treated as a right click on Mac OS X
	SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");
#endif

#ifndef VCMI_NO_THREADED_LOAD
	//we can properly play intro only in the main thread, so we have to move loading to the separate thread
	boost::thread loading(init);
#else
	init();
#endif

	if(!settings["session"]["headless"].Bool())
	{
		if(!vm.count("battle") && !vm.count("nointro") && settings["video"]["showIntro"].Bool())
			playIntro();
		SDL_SetRenderDrawColor(mainRenderer, 0, 0, 0, 255);
		SDL_RenderClear(mainRenderer);
		SDL_RenderPresent(mainRenderer);
	}


#ifndef VCMI_NO_THREADED_LOAD
	#ifdef VCMI_ANDROID // android loads the data quite slowly so we display native progressbar to prevent having only black screen for few seconds
	{
		CAndroidVMHelper vmHelper;
		vmHelper.callStaticVoidMethod(CAndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS, "showProgress");
	#endif // ANDROID
		loading.join();
	#ifdef VCMI_ANDROID
		vmHelper.callStaticVoidMethod(CAndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS, "hideProgress");
	}
	#endif // ANDROID
#endif // THREADED

	if(!settings["session"]["headless"].Bool())
	{
		pomtime.getDiff();
		graphics = new Graphics(); // should be before curh

		CCS->curh = new CursorHandler();
		logGlobal->info("Screen handler: %d ms", pomtime.getDiff());
		pomtime.getDiff();

		graphics->load();//must be after Content loading but should be in main thread
		logGlobal->info("Main graphics: %d ms", pomtime.getDiff());

		CMessage::init();
		logGlobal->info("Message handler: %d ms", pomtime.getDiff());

		CCS->curh->show();
	}


	logGlobal->info("Initialization of VCMI (together): %d ms", total.getDiff());

	session["autoSkip"].Bool()  = vm.count("autoSkip");
	session["oneGoodAI"].Bool() = vm.count("oneGoodAI");
	session["aiSolo"].Bool() = false;
	std::shared_ptr<CMainMenu> mmenu;
	
	if(vm.count("testmap"))
	{
		session["testmap"].String() = vm["testmap"].as<std::string>();
		session["onlyai"].Bool() = true;
		boost::thread(&CServerHandler::debugStartTest, CSH, session["testmap"].String(), false);
	}
	else if(vm.count("testsave"))
	{
		session["testsave"].String() = vm["testsave"].as<std::string>();
		session["onlyai"].Bool() = true;
		boost::thread(&CServerHandler::debugStartTest, CSH, session["testsave"].String(), true);
	}
	else
	{
		mmenu = CMainMenu::create();
		GH.curInt = mmenu.get();
	}
	
	std::vector<std::string> names;
	session["lobby"].Bool() = false;
	if(vm.count("lobby"))
	{
		session["lobby"].Bool() = true;
		session["host"].Bool() = false;
		session["address"].String() = vm["lobby-address"].as<std::string>();
		if(vm.count("lobby-username"))
			session["username"].String() = vm["lobby-username"].as<std::string>();
		else
			session["username"].String() = settings["launcher"]["lobbyUsername"].String();
		if(vm.count("lobby-gamemode"))
			session["gamemode"].Integer() = vm["lobby-gamemode"].as<ui16>();
		else
			session["gamemode"].Integer() = 0;
		CSH->uuid = vm["uuid"].as<std::string>();
		session["port"].Integer() = vm["lobby-port"].as<ui16>();
		logGlobal->info("Remote lobby mode at %s:%d, uuid is %s", session["address"].String(), session["port"].Integer(), CSH->uuid);
		if(vm.count("lobby-host"))
		{
			session["host"].Bool() = true;
			session["hostConnections"].String() = std::to_string(vm["lobby-connections"].as<ui16>());
			session["hostUuid"].String() = vm["lobby-uuid"].as<std::string>();
			logGlobal->info("This client will host session, server uuid is %s", session["hostUuid"].String());
		}
		
		//we should not reconnect to previous game in online mode
		Settings saveSession = settings.write["server"]["reconnect"];
		saveSession->Bool() = false;
		
		//start lobby immediately
		names.push_back(session["username"].String());
		ESelectionScreen sscreen = session["gamemode"].Integer() == 0 ? ESelectionScreen::newGame : ESelectionScreen::loadGame;
		mmenu->openLobby(sscreen, session["host"].Bool(), &names, ELoadMode::MULTI);
	}
	
	// Restore remote session - start game immediately
	if(settings["server"]["reconnect"].Bool())
	{
		CSH->restoreLastSession();
	}

	if(!settings["session"]["headless"].Bool())
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

//plays intro, ends when intro is over or button has been pressed (handles events)
void playIntro()
{
	if(CCS->videoh->openAndPlayVideo("3DOLOGO.SMK", 0, 1, true, true))
	{
		CCS->videoh->openAndPlayVideo("AZVS.SMK", 0, 1, true, true);
	}
}

#ifndef VCMI_IOS
static bool checkVideoMode(int monitorIndex, int w, int h)
{
	//we only check that our desired window size fits on screen
	SDL_DisplayMode mode;

	if (0 != SDL_GetDesktopDisplayMode(monitorIndex, &mode))
	{
		logGlobal->error("SDL_GetDesktopDisplayMode failed");
		logGlobal->error(SDL_GetError());
		return false;
	}

	logGlobal->info("Check display mode: requested %d x %d; available up to %d x %d ", w, h, mode.w, mode.h);

	if (!mode.w || !mode.h || (w <= mode.w && h <= mode.h))
	{
		return true;
	}

	return false;
}
#endif

static void cleanupRenderer()
{
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
}

static bool recreateWindow(int w, int h, int bpp, bool fullscreen, int displayIndex)
{
	// VCMI will only work with 2 or 4 bytes per pixel
	vstd::amax(bpp, 16);
	vstd::amin(bpp, 32);
	if(bpp>16)
		bpp = 32;

	if(displayIndex < 0)
	{
		if (mainWindow != nullptr)
			displayIndex = SDL_GetWindowDisplayIndex(mainWindow);
		if (displayIndex < 0)
			displayIndex = 0;
	}

#if defined(VCMI_ANDROID) || defined(VCMI_IOS)
	SDL_GetWindowSize(mainWindow, &w, &h);
#else
	if(!checkVideoMode(displayIndex, w, h))
	{
		logGlobal->error("Error: SDL says that %dx%d resolution is not available!", w, h);
	}
#endif

	bool bufOnScreen = (screenBuf == screen);
	bool realFullscreen = settings["video"]["realFullscreen"].Bool();

	/* match best rendering resolution */
	int renderWidth = 0, renderHeight = 0;
	auto aspectRatio = (float)w / (float)h;
	auto minDiff = 10.f;
	for (const auto& pair : conf.guiOptions)
	{
		int pWidth, pHeight;
		std::tie(pWidth, pHeight) = pair.first;
		/* filter out resolution which is larger than window */
		if (pWidth > w || pHeight > h)
		{
			continue;
		}
		auto ratio = (float)pWidth / (float)pHeight;
		auto diff = fabs(aspectRatio - ratio);
		/* select closest aspect ratio */
		if (diff < minDiff)
		{
			renderWidth = pWidth;
			renderHeight = pHeight;
			minDiff = diff;
		}
		/* select largest resolution meets prior conditions.
		 * since there are resolutions like 1366x768(not exactly 16:9), a deviation of 0.005 is allowed. */
		else if (fabs(diff - minDiff) < 0.005f && pWidth > renderWidth)
		{
			renderWidth = pWidth;
			renderHeight = pHeight;
		}
	}
	if (renderWidth == 0)
	{
		// no matching resolution for upscaling - complain & fallback to default resolution.
		logGlobal->error("Failed to match rendering resolution for %dx%d!", w, h);
		Settings newRes = settings.write["video"]["screenRes"];
		std::tie(w, h) = conf.guiOptions.begin()->first;
		newRes["width"].Float() = w;
		newRes["height"].Float() = h;
		conf.SetResolution(w, h);
		logGlobal->error("Falling back to %dx%d", w, h);
		renderWidth = w;
		renderHeight = h;
	}
	else
	{
		logGlobal->info("Set logical rendering resolution to %dx%d", renderWidth, renderHeight);
	}

	cleanupRenderer();

	if(nullptr == mainWindow)
	{
#if defined(VCMI_ANDROID) || defined(VCMI_IOS)
		auto createWindow = [displayIndex](Uint32 extraFlags) -> bool {
			mainWindow = SDL_CreateWindow(NAME.c_str(), SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex), SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex), 0, 0, SDL_WINDOW_FULLSCREEN | extraFlags);
			return mainWindow != nullptr;
		};

# ifdef VCMI_IOS
		SDL_SetHint(SDL_HINT_IOS_HIDE_HOME_INDICATOR, "1");
		SDL_SetHint(SDL_HINT_RETURN_KEY_HIDES_IME, "1");
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");

		Uint32 windowFlags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALLOW_HIGHDPI;
		if(!createWindow(windowFlags | SDL_WINDOW_METAL))
		{
			logGlobal->warn("Metal unavailable, using OpenGLES");
			createWindow(windowFlags);
		}
# else
		createWindow(0);
# endif // VCMI_IOS

		// SDL on mobile doesn't do proper letterboxing, and will show an annoying flickering in the blank space in case you're not using the full screen estate
		// That's why we need to make sure our width and height we'll use below have the same aspect ratio as the screen itself to ensure we fill the full screen estate

		SDL_Rect screenRect;

		if(SDL_GetDisplayBounds(0, &screenRect) == 0)
		{
			const auto screenWidth = screenRect.w;
			const auto screenHeight = screenRect.h;

			const auto aspect = static_cast<double>(screenWidth) / screenHeight;

			logGlobal->info("Screen size and aspect ratio: %dx%d (%lf)", screenWidth, screenHeight, aspect);

			if((double)w / aspect > (double)h)
			{
				h = (int)round((double)w / aspect);
			}
			else
			{
				w = (int)round((double)h * aspect);
			}

			logGlobal->info("Changing logical screen size to %dx%d", w, h);
		}
		else
		{
			logGlobal->error("Can't fix aspect ratio for screen");
		}
#else
		if(fullscreen)
		{
			if(realFullscreen)
				mainWindow = SDL_CreateWindow(NAME.c_str(), SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex), SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex), renderWidth, renderHeight, SDL_WINDOW_FULLSCREEN);
			else //in windowed full-screen mode use desktop resolution
				mainWindow = SDL_CreateWindow(NAME.c_str(), SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex),SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex), 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
		}
		else
		{
			mainWindow = SDL_CreateWindow(NAME.c_str(), SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex),SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), w, h, 0);
		}
#endif // defined(VCMI_ANDROID) || defined(VCMI_IOS)

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
		SDL_GetRendererInfo(mainRenderer, &info);
		logGlobal->info("Created renderer %s", info.name);

	}
	else
	{
#if !defined(VCMI_ANDROID) && !defined(VCMI_IOS)

		if(fullscreen)
		{
			if(realFullscreen)
			{
				SDL_SetWindowFullscreen(mainWindow, SDL_WINDOW_FULLSCREEN);

				SDL_DisplayMode mode;
				SDL_GetDesktopDisplayMode(displayIndex, &mode);
				mode.w = renderWidth;
				mode.h = renderHeight;

				SDL_SetWindowDisplayMode(mainWindow, &mode);
			}
			else
			{
				SDL_SetWindowFullscreen(mainWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
			}

			SDL_SetWindowPosition(mainWindow, SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex), SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex));

			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
		}
		else
		{
			SDL_SetWindowFullscreen(mainWindow, 0);
			SDL_SetWindowSize(mainWindow, w, h);
			SDL_SetWindowPosition(mainWindow, SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex));
		}
#endif
	}

	if(!(fullscreen && realFullscreen))
	{
		SDL_RenderSetLogicalSize(mainRenderer, renderWidth, renderHeight);

//following line is bugged not only on android, do not re-enable without checking
//#ifndef VCMI_ANDROID
//		// on android this stretches the game to fit the screen, not preserving aspect and apparently this also breaks coordinates scaling in mouse events
//		SDL_RenderSetViewport(mainRenderer, nullptr);
//#endif

	}


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

	screen = SDL_CreateRGBSurface(0,renderWidth,renderHeight,bpp,rmask,gmask,bmask,amask);
	if(nullptr == screen)
	{
		logGlobal->error("Unable to create surface %dx%d with %d bpp: %s", renderWidth, renderHeight, bpp, SDL_GetError());
		throw std::runtime_error("Unable to create surface");
	}
	//No blending for screen itself. Required for proper cursor rendering.
	SDL_SetSurfaceBlendMode(screen, SDL_BLENDMODE_NONE);

	screenTexture = SDL_CreateTexture(mainRenderer,
											SDL_PIXELFORMAT_ARGB8888,
											SDL_TEXTUREACCESS_STREAMING,
											renderWidth, renderHeight);

	if(nullptr == screenTexture)
	{
		logGlobal->error("Unable to create screen texture");
		logGlobal->error(SDL_GetError());
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

	if(!settings["session"]["headless"].Bool() && settings["general"]["notifications"].Bool())
	{
		NotificationHandler::init(mainWindow);
	}

	return true;
}

//used only once during initialization
static void setScreenRes(int w, int h, int bpp, bool fullscreen, int displayIndex, bool resetVideo)
{
	if(!recreateWindow(w, h, bpp, fullscreen, displayIndex))
	{
		throw std::runtime_error("Requested screen resolution is not available\n");
	}
}

static void fullScreenChanged()
{
	boost::unique_lock<boost::recursive_mutex> lock(*CPlayerInterface::pim);

	Settings full = settings.write["video"]["fullscreen"];
	const bool toFullscreen = full->Bool();

	auto bitsPerPixel = screen->format->BitsPerPixel;

	auto w = screen->w;
	auto h = screen->h;

	if(!recreateWindow(w, h, bitsPerPixel, toFullscreen, -1))
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
#ifdef VCMI_ANDROID
		handleQuit(false);
#else
		handleQuit();
#endif
		return;
	}
#ifdef VCMI_ANDROID
	else if (ev.type == SDL_KEYDOWN && ev.key.keysym.scancode == SDL_SCANCODE_AC_BACK)
	{
		handleQuit(true);
	}
#endif
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
		case EUserEvent::FORCE_QUIT:
			{
				handleQuit(false);
				return;
			}
			break;
		case EUserEvent::RETURN_TO_MAIN_MENU:
			{
				CSH->endGameplay();
				GH.defActionsDef = 63;
				CMM->menu->switchToTab("main");
			}
			break;
		case EUserEvent::RESTART_GAME:
			{
				CSH->sendRestartGame();
			}
			break;
		case EUserEvent::CAMPAIGN_START_SCENARIO:
			{
				CSH->campaignServerRestartLock.set(true);
				CSH->endGameplay();
				auto ourCampaign = std::shared_ptr<CCampaignState>(reinterpret_cast<CCampaignState *>(ev.user.data1));
				auto & epilogue = ourCampaign->camp->scenarios[ourCampaign->mapsConquered.back()].epilog;
				auto finisher = [=]()
				{
					if(ourCampaign->mapsRemaining.size())
					{
						GH.pushInt(CMM);
						GH.pushInt(CMM->menu);
						CMM->openCampaignLobby(ourCampaign);
					}
				};
				if(epilogue.hasPrologEpilog)
				{
					GH.pushIntT<CPrologEpilogVideo>(epilogue, finisher);
				}
				else
				{
					CSH->campaignServerRestartLock.waitUntil(false);
					finisher();
				}
			}
			break;
		case EUserEvent::RETURN_TO_MENU_LOAD:
			CSH->endGameplay();
			GH.defActionsDef = 63;
			CMM->menu->switchToTab("load");
			break;
		case EUserEvent::FULLSCREEN_TOGGLED:
			fullScreenChanged();
			break;
		case EUserEvent::INTERFACE_CHANGED:
			if(LOCPLINT)
				LOCPLINT->updateAmbientSounds();
			break;
		default:
			logGlobal->error("Unknown user event. Code %d", ev.user.code);
			break;
		}

		return;
	}
	else if(ev.type == SDL_WINDOWEVENT)
	{
		switch (ev.window.event) {
		case SDL_WINDOWEVENT_RESTORED:
#ifndef VCMI_IOS
			fullScreenChanged();
#endif
			break;
		}
		return;
	}
	else if(ev.type == SDL_SYSWMEVENT)
	{
		if(!settings["session"]["headless"].Bool() && settings["general"]["notifications"].Bool())
		{
			NotificationHandler::handleSdlEvent(ev);
		}
	}

	//preprocessing
	if(ev.type == SDL_MOUSEMOTION)
	{
		CCS->curh->cursorMove(ev.motion.x, ev.motion.y);
	}

	{
		boost::unique_lock<boost::mutex> lock(eventsM);
		SDLEventsQueue.push(ev);
	}

}


static void mainLoop()
{
	SettingsListener resChanged = settings.listen["video"]["fullscreen"];
	resChanged([](const JsonNode &newState){  CGuiHandler::pushSDLEvent(SDL_USEREVENT, EUserEvent::FULLSCREEN_TOGGLED); });

	inGuiThread.reset(new bool(true));
	GH.mainFPSmng->init();

	while(1) //main SDL events loop
	{
		SDL_Event ev;

		while(1 == SDL_PollEvent(&ev))
		{
			handleEvent(ev);
		}

		CSH->applyPacksOnLobbyScreen();
		GH.renderFrame();

	}
}

static void quitApplication()
{
	if(!settings["session"]["headless"].Bool())
	{
		if(CSH->client)
			CSH->endGameplay();
	}

	GH.listInt.clear();
	GH.objsToBlit.clear();

	CMM.reset();

	if(!settings["session"]["headless"].Bool())
	{
		// cleanup, mostly to remove false leaks from analyzer
		if(CCS)
		{
			CCS->musich->release();
			CCS->soundh->release();

			vstd::clear_pointer(CCS);
		}
		CMessage::dispose();

		vstd::clear_pointer(graphics);
	}

	vstd::clear_pointer(VLC);

	vstd::clear_pointer(console);// should be removed after everything else since used by logging

	boost::this_thread::sleep(boost::posix_time::milliseconds(750));//???
	if(!settings["session"]["headless"].Bool())
	{
		if(settings["general"]["notifications"].Bool())
		{
			NotificationHandler::destroy();
		}

		cleanupRenderer();

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

		SDL_Quit();
	}

	if(logConfig != nullptr)
	{
		logConfig->deconfigure();
		delete logConfig;
		logConfig = nullptr;
	}

	std::cout << "Ending...\n";
	exit(0);
}

void handleQuit(bool ask)
{

	if(CSH->client && LOCPLINT && ask)
	{
		CCS->curh->set(Cursor::Map::POINTER);
		LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[69], [](){
			// Workaround for assertion failure on exit:
			// handleQuit() is alway called during SDL event processing
			// during which, eventsM is kept locked
			// this leads to assertion failure if boost::mutex is in locked state
			eventsM.unlock();

			quitApplication();
		}, nullptr);
	}
	else
	{
		quitApplication();
	}
}
