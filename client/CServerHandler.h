/*
 * CServerHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/CStopWatch.h"

#include "../lib/network/NetworkInterface.h"
#include "../lib/StartInfo.h"
#include "../lib/gameState/GameStatistics.h"

VCMI_LIB_NAMESPACE_BEGIN

class CConnection;
class PlayerColor;
struct StartInfo;
struct TurnTimerInfo;

class CMapInfo;
class CGameState;
struct ClientPlayer;
struct CPackForLobby;
struct CPackForClient;

class HighScoreParameter;

VCMI_LIB_NAMESPACE_END

class CClient;
class CBaseForLobbyApply;
class GlobalLobbyClient;
class GameChatHandler;
class IServerRunner;

enum class ESelectionScreen : ui8;
enum class ELoadMode : ui8;

// TODO: Add mutex so we can't set CONNECTION_CANCELLED if client already connected, but thread not setup yet
enum class EClientState : ui8
{
	NONE = 0,
	CONNECTING, // Trying to connect to server
	CONNECTION_CANCELLED, // Connection cancelled by player, stop attempts to connect
	LOBBY, // Client is connected to lobby
	LOBBY_CAMPAIGN, // Client is on scenario bonus selection screen
	STARTING, // Gameplay interfaces being created, we pause netpacks retrieving
	GAMEPLAY, // In-game, used by some UI
	DISCONNECTING, // We disconnecting, drop all netpacks
};

enum class EServerMode : uint8_t
{
	NONE = 0,
	LOCAL, // no global lobby
	LOBBY_HOST, // We are hosting global server available via global lobby
	LOBBY_GUEST // Connecting to a remote server via proxy provided by global lobby
};

class IServerAPI
{
protected:
	virtual void sendLobbyPack(const CPackForLobby & pack) const = 0;

public:
	virtual ~IServerAPI() {}

	virtual void sendClientConnecting() const = 0;
	virtual void sendClientDisconnecting() = 0;
	virtual void setCampaignState(std::shared_ptr<CampaignState> newCampaign) = 0;
	virtual void setCampaignMap(CampaignScenarioID mapId) const = 0;
	virtual void setCampaignBonus(int bonusId) const = 0;
	virtual void setMapInfo(std::shared_ptr<CMapInfo> to, std::shared_ptr<CMapGenOptions> mapGenOpts = {}) const = 0;
	virtual void setPlayer(PlayerColor color) const = 0;
	virtual void setPlayerName(PlayerColor color, const std::string & name) const = 0;
	virtual void setPlayerHandicap(PlayerColor color, Handicap handicap) const = 0;
	virtual void setPlayerOption(ui8 what, int32_t value, PlayerColor player) const = 0;
	virtual void setDifficulty(int to) const = 0;
	virtual void setTurnTimerInfo(const TurnTimerInfo &) const = 0;
	virtual void setSimturnsInfo(const SimturnsInfo &) const = 0;
	virtual void setExtraOptionsInfo(const ExtraOptionsInfo & info) const = 0;
	virtual void sendMessage(const std::string & txt) const = 0;
	virtual void sendGuiAction(ui8 action) const = 0; // TODO: possibly get rid of it?
	virtual void sendStartGame(bool allowOnlyAI = false) const = 0;
	virtual void sendRestartGame() const = 0;
};

/// structure to handle running server and connecting to it
class CServerHandler final : public IServerAPI, public LobbyInfo, public INetworkClientListener, public INetworkTimerListener, boost::noncopyable
{
	friend class ApplyOnLobbyHandlerNetPackVisitor;

	std::unique_ptr<INetworkHandler> networkHandler;
	std::shared_ptr<INetworkConnection> networkConnection;
	std::unique_ptr<GlobalLobbyClient> lobbyClient;
	std::unique_ptr<GameChatHandler> gameChat;
	std::unique_ptr<IServerRunner> serverRunner;
	std::shared_ptr<CMapInfo> mapToStart;
	std::vector<std::string> localPlayerNames;

	std::thread threadNetwork;

	std::atomic<EClientState> state;

	void threadRunNetwork();
	void waitForServerShutdown();

	void onPacketReceived(const NetworkConnectionPtr &, const std::vector<std::byte> & message) override;
	void onConnectionFailed(const std::string & errorMessage) override;
	void onConnectionEstablished(const NetworkConnectionPtr &) override;
	void onDisconnected(const NetworkConnectionPtr &, const std::string & errorMessage) override;
	void onTimer() override;

	void applyPackOnLobbyScreen(CPackForLobby & pack);

	std::string serverHostname;
	ui16 serverPort;

	bool isServerLocal() const;

public:
	/// High-level connection overlay that is capable of (de)serializing network data
	std::shared_ptr<CConnection> logicConnection;

	////////////////////
	// FIXME: Bunch of crutches to glue it all together

	// For starting non-custom campaign and continue to next mission
	std::shared_ptr<CampaignState> campaignStateToSend;

	ESelectionScreen screenType; // To create lobby UI only after server is setup
	EServerMode serverMode;
	ELoadMode loadMode; // For saves filtering in SelectionTab
	////////////////////

	std::unique_ptr<CStopWatch> th;
	std::unique_ptr<CClient> client;

	CServerHandler();
	~CServerHandler();
	
	void resetStateForLobby(EStartMode mode, ESelectionScreen screen, EServerMode serverMode, const std::vector<std::string> & playerNames);
	void startLocalServerAndConnect(bool connectToLobby);
	void connectToServer(const std::string & addr, const ui16 port);

	GameChatHandler & getGameChat();
	GlobalLobbyClient & getGlobalLobby();
	INetworkHandler & getNetworkHandler();

	// Helpers for lobby state access
	std::set<PlayerColor> getHumanColors();
	PlayerColor myFirstColor() const;
	bool isMyColor(PlayerColor color) const;
	ui8 myFirstId() const; // Used by chat only!

	EClientState getState() const;
	void setState(EClientState newState);

	bool isHost() const;
	bool isGuest() const;
	bool inLobbyRoom() const;
	bool inGame() const;

	const std::string & getCurrentHostname() const;
	const std::string & getLocalHostname() const;
	const std::string & getRemoteHostname() const;

	ui16 getCurrentPort() const;
	ui16 getLocalPort() const;
	ui16 getRemotePort() const;

	// Lobby server API for UI
	void sendLobbyPack(const CPackForLobby & pack) const override;

	void sendClientConnecting() const override;
	void sendClientDisconnecting() override;
	void setCampaignState(std::shared_ptr<CampaignState> newCampaign) override;
	void setCampaignMap(CampaignScenarioID mapId) const override;
	void setCampaignBonus(int bonusId) const override;
	void setMapInfo(std::shared_ptr<CMapInfo> to, std::shared_ptr<CMapGenOptions> mapGenOpts = {}) const override;
	void setPlayer(PlayerColor color) const override;
	void setPlayerName(PlayerColor color, const std::string & name) const override;
	void setPlayerHandicap(PlayerColor color, Handicap handicap) const override;
	void setPlayerOption(ui8 what, int32_t value, PlayerColor player) const override;
	void setDifficulty(int to) const override;
	void setTurnTimerInfo(const TurnTimerInfo &) const override;
	void setSimturnsInfo(const SimturnsInfo &) const override;
	void setExtraOptionsInfo(const ExtraOptionsInfo &) const override;
	void sendMessage(const std::string & txt) const override;
	void sendGuiAction(ui8 action) const override;
	void sendRestartGame() const override;
	void sendStartGame(bool allowOnlyAI = false) const override;

	void startMapAfterConnection(std::shared_ptr<CMapInfo> to);
	bool validateGameStart(bool allowOnlyAI = false) const;
	void debugStartTest(std::string filename, bool save = false);

	void startGameplay(std::shared_ptr<CGameState> gameState);
	void showHighScoresAndEndGameplay(PlayerColor player, bool victory, const StatisticDataSet & statistic);
	void endNetwork();
	void endGameplay();
	void restartGameplay();
	void startCampaignScenario(HighScoreParameter param, std::shared_ptr<CampaignState> cs, const StatisticDataSet & statistic);
	void showServerError(const std::string & txt) const;

	// TODO: LobbyState must be updated within game so we should always know how many player interfaces our client handle
	int howManyPlayerInterfaces();
	ELoadMode getLoadMode();

	void visitForLobby(CPackForLobby & lobbyPack);
	void visitForClient(CPackForClient & clientPack);
};
