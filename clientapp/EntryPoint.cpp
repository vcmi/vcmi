/*
 * EntryPoint.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

// EntryPoint.cpp : Defines the entry point for the console application.

#include "StdInc.h"
#include "../Global.h"

#include "../client/CGameInfo.h"
#include "../client/ClientCommandManager.h"
#include "../client/CMT.h"
#include "../client/CPlayerInterface.h"
#include "../client/CServerHandler.h"
#include "../client/eventsSDL/InputHandler.h"
#include "../client/gui/CGuiHandler.h"
#include "../client/gui/CursorHandler.h"
#include "../client/gui/WindowHandler.h"
#include "../client/mainmenu/CMainMenu.h"
#include "../client/media/CEmptyVideoPlayer.h"
#include "../client/media/CMusicHandler.h"
#include "../client/media/CSoundHandler.h"
#include "../client/media/CVideoHandler.h"
#include "../client/render/Graphics.h"
#include "../client/render/IRenderHandler.h"
#include "../client/render/IScreenHandler.h"
#include "../client/lobby/CBonusSelection.h"
#include "../client/windows/CMessage.h"
#include "../client/windows/InfoWindows.h"

#include "../lib/CThreadHelper.h"
#include "../lib/ExceptionsCommon.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/logging/CBasicLogConfigurator.h"
#include "../lib/modding/IdentifierStorage.h"
#include "../lib/modding/CModHandler.h"
#include "../lib/modding/ModDescription.h"
#include "../lib/texts/CGeneralTextHandler.h"
#include "../lib/texts/MetaString.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/VCMIDirs.h"

#include <boost/program_options.hpp>
#include <vstd/StringUtils.h>

#include <SDL_main.h>
#include <SDL.h>

#ifdef VCMI_ANDROID
#include "../lib/CAndroidVMHelper.h"
#include <SDL_system.h>
#endif

#if __MINGW32__
#undef main
#endif

namespace po = boost::program_options;
namespace po_style = boost::program_options::command_line_style;

static std::atomic<bool> headlessQuit = false;
static std::optional<std::string> criticalInitializationError;

#ifndef VCMI_IOS
void processCommand(const std::string &message);
#endif
[[noreturn]] static void quitApplication();
static void mainLoop();

static CBasicLogConfigurator *logConfig;

static void init()
{
	CStopWatch tmh;
	try
	{
		loadDLLClasses();
		CGI->setFromLib();
	}
	catch (const DataLoadingException & e)
	{
		criticalInitializationError = e.what();
		return;
	}

	logGlobal->info("Initializing VCMI_Lib: %d ms", tmh.getDiff());

	// Debug code to load all maps on start
	//ClientCommandManager commandController;
	//commandController.processCommand("translate maps", false);
}

static void checkForModLoadingFailure()
{
	const auto & brokenMods = VLC->identifiersHandler->getModsWithFailedRequests();
	if (!brokenMods.empty())
	{
		MetaString messageText;
		messageText.appendTextID("vcmi.client.errors.modLoadingFailure");

		for (const auto & modID : brokenMods)
		{
			messageText.appendRawString(VLC->modh->getModInfo(modID).getName());
			messageText.appendEOL();
		}
		CInfoWindow::showInfoDialog(messageText.toString(), {});
	}
}

static void prog_version()
{
	printf("%s\n", GameConstants::VCMI_VERSION.c_str());
	std::cout << VCMIDirs::get().genHelpString();
}

static void prog_help(const po::options_description &opts)
{
	auto time = std::time(nullptr);
	printf("%s - A Heroes of Might and Magic 3 clone\n", GameConstants::VCMI_VERSION.c_str());
	printf("Copyright (C) 2007-%d VCMI dev team - see AUTHORS file\n", std::localtime(&time)->tm_year + 1900);
	printf("This is free software; see the source for copying conditions. There is NO\n");
	printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
	printf("\n");
	std::cout << opts;
}

#if defined(VCMI_WINDOWS) && !defined(__GNUC__) && defined(VCMI_WITH_DEBUG_CONSOLE)
int wmain(int argc, wchar_t* argv[])
#elif defined(VCMI_MOBILE)
int SDL_main(int argc, char *argv[])
#else
int main(int argc, char * argv[])
#endif
{
#ifdef VCMI_ANDROID
	CAndroidVMHelper::initClassloader(SDL_AndroidGetJNIEnv());
	// boost will crash without this
	setenv("LANG", "C", 1);
#endif

#if !defined(VCMI_MOBILE)
	// Correct working dir executable folder (not bundle folder) so we can use executable relative paths
	boost::filesystem::current_path(boost::filesystem::system_complete(argv[0]).parent_path());
#endif
	std::cout << "Starting... " << std::endl;
	po::options_description opts("Allowed options");
	po::variables_map vm;

	opts.add_options()
		("help,h", "display help and exit")
		("version,v", "display version information and exit")
		("testmap", po::value<std::string>(), "")
		("testsave", po::value<std::string>(), "")
		("logLocation", po::value<std::string>(), "new location for log files")
		("spectate,s", "enable spectator interface for AI-only games")
		("spectate-ignore-hero", "wont follow heroes on adventure map")
		("spectate-hero-speed", po::value<int>(), "hero movement speed on adventure map")
		("spectate-battle-speed", po::value<int>(), "battle animation speed for spectator")
		("spectate-skip-battle", "skip battles in spectator view")
		("spectate-skip-battle-result", "skip battle result window")
		("onlyAI", "allow one to run without human player, all players will be default AI")
		("headless", "runs without GUI, implies --onlyAI")
		("ai", po::value<std::vector<std::string>>(), "AI to be used for the player, can be specified several times for the consecutive players")
		("oneGoodAI", "puts one default AI and the rest will be EmptyAI")
		("autoSkip", "automatically skip turns in GUI")
		("disable-video", "disable video player")
		("nointro,i", "skips intro movies")
		("donotstartserver,d","do not attempt to start server and just connect to it instead server")
		("serverport", po::value<si64>(), "override port specified in config file")
		("savefrequency", po::value<si64>(), "limit auto save creation to each N days");

	if(argc > 1)
	{
		try
		{
			po::store(po::parse_command_line(argc, argv, opts, po_style::unix_style|po_style::case_insensitive), vm);
		}
		catch(boost::program_options::error &e)
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
	CStopWatch total;
	CStopWatch pomtime;
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

	setThreadNameLoggingOnly("MainGUI");
	boost::filesystem::path logPath = VCMIDirs::get().userLogsPath() / "VCMI_Client_log.txt";
	if(vm.count("logLocation"))
		logPath = vm["logLocation"].as<std::string>() + "/VCMI_Client_log.txt";
	logConfig = new CBasicLogConfigurator(logPath, console);
	logConfig->configureDefault();
	logGlobal->info("Starting client of '%s'", GameConstants::VCMI_VERSION);
	logGlobal->info("Creating console and configuring logger: %d ms", pomtime.getDiff());
	logGlobal->info("The log file will be saved to %s", logPath);

	// Init filesystem and settings
	try
	{
		preinitDLL(::console, false);
	}
	catch (const DataLoadingException & e)
	{
		handleFatalError(e.what(), true);
	}

	Settings session = settings.write["session"];
	auto setSettingBool = [&](std::string key, std::string arg) {
		Settings s = settings.write(vstd::split(key, "/"));
		if(vm.count(arg))
			s->Bool() = true;
		else if(s->isNull())
			s->Bool() = false;
	};
	auto setSettingInteger = [&](std::string key, std::string arg, si64 defaultValue) {
		Settings s = settings.write(vstd::split(key, "/"));
		if(vm.count(arg))
			s->Integer() = vm[arg].as<si64>();
		else if(s->isNull())
			s->Integer() = defaultValue;
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

	// Init special testing settings
	setSettingInteger("session/serverport", "serverport", 0);
	setSettingInteger("general/saveFrequency", "savefrequency", 1);

	// Initialize logging based on settings
	logConfig->configure();
	logGlobal->debug("settings = %s", settings.toJsonNode().toString());

	// Some basic data validation to produce better error messages in cases of incorrect install
	auto testFile = [](std::string filename, std::string message)
	{
		if (!CResourceHandler::get()->existsResource(ResourcePath(filename)))
			handleFatalError(message, false);
	};

	testFile("DATA/HELP.TXT", "VCMI requires Heroes III: Shadow of Death or Heroes III: Complete data files to run!");
	testFile("DATA/TENTCOLR.TXT", "Heroes III: Restoration of Erathia (including HD Edition) data files are not supported!");
	testFile("MODS/VCMI/MOD.JSON", "VCMI installation is corrupted!\nBuilt-in mod was not found!");
	testFile("DATA/NOTOSERIF-MEDIUM.TTF", "VCMI installation is corrupted!\nBuilt-in font was not found!\nManually deleting '" + VCMIDirs::get().userDataPath().string() + "/Mods/VCMI' directory (if it exists)\nor clearing app data and reimporting Heroes III files may fix this problem.");
	testFile("DATA/PLAYERS.PAL", "Heroes III data files (Data/H3Bitmap.lod) are incomplete or corruped!\n Please reinstall them.");
	testFile("SPRITES/DEFAULT.DEF", "Heroes III data files (Data/H3Sprite.lod) are incomplete or corruped!\n Please reinstall them.");

	srand ( (unsigned int)time(nullptr) );

	if(!settings["session"]["headless"].Bool())
		GH.init();

	CCS = new CClientState();
	CGI = new CGameInfo(); //contains all global information about game (texts, lodHandlers, map handler etc.)
	CSH = new CServerHandler();
	
	// Initialize video
#ifndef ENABLE_VIDEO
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
		CCS->soundh->setVolume((ui32)settings["general"]["sound"].Float());
		CCS->musich = new CMusicHandler();
		CCS->musich->setVolume((ui32)settings["general"]["music"].Float());
		logGlobal->info("Initializing screen and sound handling: %d ms", pomtime.getDiff());
	}

#ifndef VCMI_NO_THREADED_LOAD
	//we can properly play intro only in the main thread, so we have to move loading to the separate thread
	boost::thread loading([]()
	{
		setThreadName("initialize");
		init();
	});
#else
	init();
#endif

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

	if (criticalInitializationError.has_value())
	{
		handleFatalError(criticalInitializationError.value(), false);
	}

	if(!settings["session"]["headless"].Bool())
	{
		pomtime.getDiff();
		graphics = new Graphics(); // should be before curh
		GH.renderHandler().onLibraryLoadingFinished(CGI);

		CCS->curh = new CursorHandler();
		logGlobal->info("Screen handler: %d ms", pomtime.getDiff());

		CMessage::init();
		logGlobal->info("Message handler: %d ms", pomtime.getDiff());

		CCS->curh->show();
	}

	logGlobal->info("Initialization of VCMI (together): %d ms", total.getDiff());

	session["autoSkip"].Bool()  = vm.count("autoSkip");
	session["oneGoodAI"].Bool() = vm.count("oneGoodAI");
	session["aiSolo"].Bool() = false;
	
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
		auto mmenu = CMainMenu::create();
		GH.curInt = mmenu.get();

		bool playIntroVideo = !settings["session"]["headless"].Bool() && !vm.count("battle") && !vm.count("nointro") && settings["video"]["showIntro"].Bool();
		if(playIntroVideo)
			mmenu->playIntroVideos();
		else
			mmenu->playMusic();
	}
	
	std::vector<std::string> names;

	if(!settings["session"]["headless"].Bool())
	{
		checkForModLoadingFailure();
		mainLoop();
	}
	else
	{
		while(!headlessQuit)
			boost::this_thread::sleep_for(boost::chrono::milliseconds(200));

		boost::this_thread::sleep_for(boost::chrono::milliseconds(500));

		quitApplication();
	}

	return 0;
}

static void mainLoop()
{
#ifndef VCMI_UNIX
	// on Linux, name of main thread is also name of our process. Which we don't want to change
	setThreadName("MainGUI");
#endif

	while(1) //main SDL events loop
	{
		GH.input().fetchEvents();
		GH.renderFrame();
	}
}

[[noreturn]] static void quitApplicationImmediately(int error_code)
{
	// Perform quick exit without executing static destructors and let OS cleanup anything that we did not
	// We generally don't care about them and this leads to numerous issues, e.g.
	// destruction of locked mutexes (fails an assertion), even in third-party libraries (as well as native libs on Android)
	// Android - std::quick_exit is available only starting from API level 21
	// Mingw, macOS and iOS - std::quick_exit is unavailable (at least in current version of CI)
#if (defined(__ANDROID_API__) && __ANDROID_API__ < 21) || (defined(__MINGW32__)) || defined(VCMI_APPLE)
	::exit(error_code);
#else
	std::quick_exit(error_code);
#endif
}

[[noreturn]] static void quitApplication()
{
	CSH->endNetwork();

	if(!settings["session"]["headless"].Bool())
	{
		if(CSH->client)
			CSH->endGameplay();

		GH.windows().clear();
	}

	vstd::clear_pointer(CSH);

	CMM.reset();

	if(!settings["session"]["headless"].Bool())
	{
		// cleanup, mostly to remove false leaks from analyzer
		if(CCS)
		{
			delete CCS->consoleh;
			delete CCS->curh;
			delete CCS->videoh;
			delete CCS->musich;
			delete CCS->soundh;

			vstd::clear_pointer(CCS);
		}
		CMessage::dispose();

		vstd::clear_pointer(graphics);
	}

	vstd::clear_pointer(VLC);

	// sometimes leads to a hang. TODO: investigate
	//vstd::clear_pointer(console);// should be removed after everything else since used by logging

	if(!settings["session"]["headless"].Bool())
		GH.screenHandler().close();

	if(logConfig != nullptr)
	{
		logConfig->deconfigure();
		delete logConfig;
		logConfig = nullptr;
	}

	std::cout << "Ending...\n";
	quitApplicationImmediately(0);
}

void handleQuit(bool ask)
{
	if(!ask)
	{
		if(settings["session"]["headless"].Bool())
		{
			headlessQuit = true;
		}
		else
		{
			quitApplication();
		}

		return;
	}

	// FIXME: avoids crash if player attempts to close game while opening is still playing
	// use cursor handler as indicator that loading is not done yet
	// proper solution would be to abort init thread (or wait for it to finish)
	if (!CCS->curh)
	{
		quitApplicationImmediately(0);
	}

	if (LOCPLINT)
		LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[69], quitApplication, nullptr);
	else
		CInfoWindow::showYesNoDialog(CGI->generaltexth->allTexts[69], {}, quitApplication, {}, PlayerColor(1));
}

/// Notify user about encountered fatal error and terminate the game
/// TODO: decide on better location for this method
void handleFatalError(const std::string & message, bool terminate)
{
	logGlobal->error("FATAL ERROR ENCOUNTERED, VCMI WILL NOW TERMINATE");
	logGlobal->error("Reason: %s", message);

	std::string messageToShow = "Fatal error! " + message;

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal error!", messageToShow.c_str(), nullptr);

	if (terminate)
		throw std::runtime_error(message);
	else
		quitApplicationImmediately(1);
}
