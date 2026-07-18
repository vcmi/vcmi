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

#include "../lib/logging/CBasicLogConfigurator.h"
#include "../lib/VCMIDirs.h"
#include "../lib/GameLibrary.h"
#include "../lib/GameSettings.h"
#include "../lib/IGameSettings.h"
#include "../lib/json/JsonNode.h"
#include "../lib/logging/CLogger.h"
#include "../lib/CConfigHandler.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/CFilesystemLoader.h"
#include "../lib/filesystem/AdapterLoaders.h"
#include "../lib/mapping/MapFormatSettings.h"

void CVcmiTestConfig::SetUp()
{
	LIBRARY = new GameLibrary;
	// useTestPreset forces a separate config/testModSettings.json holding only the
	// default core+vcmi preset, so tests are independent of the developer's active
	// mods and never overwrite the real modSettings.json.
	LIBRARY->initializeFilesystem(false, /*useTestPreset*/ true);
	LIBRARY->initializeLibrary();

	// With only core+vcmi active the HOTA map format is unsupported by default and
	// HOTA fixtures (TinyH3MBuilder) fail to load. A dummy override that only flips
	// "supported" is enough: with no identifier remappings the loader uses identity
	// mapping, which is correct for the SOD-range identifiers the fixtures use.
	// Rebuild the format table to pick it up.
	JsonNode hotaFormatOverride;
	hotaFormatOverride["supported"].Bool() = true;
	LIBRARY->settingsHandler->addOverride(EGameSettings::MAP_FORMAT_HORN_OF_THE_ABYSS, hotaFormatOverride);
	LIBRARY->mapFormat = std::make_unique<MapFormatSettings>();

	/* TEST_DATA_DIR may be wrong, if yes below test don't run,
	find your test data folder in your build and change TEST_DATA_DIR for it*/
	const std::string TEST_DATA_DIR = "test/testdata/";
	auto path = boost::filesystem::current_path();
	path+= "/" + TEST_DATA_DIR;
	if(boost::filesystem::exists(path)){
		auto loader = std::make_unique<CFilesystemLoader>("test/", TEST_DATA_DIR);
		dynamic_cast<CFilesystemList*>(CResourceHandler::get("core"))->addLoader(std::move(loader), false);

		loader = std::make_unique<CFilesystemLoader>("scripts/test/lua/", TEST_DATA_DIR+"lua/");
		dynamic_cast<CFilesystemList*>(CResourceHandler::get("core"))->addLoader(std::move(loader), false);

	}
}

void CVcmiTestConfig::TearDown()
{
	std::cout << "Ending global test tear-down." << std::endl;
}

