/*
 * CVcmiTestConfig.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CVcmiTestConfig.h"

#include "../lib/CConsoleHandler.h"
#include "../lib/logging/CBasicLogConfigurator.h"
#include "../lib/VCMIDirs.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/logging/CLogger.h"
#include "../lib/CConfigHandler.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/CFilesystemLoader.h"
#include "../lib/filesystem/AdapterLoaders.h"

CVcmiTestConfig::CVcmiTestConfig()
{
	console = new CConsoleHandler();
	CBasicLogConfigurator logConfig(VCMIDirs::get().userCachePath() / "VCMI_Test_log.txt", console);
	logConfig.configureDefault();
	preinitDLL(console);
	settings.init();
	logConfig.configure();
	loadDLLClasses();
	logGlobal->info("Initialized global test setup.");

	/* TEST_DATA_DIR may be wrong, if yes below test don't run,
	find your test data folder in your build and change TEST_DATA_DIR for it*/
	const std::string TEST_DATA_DIR = "test/testdata/";
	auto path = boost::filesystem::current_path();
	path += "/" + TEST_DATA_DIR;
	if(boost::filesystem::exists(path))
	{
		auto loader = new CFilesystemLoader("test/", TEST_DATA_DIR);
		dynamic_cast<CFilesystemList *>(CResourceHandler::get())->addLoader(loader, false);
	}
}

CVcmiTestConfig::~CVcmiTestConfig()
{
	std::cout << "Ending global test tear-down." << std::endl;
}
