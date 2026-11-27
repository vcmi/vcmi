/*
 * ServerLibraryReloader.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ServerLibraryReloader.h"

#include "../lib/CStopWatch.h"
#include "../lib/ExceptionsCommon.h"
#include "../lib/GameLibrary.h"
#include "../lib/modding/CModHandler.h"
#include "../lib/modding/ModManager.h"

ServerLibraryReloader::ServerLibraryReloader(std::unique_ptr<ModManager> modManager)
	: modManager(std::move(modManager))
{
}

ServerLibraryReloader::~ServerLibraryReloader() = default;

const ModManager & ServerLibraryReloader::mods() const
{
	return *modManager;
}

bool ServerLibraryReloader::reload(const ModSnapshot & mods, std::string & error)
{
	try
	{
		CStopWatch timer;
		auto library = std::make_unique<GameLibrary>();
		library->initializeFilesystem(mods);
		library->initializeLibrary();
		modManager->setValidatedChecksums(library->modh->getValidatedChecksums());
		setGameLibrary(std::move(library));
		logGlobal->info("Game library reloaded: %d ms", timer.getDiff());
		return true;
	}
	catch(const DataLoadingException & e)
	{
		error = e.what();
		logGlobal->error("Failed to reload game library: %s", error);
		return false;
	}
}
