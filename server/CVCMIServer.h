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

#include "IGameServer.h"

#include "../lib/network/NetworkInterface.h"
#include "../lib/StartInfo.h"

VCMI_LIB_NAMESPACE_BEGIN

class CMapInfo;

struct CPackForLobby;

class GameConnection;
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

class CVCMIServer : public LobbyInfo, public INetworkServerListener, public INetworkTimerListener, public IGameServer
{
	std::chrono::steady_clock::time_point gameplayStartTime;
	std::chrono::steady_clock::time_point lastTimerUpdateTime;

	std::unique_ptr<INetworkHandler> networkHandler;
	/// Network server instance that receives and processes incoming connections on active socket
	std::unique_ptr<INetworkServer> networkServer;

	/// Handles connection with global lobby. Must be constructed and destroyed after network handler
	std::unique_ptr<GlobalLobbyProcessor> lobbyProcessor;

	EServerState state = EServerState::LOBBY;

	std::shared_ptr<GameConnection> findConnection(const std::shared_ptr<INetworkConnection> &);

	GameConnectionID currentClientId;
	PlayerConnectionID currentPlayerId;
	uint16_t port;
	bool runByClient;


	bool loadSavedGame(CGameHandler & handler, const StartInfo & info);
public:

	// IGameServer impl
	void setState(EServerState value) override;
	EServerState getState() const override;
	bool isPlayerHost(const PlayerColor & color) const override;
	bool hasPlayerAt(PlayerColor player, GameConnectionID connectionID) const override;
	bool hasBothPlayersAtSameConnection(PlayerColor left, PlayerColor right) const override;
	void applyPack(CPackForClient & pack) override;
	void sendPack(CPackForClient & pack, GameConnectionID connectionID) override;

	/// List of all active connections
	std::vector<std::shared_ptr<GameConnection>> activeConnections;

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

	void threadHandleClient(std::shared_ptr<GameConnection> c);

	void announcePack(CPackForLobby & pack);
	bool passHost(GameConnectionID toConnectionId);

	void announceTxt(const MetaString & txt, const std::string & playerName = "system");
	void announceTxt(const std::string & txt, const std::string & playerName = "system");

	void setPlayerConnectedId(PlayerSettings & pset, PlayerConnectionID player) const;
	void updateStartInfoOnMapChange(std::shared_ptr<CMapInfo> mapInfo, std::shared_ptr<CMapGenOptions> mapGenOpt = {});

	void clientConnected(std::shared_ptr<GameConnection> c, std::vector<std::string> & names, const std::string & uuid, EStartMode mode);
	void clientDisconnected(std::shared_ptr<GameConnection> c);

	void announceMessage(const MetaString & txt);
	void announceMessage(const std::string & txt);

	void handleReceivedPack(std::shared_ptr<GameConnection> connection, CPackForLobby & pack);

	void updateAndPropagateLobbyState();

	INetworkHandler & getNetworkHandler();
	INetworkServer & getNetworkServer();

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

	PlayerConnectionID getIdOfFirstUnallocatedPlayer() const;

	void multiplayerWelcomeMessage();
};
