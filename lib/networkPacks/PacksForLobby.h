/*
 * PacksForLobby.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "StartInfo.h"
#include "NetPacksBase.h"

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
	bool isForServer() const override;
};

struct DLL_LINKAGE LobbyClientConnected : public CLobbyPackToPropagate
{
	// Set by client before sending pack to server
	std::string uuid;
	std::vector<std::string> names;
	EStartMode mode = EStartMode::INVALID;
	// Changed by server before announcing pack
	int clientId = -1;
	int hostClientId = -1;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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


	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & clientId;
		h & shutdownServer;
	}
};

struct DLL_LINKAGE LobbyChatMessage : public CLobbyPackToPropagate
{
	std::string playerName;
	std::string message;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & playerName;
		h & message;
	}
};

struct DLL_LINKAGE LobbyGuiAction : public CLobbyPackToPropagate
{
	enum EAction : ui8 {
		NONE, NO_TAB, OPEN_OPTIONS, OPEN_SCENARIO_LIST, OPEN_RANDOM_MAP_OPTIONS, OPEN_TURN_OPTIONS, OPEN_EXTRA_OPTIONS
	} action = NONE;


	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & action;
	}
};

struct DLL_LINKAGE LobbyLoadProgress : public CLobbyPackToPropagate
{
	unsigned char progress;
	
	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & progress;
	}
};

struct DLL_LINKAGE LobbyRestartGame : public CLobbyPackToPropagate
{
	void visitTyped(ICPackVisitor & visitor) override;
	
	template <typename Handler> void serialize(Handler &h)
	{
	}
};

struct DLL_LINKAGE LobbyPrepareStartGame : public CLobbyPackToPropagate
{
	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
	}
};

struct DLL_LINKAGE LobbyStartGame : public CLobbyPackToPropagate
{
	// Set by server
	std::shared_ptr<StartInfo> initializedStartInfo = nullptr;
	CGameState * initializedGameState = nullptr;
	int clientId = -1; //-1 means to all clients

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & newHostConnectionId;
	}
};

struct DLL_LINKAGE LobbyUpdateState : public CLobbyPackToPropagate
{
	LobbyState state;
	bool hostChanged = false; // Used on client-side only

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & state;
	}
};

struct DLL_LINKAGE LobbySetMap : public CLobbyPackToServer
{
	std::shared_ptr<CMapInfo> mapInfo;
	std::shared_ptr<CMapGenOptions> mapGenOpts;

	LobbySetMap() : mapInfo(nullptr), mapGenOpts(nullptr) {}

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & mapInfo;
		h & mapGenOpts;
	}
};

struct DLL_LINKAGE LobbySetCampaign : public CLobbyPackToServer
{
	std::shared_ptr<CampaignState> ourCampaign;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & ourCampaign;
	}
};

struct DLL_LINKAGE LobbySetCampaignMap : public CLobbyPackToServer
{
	CampaignScenarioID mapId = CampaignScenarioID::NONE;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & mapId;
	}
};

struct DLL_LINKAGE LobbySetCampaignBonus : public CLobbyPackToServer
{
	int bonusId = -1;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & bonusId;
	}
};

struct DLL_LINKAGE LobbyChangePlayerOption : public CLobbyPackToServer
{
	enum EWhat : ui8 {UNKNOWN, TOWN, HERO, BONUS, TOWN_ID, HERO_ID, BONUS_ID};
	ui8 what = UNKNOWN;
	int32_t value = 0;
	PlayerColor color = PlayerColor::CANNOT_DETERMINE;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & what;
		h & value;
		h & color;
	}
};

struct DLL_LINKAGE LobbySetPlayer : public CLobbyPackToServer
{
	PlayerColor clickedColor = PlayerColor::CANNOT_DETERMINE;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & clickedColor;
	}
};

struct DLL_LINKAGE LobbySetPlayerName : public CLobbyPackToServer
{
	PlayerColor color = PlayerColor::CANNOT_DETERMINE;
	std::string name = "";

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & color;
		h & name;
	}
};

struct DLL_LINKAGE LobbySetSimturns : public CLobbyPackToServer
{
	SimturnsInfo simturnsInfo;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & simturnsInfo;
	}
};

struct DLL_LINKAGE LobbySetTurnTime : public CLobbyPackToServer
{
	TurnTimerInfo turnTimerInfo;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & turnTimerInfo;
	}
};

struct DLL_LINKAGE LobbySetExtraOptions : public CLobbyPackToServer
{
	ExtraOptionsInfo extraOptionsInfo;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & extraOptionsInfo;
	}
};

struct DLL_LINKAGE LobbySetDifficulty : public CLobbyPackToServer
{
	ui8 difficulty = 0;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & difficulty;
	}
};

struct DLL_LINKAGE LobbyForceSetPlayer : public CLobbyPackToServer
{
	ui8 targetConnectedPlayer = -1;
	PlayerColor targetPlayerColor = PlayerColor::CANNOT_DETERMINE;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & targetConnectedPlayer;
		h & targetPlayerColor;
	}
};

struct DLL_LINKAGE LobbyShowMessage : public CLobbyPackToPropagate
{
	std::string message;
	
	void visitTyped(ICPackVisitor & visitor) override;
	
	template <typename Handler> void serialize(Handler & h)
	{
		h & message;
	}
};

VCMI_LIB_NAMESPACE_END
