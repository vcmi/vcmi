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

class NetworkConnection;

class DLL_LINKAGE NetworkServer : boost::noncopyable
{
	std::shared_ptr<NetworkService> io;
	std::shared_ptr<NetworkAcceptor> acceptor;
	std::set<std::shared_ptr<NetworkConnection>> connections;

	void connectionAccepted(std::shared_ptr<NetworkSocket>, const boost::system::error_code & ec);
	void startAsyncAccept();
protected:
	virtual void onNewConnection(std::shared_ptr<NetworkConnection>) = 0;
	virtual void onPacketReceived(std::shared_ptr<NetworkConnection>, const std::vector<uint8_t> & message) = 0;

	void sendPacket(const std::shared_ptr<NetworkConnection> &, const std::vector<uint8_t> & message);

public:
	virtual ~NetworkServer() = default;

	void start(uint16_t port);
	void run();
};

VCMI_LIB_NAMESPACE_END
