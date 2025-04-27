/*
 * NetworkHandler.h, part of VCMI engine
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

class NetworkHandler : public INetworkHandler
{
	std::unique_ptr<NetworkContext> io;

public:
	NetworkHandler();

	std::unique_ptr<INetworkServer> createServerTCP(INetworkServerListener & listener) override;
	void connectToRemote(INetworkClientListener & listener, const std::string & host, uint16_t port) override;
	void createInternalConnection(INetworkClientListener & listener, INetworkServer & server) override;
	void createTimer(INetworkTimerListener & listener, std::chrono::milliseconds duration) override;

	void run() override;
	void stop() override;
};

VCMI_LIB_NAMESPACE_END
