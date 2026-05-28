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

VCMI_LIB_NAMESPACE_BEGIN

class NetworkServer : public INetworkConnectionListener, public INetworkServer
{
	NetworkContext & context;
	std::shared_ptr<NetworkAcceptor> acceptor;
	std::set<std::shared_ptr<INetworkConnection>> connections;

	INetworkServerListener & listener;

	void connectionAccepted(std::shared_ptr<NetworkSocket>, const boost::system::error_code & ec);
	uint16_t startAsyncAccept();

	void onDisconnected(const std::shared_ptr<INetworkConnection> & connection, const std::string & errorMessage) override;
	void onPacketReceived(const std::shared_ptr<INetworkConnection> & connection, const std::vector<std::byte> & message) override;
public:
	NetworkServer(INetworkServerListener & listener, NetworkContext & context);

	void receiveInternalConnection(std::shared_ptr<IInternalConnection> remoteConnection) override;

	uint16_t start(uint16_t port) override;
};

VCMI_LIB_NAMESPACE_END
