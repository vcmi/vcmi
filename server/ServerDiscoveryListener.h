/*
 * ServerDiscoveryListener.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <boost/asio.hpp>

class CVCMIServer;

class ServerDiscoveryListener
{
public:
	ServerDiscoveryListener(CVCMIServer & server, unsigned short port = 3030);
	~ServerDiscoveryListener();
	void start();
	void stop();

private:
	void listen();
	void handle_receive(const boost::system::error_code & error, std::size_t bytes_recvd);
	CVCMIServer & server;
	unsigned short port;
	std::unique_ptr<std::thread> thread;
	std::atomic<bool> running;
	boost::asio::io_context ioContext;
	std::unique_ptr<boost::asio::ip::udp::socket> socket;
	char recvBuffer[1024];
	boost::asio::ip::udp::endpoint remoteEndpoint;
};
