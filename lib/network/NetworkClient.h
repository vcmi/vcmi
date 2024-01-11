/*
 * NetworkClient.h, part of VCMI engine
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

class NetworkConnection;

class NetworkClient : public INetworkConnectionListener, public INetworkClient
{
	std::shared_ptr<NetworkContext> io;
	std::shared_ptr<NetworkSocket> socket;
	std::shared_ptr<NetworkConnection> connection;

	INetworkClientListener & listener;

	void onConnected(const boost::system::error_code & ec);

	void onDisconnected(const std::shared_ptr<INetworkConnection> & connection) override;
	void onPacketReceived(const std::shared_ptr<INetworkConnection> & connection, const std::vector<uint8_t> & message) override;

public:
	NetworkClient(INetworkClientListener & listener, const std::shared_ptr<NetworkContext> & context);

	bool isConnected() const override;

	void sendPacket(const std::vector<uint8_t> & message) override;

	void start(const std::string & host, uint16_t port) override;
};

VCMI_LIB_NAMESPACE_END
