#ifndef __CVCMISERVER_H__
#define __CVCMISERVER_H__
#include "../global.h"
#include <set>

/*
 * CVCMIServer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CConnection;
namespace boost
{
	namespace asio
	{
		class io_service;
		namespace ip
		{
			class tcp;
		}
		template <typename Protocol> class socket_acceptor_service;
		template <typename Protocol,typename SocketAcceptorService>
		class basic_socket_acceptor;
	}
};

class CVCMIServer
{
	boost::asio::io_service *io;
	boost::asio::basic_socket_acceptor<boost::asio::ip::tcp, boost::asio::socket_acceptor_service<boost::asio::ip::tcp> > * acceptor;
	std::map<int,CConnection*> connections;
	std::set<CConnection*> conns;
public:
	CVCMIServer();
	~CVCMIServer();
	void setUpConnection(CConnection *c, std::string mapname, si32 checksum);
	void newGame(CConnection *c);
	void loadGame(CConnection *c);
	void start();
};
#endif // __CVCMISERVER_H__
