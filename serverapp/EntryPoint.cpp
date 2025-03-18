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

#include <boost/program_options.hpp>

static const std::string SERVER_NAME_AFFIX = "server";
static const std::string SERVER_NAME = GameConstants::VCMI_VERSION + std::string(" (") + SERVER_NAME_AFFIX + ')';

static void handleCommandOptions(int argc, const char * argv[], boost::program_options::variables_map & options)
{
	boost::program_options::options_description opts("Allowed options");
	opts.add_options()
	("help,h", "display help and exit")
	("version,v", "display version information and exit")
	("run-by-client", "indicate that server launched by client on same machine")
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
	std::srand(static_cast<uint32_t>(time(nullptr)));

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
	vstd::clear_pointer(LIBRARY);

	return 0;
}
