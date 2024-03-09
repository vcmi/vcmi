/*
 * NetworkConnection.h, part of VCMI engine
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

class NetworkConnection : public INetworkConnection, public std::enable_shared_from_this<NetworkConnection>
{
	static const int messageHeaderSize = sizeof(uint32_t);
	static const int messageMaxSize = 64 * 1024 * 1024; // arbitrary size to prevent potential massive allocation if we receive garbage input

	std::shared_ptr<NetworkSocket> socket;

	NetworkBuffer readBuffer;
	INetworkConnectionListener & listener;

	void onHeaderReceived(const boost::system::error_code & ec);
	void onPacketReceived(const boost::system::error_code & ec, uint32_t expectedPacketSize);

public:
	NetworkConnection(INetworkConnectionListener & listener, const std::shared_ptr<NetworkSocket> & socket);

	void start();
	void close() override;
	void sendPacket(const std::vector<std::byte> & message) override;
};

VCMI_LIB_NAMESPACE_END
