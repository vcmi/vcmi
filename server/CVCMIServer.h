#pragma once



/*
 * CVCMIServer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CMapInfo;

class CConnection;
struct CPackForSelectionScreen;
class CGameHandler;

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

typedef boost::asio::basic_socket_acceptor<boost::asio::ip::tcp, boost::asio::socket_acceptor_service<boost::asio::ip::tcp> > TAcceptor;
typedef boost::asio::basic_stream_socket < boost::asio::ip::tcp , boost::asio::stream_socket_service<boost::asio::ip::tcp>  > TSocket;

class CVCMIServer
{
	boost::asio::io_service *io;
	TAcceptor * acceptor;

	CConnection *firstConnection;
public:
	CVCMIServer(); //c-tor
	~CVCMIServer(); //d-tor

	void start();
	CGameHandler *initGhFromHostingConnection(CConnection &c);

	void newGame();
	void loadGame();
	void newPregame();
};

class CPregameServer
{
public:
	CConnection *host;
	int listeningThreads;
	std::set<CConnection *> connections;
	std::list<CPackForSelectionScreen*> toAnnounce;
	boost::recursive_mutex mx;

	//std::vector<CMapInfo> maps;
	TAcceptor *acceptor;
	TSocket *upcomingConnection;

	const CMapInfo *curmap;
	StartInfo *curStartInfo;

	CPregameServer(CConnection *Host, TAcceptor *Acceptor = nullptr);
	~CPregameServer();

	void run();

	void processPack(CPackForSelectionScreen * pack);
	void handleConnection(CConnection *cpc);
	void connectionAccepted(const boost::system::error_code& ec);
	void initConnection(CConnection *c);

	void start_async_accept();

	enum { INVALID, RUNNING, ENDING_WITHOUT_START, ENDING_AND_STARTING_GAME
	} state;

	void announceTxt(const std::string &txt, const std::string &playerName = "system");
	void announcePack(const CPackForSelectionScreen &pack);

	void sendPack(CConnection * pc, const CPackForSelectionScreen & pack);
	void startListeningThread(CConnection * pc);
};

extern boost::program_options::variables_map cmdLineOptions;
