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

class CServerHandler;
class CVCMIServer;

VCMI_LIB_NAMESPACE_BEGIN

class CampaignState;
class CMapInfo;
struct StartInfo;
class CMapGenOptions;
struct ClientPlayer;

struct DLL_LINKAGE CLobbyPackToPropagate : public CPackForLobby
{
};

struct DLL_LINKAGE CLobbyPackToServer : public CPackForLobby
{
	virtual bool isForServer() const override;
};

struct DLL_LINKAGE LobbyClientConnected : public CLobbyPackToPropagate
{
	// Set by client before sending pack to server
	std::string uuid;
	std::vector<std::string> names;
	StartInfo::EMode mode = StartInfo::INVALID;
	// Changed by server before announcing pack
	int clientId = -1;
	int hostClientId = -1;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & uuid;
		h & names;
		h & mode;

		h & clientId;
		h & hostClientId;
	}
};

struct DLL_LINKAGE LobbyClientDisconnected : public CLobbyPackToPropagate
{
	int clientId;
	bool shutdownServer = false;


	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & clientId;
		h & shutdownServer;
	}
};

struct DLL_LINKAGE LobbyChatMessage : public CLobbyPackToPropagate
{
	std::string playerName, message;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & playerName;
		h & message;
	}
};

struct DLL_LINKAGE LobbyGuiAction : public CLobbyPackToPropagate
{
	enum EAction : ui8 {
		NONE, NO_TAB, OPEN_OPTIONS, OPEN_SCENARIO_LIST, OPEN_RANDOM_MAP_OPTIONS
	} action = NONE;


	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & action;
	}
};

struct DLL_LINKAGE LobbyEndGame : public CLobbyPackToPropagate
{
	bool closeConnection = false, restart = false;
	
	virtual void visitTyped(ICPackVisitor & visitor) override;
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & closeConnection;
		h & restart;
	}
};

struct DLL_LINKAGE LobbyStartGame : public CLobbyPackToPropagate
{
	// Set by server
	std::shared_ptr<StartInfo> initializedStartInfo = nullptr;
	CGameState * initializedGameState = nullptr;
	int clientId = -1; //-1 means to all clients

	virtual void visitTyped(ICPackVisitor & visitor) override;

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

struct DLL_LINKAGE LobbyChangeHost : public CLobbyPackToPropagate
{
	int newHostConnectionId = -1;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & newHostConnectionId;
	}
};

struct DLL_LINKAGE LobbyUpdateState : public CLobbyPackToPropagate
{
	LobbyState state;
	bool hostChanged = false; // Used on client-side only

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & state;
	}
};

struct DLL_LINKAGE LobbySetMap : public CLobbyPackToServer
{
	std::shared_ptr<CMapInfo> mapInfo;
	std::shared_ptr<CMapGenOptions> mapGenOpts;

	LobbySetMap() : mapInfo(nullptr), mapGenOpts(nullptr) {}

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & mapInfo;
		h & mapGenOpts;
	}
};

struct DLL_LINKAGE LobbySetCampaign : public CLobbyPackToServer
{
	std::shared_ptr<CampaignState> ourCampaign;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ourCampaign;
	}
};

struct DLL_LINKAGE LobbySetCampaignMap : public CLobbyPackToServer
{
	CampaignScenarioID mapId = CampaignScenarioID::NONE;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & mapId;
	}
};

struct DLL_LINKAGE LobbySetCampaignBonus : public CLobbyPackToServer
{
	int bonusId = -1;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & bonusId;
	}
};

struct DLL_LINKAGE LobbyChangePlayerOption : public CLobbyPackToServer
{
	enum EWhat : ui8 {UNKNOWN, TOWN, HERO, BONUS};
	ui8 what = UNKNOWN;
	si8 direction = 0; //-1 or +1
	PlayerColor color = PlayerColor::CANNOT_DETERMINE;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & what;
		h & direction;
		h & color;
	}
};

struct DLL_LINKAGE LobbySetPlayer : public CLobbyPackToServer
{
	PlayerColor clickedColor = PlayerColor::CANNOT_DETERMINE;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & clickedColor;
	}
};

struct DLL_LINKAGE LobbySetTurnTime : public CLobbyPackToServer
{
	TurnTimerInfo turnTimerInfo;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & turnTimerInfo;
	}
};

struct DLL_LINKAGE LobbySetDifficulty : public CLobbyPackToServer
{
	ui8 difficulty = 0;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & difficulty;
	}
};

struct DLL_LINKAGE LobbyForceSetPlayer : public CLobbyPackToServer
{
	ui8 targetConnectedPlayer = -1;
	PlayerColor targetPlayerColor = PlayerColor::CANNOT_DETERMINE;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & targetConnectedPlayer;
		h & targetPlayerColor;
	}
};

struct DLL_LINKAGE LobbyShowMessage : public CLobbyPackToPropagate
{
	std::string message;
	
	virtual void visitTyped(ICPackVisitor & visitor) override;
	
	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & message;
	}
};

VCMI_LIB_NAMESPACE_END
