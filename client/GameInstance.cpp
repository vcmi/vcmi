/*
 * GameInstance.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "GameInstance.h"

#include "CPlayerInterface.h"
#include "CMT.h"
#include "CServerHandler.h"
#include "mapView/mapHandler.h"
#include "globalLobby/GlobalLobbyClient.h"
#include "mainmenu/CMainMenu.h"
#include "windows/InfoWindows.h"

#include "../lib/CConfigHandler.h"
#include "../lib/GameLibrary.h"
#include "../lib/texts/CGeneralTextHandler.h"

std::unique_ptr<GameInstance> GAME = nullptr;

GameInstance::GameInstance()
	: serverInstance(std::make_unique<CServerHandler>())
	, interfaceInstance(nullptr)
{
}

GameInstance::~GameInstance() = default;

CServerHandler & GameInstance::server()
{
	if (!serverInstance)
		throw std::runtime_error("Invalid access to GameInstance::server");

	return *serverInstance;
}

CMapHandler & GameInstance::map()
{
	if (!mapInstance)
		throw std::runtime_error("Invalid access to GameInstance::map");

	return *mapInstance;
}

std::shared_ptr<CMainMenu> GameInstance::mainmenu()
{
	if(settings["session"]["headless"].Bool())
		return nullptr;

	if (!mainMenuInstance)
		mainMenuInstance = std::make_shared<CMainMenu>();

	return mainMenuInstance;
}

CPlayerInterface * GameInstance::interface()
{
	return interfaceInstance;
}

void GameInstance::setServerInstance(std::unique_ptr<CServerHandler> ptr)
{
	serverInstance = std::move(ptr);
}

void GameInstance::setMapInstance(std::unique_ptr<CMapHandler> ptr)
{
	mapInstance = std::move(ptr);
}

void GameInstance::setInterfaceInstance(CPlayerInterface * ptr)
{
	interfaceInstance = ptr;
}

void GameInstance::onGlobalLobbyInterfaceActivated()
{
	server().getGlobalLobby().activateInterface();
}

void GameInstance::onUpdate()
{
	if (interfaceInstance)
		interfaceInstance->update();
}

bool GameInstance::capturedAllEvents()
{
	if (interfaceInstance)
		return interfaceInstance->capturedAllEvents();
	else
		return false;
}

void GameInstance::onShutdownRequested(bool ask)
{
	auto doQuit = [](){ throw GameShutdownException(); };

	if(!ask)
		doQuit();
	else
	{
		if (interface())
			interface()->showYesNoDialog(LIBRARY->generaltexth->allTexts[69], doQuit, nullptr);
		else
			CInfoWindow::showYesNoDialog(LIBRARY->generaltexth->allTexts[69], {}, doQuit, {}, PlayerColor(1));
	}
}
