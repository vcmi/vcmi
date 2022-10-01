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

VCMI_LIB_NAMESPACE_BEGIN

class CMapInfo;

struct CPackForLobby;
struct SharedMemory;

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
	std::unique_ptr<boost::thread> announceLobbyThread;

public:
	std::shared_ptr<CGameHandler> gh;
	std::atomic<EServerState> state;
	ui16 port;

	boost::program_options::variables_map cmdLineOptions;
	std::set<std::shared_ptr<CConnection>> connections;
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

	// Work with LobbyInfo
	void setPlayer(PlayerColor clickedColor);
	void optionNextHero(PlayerColor player, int dir); //dir == -1 or +1
	int nextAllowedHero(PlayerColor player, int min, int max, int incl, int dir);
	bool canUseThisHero(PlayerColor player, int ID);
	std::vector<int> getUsedHeroes();
	void optionNextBonus(PlayerColor player, int dir); //dir == -1 or +1
	void optionNextCastle(PlayerColor player, int dir); //dir == -1 or +

	// Campaigns
	void setCampaignMap(int mapId);
	void setCampaignBonus(int bonusId);

	ui8 getIdOfFirstUnallocatedPlayer() const;

#ifdef VCMI_ANDROID
	static void create();
#elif defined(SINGLE_PROCESS_APP)
    static void create(boost::condition_variable * cond, const std::string & uuid);
#endif
};
