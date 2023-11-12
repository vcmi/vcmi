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

class DLL_LINKAGE NetworkConnection :public std::enable_shared_from_this<NetworkConnection>, boost::noncopyable
{
	static const int messageHeaderSize = sizeof(uint32_t);
	static const int messageMaxSize = 65536; // arbitrary size to prevent potential massive allocation if we receive garbage input

	std::shared_ptr<NetworkSocket> socket;

	NetworkBuffer readBuffer;
	INetworkConnectionListener & listener;

	void onHeaderReceived(const boost::system::error_code & ec);
	void onPacketReceived(const boost::system::error_code & ec, uint32_t expectedPacketSize);
	uint32_t readPacketSize();

public:
	NetworkConnection(const std::shared_ptr<NetworkSocket> & socket, INetworkConnectionListener & listener);

	void start();
	void sendPacket(const std::vector<uint8_t> & message);
};

VCMI_LIB_NAMESPACE_END
