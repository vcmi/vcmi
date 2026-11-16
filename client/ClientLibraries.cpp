/*
 * ClientLibraries.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ClientLibraries.h"

#include "GameEngine.h"
#include "GameInstance.h"
#include "CServerHandler.h"
#include "gui/WindowHandler.h"
#include "mainmenu/CMainMenu.h"
#include "windows/InfoWindows.h"

#include "../lib/AsyncRunner.h"
#include "../lib/CStopWatch.h"
#include "../lib/ExceptionsCommon.h"
#include "../lib/GameLibrary.h"
#include "../lib/modding/CModHandler.h"
#include "../lib/modding/ModManager.h"
#include "../lib/modding/ModSnapshot.h"
#include "../lib/texts/MetaString.h"

ClientLibraries::ClientLibraries(std::unique_ptr<ModManager> modManager)
	: modManager(std::move(modManager))
{
}

ClientLibraries::~ClientLibraries()
{
	// loading task writes into this object and can not be interrupted
	if(ENGINE)
		ENGINE->async().wait();
}

ModManager & ClientLibraries::mods()
{
	return *modManager;
}

void ClientLibraries::loadLibrary(const ModSnapshot & mods)
{
	try
	{
		CStopWatch timer;
		auto library = std::make_unique<GameLibrary>();
		library->initializeFilesystem(mods);
		library->initializeLibrary();
		loadedLibrary = std::move(library);
		logGlobal->info("Game library reloaded: %d ms", timer.getDiff());
	}
	catch(const DataLoadingException & e)
	{
		loadingError = e.what();
		logGlobal->error("Failed to reload game library: %s", loadingError);
	}
}

void ClientLibraries::reloadLibrary(const ModSnapshot & mods, const std::function<void(bool success)> & onFinished)
{
	assert(!GAME->server().client);

	loadedLibrary.reset();
	loadingError.clear();

	if(!ENGINE)
	{
		loadLibrary(mods);
		finishReload(onFinished);
		return;
	}

	ENGINE->windows().clear();
	ENGINE->windows().createAndPushWindow<CLoadingScreen>();

	ENGINE->async().run([this, mods, onFinished]()
	{
		loadLibrary(mods);
		ENGINE->dispatchMainThread([this, onFinished]()
		{
			finishReload(onFinished);
		});
	});
}

void ClientLibraries::finishReload(const std::function<void(bool success)> & onFinished)
{
	if(!loadedLibrary)
	{
		if(ENGINE)
			ENGINE->windows().clear();

		// caller reopens its interface, the error is shown on top of it
		onFinished(false);

		if(ENGINE)
		{
			MetaString message;
			message.appendTextID("vcmi.client.errors.modReloadFailure");
			message.replaceRawString(loadingError);
			CInfoWindow::showInfoDialog(message.toString(&GAME->translator()), {});
		}
		return;
	}

	modManager->setValidatedChecksums(loadedLibrary->modh->getValidatedChecksums());
	GAME->releaseLibraryClientState();
	setGameLibrary(std::move(loadedLibrary));
	GAME->createLibraryClientState();
	onFinished(true);
	GAME->checkForModLoadingFailure();
}
