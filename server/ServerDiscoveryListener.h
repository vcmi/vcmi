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
	void asyncReceive();
	CVCMIServer & server;
	unsigned short port;
	std::shared_ptr<boost::asio::ip::udp::socket> socket;
	std::array<char, 1024> recvBuffer;
	boost::asio::ip::udp::endpoint remoteEndpoint;
};
