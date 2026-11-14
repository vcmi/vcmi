/*
 * editorstartup.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "editorstartup.h"

#include "../lib/CConfigHandler.h"
#include "../lib/CConsoleHandler.h"
#include "../lib/GameConstants.h"
#include "../lib/GameLibrary.h"
#include "../lib/VCMIDirs.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/logging/CBasicLogConfigurator.h"
#include "../lib/modding/CModHandler.h"
#include "../lib/modding/ModManager.h"
#include "../lib/texts/TextOperations.h"

EditorStartup::EditorStartup() = default;
EditorStartup::~EditorStartup() = default;

bool EditorStartup::initFilesystem(bool extractArchives)
{
	const boost::filesystem::path logPath = VCMIDirs::get().userLogsPath() / "VCMI_Editor_log.txt";
#ifndef VCMI_MOBILE
	console = std::make_unique<CConsoleHandler>();
	logConfig = std::make_unique<CBasicLogConfigurator>(logPath, console.get());
#else
	logConfig = std::make_unique<CBasicLogConfigurator>(logPath, nullptr);
#endif
	logConfig->configureDefault();
	logGlobal->info("Starting map editor of '%s'", GameConstants::VCMI_PROJECT_NAME_VERSIONED);
	logGlobal->info("The log file will be saved to %s", TextOperations::filesystemPathToUtf8(logPath));

	initializeBaseFilesystem(extractArchives);
	mods = std::make_unique<ModManager>();
	library = std::make_unique<GameLibrary>();
	library->initializeFilesystem(mods->createSnapshot());

	logConfig->configure();
	logGlobal->debug("settings = %s", settings.toJsonNode().toString());

	auto testFile = [](const std::string & filename, const std::string & message) -> bool
	{
		if(CResourceHandler::get()->existsResource(ResourcePath(filename)))
			return true;

		logGlobal->error("Error: %s was not found!", message);
		return false;
	};

	if(!testFile("DATA/HELP.TXT", "Heroes III data") ||
		!testFile("MODS/VCMI/MOD.JSON", "VCMI data"))
	{
		return false;
	}

	return true;
}

void EditorStartup::initLibrary()
{
	library->initializeLibrary();
	mods->setValidatedChecksums(library->modh->getValidatedChecksums());
	setGameLibrary(std::move(library));

	Settings config = settings.write["session"]["editor"];
	config->Bool() = true;

	logGlobal->info("Initializing VCMI_Lib");
}
