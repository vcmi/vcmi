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

class DLL_LINKAGE NetworkClient : boost::noncopyable, public INetworkConnectionListener
{
	std::shared_ptr<NetworkService> io;
	std::shared_ptr<NetworkSocket> socket;
	std::shared_ptr<NetworkConnection> connection;
	std::shared_ptr<NetworkTimer> timer;

	void onConnected(const boost::system::error_code & ec);

	void onDisconnected(const std::shared_ptr<NetworkConnection> & connection) override;
	void onPacketReceived(const std::shared_ptr<NetworkConnection> & connection, const std::vector<uint8_t> & message) override;

protected:
	virtual void onPacketReceived(const std::vector<uint8_t> & message) = 0;
	virtual void onConnectionFailed(const std::string & errorMessage) = 0;
	virtual void onConnectionEstablished() = 0;
	virtual void onDisconnected() = 0;

	void sendPacket(const std::vector<uint8_t> & message);
public:
	NetworkClient();
	virtual ~NetworkClient() = default;

	void start(const std::string & host, uint16_t port);
	void run();
	void poll();
};

VCMI_LIB_NAMESPACE_END
