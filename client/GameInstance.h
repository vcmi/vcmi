/*
 * GameInstance.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "GameEngineUser.h"

class CServerHandler;
class GlobalLobbyClient;
class CPlayerInterface;
class CMapHandler;

VCMI_LIB_NAMESPACE_BEGIN
class INetworkHandler;
VCMI_LIB_NAMESPACE_END

class GameInstance final : boost::noncopyable, public IGameEngineUser
{
	std::unique_ptr<CServerHandler> serverInstance;
	std::unique_ptr<CMapHandler> mapInstance;
	CPlayerInterface * interfaceInstance;

public:
	GameInstance();
	~GameInstance();

	CServerHandler & server();
	CMapHandler & map();

	CPlayerInterface * interface();

	void setServerInstance(std::unique_ptr<CServerHandler> ptr);
	void setMapInstance(std::unique_ptr<CMapHandler> ptr);
	void setInterfaceInstance(CPlayerInterface * ptr);

	void onGlobalLobbyInterfaceActivated() final;
	void onUpdate() final;
	bool capturedAllEvents() final;
};

extern std::unique_ptr<GameInstance> GAME;
