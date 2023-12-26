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
#include "NetworkListener.h"

VCMI_LIB_NAMESPACE_BEGIN

class NetworkConnection;

class DLL_LINKAGE NetworkClient : boost::noncopyable, public INetworkConnectionListener
{
	std::shared_ptr<NetworkService> io;
	std::shared_ptr<NetworkSocket> socket;
	std::shared_ptr<NetworkConnection> connection;

	INetworkClientListener & listener;

	void onConnected(const boost::system::error_code & ec);

	void onDisconnected(const std::shared_ptr<NetworkConnection> & connection) override;
	void onPacketReceived(const std::shared_ptr<NetworkConnection> & connection, const std::vector<uint8_t> & message) override;

public:
	NetworkClient(INetworkClientListener & listener);
	virtual ~NetworkClient() = default;

	void setTimer(std::chrono::milliseconds duration);
	void sendPacket(const std::vector<uint8_t> & message);

	void start(const std::string & host, uint16_t port);
	void run();
	void poll();
	void stop();
};

VCMI_LIB_NAMESPACE_END
