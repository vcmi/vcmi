/*
 * NetworkServer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "NetworkDefines.h"
#include "NetworkListener.h"

VCMI_LIB_NAMESPACE_BEGIN

class NetworkConnection;

class DLL_LINKAGE NetworkServer : boost::noncopyable, public INetworkConnectionListener
{
	std::shared_ptr<NetworkService> io;
	std::shared_ptr<NetworkAcceptor> acceptor;
	std::set<std::shared_ptr<NetworkConnection>> connections;

	INetworkServerListener & listener;

	void connectionAccepted(std::shared_ptr<NetworkSocket>, const boost::system::error_code & ec);
	void startAsyncAccept();

	void onDisconnected(const std::shared_ptr<NetworkConnection> & connection) override;
	void onPacketReceived(const std::shared_ptr<NetworkConnection> & connection, const std::vector<uint8_t> & message) override;
public:
	explicit NetworkServer(INetworkServerListener & listener);

	void sendPacket(const std::shared_ptr<NetworkConnection> &, const std::vector<uint8_t> & message);

	void start(uint16_t port);
	void run(std::chrono::milliseconds duration);
	void run();
};

VCMI_LIB_NAMESPACE_END
