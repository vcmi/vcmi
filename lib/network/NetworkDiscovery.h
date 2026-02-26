/*
 * NetworkDiscovery.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "NetworkDefines.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE ServerDiscovery : public IServerDiscovery, public std::enable_shared_from_this<ServerDiscovery>
{
public:
	ServerDiscovery(NetworkContext & context, IServerDiscoveryObserver & observer);
	~ServerDiscovery();
	void start() override;
	void abort() override;
	static std::vector<std::string> ipAddresses();

private:
	NetworkContext & context;
	IServerDiscoveryObserver & observer;
	std::shared_ptr<boost::asio::ip::udp::socket> socket;
	std::shared_ptr<boost::asio::steady_timer> sendTimer;
	std::set<std::pair<std::string, uint16_t>> discoveredServers;
};

class DLL_LINKAGE ServerDiscoveryListener : public IServerDiscoveryListener, public std::enable_shared_from_this<ServerDiscoveryListener>
{
public:
	ServerDiscoveryListener(NetworkContext & context, IServerDiscoveryAnnouncer & announcer, uint16_t port = 3030);
	~ServerDiscoveryListener();
	void start() override;
	void stop() override;

private:
	void asyncReceive();
	NetworkContext & context;
	IServerDiscoveryAnnouncer & announcer;
	uint16_t port;
	std::shared_ptr<boost::asio::ip::udp::socket> socket;
	std::array<char, 1024> recvBuffer;
	boost::asio::ip::udp::endpoint remoteEndpoint;
};

VCMI_LIB_NAMESPACE_END
