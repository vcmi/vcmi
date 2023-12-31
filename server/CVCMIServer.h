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

#include "../lib/serializer/Connection.h"
#include "../lib/StartInfo.h"

#include <boost/program_options.hpp>

#if defined(VCMI_ANDROID) && !defined(SINGLE_PROCESS_APP)
#define VCMI_ANDROID_DUAL_PROCESS 1
#endif

VCMI_LIB_NAMESPACE_BEGIN

class CMapInfo;

struct CPackForLobby;

struct StartInfo;
struct LobbyInfo;
struct PlayerSettings;
class PlayerColor;

template<typename T> class CApplier;

VCMI_LIB_NAMESPACE_END

class CGameHandler;
class CBaseForServerApply;
class CBaseForGHApply;

enum class EServerState : ui8
{
	LOBBY,
	GAMEPLAY_STARTING,
	GAMEPLAY,
	GAMEPLAY_ENDED,
	SHUTDOWN
};

class CVCMIServer : public LobbyInfo
{
	std::atomic<bool> restartGameplay; // FIXME: this is just a hack
	std::shared_ptr<boost::asio::io_service> io;
	std::shared_ptr<TAcceptor> acceptor;
	std::shared_ptr<TSocket> upcomingConnection;
	std::list<std::unique_ptr<CPackForLobby>> announceQueue;
	boost::recursive_mutex mx;
	std::shared_ptr<CApplier<CBaseForServerApply>> applier;
	std::unique_ptr<boost::thread> announceLobbyThread, remoteConnectionsThread;
	std::atomic<EServerState> state;

public:
	std::shared_ptr<CGameHandler> gh;
	ui16 port;

	boost::program_options::variables_map cmdLineOptions;
	std::set<std::shared_ptr<CConnection>> connections;
	std::set<std::shared_ptr<CConnection>> remoteConnections;
	std::set<std::shared_ptr<CConnection>> hangingConnections; //keep connections of players disconnected during the game
	
	std::atomic<int> currentClientId;
	std::atomic<ui8> currentPlayerId;
	std::shared_ptr<CConnection> hostClient;

	CVCMIServer(boost::program_options::variables_map & opts);
	~CVCMIServer();
	void run();
	bool prepareToStartGame();
	void prepareToRestart();
	void startGameImmidiately();

	void establishRemoteConnections();
	void connectToRemote();
	void startAsyncAccept();
	void connectionAccepted(const boost::system::error_code & ec);
	void threadHandleClient(std::shared_ptr<CConnection> c);
	void threadAnnounceLobby();
	void handleReceivedPack(std::unique_ptr<CPackForLobby> pack);

	void announcePack(std::unique_ptr<CPackForLobby> pack);
	bool passHost(int toConnectionId);

	void announceTxt(const std::string & txt, const std::string & playerName = "system");
	void announceMessage(const std::string & txt);
	void addToAnnounceQueue(std::unique_ptr<CPackForLobby> pack);

	void setPlayerConnectedId(PlayerSettings & pset, ui8 player) const;
	void updateStartInfoOnMapChange(std::shared_ptr<CMapInfo> mapInfo, std::shared_ptr<CMapGenOptions> mapGenOpt = {});

	void clientConnected(std::shared_ptr<CConnection> c, std::vector<std::string> & names, std::string uuid, StartInfo::EMode mode);
	void clientDisconnected(std::shared_ptr<CConnection> c);
	void reconnectPlayer(int connId);

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
