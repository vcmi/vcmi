/*
 * NetPacksLobby.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "NetPacksBase.h"

#include "StartInfo.h"

class CLobbyScreen;
class CServerHandler;
class CVCMIServer;

VCMI_LIB_NAMESPACE_BEGIN

class CCampaignState;
class CMapInfo;
struct StartInfo;
class CMapGenOptions;
struct ClientPlayer;

struct CPackForLobby : public CPack
{
	bool checkClientPermissions(CVCMIServer * srv) const
	{
		return false;
	}

	bool applyOnServer(CVCMIServer * srv)
	{
		return true;
	}

	void applyOnServerAfterAnnounce(CVCMIServer * srv) {}

	bool applyOnLobbyHandler(CServerHandler * handler)
	{
		return true;
	}

	void applyOnLobbyScreen(CLobbyScreen * lobby, CServerHandler * handler) {}
};

struct CLobbyPackToPropagate : public CPackForLobby
{

};

struct CLobbyPackToServer : public CPackForLobby
{
	bool checkClientPermissions(CVCMIServer * srv) const;
	void applyOnServerAfterAnnounce(CVCMIServer * srv);
};

struct LobbyClientConnected : public CLobbyPackToPropagate
{
	// Set by client before sending pack to server
	std::string uuid;
	std::vector<std::string> names;
	StartInfo::EMode mode = StartInfo::INVALID;
	// Changed by server before announcing pack
	int clientId = -1;
	int hostClientId = -1;

	bool checkClientPermissions(CVCMIServer * srv) const;
	bool applyOnLobbyHandler(CServerHandler * handler);
	void applyOnLobbyScreen(CLobbyScreen * lobby, CServerHandler * handler);
	bool applyOnServer(CVCMIServer * srv);
	void applyOnServerAfterAnnounce(CVCMIServer * srv);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & uuid;
		h & names;
		h & mode;

		h & clientId;
		h & hostClientId;
	}
};

struct LobbyClientDisconnected : public CLobbyPackToPropagate
{
	int clientId;
	bool shutdownServer = false;

	bool checkClientPermissions(CVCMIServer * srv) const;
	bool applyOnServer(CVCMIServer * srv);
	void applyOnServerAfterAnnounce(CVCMIServer * srv);
	bool applyOnLobbyHandler(CServerHandler * handler);
	void applyOnLobbyScreen(CLobbyScreen * lobby, CServerHandler * handler);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & clientId;
		h & shutdownServer;
	}
};

struct LobbyChatMessage : public CLobbyPackToPropagate
{
	std::string playerName, message;

	bool checkClientPermissions(CVCMIServer * srv) const;
	void applyOnLobbyScreen(CLobbyScreen * lobby, CServerHandler * handler);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & playerName;
		h & message;
	}
};

struct LobbyGuiAction : public CLobbyPackToPropagate
{
	enum EAction : ui8 {
		NONE, NO_TAB, OPEN_OPTIONS, OPEN_SCENARIO_LIST, OPEN_RANDOM_MAP_OPTIONS
	} action = NONE;

	bool checkClientPermissions(CVCMIServer * srv) const;
	void applyOnLobbyScreen(CLobbyScreen * lobby, CServerHandler * handler);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & action;
	}
};

struct LobbyEndGame : public CLobbyPackToPropagate
{
	bool closeConnection = false, restart = false;
	
	bool checkClientPermissions(CVCMIServer * srv) const;
	bool applyOnServer(CVCMIServer * srv);
	void applyOnServerAfterAnnounce(CVCMIServer * srv);
	bool applyOnLobbyHandler(CServerHandler * handler);
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & closeConnection;
		h & restart;
	}
};

struct LobbyStartGame : public CLobbyPackToPropagate
{
	// Set by server
	std::shared_ptr<StartInfo> initializedStartInfo = nullptr;
	CGameState * initializedGameState = nullptr;
	int clientId = -1; //-1 means to all clients

	bool checkClientPermissions(CVCMIServer * srv) const;
	bool applyOnServer(CVCMIServer * srv);
	void applyOnServerAfterAnnounce(CVCMIServer * srv);
	bool applyOnLobbyHandler(CServerHandler * handler);
	void applyOnLobbyScreen(CLobbyScreen * lobby, CServerHandler * handler);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & clientId;
		h & initializedStartInfo;
		bool sps = h.smartPointerSerialization;
		h.smartPointerSerialization = true;
		h & initializedGameState;
		h.smartPointerSerialization = sps;
	}
};

struct LobbyChangeHost : public CLobbyPackToPropagate
{
	int newHostConnectionId = -1;

	bool checkClientPermissions(CVCMIServer * srv) const;
	bool applyOnServer(CVCMIServer * srv);
	bool applyOnServerAfterAnnounce(CVCMIServer * srv);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & newHostConnectionId;
	}
};

struct LobbyUpdateState : public CLobbyPackToPropagate
{
	LobbyState state;
	bool hostChanged = false; // Used on client-side only

	bool applyOnLobbyHandler(CServerHandler * handler);
	void applyOnLobbyScreen(CLobbyScreen * lobby, CServerHandler * handler);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & state;
	}
};

struct LobbySetMap : public CLobbyPackToServer
{
	std::shared_ptr<CMapInfo> mapInfo;
	std::shared_ptr<CMapGenOptions> mapGenOpts;

	LobbySetMap() : mapInfo(nullptr), mapGenOpts(nullptr) {}
	bool applyOnServer(CVCMIServer * srv);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & mapInfo;
		h & mapGenOpts;
	}
};

struct LobbySetCampaign : public CLobbyPackToServer
{
	std::shared_ptr<CCampaignState> ourCampaign;

	bool applyOnServer(CVCMIServer * srv);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ourCampaign;
	}
};

struct LobbySetCampaignMap : public CLobbyPackToServer
{
	int mapId = -1;

	bool applyOnServer(CVCMIServer * srv);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & mapId;
	}
};

struct LobbySetCampaignBonus : public CLobbyPackToServer
{
	int bonusId = -1;

	bool applyOnServer(CVCMIServer * srv);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & bonusId;
	}
};

struct LobbyChangePlayerOption : public CLobbyPackToServer
{
	enum EWhat : ui8 {UNKNOWN, TOWN, HERO, BONUS};
	ui8 what = UNKNOWN;
	si8 direction = 0; //-1 or +1
	PlayerColor color = PlayerColor::CANNOT_DETERMINE;

	bool checkClientPermissions(CVCMIServer * srv) const;
	bool applyOnServer(CVCMIServer * srv);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & what;
		h & direction;
		h & color;
	}
};

struct LobbySetPlayer : public CLobbyPackToServer
{
	PlayerColor clickedColor = PlayerColor::CANNOT_DETERMINE;

	bool applyOnServer(CVCMIServer * srv);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & clickedColor;
	}
};

struct LobbySetTurnTime : public CLobbyPackToServer
{
	ui8 turnTime = 0;

	bool applyOnServer(CVCMIServer * srv);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & turnTime;
	}
};

struct LobbySetDifficulty : public CLobbyPackToServer
{
	ui8 difficulty = 0;

	bool applyOnServer(CVCMIServer * srv);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & difficulty;
	}
};

struct LobbyForceSetPlayer : public CLobbyPackToServer
{
	ui8 targetConnectedPlayer = -1;
	PlayerColor targetPlayerColor = PlayerColor::CANNOT_DETERMINE;

	bool applyOnServer(CVCMIServer * srv);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & targetConnectedPlayer;
		h & targetPlayerColor;
	}
};

struct LobbyShowMessage : public CLobbyPackToPropagate
{
	std::string message;
	
	void applyOnLobbyScreen(CLobbyScreen * lobby, CServerHandler * handler);
	
	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & message;
	}
};

VCMI_LIB_NAMESPACE_END
