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

#include <boost/program_options/variables_map.hpp>

#if defined(VCMI_ANDROID) && !defined(SINGLE_PROCESS_APP)
#define VCMI_ANDROID_DUAL_PROCESS 1
#endif

VCMI_LIB_NAMESPACE_BEGIN

class CMapInfo;

struct CPackForLobby;

class CConnection;
struct StartInfo;
struct LobbyInfo;
struct PlayerSettings;
class PlayerColor;

template<typename T> class CApplier;

VCMI_LIB_NAMESPACE_END

class CGameHandler;
class CBaseForServerApply;
class CBaseForGHApply;
class GlobalLobbyProcessor;

enum class EServerState : ui8
{
	LOBBY,
	GAMEPLAY_STARTING,
	GAMEPLAY,
	GAMEPLAY_ENDED,
	SHUTDOWN
};

class CVCMIServer : public LobbyInfo, public INetworkServerListener, public INetworkTimerListener
{
	/// Network server instance that receives and processes incoming connections on active socket
	std::unique_ptr<INetworkServer> networkServer;
	std::unique_ptr<GlobalLobbyProcessor> lobbyProcessor;

	std::chrono::steady_clock::time_point gameplayStartTime;
	std::chrono::steady_clock::time_point lastTimerUpdateTime;

public:
	/// List of all active connections
	std::vector<std::shared_ptr<CConnection>> activeConnections;

	std::unique_ptr<INetworkHandler> networkHandler;

private:
	bool restartGameplay; // FIXME: this is just a hack

	boost::recursive_mutex mx;
	std::shared_ptr<CApplier<CBaseForServerApply>> applier;
	EServerState state;

	std::shared_ptr<CConnection> findConnection(const std::shared_ptr<INetworkConnection> &);

	int currentClientId;
	ui8 currentPlayerId;

public:
	// INetworkListener impl
	void onDisconnected(const std::shared_ptr<INetworkConnection> & connection) override;
	void onPacketReceived(const std::shared_ptr<INetworkConnection> & connection, const std::vector<uint8_t> & message) override;
	void onNewConnection(const std::shared_ptr<INetworkConnection> &) override;
	void onTimer() override;

	std::shared_ptr<CGameHandler> gh;
	boost::program_options::variables_map cmdLineOptions;

	CVCMIServer(boost::program_options::variables_map & opts);
	~CVCMIServer();

	void run();

	bool prepareToStartGame();
	void prepareToRestart();
	void startGameImmediately();
	void startAcceptingIncomingConnections();

	void threadHandleClient(std::shared_ptr<CConnection> c);

	void announcePack(std::unique_ptr<CPackForLobby> pack);
	bool passHost(int toConnectionId);

	void announceTxt(const std::string & txt, const std::string & playerName = "system");

	void setPlayerConnectedId(PlayerSettings & pset, ui8 player) const;
	void updateStartInfoOnMapChange(std::shared_ptr<CMapInfo> mapInfo, std::shared_ptr<CMapGenOptions> mapGenOpt = {});

	void clientConnected(std::shared_ptr<CConnection> c, std::vector<std::string> & names, std::string uuid, EStartMode mode);
	void clientDisconnected(std::shared_ptr<CConnection> c);
	void reconnectPlayer(int connId);

public:
	void announceMessage(const std::string & txt);

	void handleReceivedPack(std::unique_ptr<CPackForLobby> pack);

	void updateAndPropagateLobbyState();

	void setState(EServerState value);
	EServerState getState() const;

	// Work with LobbyInfo
	void setPlayer(PlayerColor clickedColor);
	void setPlayerName(PlayerColor player, std::string name);
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

#if VCMI_ANDROID_DUAL_PROCESS
	static void create();
#elif defined(SINGLE_PROCESS_APP)
	static void create(boost::condition_variable * cond, const std::vector<std::string> & args);
# ifdef VCMI_ANDROID
	static void reuseClientJNIEnv(void * jniEnv);
# endif // VCMI_ANDROID
#endif // VCMI_ANDROID_DUAL_PROCESS
};
