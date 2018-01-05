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

#include "../lib/StartInfo.h"

#include <boost/program_options.hpp>

class CMapInfo;

class CConnection;
struct CPackForLobby;
class CGameHandler;
struct SharedMemory;

namespace boost
{
	namespace asio
	{
		namespace ip
		{
			class tcp;
		}
		class io_service;

		template <typename Protocol> class stream_socket_service;
		template <typename Protocol,typename StreamSocketService>
		class basic_stream_socket;

		template <typename Protocol> class socket_acceptor_service;
		template <typename Protocol,typename SocketAcceptorService>
		class basic_socket_acceptor;
	}
};

typedef boost::asio::basic_socket_acceptor<boost::asio::ip::tcp, boost::asio::socket_acceptor_service<boost::asio::ip::tcp>> TAcceptor;
typedef boost::asio::basic_stream_socket<boost::asio::ip::tcp, boost::asio::stream_socket_service<boost::asio::ip::tcp>> TSocket;

struct StartInfo;
struct LobbyInfo;
class PlayerSettings;
class PlayerColor;

template<typename T> class CApplier;
class CBaseForServerApply;
class CBaseForGHApply;

class CVCMIServer : public LobbyInfo
{
	SharedMemory * shm;
	boost::asio::io_service * io;
	TAcceptor * acceptor;
	TSocket * upcomingConnection;
	std::list<CPackForLobby *> announceQueue;
	boost::recursive_mutex mx;
	CApplier<CBaseForServerApply> * applier;

public:
	std::shared_ptr<CGameHandler> gh;
	enum
	{
		INVALID, RUNNING, ENDING_WITHOUT_START, ENDING_AND_STARTING_GAME
	} state;
	ui16 port;

	static std::atomic<bool> shuttingDown;
	boost::program_options::variables_map cmdLineOptions;
	std::set<std::shared_ptr<CConnection>> connections;
	std::atomic<int> currentClientId;
	std::atomic<ui8> currentPlayerId;
	std::shared_ptr<CConnection> hostClient;

	CVCMIServer(boost::program_options::variables_map & opts);
	~CVCMIServer();
	void run();
	void prepareToStartGame();
	void startGameImmidiately();

	void startAsyncAccept();
	void connectionAccepted(const boost::system::error_code & ec);
	void threadHandleClient(std::shared_ptr<CConnection> c);
	void handleReceivedPack(CPackForLobby * pack);

	void announcePack(CPackForLobby * pack);
	bool passHost(int toConnectionId);

	void announceTxt(const std::string & txt, const std::string & playerName = "system");
	void addToAnnounceQueue(CPackForLobby * pack);

	void setPlayerConnectedId(PlayerSettings & pset, ui8 player) const;
	void updateStartInfoOnMapChange(std::shared_ptr<CMapInfo> mapInfo, std::shared_ptr<CMapGenOptions> mapGenOpt = {});

	void clientConnected(std::shared_ptr<CConnection> c, std::vector<std::string> & names, std::string uuid, StartInfo::EMode mode);
	void clientDisconnected(std::shared_ptr<CConnection> c);

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

	ui8 getIdOfFirstUnallocatedPlayer();

#ifdef VCMI_ANDROID
	static void create();
#endif
};
