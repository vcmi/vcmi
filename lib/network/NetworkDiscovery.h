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

struct DLL_LINKAGE DiscoveredServer
{
	std::string address;
	uint16_t port;
};

class DLL_LINKAGE ServerDiscovery
{
public:
	ServerDiscovery(NetworkContext & context, std::function<void(const DiscoveredServer &)> callback);
	~ServerDiscovery();
	void start();
	void abort();
	static std::vector<std::string> ipAddresses();

private:
	NetworkContext & context;
	std::function<void(const DiscoveredServer &)> callback;
	std::shared_ptr<boost::asio::ip::udp::socket> socket;
	std::shared_ptr<boost::asio::steady_timer> sendTimer;
	std::set<std::pair<std::string, uint16_t>> discoveredServers;
};

class DLL_LINKAGE ServerDiscoveryListener
{
public:
	ServerDiscoveryListener(NetworkContext & context, std::function<bool()> isInLobby, std::function<uint16_t()> getPort, unsigned short port = 3030);
	~ServerDiscoveryListener();
	void start();
	void stop();

private:
	void asyncReceive();
	NetworkContext & context;
	std::function<bool()> isInLobby;
	std::function<uint16_t()> getPort;
	unsigned short port;
	std::shared_ptr<boost::asio::ip::udp::socket> socket;
	std::array<char, 1024> recvBuffer;
	boost::asio::ip::udp::endpoint remoteEndpoint;
};

VCMI_LIB_NAMESPACE_END
