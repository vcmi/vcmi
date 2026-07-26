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

#include "../lib/GameLibrary.h"

void CVcmiTestConfig::SetUp()
{
	LIBRARY = new GameLibrary;
	// useTestPreset activates the core+vcmi+vcmi-test preset (config/testModSettings.json),
	// so tests are independent of the developer's active mods and never overwrite the real
	// modSettings.json. The vcmi-test mod supplies all fixtures and flips on the HOTA map
	// format needed by TinyH3MBuilder.
	LIBRARY->initializeFilesystem(false, /*useTestPreset*/ true);
	LIBRARY->initializeLibrary();
}

void CVcmiTestConfig::TearDown()
{
	std::cout << "Ending global test tear-down." << std::endl;
}

