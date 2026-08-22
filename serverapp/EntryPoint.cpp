/*
 * EntryPoint.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include <vstd/DateUtils.h>

#include "../server/CVCMIServer.h"

#include "../lib/CConsoleHandler.h"
#include "../lib/logging/CBasicLogConfigurator.h"
#include "../lib/VCMIDirs.h"
#include "../lib/GameLibrary.h"
#include "../lib/CConfigHandler.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/modding/CModHandler.h"
#include "../lib/modding/ModManager.h"
#include "callback/EditorCallback.h"
#include "campaign/CampaignHandler.h"
#include "mapping/CMap.h"
#include "mapping/CMapHeader.h"
#include "mapping/CMapService.h"
#include "rmg/CMapGenerator.h"
#include "rmg/CMapGenOptions.h"
#include "rmg/CRmgTemplate.h"
#include "rmg/CRmgTemplateStorage.h"
#include "rmg/ConnectionReport.h"
#include "modding/ModDescription.h"
#include "texts/CGeneralTextHandler.h"
#include "../luascript/LuaModule.h"

#include <boost/program_options.hpp>

#include <vcmi/scripting/Service.h>

static const std::string SERVER_NAME_AFFIX = "server";
static const std::string SERVER_NAME = std::string(GameConstants::VCMI_PROJECT_NAME_VERSIONED) + " (" + SERVER_NAME_AFFIX + ')';

static void exportLuaApiDocs(const boost::filesystem::path & outPath)
{
	auto scriptHandler = std::make_unique<scripting::LuaModule>();
	scriptHandler->exportDocs(outPath);

	logGlobal->info("Lua API documentation export complete");
	logGlobal->info("Generated files can be found in " + outPath.string() + " directory");
}

// Temporary debug tool: generate maps with a fixed template and report how connections were realized.
// Toggles/attempts come from config/randomMap.json (the calling script rewrites it between runs).
static void runRmgBenchmark(int mapCount, int baseSeed, int mapSize, int mapLevels, const std::string & templateName)
{
	LIBRARY = new GameLibrary;
	LIBRARY->initializeFilesystem(false);
	LIBRARY->initializeLibrary();

	auto toLower = [](std::string s) { for(auto & c : s) c = std::tolower(static_cast<unsigned char>(c)); return s; };
	const std::string wanted = toLower(templateName);

	const CRmgTemplate * tmpl = nullptr;
	for(const auto * t : LIBRARY->tplh->getTemplates())
		if(toLower(t->getName()) == wanted)
			tmpl = t;

	if(!tmpl)
	{
		logGlobal->error("RMG benchmark: template %s not found", templateName);
		printf("RMG_BENCH_SUMMARY error=template-not-found\n");
		fflush(stdout);
		return;
	}

	const int players = std::clamp(tmpl->getPlayers().maxValue(), 2, static_cast<int>(PlayerColor::PLAYER_LIMIT_I));
	logGlobal->info("RMG benchmark: template %s, %d players, %dx%d, %d level(s)", tmpl->getName(), players, mapSize, mapSize, mapLevels);

	int totalConns = 0, totalDirect = 0, totalWide = 0, totalWater = 0, totalGates = 0, totalMonoliths = 0, totalCrossMono = 0, failures = 0;
	int totalNaDist = 0, totalNaOrth = 0, totalNaDiag = 0, totalNaOther = 0, totalNoGuard = 0, totalTerrain = 0;
	double sumSizeMin = 0, sumSizeMax = 0; //averaged smallest/largest actual-vs-expected zone-size ratio per map
	int sizeSamples = 0;

	for(int i = 0; i < mapCount; ++i)
	{
		CMapGenOptions opt;
		opt.setMapTemplate(tmpl);
		opt.setWidth(mapSize);
		opt.setHeight(mapSize);
		opt.setLevels(mapLevels);
		opt.setWaterContent(EWaterContent::NONE); //no water zone, so connections resolve cleanly as direct/gate/monolith
		opt.setHumanOrCpuPlayerCount(players);
		opt.setPlayerTypeForStandardPlayer(PlayerColor(0), EPlayerType::HUMAN);
		for(int p = 1; p < players; ++p)
			opt.setPlayerTypeForStandardPlayer(PlayerColor(p), EPlayerType::AI);

		try
		{
			EditorCallback cb(nullptr); //lightweight callback the map editor also uses for headless RMG
			CMapGenerator gen(opt, &cb, baseSeed + i);
			auto generatedMap = gen.generate();
			auto t = gen.getConnectionReport().totals();
			totalConns += t.total;
			totalDirect += t.direct;
			totalWide += t.wide;
			totalWater += t.water;
			totalGates += t.subterraneanGates;
			totalMonoliths += t.monoliths;
			totalCrossMono += t.crossLevelMonoliths;
			totalNaDist += t.naDistant;
			totalNaOrth += t.naOrthogonal;
			totalNaDiag += t.naDiagonal;
			totalNaOther += t.naOther;
			totalNoGuard += t.noGuard;
			totalTerrain += t.terrain;
			double sizeMin = 0, sizeMax = 0;
			if(gen.zoneSizeDeviation(sizeMin, sizeMax))
			{
				sumSizeMin += sizeMin;
				sumSizeMax += sizeMax;
				sizeSamples++;
			}
			logGlobal->info("RMG_BENCH_MAP seed=%d direct=%d gates=%d monoliths=%d crossMono=%d naDist=%d naOrth=%d naDiag=%d noGuard=%d sizeMin=%.2f sizeMax=%.2f",
				baseSeed + i, t.direct, t.subterraneanGates, t.monoliths, t.crossLevelMonoliths, t.naDistant, t.naOrthogonal, t.naDiagonal, t.noGuard, sizeMin, sizeMax);
		}
		catch(const std::exception & e)
		{
			failures++;
			logGlobal->error("RMG_BENCH_MAP seed=%d FAILED: %s", baseSeed + i, e.what());
		}
	}

	const double avgSizeMin = sizeSamples ? sumSizeMin / sizeSamples : 0.0;
	const double avgSizeMax = sizeSamples ? sumSizeMax / sizeSamples : 0.0;

	// Single machine-parseable line on stdout for the driver script
	printf("RMG_BENCH_SUMMARY maps=%d failures=%d connections=%d direct=%d wide=%d water=%d gates=%d monoliths=%d crossMono=%d"
		" naDist=%d naOrth=%d naDiag=%d naOther=%d noGuard=%d terrain=%d sizeMin=%.3f sizeMax=%.3f\n",
		mapCount, failures, totalConns, totalDirect, totalWide, totalWater, totalGates, totalMonoliths, totalCrossMono,
		totalNaDist, totalNaOrth, totalNaDiag, totalNaOther, totalNoGuard, totalTerrain, avgSizeMin, avgSizeMax);
	fflush(stdout);
}

static void generateTranslations(const std::string & modID)
{
	LIBRARY = new GameLibrary;
	LIBRARY->loadFilesystem(false);
	settings.init("config/settings.json", "vcmi:settings");

	auto mods = std::make_unique<ModManager>();

	std::string oldPresetName = mods->getActivePreset();
	mods->createNewPreset("translation-export");
	mods->activatePreset("translation-export");
	mods = std::make_unique<ModManager>();
	mods->tryEnableMods({modID});

	for (const auto & submod : mods->getModSettings(modID))
	{
		try
		{
			if (!submod.second)
				mods->tryEnableMods({modID + '.' + submod.first});
		}
		catch (const std::exception &)
		{
			// failed to enable mod - ignore, will be logged later
		}
	}

	for (const auto & submod : mods->getModSettings(modID))
		if (!submod.second)
			logGlobal->warn("Failed to enable submod %s", submod.first);

	std::map<std::string, ExportedStrings> textsByMod;
	std::vector<std::string> modsWithOverrides;

	delete LIBRARY;
	LIBRARY = new GameLibrary;
	LIBRARY->initializeFilesystem(false);
	LIBRARY->initializeLibrary();

	{
		CMapService mapService;

		logGlobal->info("Searching for available maps");
		std::unordered_set<ResourcePath> mapList = CResourceHandler::get()->getFilteredFiles([&](const ResourcePath & ident)
		{
			return ident.getType() == EResType::MAP;
		});

		std::vector<std::unique_ptr<CMap>> loadedMaps;
		std::vector<std::shared_ptr<CampaignState>> loadedCampaigns;

		logGlobal->info("Loading maps for export");
		for (auto const & mapName : mapList)
		{
			try
			{
				std::string mapModName = LIBRARY->modh->findResourceOrigin(mapName);
				if (mapModName != modID && !mapModName.starts_with(modID + '.'))
					continue;

				EditorCallback cb(nullptr);
				// load and drop loaded map - we only need loader to run over all maps
				loadedMaps.push_back(mapService.loadMap(mapName, &cb));
			}
			catch(std::exception & e)
			{
				logGlobal->warn("Map %s is invalid. Message: %s", mapName.getName(), e.what());
			}
		}

		logGlobal->info("Searching for available campaigns");
		std::unordered_set<ResourcePath> campaignList = CResourceHandler::get()->getFilteredFiles([&](const ResourcePath & ident)
		{
			return ident.getType() == EResType::CAMPAIGN;
		});

		logGlobal->info("Loading campaigns for export");
		for (auto const & campaignName : campaignList)
		{
			try
			{
				std::string campaignModName = LIBRARY->modh->findResourceOrigin(campaignName);
				if (campaignModName != modID && !campaignModName.starts_with(modID + '.'))
					continue;

				loadedCampaigns.push_back(CampaignHandler::getCampaign(campaignName.getName()));
				for (auto const & part : loadedCampaigns.back()->allScenarios())
				{
					EditorCallback cb(nullptr);
					loadedCampaigns.back()->getMap(part, &cb);
				}
			}
			catch(std::exception & e)
			{
				logGlobal->warn("Campaign %s is invalid. Message: %s", campaignName.getName(), e.what());
			}
		}

		LIBRARY->generaltexth->exportAllTexts(textsByMod, false);
	}

	for(const auto & modEntry : textsByMod)
	{
		if (modEntry.first.find('.') != std::string::npos)
		{
			for (const auto & otherModID : modEntry.second.overridenMods)
			{
				if (otherModID == modID || otherModID.starts_with(modID + '.'))
				{
					modsWithOverrides.push_back(modEntry.first);
					break;
				}
			}
		}
	}

	const boost::filesystem::path outPath = VCMIDirs::get().userExtractedPath() / "translationFull";
	boost::filesystem::create_directories(outPath);

	mods->createNewPreset("translation-export-base");
	mods->activatePreset("translation-export-base");
	mods = std::make_unique<ModManager>();
	mods->tryEnableMods({modID});

	for (const auto & submod : mods->getModSettings(modID))
	{
		try
		{
			std::string fullModID = modID + '.' + submod.first;
			bool hasOverrides = vstd::contains(modsWithOverrides, fullModID);
			if (!submod.second && !hasOverrides)
				mods->tryEnableMods({fullModID});

			if (submod.second && hasOverrides)
				mods->tryDisableMod(fullModID);
		}
		catch (const std::exception &)
		{
			// failed to enable mod - ignore, will be logged later
		}
	}


	CResourceHandler::destroy();
	delete LIBRARY;
	LIBRARY = new GameLibrary;
	LIBRARY->initializeFilesystem(false);
	LIBRARY->initializeLibrary();
	LIBRARY->generaltexth->exportAllTexts(textsByMod, false);

	for(const auto & modEntry : textsByMod)
	{
		JsonNode output;

		if (modEntry.first != modID && !modEntry.first.starts_with(modID + '.'))
			continue;

		for(const auto & stringEntry : modEntry.second.strings)
			output[stringEntry.first].String() = stringEntry.second;

		if (!output.isNull())
		{
			std::string preferredLanguage = LIBRARY->generaltexth->getPreferredLanguage();
			std::string filename = boost::replace_all_copy(modEntry.first, ".", "/Mods/");
			const boost::filesystem::path dirPath = outPath / filename / "Content/translation/";
			boost::filesystem::create_directories(dirPath);
			const boost::filesystem::path filePath = dirPath / (preferredLanguage + ".json");
			std::ofstream file(filePath.c_str());
			file << output.toString();
		}
	}

	mods->activatePreset(oldPresetName);
	mods->deletePreset("translation-export");
	mods->deletePreset("translation-export-base");
	logGlobal->info("Translation export complete");
	logGlobal->info("Extracted files can be found in " + outPath.string() + " directory\n");

}

static void handleCommandOptions(int argc, const char * argv[], boost::program_options::variables_map & options)
{
	boost::program_options::options_description opts("Allowed options");
	opts.add_options()
	("help,h", "display help and exit")
	("version,v", "display version information and exit")
	("run-by-client", "indicate that server launched by client on same machine")
	("dummy-run", "Shutdown immediately after loading was sucessful")
	("translate-mod", boost::program_options::value<std::string>(), "Export translations for specified mod")
	("export-lua-docs", boost::program_options::value<std::string>(), "Export Lua scripting API documentation to specified directory")
	("rmg-benchmark", "DEBUG: generate maps with the 8XM8 template and report connection outcomes")
	("rmg-maps", boost::program_options::value<int>(), "number of maps for --rmg-benchmark (default 20)")
	("rmg-seed", boost::program_options::value<int>(), "base random seed for --rmg-benchmark (default time)")
	("rmg-size", boost::program_options::value<int>(), "map width/height in tiles for --rmg-benchmark (default 144)")
	("rmg-levels", boost::program_options::value<int>(), "map levels for --rmg-benchmark (default 2)")
	("rmg-template", boost::program_options::value<std::string>(), "template name for --rmg-benchmark, case-insensitive (default 8XM8)")
	("port", boost::program_options::value<ui16>(), "port at which server will listen to connections from client")
	("lobby", "start server in lobby mode in which server connects to a global lobby");

	if(argc > 1)
	{
		try
		{
			boost::program_options::store(boost::program_options::parse_command_line(argc, argv, opts), options);
		}
		catch(boost::program_options::error & e)
		{
			std::cerr << "Failure during parsing command-line options:\n" << e.what() << std::endl;
		}
	}

	boost::program_options::notify(options);

	if(options.count("help"))
	{
		auto time = std::time(nullptr);
		std::tm tm = vstd::safeLocalTime(time);
		printf("%s - A Heroes of Might and Magic 3 clone\n", GameConstants::VCMI_PROJECT_NAME_VERSIONED);
		printf("Copyright (C) 2007-%d VCMI dev team - see AUTHORS file\n", tm.tm_year + 1900);
		printf("This is free software; see the source for copying conditions. There is NO\n");
		printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
		printf("\n");
		std::cout << opts;
		exit(0);
	}

	if(options.count("translate-mod"))
	{
		std::string modID = options["translate-mod"].as<std::string>();
		generateTranslations(modID);
		exit(0);
	}

	if(options.count("export-lua-docs"))
	{
		exportLuaApiDocs(options["export-lua-docs"].as<std::string>());
		exit(0);
	}

	if(options.count("rmg-benchmark"))
	{
		int maps = options.count("rmg-maps") ? options["rmg-maps"].as<int>() : 20;
		int seed = options.count("rmg-seed") ? options["rmg-seed"].as<int>() : static_cast<int>(std::time(nullptr));
		int size = options.count("rmg-size") ? options["rmg-size"].as<int>() : CMapHeader::MAP_SIZE_XLARGE;
		int levels = options.count("rmg-levels") ? options["rmg-levels"].as<int>() : 2;
		std::string tmplName = options.count("rmg-template") ? options["rmg-template"].as<std::string>() : "8XM8";
		runRmgBenchmark(maps, seed, size, levels, tmplName);
		exit(0);
	}

	if(options.count("version"))
	{
		printf("%s\n", GameConstants::VCMI_VERSION);
		std::cout << VCMIDirs::get().genHelpString();
		exit(0);
	}
}

int main(int argc, const char * argv[])
{
	// Correct working dir executable folder (not bundle folder) so we can use executable relative paths
	boost::filesystem::current_path(boost::filesystem::system_complete(argv[0]).parent_path());

	CConsoleHandler console;
	CBasicLogConfigurator logConfigurator(VCMIDirs::get().userLogsPath() / "VCMI_Server_log.txt", &console);
	logConfigurator.configureDefault();
	logGlobal->info(SERVER_NAME);

	boost::program_options::variables_map opts;
	handleCommandOptions(argc, argv, opts);
	LIBRARY = new GameLibrary;
	LIBRARY->initializeFilesystem(false);
	logConfigurator.configure();

	LIBRARY->initializeLibrary();

	if(!opts.count("dummy-run"))
	{
		bool connectToLobby = opts.count("lobby");
		bool runByClient = opts.count("runByClient");
		uint16_t port = settings["server"]["localPort"].Integer();
		if(opts.count("port"))
			port = opts["port"].as<uint16_t>();

		CVCMIServer server(port, runByClient);
		server.prepare(connectToLobby, true);
		server.run();
		// CVCMIServer destructor must be called here - before LIBRARY cleanup
	}

	logConfigurator.deconfigure();

	delete LIBRARY;
	return 0;
}
