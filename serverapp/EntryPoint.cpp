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
#include "mapping/CMapService.h"
#include "modding/ModDescription.h"
#include "texts/CGeneralTextHandler.h"

#include <boost/program_options.hpp>

static const std::string SERVER_NAME_AFFIX = "server";
static const std::string SERVER_NAME = GameConstants::VCMI_VERSION + std::string(" (") + SERVER_NAME_AFFIX + ')';

static void generateTranslations(const std::string & modID)
{
	LIBRARY = new GameLibrary;
	LIBRARY->loadFilesystem(false);
	settings.init("config/settings.json", "vcmi:settings");

	ModManager mods;

	if (!mods.isModActive(modID))
		mods.tryEnableMods({modID});

	for (const auto & submod : mods.getModSettings(modID))
	{
		try
		{
			if (!submod.second)
				mods.tryEnableMods({modID + '.' + submod.first});
		}
		catch (const std::exception &)
		{
			// failed to enable mod - ignore, will be logged later
		}
	}

	for (const auto & submod : mods.getModSettings(modID))
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

	for (const auto & modWithOverrides : modsWithOverrides)
		mods.tryDisableMod(modWithOverrides);

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
		printf("%s - A Heroes of Might and Magic 3 clone\n", GameConstants::VCMI_VERSION.c_str());
		printf("Copyright (C) 2007-%d VCMI dev team - see AUTHORS file\n", std::localtime(&time)->tm_year + 1900);
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

	if(options.count("version"))
	{
		printf("%s\n", GameConstants::VCMI_VERSION.c_str());
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
