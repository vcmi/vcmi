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

#include "CServerHandler.h"
#include "mapView/mapHandler.h"

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
	interfaceInstance = std::move(ptr);
}
