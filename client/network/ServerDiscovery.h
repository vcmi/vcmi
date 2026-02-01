/*
 * ServerDiscovery.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../lib/network/NetworkDefines.h"

struct DiscoveredServer
{
	std::string address;
	uint16_t port;
};

class ServerDiscovery
{
public:
	static void discoverAsync(NetworkContext & context, std::function<void(const DiscoveredServer &)> callback);
	static std::vector<std::string> ipAddresses();
};
