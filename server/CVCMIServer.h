/*
 * CVCMIServer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/network/NetworkInterface.h"
#include "../lib/StartInfo.h"

VCMI_LIB_NAMESPACE_BEGIN

class CMapInfo;

struct CPackForLobby;

class CConnection;
struct StartInfo;
struct LobbyInfo;
struct PlayerSettings;
class PlayerColor;
class MetaString;

VCMI_LIB_NAMESPACE_END

class CGameHandler;
class CBaseForServerApply;
class CBaseForGHApply;
class GlobalLobbyProcessor;

enum class EServerState : ui8
{
	LOBBY,
	GAMEPLAY,
	SHUTDOWN
};

class CVCMIServer : public LobbyInfo, public INetworkServerListener, public INetworkTimerListener
{
	/// Network server instance that receives and processes incoming connections on active socket
	std::unique_ptr<INetworkServer> networkServer;
	std::unique_ptr<GlobalLobbyProcessor> lobbyProcessor;

	std::chrono::steady_clock::time_point gameplayStartTime;
	std::chrono::steady_clock::time_point lastTimerUpdateTime;

	std::unique_ptr<INetworkHandler> networkHandler;

	EServerState state = EServerState::LOBBY;

	std::shared_ptr<CConnection> findConnection(const std::shared_ptr<INetworkConnection> &);

	int currentClientId;
	ui8 currentPlayerId;
	uint16_t port;
	bool runByClient;

public:
	/// List of all active connections
	std::vector<std::shared_ptr<CConnection>> activeConnections;

	uint16_t prepare(bool connectToLobby, bool listenForConnections);

	// INetworkListener impl
	void onDisconnected(const std::shared_ptr<INetworkConnection> & connection, const std::string & errorMessage) override;
	void onPacketReceived(const std::shared_ptr<INetworkConnection> & connection, const std::vector<std::byte> & message) override;
	void onNewConnection(const std::shared_ptr<INetworkConnection> &) override;
	void onTimer() override;

	std::shared_ptr<CGameHandler> gh;

	CVCMIServer(uint16_t port, bool runByClient);
	~CVCMIServer();

	void run();

	bool wasStartedByClient() const;
	bool prepareToStartGame();
	void prepareToRestart();
	void startGameImmediately();
	uint16_t startAcceptingIncomingConnections(bool listenForConnections);

	void threadHandleClient(std::shared_ptr<CConnection> c);

	void announcePack(CPackForLobby & pack);
	bool passHost(int toConnectionId);

	void announceTxt(const MetaString & txt, const std::string & playerName = "system");
	void announceTxt(const std::string & txt, const std::string & playerName = "system");

	void setPlayerConnectedId(PlayerSettings & pset, ui8 player) const;
	void updateStartInfoOnMapChange(std::shared_ptr<CMapInfo> mapInfo, std::shared_ptr<CMapGenOptions> mapGenOpt = {});

	void clientConnected(std::shared_ptr<CConnection> c, std::vector<std::string> & names, const std::string & uuid, EStartMode mode);
	void clientDisconnected(std::shared_ptr<CConnection> c);
	void reconnectPlayer(int connId);

	void announceMessage(const MetaString & txt);
	void announceMessage(const std::string & txt);

	void handleReceivedPack(std::shared_ptr<CConnection> connection, CPackForLobby & pack);

	void updateAndPropagateLobbyState();

	INetworkHandler & getNetworkHandler();
	INetworkServer & getNetworkServer();

	void setState(EServerState value);
	EServerState getState() const;

	// Work with LobbyInfo
	void setPlayer(PlayerColor clickedColor);
	void setPlayerName(PlayerColor player, const std::string & name);
	void setPlayerHandicap(PlayerColor player, Handicap handicap);
	void optionNextHero(PlayerColor player, int dir); //dir == -1 or +1
	void optionSetHero(PlayerColor player, HeroTypeID id);
	HeroTypeID nextAllowedHero(PlayerColor player, HeroTypeID id, int direction);
	bool canUseThisHero(PlayerColor player, HeroTypeID ID);
	std::vector<HeroTypeID> getUsedHeroes();
	void optionNextBonus(PlayerColor player, int dir); //dir == -1 or +1
	void optionSetBonus(PlayerColor player, PlayerStartingBonus id);
	void optionNextCastle(PlayerColor player, int dir); //dir == -1 or +
	void optionSetCastle(PlayerColor player, FactionID id);

	// Campaigns
	void setCampaignMap(CampaignScenarioID mapId);
	void setCampaignBonus(int bonusId);

	ui8 getIdOfFirstUnallocatedPlayer() const;

	void multiplayerWelcomeMessage();
};
