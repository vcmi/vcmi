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
#include <SDL_mixer.h>

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
#include "../lib/CGeneralTextHandler.h"
#include "Graphics.h"
#include "Client.h"
#include "../lib/CConfigHandler.h"
#include "../lib/serializer/BinaryDeserializer.h"
#include "../lib/serializer/BinarySerializer.h"
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
#include "../lib/StringConstants.h"
#include "../lib/CPlayerState.h"
#include "gui/CAnimation.h"
#include "../lib/serializer/Connection.h"
#include "CServerHandler.h"

#include <boost/asio.hpp>

#include "mainmenu/CPrologEpilogVideo.h"
#include <vstd/StringUtils.h>

#ifdef VCMI_WINDOWS
#include "SDL_syswm.h"
#endif
#ifdef VCMI_ANDROID
#include "lib/CAndroidVMHelper.h"
#endif
#include "../lib/UnlockGuard.h"
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

std::queue<SDL_Event> events;
boost::mutex eventsM;

static po::variables_map vm;

//static bool setResolution = false; //set by event handling thread after resolution is adjusted

static bool ermInteractiveMode = false; //structurize when time is right
void processCommand(const std::string &message);
static void setScreenRes(int w, int h, int bpp, bool fullscreen, int displayIndex, bool resetVideo=true);
void dispose();
void playIntro();
static void mainLoop();

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
#elif defined(VCMI_APPLE) || defined(VCMI_ANDROID)
int SDL_main(int argc, char *argv[])
#else
int main(int argc, char * argv[])
#endif
{
#ifdef VCMI_ANDROID
	// boost will crash without this
	setenv("LANG", "C", 1);
#endif

#ifndef VCMI_ANDROID
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
		("savefrequency", po::value<si64>(), "limit auto save creation to each N days");

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
		return 0;
	}
	if(vm.count("version"))
	{
		prog_version();
		return 0;
	}

	// Init old logging system and new (temporary) logging system
	CStopWatch total, pomtime;
	std::cout.flags(std::ios::unitbuf);
	console = new CConsoleHandler();
	*console->cb = processCommand;
	console->start();

	const bfs::path logPath = VCMIDirs::get().userCachePath() / "VCMI_Client_log.txt";
	CBasicLogConfigurator logConfig(logPath, console);
	logConfig.configureDefault();
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
	logConfig.configure();
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

	srand ( time(nullptr) );


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

		config::CConfigHandler::GuiOptionsMap::key_type resPair(res["width"].Float(), res["height"].Float());
		if (conf.guiOptions.count(resPair) == 0)
		{
			// selected resolution was not found - complain & fallback to something that we do have.
			logGlobal->error("Selected resolution %dx%d was not found!", resPair.first, resPair.second);
			if (conf.guiOptions.empty())
			{
				logGlobal->error("Unable to continue - no valid resolutions found! Please reinstall VCMI to fix this");
				exit(1);
			}
			else
			{
				Settings newRes = settings.write["video"]["screenRes"];
				newRes["width"].Float()  = conf.guiOptions.begin()->first.first;
				newRes["height"].Float() = conf.guiOptions.begin()->first.second;
				conf.SetResolution(newRes["width"].Float(), newRes["height"].Float());

				logGlobal->error("Falling back to %dx%d", newRes["width"].Integer(), newRes["height"].Integer());
			}
		}

		setScreenRes(res["width"].Float(), res["height"].Float(), video["bitsPerPixel"].Float(), video["fullscreen"].Bool(), video["displayIndex"].Float());
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
		CCS->soundh->setVolume(settings["general"]["sound"].Float());
		CCS->musich = new CMusicHandler();
		CCS->musich->init();
		CCS->musich->setVolume(settings["general"]["music"].Float());
		logGlobal->info("Initializing screen and sound handling: %d ms", pomtime.getDiff());
	}
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
		CCS->curh = new CCursorHandler();
		graphics = new Graphics(); // should be before curh->init()

		CCS->curh->initCursor();
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
		GH.curInt = CMainMenu::create().get();
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
	logGlobal->info(sbuffer.str());

	for(const CIntObject *child : obj->children)
		printInfoAboutIntObject(child, level+1);
}

void removeGUI()
{
	// CClient::endGame
	GH.curInt = nullptr;
	if(GH.topInt())
		GH.topInt()->deactivate();
	GH.listInt.clear();
	GH.objsToBlit.clear();
	GH.statusbar = nullptr;
	logGlobal->info("Removed GUI.");

	LOCPLINT = nullptr;
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
			if(CSH->client && CSH->client->erm)
				CSH->client->erm->executeUserCommand(message);
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
			logGlobal->error("screen");
		else if(screenBuf == screen2)
			logGlobal->error("screen2");
		else
			logGlobal->error("?!?");

		SDL_SaveBMP(screen, "Screen_c.bmp");
		SDL_SaveBMP(screen2, "Screen2_c.bmp");
	}
	else if(cn=="save")
	{
		if(!CSH->client)
		{
			std::cout << "Game in not active";
			return;
		}
		std::string fname;
		readed >> fname;
		CSH->client->save(fname);
	}
//	else if(cn=="load")
//	{
//		// TODO: this code should end the running game and manage to call startGame instead
//		std::string fname;
//		readed >> fname;
//		CSH->client->loadGame(fname);
//	}
	else if(message=="convert txt")
	{
		std::cout << "Command accepted.\t";

		const bfs::path outPath =
			VCMIDirs::get().userCachePath() / "extracted";

		bfs::create_directories(outPath);

		auto extractVector = [=](const std::vector<std::string> & source, const std::string & name)
		{
			JsonNode data(JsonNode::JsonType::DATA_VECTOR);
			size_t index = 0;
			for(auto & line : source)
			{
				JsonNode lineNode(JsonNode::JsonType::DATA_STRUCT);
				lineNode["text"].String() = line;
				lineNode["index"].Integer() = index++;
				data.Vector().push_back(lineNode);
			}

			const bfs::path filePath = outPath / (name + ".json");
			bfs::ofstream file(filePath);
			file << data.toJson();
		};

		extractVector(VLC->generaltexth->allTexts, "generalTexts");
		extractVector(VLC->generaltexth->jktexts, "jkTexts");
		extractVector(VLC->generaltexth->arraytxt, "arrayTexts");

		std::cout << "\rExtracting done :)\n";
		std::cout << " Extracted files can be found in " << outPath << " directory\n";
	}
	else if(message=="get config")
	{
		std::cout << "Command accepted.\t";

		const bfs::path outPath =
			VCMIDirs::get().userCachePath() / "extracted" / "configuration";

		bfs::create_directories(outPath);

		const std::vector<std::string> contentNames = {"heroClasses", "artifacts", "creatures", "factions", "objects", "heroes", "spells", "skills"};

		for(auto contentName : contentNames)
		{
			auto & content = VLC->modh->content[contentName];

			auto contentOutPath = outPath / contentName;
			bfs::create_directories(contentOutPath);

			for(auto & iter : content.modData)
			{
				const JsonNode & modData = iter.second.modData;

				for(auto & nameAndObject : modData.Struct())
				{
					const JsonNode & object = nameAndObject.second;

					std::string name = CModHandler::normalizeIdentifier(object.meta, "core", nameAndObject.first);

					boost::algorithm::replace_all(name,":","_");

					const bfs::path filePath = contentOutPath / (name + ".json");
					bfs::ofstream file(filePath);
					file << object.toJson();
				}
			}
		}

		std::cout << "\rExtracting done :)\n";
		std::cout << " Extracted files can be found in " << outPath << " directory\n";
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
	else if(cn == "mp" && adventureInt)
	{
		if(const CGHeroInstance *h = dynamic_cast<const CGHeroInstance *>(adventureInt->selection))
			std::cout << h->movement << "; max: " << h->maxMovePoints(true) << "/" << h->maxMovePoints(false) << std::endl;
	}
	else if(cn == "bonuses")
	{
		bool jsonFormat = (message == "bonuses json");
		auto format = [jsonFormat](const BonusList & b) -> std::string
		{
			if(jsonFormat)
				return b.toJsonNode().toJson(true);
			std::ostringstream ss;
			ss << b;
			return ss.str();
		};
		std::cout << "Bonuses of " << adventureInt->selection->getObjectName() << std::endl
			<< format(adventureInt->selection->getBonusList()) << std::endl;

		std::cout << "\nInherited bonuses:\n";
		TCNodes parents;
		adventureInt->selection->getParents(parents);
		for(const CBonusSystemNode *parent : parents)
		{
			std::cout << "\nBonuses from " << typeid(*parent).name() << std::endl << format(*parent->getAllBonuses(Selector::all, Selector::all)) << std::endl;
		}
	}
	else if(cn == "not dialog")
	{
		LOCPLINT->showingDialog->setn(false);
	}
	else if(cn == "gui")
	{
		for(auto child : GH.listInt)
		{
			if(const CIntObject *obj = dynamic_cast<const CIntObject *>(child.get()))
				printInfoAboutIntObject(obj, 0);
			else
				std::cout << typeid(*child).name() << std::endl;
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
		{
			conf->Bool() = true;
			logGlobal->info("Option %s enabled!", what);
		}
		else if (value == "off")
		{
			conf->Bool() = false;
			logGlobal->info("Option %s disabled!", what);
		}
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
		std::unique_ptr<CAnimation> anim = make_unique<CAnimation>(URI);
		anim->preload();
		anim->exportBitmaps(VCMIDirs::get().userCachePath() / "extracted");
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
			logGlobal->error("File not found!");
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
			logGlobal->warn("Failed opening %s: %s", fname, e.what());
			logGlobal->warn("Setting not changes, AI not found or invalid!");
		}
	}

	auto giveTurn = [&](PlayerColor player)
	{
		YourTurn yt;
		yt.player = player;
		yt.daysWithoutCastle = CSH->client->getPlayer(player)->daysWithoutCastle;
		yt.applyCl(CSH->client);
	};

	Settings session = settings.write["session"];
	if(cn == "autoskip")
	{
		session["autoSkip"].Bool() = !session["autoSkip"].Bool();
	}
	else if(cn == "gosolo")
	{
		boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim);
		if(!CSH->client)
		{
			std::cout << "Game in not active";
			return;
		}
		PlayerColor color;
		if(session["aiSolo"].Bool())
		{
			for(auto & elem : CSH->client->gameState()->players)
			{
				if(elem.second.human)
					CSH->client->installNewPlayerInterface(std::make_shared<CPlayerInterface>(elem.first), elem.first);
			}
		}
		else
		{
			color = LOCPLINT->playerID;
			removeGUI();
			for(auto & elem : CSH->client->gameState()->players)
			{
				if(elem.second.human)
				{
					auto AiToGive = CSH->client->aiNameForPlayer(*CSH->client->getPlayerSettings(elem.first), false);
					logNetwork->info("Player %s will be lead by %s", elem.first, AiToGive);
					CSH->client->installNewPlayerInterface(CDynLibHandler::getNewAI(AiToGive), elem.first);
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

		boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim);
		if(!CSH->client)
		{
			std::cout << "Game in not active";
			return;
		}
		PlayerColor color;
		if(LOCPLINT)
			color = LOCPLINT->playerID;
		for(auto & elem : CSH->client->gameState()->players)
		{
			if(elem.second.human || (colorName.length() &&
				elem.first.getNum() != vstd::find_pos(GameConstants::PLAYER_COLOR_NAMES, colorName)))
			{
				continue;
			}

			removeGUI();
			CSH->client->installNewPlayerInterface(std::make_shared<CPlayerInterface>(elem.first), elem.first);
		}
		GH.totalRedraw();
		if(color != PlayerColor::NEUTRAL)
			giveTurn(color);
	}
	// Check mantis issue 2292 for details
/* 	else if(client && client->serv && client->serv->connected && LOCPLINT) //send to server
	{
		boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim);
		LOCPLINT->cb->sendMessage(message);
	}*/
}

//plays intro, ends when intro is over or button has been pressed (handles events)
void playIntro()
{
	if(CCS->videoh->openAndPlayVideo("3DOLOGO.SMK", 0, 1, true, true))
	{
		CCS->videoh->openAndPlayVideo("AZVS.SMK", 0, 1, true, true);
	}
}

void dispose()
{
	if(VLC)
	{
		delete VLC;
		VLC = nullptr;
	}

	// cleanup, mostly to remove false leaks from analyzer
	if(CCS)
	{
		CCS->musich->release();
		CCS->soundh->release();
	}
	CMessage::dispose();

	vstd::clear_pointer(graphics);

	if(console)
	{
		delete console; // should be removed after everything else since used by logging
		console = nullptr;
	}
}

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
	if(!checkVideoMode(displayIndex, w, h))
	{
		logGlobal->error("Error: SDL says that %dx%d resolution is not available!", w, h);
		return false;
	}

	bool bufOnScreen = (screenBuf == screen);
	bool realFullscreen = settings["video"]["realFullscreen"].Bool();

	cleanupRenderer();

	if(nullptr == mainWindow)
	{

	#ifdef VCMI_ANDROID
		mainWindow = SDL_CreateWindow(NAME.c_str(), SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex),SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex), 0, 0, SDL_WINDOW_FULLSCREEN);
	#else

		if(fullscreen)
		{
			if(realFullscreen)
				mainWindow = SDL_CreateWindow(NAME.c_str(), SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex), SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex), w, h, SDL_WINDOW_FULLSCREEN);
			else //in windowed full-screen mode use desktop resolution
				mainWindow = SDL_CreateWindow(NAME.c_str(), SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex),SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex), 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
		}
		else
		{
			mainWindow = SDL_CreateWindow(NAME.c_str(), SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex),SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), w, h, 0);
		}
	#endif

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
#ifndef VCMI_ANDROID

		if(fullscreen)
		{
			if(realFullscreen)
			{
				SDL_SetWindowFullscreen(mainWindow, SDL_WINDOW_FULLSCREEN);

				SDL_DisplayMode mode;
				SDL_GetDesktopDisplayMode(displayIndex, &mode);
				mode.w = w;
				mode.h = h;

				SDL_SetWindowDisplayMode(mainWindow, &mode);
			}
			else
			{
				SDL_SetWindowFullscreen(mainWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
			}

			SDL_SetWindowPosition(mainWindow, SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex), SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex));

			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
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
		SDL_RenderSetLogicalSize(mainRenderer, w, h);

#ifndef VCMI_ANDROID
		// on android this stretches the game to fit the screen, not preserving aspect and apparently this also breaks coordinates scaling in mouse events
		SDL_RenderSetViewport(mainRenderer, nullptr);
#endif
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

	screen = SDL_CreateRGBSurface(0,w,h,bpp,rmask,gmask,bmask,amask);
	if(nullptr == screen)
	{
		logGlobal->error("Unable to create surface %dx%d with %d bpp: %s", w, h, bpp, SDL_GetError());
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
			}
			break;
		case EUserEvent::RESTART_GAME:
			{
				CSH->sendStartGame();
			}
			break;
		case EUserEvent::CAMPAIGN_START_SCENARIO:
			{
				CSH->endGameplay();
				auto ourCampaign = std::shared_ptr<CCampaignState>(reinterpret_cast<CCampaignState *>(ev.user.data1));
				auto & epilogue = ourCampaign->camp->scenarios[ourCampaign->mapsConquered.back()].epilog;
				auto finisher = [=]()
				{
					if(ourCampaign->mapsRemaining.size())
					{
						CMM->openCampaignLobby(ourCampaign);
					}
				};
				if(epilogue.hasPrologEpilog)
				{
					GH.pushIntT<CPrologEpilogVideo>(epilogue, finisher);
				}
				else
				{
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
			fullScreenChanged();
			break;
		}
		return;
	}

	//preprocessing
	if(ev.type == SDL_MOUSEMOTION)
	{
		CCS->curh->cursorMove(ev.motion.x, ev.motion.y);
	}

	{
		boost::unique_lock<boost::mutex> lock(eventsM);
		events.push(ev);
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

void handleQuit(bool ask)
{
	auto quitApplication = []()
	{
		if(CSH->client)
			CSH->endGameplay();
		dispose();
		vstd::clear_pointer(console);
		boost::this_thread::sleep(boost::posix_time::milliseconds(750));
		if(!settings["session"]["headless"].Bool())
		{
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

		std::cout << "Ending...\n";
		exit(0);
	};

	if(CSH->client && LOCPLINT && ask)
	{
		CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);
		LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[69], quitApplication, nullptr);
	}
	else
	{
		quitApplication();
	}
}
